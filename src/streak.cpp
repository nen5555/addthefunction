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
