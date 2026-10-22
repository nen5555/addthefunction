#include "kavita_client.hpp"

#include <cctype>

#include <nlohmann/json.hpp>

#include "httplib.h"

using json = nlohmann::json;

namespace {

// URL 查询参数编码：把空格、/ 等特殊字符转成 %XX
std::string urlEncode(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

httplib::Headers authHeaders(const std::string& token) {
    return {{"Authorization", "Bearer " + token}};
}

}  // namespace

std::optional<std::string> KavitaClient::login() {
    httplib::Client cli(cfg_.kavitaUrl);
    cli.set_connection_timeout(5);
    auto res = cli.Post("/api/plugin/authenticate?apiKey=" + urlEncode(cfg_.apiKey) +
                            "&pluginName=reading-goal",
                        "", "application/json");
    if (!res) {
        lastError_ = "无法连接 Kavita（" + cfg_.kavitaUrl + "）";
        return std::nullopt;
    }
    if (res->status != 200) {
        lastError_ = "登录失败，HTTP " + std::to_string(res->status) + "（检查 API Key）";
        return std::nullopt;
    }
    try {
        json j = json::parse(res->body);
        return j.at("token").get<std::string>();
    } catch (const std::exception& e) {
        lastError_ = std::string("登录返回内容无法解析: ") + e.what();
        return std::nullopt;
    }
}

std::optional<std::vector<int>> KavitaClient::fetchLibraryIds(const std::string& token) {
    httplib::Client cli(cfg_.kavitaUrl);
    cli.set_connection_timeout(5);
    auto res = cli.Get("/api/library/libraries", authHeaders(token));
    if (!res || res->status != 200) {
        lastError_ = "获取书库列表失败";
        return std::nullopt;
    }
    try {
        const json j = json::parse(res->body);
        std::vector<int> ids;
        for (const auto& lib : j) {   // 返回的是数组，每个元素是一个书库对象
            ids.push_back(lib.at("id").get<int>());
        }
        return ids;
    } catch (const std::exception& e) {
        lastError_ = std::string("书库列表无法解析: ") + e.what();
        return std::nullopt;
    }
}

std::optional<std::map<std::string, int>> KavitaClient::fetchYear(const std::string& token,
                                                                  const std::vector<int>& libraryIds,
                                                                  int year) {
    std::string path = "/api/stats/reading-activity?userId=" + std::to_string(cfg_.userId) +
                       "&year=" + std::to_string(year) +
                       "&TimeZoneId=" + urlEncode(cfg_.timeZone);
    for (int id : libraryIds) path += "&Libraries=" + std::to_string(id);

    httplib::Client cli(cfg_.kavitaUrl);
    cli.set_connection_timeout(5);
    auto res = cli.Get(path, authHeaders(token));
    if (!res || res->status != 200) {
        lastError_ = "获取 " + std::to_string(year) + " 年阅读数据失败";
        return std::nullopt;
    }
    try {
        // 注意：先把解析结果存进变量。对临时对象直接调用 .items() 再遍历，
        // 临时对象会在循环开始前被销毁，导致悬垂引用（dangling reference）
        const json j = json::parse(res->body);
        std::map<std::string, int> out;
        for (const auto& [date, day] : j.items()) {
            long long sec = day.value("totalTimeReadingSeconds", 0LL);
            if (sec > 0) out[date] = static_cast<int>(sec);
        }
        return out;
    } catch (const std::exception& e) {
        lastError_ = std::string("阅读数据无法解析: ") + e.what();
        return std::nullopt;
    }
}

std::optional<std::map<std::string, int>> KavitaClient::fetchActivity(int currentYear, std::string& error) {
    auto token = login();
    if (!token) { error = lastError_; return std::nullopt; }

    auto libs = fetchLibraryIds(*token);
    if (!libs) { error = lastError_; return std::nullopt; }
    if (libs->empty()) { error = "当前用户没有任何书库"; return std::nullopt; }

    auto thisYear = fetchYear(*token, *libs, currentYear);
    if (!thisYear) { error = lastError_; return std::nullopt; }
    auto lastYear = fetchYear(*token, *libs, currentYear - 1);
    if (!lastYear) { error = lastError_; return std::nullopt; }

    thisYear->insert(lastYear->begin(), lastYear->end());   // 合并两年，日期键不会重复
    return thisYear;
}

bool KavitaClient::scanLibraries(int libraryId, std::string& error) {
    auto token = login();
    if (!token) { error = lastError_; return false; }

    std::vector<int> ids;
    if (libraryId > 0) {
        ids.push_back(libraryId);
    } else {
        auto libs = fetchLibraryIds(*token);
        if (!libs) { error = lastError_; return false; }
        ids = std::move(*libs);
    }

    httplib::Client cli(cfg_.kavitaUrl);
    cli.set_connection_timeout(5);
    for (int id : ids) {
        // 扫描在 Kavita 后台异步进行，这个请求会立即返回
        auto res = cli.Post("/api/Library/scan?libraryId=" + std::to_string(id), authHeaders(*token),
                            "", "application/json");
        if (!res) { error = "无法连接 Kavita"; return false; }
        if (res->status != 200) {
            error = "书库 " + std::to_string(id) + " 扫描请求失败，HTTP " + std::to_string(res->status) +
                    (res->status == 401 || res->status == 403 ? "（需要管理员账号的 API Key）" : "");
            return false;
        }
    }
    return true;
}
