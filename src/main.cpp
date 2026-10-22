// reading_goal：读取 Kavita 每日阅读时长，计算每日目标和连续天数，并提供一个网页
//
// 用法：
//   ./reading_goal          启动网页服务（默认 http://127.0.0.1:5100）
//   ./reading_goal --once   只在终端打印一次今天的结果，然后退出
//
// 设置了 LIBRARY_DIR 时，网页上还可以上传书籍（POST /api/upload），上传后自动让 Kavita 扫描书库

#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "config.hpp"
#include "httplib.h"
#include "kavita_client.hpp"
#include "page.hpp"
#include "streak.hpp"
#include "upload.hpp"

using json = nlohmann::json;

namespace {

constexpr int kHeatmapDays = 16 * 7;   // 网页热力图显示最近 16 周

std::string nowHHMM() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[6];
    std::strftime(buf, sizeof buf, "%H:%M", &tm);
    return buf;
}

// 把原始数据整理成网页需要的 JSON
json buildStatus(const std::map<std::string, int>& activity, const Config& cfg) {
    const std::string today = todayString();
    const auto days = lastNDays(activity, today, kHeatmapDays);

    int weekSeconds = 0;
    for (const auto& [date, sec] : lastNDays(activity, today, 7)) weekSeconds += sec;

    json daysJson = json::array();
    for (const auto& [date, sec] : days) daysJson.push_back({{"date", date}, {"seconds", sec}});

    return {
        {"today", today},
        {"goalMinutes", cfg.goalMinutes},
        {"todaySeconds", secondsOn(activity, today)},
        {"streak", calcStreak(activity, today, cfg.goalMinutes)},
        {"best", bestStreak(activity, cfg.goalMinutes)},
        {"weekSeconds", weekSeconds},
        {"days", daysJson},
        {"updatedAt", nowHHMM()},
        {"uploadEnabled", !cfg.libraryDir.empty()},
        {"uploadMaxMB", cfg.uploadMaxMB},
    };
}

int currentYear() {
    return std::stoi(todayString().substr(0, 4));
}

// 带缓存的数据源：cacheSeconds 内重复请求直接返回上次结果，避免频繁访问 Kavita
// httplib 的服务器用多线程处理请求，所以用 mutex 保护共享数据
class StatusCache {
public:
    explicit StatusCache(const Config& cfg) : cfg_(cfg), client_(cfg) {}

    // 成功返回 JSON；失败返回 nullopt 并填写 error
    std::optional<json> get(std::string& error) {
        std::lock_guard<std::mutex> lock(mu_);
        const auto now = std::chrono::steady_clock::now();
        if (cached_ && now - fetchedAt_ < std::chrono::seconds(cfg_.cacheSeconds)) {
            return buildStatus(*cached_, cfg_);   // 原始数据走缓存，但日期、连续天数每次重新算
        }
        auto activity = client_.fetchActivity(currentYear(), error);
        if (!activity) return std::nullopt;
        cached_ = std::move(*activity);
        fetchedAt_ = now;
        return buildStatus(*cached_, cfg_);
    }

private:
    Config cfg_;
    KavitaClient client_;
    std::mutex mu_;
    std::optional<std::map<std::string, int>> cached_;
    std::chrono::steady_clock::time_point fetchedAt_{};
};

int runOnce(const Config& cfg) {
    KavitaClient client(cfg);
    std::string error;
    auto activity = client.fetchActivity(currentYear(), error);
    if (!activity) {
        std::cerr << "错误：" << error << "\n";
        return 1;
    }
    const json s = buildStatus(*activity, cfg);
    const int todaySec = s["todaySeconds"].get<int>();
    std::cout << "日期：" << s["today"].get<std::string>() << "\n"
              << "今天：" << todaySec / 60 << " 分钟 / 目标 " << cfg.goalMinutes << " 分钟 "
              << (todaySec >= cfg.goalMinutes * 60 ? "✅" : "⏳") << "\n"
              << "连续打卡：" << s["streak"].get<int>() << " 天（最长 " << s["best"].get<int>() << " 天）\n"
              << "近 7 天：" << s["weekSeconds"].get<int>() / 60 << " 分钟\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    auto cfg = loadConfig();
    if (!cfg) {
        std::cerr << "缺少环境变量 KAVITA_API_KEY\n";
        return 1;
    }

    if (argc > 1 && std::strcmp(argv[1], "--once") == 0) return runOnce(*cfg);

    StatusCache cache(*cfg);
    httplib::Server svr;
    svr.set_payload_max_length(static_cast<size_t>(cfg->uploadMaxMB) * 1024 * 1024);
    svr.set_read_timeout(300);   // 手机网络慢，大文件上传需要更长的读取超时（秒）

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(kIndexHtml, "text/html; charset=utf-8");
    });

    svr.Get("/api/status", [&cache](const httplib::Request&, httplib::Response& res) {
        std::string error;
        auto status = cache.get(error);
        if (!status) {
            res.status = 502;   // 502 Bad Gateway：上游（Kavita）出了问题
            res.set_content(json{{"error", error}}.dump(), "application/json; charset=utf-8");
            return;
        }
        res.set_header("Cache-Control", "no-store");
        res.set_content(status->dump(), "application/json; charset=utf-8");
    });

    // 上传书籍：multipart/form-data，字段名 file，可以一次多个
    // 返回 {"results":[{"name","ok","message"}], "scan": "..."}
    std::mutex uploadMu;   // 同一时间只处理一个上传，避免两个请求同时写同名文件
    svr.Post("/api/upload", [&cfg, &uploadMu](const httplib::Request& req, httplib::Response& res) {
        auto fail = [&res](int code, const std::string& msg) {
            res.status = code;
            res.set_content(json{{"error", msg}}.dump(), "application/json; charset=utf-8");
        };
        if (cfg->libraryDir.empty()) return fail(403, "未开启上传：请在配置里设置 LIBRARY_DIR");
        if (!req.is_multipart_form_data() || !req.form.has_file("file")) return fail(400, "没有收到文件");

        std::lock_guard<std::mutex> lock(uploadMu);
        json results = json::array();
        int saved = 0;
        for (const auto& f : req.form.get_files("file")) {
            json r = {{"name", f.filename}, {"ok", false}};
            auto name = sanitizeFilename(f.filename);
            if (!name) {
                r["message"] = "文件名无效";
            } else if (!isAllowedExtension(*name)) {
                r["message"] = "不支持的格式（支持 epub / pdf / cbz / cbr / zip 等）";
            } else if (f.content.empty()) {
                r["message"] = "文件为空";
            } else {
                std::string path, error;
                switch (saveBook(cfg->libraryDir, *name, f.content, path, error)) {
                    case SaveResult::Ok:
                        r["ok"] = true;
                        r["message"] = "已保存";
                        ++saved;
                        std::cout << "上传：" << path << "（" << f.content.size() / 1024 << " KB）" << std::endl;
                        break;
                    case SaveResult::AlreadyExists:
                        r["message"] = "书库里已有同名文件，跳过";
                        break;
                    case SaveResult::IoError:
                        r["message"] = error;
                        std::cerr << "上传失败：" << error << std::endl;
                        break;
                }
            }
            results.push_back(r);
        }

        std::string scan = "没有新文件，未扫描";
        if (saved > 0) {
            KavitaClient client(*cfg);
            std::string error;
            scan = client.scanLibraries(cfg->uploadLibraryId, error)
                       ? "已通知 Kavita 扫描书库，稍等片刻即可看到"
                       : "文件已保存，但扫描请求失败：" + error + "（可在 Kavita 里手动 Scan Library）";
        }
        res.set_content(json{{"results", results}, {"scan", scan}}.dump(), "application/json; charset=utf-8");
    });

    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("ok", "text/plain");
    });

    std::cout << "reading_goal 已启动：http://" << cfg->listenHost << ":" << cfg->listenPort
              << "（目标 " << cfg->goalMinutes << " 分钟/天）" << std::endl;
    std::cout << (cfg->libraryDir.empty() ? std::string("上传功能：关闭（未设置 LIBRARY_DIR）")
                                          : "上传功能：开启，保存到 " + cfg->libraryDir)
              << std::endl;
    if (!svr.listen(cfg->listenHost, cfg->listenPort)) {
        std::cerr << "端口 " << cfg->listenPort << " 监听失败（可能已被占用）\n";
        return 1;
    }
    return 0;
}
