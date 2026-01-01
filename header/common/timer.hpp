#pragma once

#include "display.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

/**
 * @brief Lightweight timer with thread-safe timestamps and precise elapsed time.
 *
 * Uses std::steady_clock for durations (monotonic, not affected by NTP/clock jumps)
 * and std::system_clock only for human-readable start/end timestamps.
 *
 * Example:
 *   Timer t;
 *   std::cout << t.start_message();
 *   // ... work ...
 *   std::cout << t.end_message();
 */
class Timer
{
private:
    using steady_clock = std::chrono::steady_clock;
    using system_clock = std::chrono::system_clock;

    steady_clock::time_point start_wall_;
    std::clock_t start_cpu_;

    static std::tm safe_localtime(std::time_t t)
    {
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        return tm;
    }

public:
    /// Start immediately.
    Timer() : start_wall_(steady_clock::now()), start_cpu_(std::clock()) {}

    /// Reset the starting points.
    void reset()
    {
        start_wall_ = steady_clock::now();
        start_cpu_ = std::clock();
    }

    /// Elapsed wall time in milliseconds since construction/reset.
    long long elapsed_ms() const
    {
        auto now = steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_wall_).count();
    }

    /// Elapsed CPU time (process) in milliseconds since construction/reset.
    double cpu_ms() const
    {
        std::clock_t now = std::clock();
        return 1000.0 * (now - start_cpu_) / CLOCKS_PER_SEC;
    }

    /// Pretty start banner with current local time.
    std::string start_message() const
    {
        return display::formatTime("Execution started");
    }

    /// Pretty end banner including wall duration since start/reset.
    std::string end_message() const
    {
        const long long wall_ms = elapsed_ms();

        std::ostringstream ss;
        ss << std::left << std::setw(35) << "Wall time elapsed " << " : " << display::formatMSduration(wall_ms) << ".\n";
        ss << display::formatTime("Execution ended");
        return ss.str();
    }
};
