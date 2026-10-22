#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "config.hpp"

// 负责和 Kavita 的 HTTP API 打交道：登录、查书库、查每日阅读时长
class KavitaClient {
public:
    explicit KavitaClient(Config cfg) : cfg_(std::move(cfg)) {}

    // 用 API Key 换取 JWT token
    std::optional<std::string> login();

    // 当前用户能访问的所有书库 ID（统计接口必须带上，否则返回全 0）
    std::optional<std::vector<int>> fetchLibraryIds(const std::string& token);

    // 某一年每天的阅读秒数，只保留 > 0 的日期
    std::optional<std::map<std::string, int>> fetchYear(const std::string& token,
                                                        const std::vector<int>& libraryIds,
                                                        int year);

    // 一次完成：登录 → 查书库 → 拉今年和去年的数据并合并（这样跨年时连续天数不会断）
    // 失败时返回 nullopt，并把原因写进 error
    std::optional<std::map<std::string, int>> fetchActivity(int currentYear, std::string& error);

    // 让 Kavita 重新扫描书库，新上传的书才会出现
    // libraryId > 0 只扫描这个书库；0 扫描当前用户的全部书库
    // 需要管理员（admin）账号的 API Key
    bool scanLibraries(int libraryId, std::string& error);

private:
    Config cfg_;
    std::string lastError_;
};
