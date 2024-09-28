#include "stdafx.h"
#include <iostream>
#include "timemeasurer.h"

TimeMeasurer::TimeMeasurer(const std::string_view name)
    :startPoint{name, std::chrono::high_resolution_clock::now()}
{
}
TimeMeasurer::~TimeMeasurer()
{
    const TIME_POINT_TYPE now = std::chrono::high_resolution_clock::now();
    for (auto&& tp: timePoints)
    {
        std::cout << tp.name << ": " << FormatNanoseconds(NanosecondsElapsed(tp.elapsed)) << " ns.\n";
    }
    std::cout << startPoint.name << ": " << FormatNanoseconds(NanosecondsElapsed(now)) << " ns.\n";
}

const long long TimeMeasurer::NanosecondsElapsed(const TIME_POINT_TYPE at) const
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(at - ExecutionTimePoint()).count();
}

void TimeMeasurer::RegisterTimePoint(const std::string_view name)
{
    timePoints.emplace_back(TimePoint{name, std::chrono::high_resolution_clock::now()});
}

TimeMeasurer::TIME_POINT_TYPE TimeMeasurer::ExecutionTimePoint() const
{
    return startPoint.elapsed;
}


std::string TimeMeasurer::FormatNanoseconds(const long long nanoseconds)
{
    std::string s;
    s.reserve(32); // max length of long long in string representation is 21
    s = std::to_string(nanoseconds);
    const size_t len = s.length();
    for (size_t i = 0; len > 3 && i < (len - 1) / 3; ++i)
    {
        s.insert(len - ((i + 1) * 3), 1, ','); // separate number of nanoseconds with ',' at every 3-d position
    }
    return s;
}
