#include <cstdlib>
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <optional>
#include <map>
#include "streak.hpp"


std::optional<std::string> login(const std::string& apiKey){
    httplib::Client cli("http://localhost:5000");
    auto res = cli.Post("/api/plugin/authenticate?apiKey=" + apiKey + "&pluginName=kavitaVersion", "", "application/json");
    if (!res) {
        std::cerr << "Request failed: " << res.error() << std::endl;
        return std::nullopt;
    }
    if( res->status == 200) {
        std::cout << "login successful: "  << std::endl;
    } else {
        std::cerr << "Request failed with status: " << (res ? std::to_string(res->status) : "No response") << std::endl;
        return std::nullopt;
    }

    using json = nlohmann::json;

    json j = json::parse(res->body);                    // 格式不对会抛出 json::parse_error 异常
    if (!j.contains("token")) {
        std::cerr << "Response does not contain 'token' field." << std::endl;
        return std::nullopt;
    }
    std::string token = j.at("token").get<std::string>(); // 字段不存在时 at() 会抛出异常
    printf("token: %s\n", token.substr(0,20).c_str());

    return token;
}

std::optional<std::map<std::string, int>> fetchActivity(const std::string& token){
    httplib::Client cli("http://localhost:5000");
    std::map<std::string, int> dateToSeconds;
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto res = cli.Get("/api/stats/reading-activity?userId=1&year=2026&TimeZoneId=Asia/Shanghai&Libraries=2", headers);
    if (!res) {
        std::cerr << "Request failed: " << res.error() << std::endl;
        return std::nullopt;
    }
    if (res->status != 200) {
        std::cerr << "Request failed with status: " << res->status << std::endl;
        return std::nullopt;
    }
    using json = nlohmann::json;
    json j = json::parse(res->body);
    for (auto& [date, entry] : j.items()) {
        long long seconds = entry.value("totalTimeReadingSeconds", 0LL);
        if (seconds > 0) {
            dateToSeconds[date] = static_cast<int>(seconds );
        }
    }
    return dateToSeconds;
};

int main(){
const char* key = std::getenv("KAVITA_API_KEY");   // 没设置时返回 nullptr
if (key == nullptr) {
    std::cerr << "KAVITA_API_KEY environment variable is not set." << std::endl;
    return 1;
}
auto token = login(key);
if (!token) return 1;
std::cout << "登录成功: " << token->substr(0, 20) << "...\n";
auto activity = fetchActivity(*token);
if (!activity) return 1;
std::cout << "阅读活动数据:\n";
for (const auto& [date, seconds] : *activity) {
    std::cout << date << ": " << seconds << " 秒\n";
}

std::string today = todayString();
std::cout << "今天是: " << today << std::endl;

std::string nextDay = addDays("2026-10-31", 1); // 测试日期加减函数
std::cout << "明天是: " << nextDay << std::endl;

int goldMinutes = 10; // 设定目标分钟数
int streak = calcStreak(*activity, today, goldMinutes);
auto it = activity->find(today);
int todaySeconds = (it == activity->end()) ? 0 : it->second;
std::cout << "today: " << todaySeconds / 60 << " 目标分钟数: " << goldMinutes << std::endl;


std::cout << "当前连续阅读天数: " << streak << std::endl;

    return 0;
}
