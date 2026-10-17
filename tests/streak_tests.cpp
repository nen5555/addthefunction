#include "streak.hpp"
#include <iostream>
#include <vector>

int main() {
    struct TestCase {
        const char* name;
        std::map<std::string, int> activity;
        const char* today;
        int expected;
    };
    const std::vector<TestCase> cases = {
        {"empty activity", {}, "2026-10-18", 0},
        {"today missing preserves yesterday's streak",
         {{"2026-10-16", 600}, {"2026-10-17", 600}}, "2026-10-18", 2},
        {"today below goal preserves yesterday's streak",
         {{"2026-10-16", 600}, {"2026-10-17", 600}, {"2026-10-18", 599}},
         "2026-10-18", 2},
        {"today at goal extends streak",
         {{"2026-10-16", 600}, {"2026-10-17", 600}, {"2026-10-18", 600}},
         "2026-10-18", 3},
        {"missing yesterday breaks streak",
         {{"2026-10-16", 600}}, "2026-10-18", 0},
        {"yesterday below goal breaks streak",
         {{"2026-10-16", 600}, {"2026-10-17", 599}}, "2026-10-18", 0},
        {"today counts even when yesterday is missing",
         {{"2026-10-16", 600}, {"2026-10-18", 600}}, "2026-10-18", 1},
        {"streak crosses year boundary",
         {{"2025-12-30", 600}, {"2025-12-31", 600}}, "2026-01-01", 2},
        {"streak crosses leap day",
         {{"2024-02-28", 600}, {"2024-02-29", 600}}, "2024-03-01", 2},
    };
    int failures = 0;
    for (const auto& test : cases) {
        const int actual = calcStreak(test.activity, test.today, 10);
        if (actual != test.expected) {
            std::cerr << test.name << ": expected " << test.expected
                      << ", got " << actual << '\n';
            ++failures;
        }
    }
    return failures == 0 ? 0 : 1;
}
