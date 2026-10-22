#pragma once
#include <cstdlib>
#include <optional>
#include <string>

// 所有可调参数都从环境变量读取，代码里不写死任何密钥
struct Config {
    std::string apiKey;                                // KAVITA_API_KEY（必填）
    std::string kavitaUrl = "http://localhost:5000";   // KAVITA_URL
    int userId = 1;                                    // KAVITA_USER_ID
    std::string timeZone = "Asia/Shanghai";            // KAVITA_TZ
    int goalMinutes = 10;                              // GOAL_MINUTES
    std::string listenHost = "127.0.0.1";              // LISTEN_HOST（只监听本机，由 tailscale serve 转发）
    int listenPort = 5100;                             // LISTEN_PORT
    int cacheSeconds = 60;                             // CACHE_SECONDS：多久向 Kavita 重新拉一次数据

    // 网页上传书籍（LIBRARY_DIR 为空时关闭上传功能）
    std::string libraryDir;                            // LIBRARY_DIR：本机上 Kavita 书库文件夹的路径（宿主机路径，不是容器内路径）
    int uploadLibraryId = 0;                           // UPLOAD_LIBRARY_ID：上传后扫描哪个书库；0 = 扫描全部书库
    int uploadMaxMB = 300;                             // UPLOAD_MAX_MB：单次上传大小上限
};

namespace detail {
inline std::string envOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v && *v) ? std::string(v) : fallback;
}
inline int envIntOr(const char* name, int fallback) {
    const char* v = std::getenv(name);
    if (!v || !*v) return fallback;
    try { return std::stoi(v); } catch (...) { return fallback; }
}
}  // namespace detail

// 读取配置；缺少 KAVITA_API_KEY 时返回 nullopt
inline std::optional<Config> loadConfig() {
    Config c;
    c.apiKey = detail::envOr("KAVITA_API_KEY", "");
    if (c.apiKey.empty()) return std::nullopt;
    c.kavitaUrl    = detail::envOr("KAVITA_URL", c.kavitaUrl);
    c.userId       = detail::envIntOr("KAVITA_USER_ID", c.userId);
    c.timeZone     = detail::envOr("KAVITA_TZ", c.timeZone);
    c.goalMinutes  = detail::envIntOr("GOAL_MINUTES", c.goalMinutes);
    c.listenHost   = detail::envOr("LISTEN_HOST", c.listenHost);
    c.listenPort   = detail::envIntOr("LISTEN_PORT", c.listenPort);
    c.cacheSeconds = detail::envIntOr("CACHE_SECONDS", c.cacheSeconds);
    c.libraryDir      = detail::envOr("LIBRARY_DIR", c.libraryDir);
    c.uploadLibraryId = detail::envIntOr("UPLOAD_LIBRARY_ID", c.uploadLibraryId);
    c.uploadMaxMB     = detail::envIntOr("UPLOAD_MAX_MB", c.uploadMaxMB);
    return c;
}
