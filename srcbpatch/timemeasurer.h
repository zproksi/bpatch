#pragma once
#include <chrono>
#include <vector>
#include <string>
#include <string_view>

/// @brief TimeMeasurer allows to measure time between creation and destruction of the class
class TimeMeasurer final
{
    TimeMeasurer(const TimeMeasurer&) = delete;
    TimeMeasurer(TimeMeasurer&&) = delete;
    TimeMeasurer& operator =(const TimeMeasurer&) = delete;
    TimeMeasurer&& operator =(TimeMeasurer&&) = delete;

    typedef decltype(std::chrono::high_resolution_clock::now()) TIME_POINT_TYPE;

    struct TimePoint
    {
        std::string_view name;
        TIME_POINT_TYPE elapsed;
    };
    using DataVector = std::vector<TimePoint>;

public:
    /// @brief Full name of meaure should be passed in constructor
    TimeMeasurer(const std::string_view& name);

    /// @brief all time points from shortes moments to longest will be printed 
    ///   with corresponding names
    ///   Main name provided in constructor will be printed last
    /// 
    ~TimeMeasurer();

    /// @brief returns amount of time (nanoseconds) from the class creation till the time point 'at'
    const long long NanosecondsElapsed(const TIME_POINT_TYPE at) const;

    /// @brief save time point in the class
    void RegisterTimePoint(const std::string_view& name);

    /// @brief returns initial moment of measurements
    TIME_POINT_TYPE ExecutionTimePoint() const;


    /// @brief returns amount of time (nanoseconds) in format "...,XXX,XXX,XXX,XXX"
    /// @note no conversion to seconds, minutes and etc.
    static std::string FormatNanoseconds(const long long nanoseconds);
protected:
    DataVector timePoints;
    TimePoint startPoint;
};


