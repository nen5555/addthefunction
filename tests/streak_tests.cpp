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

    auto expect = [&](const char* name, bool ok) {
        if (!ok) {
            std::cerr << name << ": failed\n";
            ++failures;
        }
    };

    // secondsOn
    expect("secondsOn existing", secondsOn({{"2026-10-17", 1782}}, "2026-10-17") == 1782);
    expect("secondsOn missing", secondsOn({{"2026-10-17", 1782}}, "2026-10-16") == 0);

    // bestStreak
    expect("bestStreak empty", bestStreak({}, 10) == 0);
    expect("bestStreak picks longest run",
           bestStreak({{"2026-10-01", 600}, {"2026-10-02", 600}, {"2026-10-03", 600},
                       {"2026-10-05", 600}, {"2026-10-06", 600}}, 10) == 3);
    expect("bestStreak ignores below-goal days",
           bestStreak({{"2026-10-01", 600}, {"2026-10-02", 599}, {"2026-10-03", 600}}, 10) == 1);
    expect("bestStreak across year",
           bestStreak({{"2025-12-31", 600}, {"2026-01-01", 600}}, 10) == 2);

    // lastNDays
    {
        auto days = lastNDays({{"2026-10-16", 300}, {"2026-10-17", 1782}}, "2026-10-17", 3);
        expect("lastNDays size", days.size() == 3);
        expect("lastNDays order and values",
               days.size() == 3 &&
               days[0] == std::make_pair(std::string("2026-10-15"), 0) &&
               days[1] == std::make_pair(std::string("2026-10-16"), 300) &&
               days[2] == std::make_pair(std::string("2026-10-17"), 1782));
        expect("lastNDays crosses month",
               lastNDays({}, "2026-11-01", 2).front().first == "2026-10-31");
        expect("lastNDays zero", lastNDays({}, "2026-11-01", 0).empty());
    }

    if (failures == 0) std::cout << "All streak tests passed\n";
    return failures == 0 ? 0 : 1;
}
