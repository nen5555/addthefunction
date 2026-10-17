#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>

// 返回今天的日期，格式为 "YYYY-MM-DD"（本地时区）
std::string todayString();

// 日期加减天数：addDays("2026-10-31", 1) 返回 "2026-11-01"；delta 可以是负数
std::string addDays(const std::string& date, int delta);

// 计算连续达标天数
int calcStreak(const std::map<std::string, int>& secondsByDate,
               const std::string& today, int goalMinutes);

// 某一天的阅读秒数，没有记录返回 0
int secondsOn(const std::map<std::string, int>& secondsByDate, const std::string& date);

// 历史最长连续达标天数
int bestStreak(const std::map<std::string, int>& secondsByDate, int goalMinutes);

// 以 today 结尾的最近 n 天（从早到晚），每项是 {日期, 秒数}，没读的天秒数为 0
std::vector<std::pair<std::string, int>> lastNDays(const std::map<std::string, int>& secondsByDate,
                                                   const std::string& today, int n);
