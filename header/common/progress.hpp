#pragma once

#define SPINNER_WITH_ETA_AVAILABLE 1

#include "types.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class SpinnerWithETA
{
public:
    /**
     * @brief Constructor for the SpinnerWithETA class.
     * @param msg The message displayed next to the spinner.
     * @param done_ptr Pointer to the atomic counter storing completed work.
     * @param total_work The total amount of work to be done.
     * @param delay_ms The update frequency in milliseconds.
     */
    SpinnerWithETA(std::string msg,
                   std::atomic<u64> *done_ptr,
                   u64 total_work,
                   int delay_ms = 120)
        : message(std::move(msg)),
          done(done_ptr),
          total(total_work),
          delay(delay_ms),
          running(false)
    {
    }

    /**
     * @brief Starts the spinner thread.
     */
    void start()
    {
        if (!done || total == 0)
            return; // nothing to track, silently do nothing

        running = true;
        start_time = std::chrono::steady_clock::now();
        th = std::thread([this]()
                         { this->run(); });
    }

    /**
     * @brief Stops the spinner thread, joins it, and cleans up the terminal line.
     */
    void stop()
    {
        running = false;
        if (th.joinable())
            th.join();

        // REVISED CLEANUP: Clear the line with spaces, then output a new line (\n)
        // to ensure the prompt starts on a fresh line.
        std::cout << "\r" << std::string(100, ' ') << "\r"
                  << std::flush;
    }

    /**
     * @brief Destructor ensures stop() is called on destruction (RAII).
     */
    ~SpinnerWithETA()
    {
        stop();
    }

private:
    std::string message;
    std::atomic<u64> *done;
    u64 total;
    int delay;
    std::atomic<bool> running;
    std::thread th;
    std::chrono::steady_clock::time_point start_time;

    /**
     * @brief Formats seconds into a human-readable ETA string (e.g., 01m 30s).
     */
    static std::string format_eta(double seconds)
    {
        if (!std::isfinite(seconds) || seconds < 0)
            return "ETA: ??";

        auto s = static_cast<long long>(seconds + 0.5);
        long long h = s / 3600;
        long long m = (s % 3600) / 60;
        long long sec = s % 60;

        char buf[32];
        if (h > 0)
            std::snprintf(buf, sizeof(buf), "ETA: %lldh %02lldm %02llds", h, m, sec);
        else if (m > 0)
            std::snprintf(buf, sizeof(buf), "ETA: %02lldm %02llds", m, sec);
        else
            std::snprintf(buf, sizeof(buf), "ETA: %2llds", sec);
        return std::string(buf);
    }

    /**
     * @brief The main loop that runs in the background thread.
     */
    void run()
    {
        // --- ANSI Color Definitions ---
        const std::string ANSI_C32 = "\033[32m"; // Green FG
        const std::string ANSI_C33 = "\033[33m"; // Yellow FG
        const std::string ANSI_C34 = "\033[34m"; // Blue FG
        const std::string ANSI_C35 = "\033[35m"; // Magenta FG
        const std::string ANSI_C36 = "\033[36m"; // Cyan FG
        const std::string ANSI_C37 = "\033[37m"; // White FG

        // Background Colors
        const std::string B_BLACK = "\033[40m";
        const std::string B_RED = "\033[41m";
        const std::string B_GREEN = "\033[42m";
        const std::string B_YELLOW = "\033[43m";
        const std::string B_BLUE = "\033[44m";
        const std::string B_MAGENTA = "\033[45m";
        const std::string B_CYAN = "\033[46m";

        const std::string ANSI_BOLD = "\033[1m"; // Bold FG
        const std::string ANSI_DIM = "\033[2m";  // Dim FG

        const std::string ANSI_RESET = "\033[0m"; // Reset all formatting

        // --- Standard Frame Sets (using const char* directly for simplicity) ---
        static const char *frames_vert[] = {
            " ", "▂", "▃", "▄", "▅", "▆", "▇", "█", "▇", "▆", "▅", "▄", "▃", "▂", " "};
        static const char *frames_quadrant[] = {
            "▖", "▘", "▝", "▗"};
        static const char *frames_smooth[] = {
            "░░░░░░░░░░",
            "▒░░░░░░░░░",
            "▒▒░░░░░░░░",
            "▓▒▒░░░░░░░",
            "▓▓▒▒░░░░░░",
            "██▓▒▒░░░░░",
            "███▓▒▒░░░░",
            "████▓▒▒░░░",
            "█████▓▒▒░░",
            "██████▓▒▒░",
            "███████▓▒▒",
            "████████▓▒",
            "█████████▓",
            "██████████",
            "█████████▓",
            "████████▓▒",
            "███████▓▒▒",
            "██████▓▒▒░",
            "█████▓▒▒░░",
            "████▓▒▒░░░",
            "███▓▒▒░░░░",
            "██▓▒▒░░░░░",
            "▓▓▒▒░░░░░░",
            "▓▒▒░░░░░░░",
            "▒▒░░░░░░░░",
            "▒░░░░░░░░░",
        };
        static const char *frames_widebars[] = {"░", "▒", "▓", "█", "▓", "▒", "░"};

        // --- NEW Dynamic Frame Sets (using static std::string to fix dangling pointer) ---

        // 1. Rainbow Transition Bar
        static const std::string frames_rainbow_bar[] = {
            ANSI_C32 + "█" + ANSI_RESET,
            ANSI_C33 + "█" + ANSI_RESET,
            ANSI_C34 + "█" + ANSI_RESET,
            ANSI_C35 + "█" + ANSI_RESET,
            ANSI_C36 + "█" + ANSI_RESET,
            ANSI_C37 + "█" + ANSI_RESET,
        };

        // 2. Waving Pulse
        const std::string ANSI_BLUE_STAR = ANSI_C34 + "\u25CF" + ANSI_RESET;
        const std::string SP = " ";
        static const std::string frames_wave_pulse[] = {
            ANSI_BLUE_STAR + SP + SP + SP + SP,
            SP + ANSI_BLUE_STAR + SP + SP + SP,
            SP + SP + ANSI_BLUE_STAR + SP + SP,
            SP + SP + SP + ANSI_BLUE_STAR + SP,
            SP + SP + SP + SP + ANSI_BLUE_STAR,
            SP + SP + SP + ANSI_BLUE_STAR + SP,
            SP + SP + ANSI_BLUE_STAR + SP + SP,
            SP + ANSI_BLUE_STAR + SP + SP + SP,
        };

        // 3. Blinking Indicator (Bold/Dim)
        static const std::string frames_blinking_dot[] = {
            ANSI_BOLD + ANSI_C35 + "\u25CF" + ANSI_RESET,
            ANSI_DIM + ANSI_C35 + "\u25CF" + ANSI_RESET,
        };

        // 4. WIDE RAINBOW BAR (Generated once)
        static std::vector<std::string> frames_rainbow_bar_wide;
        if (frames_rainbow_bar_wide.empty())
        {
            const int WIDE_BAR_LENGTH = 20;
            const int SWEEP_SIZE = 4;
            const std::string BASE_CHAR = " ";

            static const std::string SWEEP_COLORS[] = {
                B_RED, B_YELLOW, B_GREEN, B_CYAN, B_BLUE, B_MAGENTA};
            const int NUM_COLORS = sizeof(SWEEP_COLORS) / sizeof(SWEEP_COLORS[0]);

            for (int i = 0; i < WIDE_BAR_LENGTH; ++i)
            {
                std::string frame_str = "";
                const std::string &current_sweep_color = SWEEP_COLORS[i % NUM_COLORS];

                for (int j = 0; j < WIDE_BAR_LENGTH; ++j)
                {
                    if (j >= i && j < i + SWEEP_SIZE)
                    {
                        frame_str += current_sweep_color + BASE_CHAR + ANSI_RESET;
                    }
                    else
                    {
                        frame_str += B_BLACK + BASE_CHAR + ANSI_RESET;
                    }
                }
                frames_rainbow_bar_wide.push_back(frame_str);
            }
        }

        // --- CHOOSE FRAME SET HERE ---
        // Change this line to switch patterns!
        // Options: frames_vert, frames_quadrant, frames_smooth, frames_widebars (char*)
        //          frames_rainbow_bar, frames_wave_pulse, frames_blinking_dot (std::string*)
        //          frames_rainbow_bar_wide (std::vector<std::string>*)

        const void *current_frames_base = frames_rainbow_bar_wide.data(); // <--- SET YOUR PATTERN HERE
        // -----------------------------

        std::size_t frame_count = 0;
        const char *const *char_frames_ptr = nullptr;
        const std::string *string_frames_ptr = nullptr;

        // Logic to determine frame type and count
        if (current_frames_base == frames_vert)
        {
            char_frames_ptr = frames_vert;
            frame_count = sizeof(frames_vert) / sizeof(frames_vert[0]);
        }
        else if (current_frames_base == frames_quadrant)
        {
            char_frames_ptr = frames_quadrant;
            frame_count = sizeof(frames_quadrant) / sizeof(frames_quadrant[0]);
        }
        else if (current_frames_base == frames_smooth)
        {
            char_frames_ptr = frames_smooth;
            frame_count = sizeof(frames_smooth) / sizeof(frames_smooth[0]);
        }
        else if (current_frames_base == frames_widebars)
        {
            char_frames_ptr = frames_widebars;
            frame_count = sizeof(frames_widebars) / sizeof(frames_widebars[0]);
        }
        else if (current_frames_base == frames_rainbow_bar)
        {
            string_frames_ptr = frames_rainbow_bar;
            frame_count = sizeof(frames_rainbow_bar) / sizeof(frames_rainbow_bar[0]);
        }
        else if (current_frames_base == frames_wave_pulse)
        {
            string_frames_ptr = frames_wave_pulse;
            frame_count = sizeof(frames_wave_pulse) / sizeof(frames_wave_pulse[0]);
        }
        else if (current_frames_base == frames_blinking_dot)
        {
            string_frames_ptr = frames_blinking_dot;
            frame_count = sizeof(frames_blinking_dot) / sizeof(frames_blinking_dot[0]);
        }
        else if (current_frames_base == frames_rainbow_bar_wide.data())
        {
            string_frames_ptr = frames_rainbow_bar_wide.data();
            frame_count = frames_rainbow_bar_wide.size();
        }

        std::size_t idx = 0;
        std::size_t last_len{0};

        while (running)
        {
            double frac = 0.0;
            double eta_sec = std::numeric_limits<double>::infinity();

            std::ostringstream line;

            // 1. frame
            if (frame_count > 0)
            {
                if (char_frames_ptr)
                    line << char_frames_ptr[idx % frame_count];
                else if (string_frames_ptr)
                    line << string_frames_ptr[idx % frame_count];
            }
            else
                line << " ";

            // 2. message
            if (!message.empty())
                line << " " << message;

            // 3. progress + ETA
            if (total > 0 && done)
            {
                u64 cur = done->load(std::memory_order_relaxed);
                if (cur > total)
                    cur = total;

                if (cur > 0)
                {
                    auto now = std::chrono::steady_clock::now();
                    std::chrono::duration<double> elapsed = now - start_time;
                    frac = static_cast<double>(cur) / static_cast<double>(total);
                    double rate = elapsed.count() / frac;
                    eta_sec = rate * (1.0 - frac);

                    int pct = static_cast<int>(frac * 100.0 + 0.5);
                    line << " [" << pct << "%]  " << format_eta(eta_sec);
                }
                else
                {
                    line << " [ 0% ]  ETA: ??";
                }
            }

            std::string s = line.str();
            std::size_t len = s.size();

            // move to start, print new line
            std::cout << "\r" << s;

            // pad with spaces if new line is shorter than previous
            if (len < last_len)
                std::cout << std::string(last_len - len, ' ');

            std::cout << std::flush;
            last_len = len;

            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            idx++;
        }
    }
};
