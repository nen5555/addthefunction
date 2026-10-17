#pragma once
#include <map>
#include <string>

// 返回今天的日期，格式为 "YYYY-MM-DD"（本地时区）
std::string todayString();

// 日期加减天数：addDays("2026-10-31", 1) 返回 "2026-11-01"；delta 可以是负数
std::string addDays(const std::string& date, int delta);

// 计算连续达标天数
int calcStreak(const std::map<std::string, int>& secondsByDate,
               const std::string& today, int goalMinutes);
