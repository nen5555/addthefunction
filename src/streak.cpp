#include <ctime>
#include <cstdio>
#include <string>
#include <stdexcept>
#include "streak.hpp"
#include <map>

// 返回今天的日期，格式为 "YYYY-MM-DD"（本地时区）
std::string todayString(){
    std::tm tm{};
    std::time_t t = std::time(nullptr);
    localtime_r(&t, &tm);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return std::string(buffer);
};

std::string addDays(const std::string& date, int delta){
    std::tm tm{};
    if (strptime(date.c_str(), "%Y-%m-%d", &tm) == nullptr) {
        throw std::invalid_argument("Invalid date format. Expected YYYY-MM-DD.");
    }
    std::time_t t = std::mktime(&tm);
    t += delta * 24 * 60 * 60; // 增加或减少天数
    localtime_r(&t, &tm);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return std::string(buffer);
};

int calcStreak(const std::map<std::string, int>& secondsByDate,
               const std::string& today, int goalMinutes){
                if (secondsByDate.empty()) {
                    return 0;
                }
                int streak = 0;
                std::string current = today;
                auto todayIt = secondsByDate.find(today);
                if (todayIt == secondsByDate.end() || todayIt->second < goalMinutes * 60) {
                    current = addDays(today, -1);
                }
                while (true) {
                    auto it = secondsByDate.find(current);
                    if (it != secondsByDate.end() && it->second >= goalMinutes * 60) {
                        streak++;
                    } else {
                        break;
                    }
                    current = addDays(current, -1);
                }
                return streak;
            };

int secondsOn(const std::map<std::string, int>& secondsByDate, const std::string& date) {
    auto it = secondsByDate.find(date);
    return (it == secondsByDate.end()) ? 0 : it->second;
}

int bestStreak(const std::map<std::string, int>& secondsByDate, int goalMinutes) {
    // map 按日期从早到晚排好序，逐个检查是否和上一个达标日相邻
    int best = 0;
    int run = 0;
    std::string prevMet;
    for (const auto& [date, seconds] : secondsByDate) {
        if (seconds < goalMinutes * 60) continue;
        run = (!prevMet.empty() && addDays(prevMet, 1) == date) ? run + 1 : 1;
        prevMet = date;
        if (run > best) best = run;
    }
    return best;
}

std::vector<std::pair<std::string, int>> lastNDays(const std::map<std::string, int>& secondsByDate,
                                                   const std::string& today, int n) {
    std::vector<std::pair<std::string, int>> out;
    if (n <= 0) return out;
    out.reserve(n);
    std::string day = addDays(today, -(n - 1));
    for (int i = 0; i < n; ++i) {
        out.emplace_back(day, secondsOn(secondsByDate, day));
        day = addDays(day, 1);
    }
    return out;
}
