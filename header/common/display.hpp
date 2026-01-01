#pragma once

#include "config.hpp"
#include "ops.hpp"
#include "types.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace display
{
    // ---------------- generic label printer -----------------
    template <typename T>
    inline void printField(std::ostream &out,
                           const std::string &label,
                           const T &value,
                           int width = 35,
                           const std::string &sep = " : ")
    {
        out << std::left << std::setw(width) << label
            << sep << value << "\n";
    }

    // ---------------- width constants -----------------
    // constexpr int WIDTH_LOOP = 13;
    // constexpr int WIDTH_PROB = 23;
    // constexpr int WIDTH_BIAS = 23;
    // constexpr int WIDTH_CORR = 23;
    // constexpr int WIDTH_TIME = 20;
    // constexpr int WIDTH_REM = 17;

    // constexpr int WIDTH_PNB = 11;
    // constexpr int WIDTH_INDEX = 11;
    // constexpr int WIDTH_COORD = 13;
    // constexpr int WIDTH_NM = 11;

    // ---------------- layout struct -----------------
    struct BiasTableLayout
    {
        bool show_label_1 = true; // samples
        bool show_label_2 = true; // probability
        bool show_label_3 = true; // bias
        bool show_label_4 = true; // correlation
        bool show_time = true;    // time

        bool show_label_5 = false;
        bool show_label_6 = false;
        bool show_label_7 = false; // remark, string basically

        // bool show_probability = true;
        // bool show_bias = true;
        // bool show_correlation = true;

        // bool show_remark = false;

        std::string label_1 = "# Samples";
        std::string label_2 = "Probability";
        std::string label_3 = "Bias";
        std::string label_4 = "Correlation";
        std::string label_5 = "Exec. Time";
        std::string label_6 = "";
        std::string label_7 = "";

        int width_label_1 = 17;
        int width_label_2 = 25;
        int width_label_3 = 25;
        int width_label_4 = 25;
        int width_label_5 = 20;
        int width_label_6 = 15;
        int width_label_7 = 15;

        int width_time = 15;

        // int width_rem = WIDTH_REM;

        int precision_label_1 = 5;
        int precision_label_2 = 5;
        int precision_label_3 = 5;
        int precision_label_4 = 5;

        // int bias_precision = 5;
        // int corr_precision = 5;
        // int prob_precision = 5;
    };

    // ---------------- visible width (UTF-8 safe) -----------------
    inline int visibleWidth(const std::string &s)
    {
        int w = 0;
        for (size_t i{0}; i < s.size();)
        {
            unsigned char c = s[i];
            if (c >= 0xF0 && i + 3 < s.size())
            {
                w++;
                i += 4;
            }
            else if (c >= 0xE0 && i + 2 < s.size())
            {
                w++;
                i += 3;
            }
            else if (c >= 0xC0 && i + 1 < s.size())
            {
                w++;
                i += 2;
            }
            else
            {
                w++;
                i++;
            }
        }
        return w;
    }

    // ---------------- center text -----------------
    inline std::string center(std::string_view s, int width)
    {
        int len = visibleWidth(std::string(s));
        if (len >= width)
            return std::string(s);
        int left = (width - len) / 2;
        int right = width - len - left;
        return std::string(left, ' ') + std::string(s) + std::string(right, ' ');
    }

    // ---------------- format time duration -----------------
    inline std::string formatMSduration(u32 ms)
    {
        int sec = ms / 1000;
        int m = sec / 60;
        int h = m / 60;

        int s = sec % 60;
        int mm = m % 60;
        int milli = ms % 1000;

        std::ostringstream out;
        if (h)
            out << h << "h ";
        if (mm)
            out << mm << "m ";
        if (s)
            out << s << "s ";
        if (!h && !mm && !s)
            out << milli << "ms";
        else
            out << milli << "ms";

        return out.str();
    }

    // ---------------- format duration in ms with unit scaling -----------------
    inline std::string formatDurationMs(double ms, int precision = 3)
    {
        if (!std::isfinite(ms))
            return "nan";
        if (ms < 0.0)
            ms = 0.0;

        double total_seconds = ms / 1000.0;
        long long h = static_cast<long long>(total_seconds / 3600.0);
        total_seconds -= static_cast<double>(h) * 3600.0;
        long long m = static_cast<long long>(total_seconds / 60.0);
        total_seconds -= static_cast<double>(m) * 60.0;
        double s = total_seconds;

        std::ostringstream out;
        out.setf(std::ios::fixed, std::ios::floatfield);

        if (h > 0)
            out << h << "h ";
        if (m > 0)
            out << m << "m ";

        if (h > 0 || m > 0 || s >= 1.0)
        {
            out << std::setprecision(precision) << s << "s";
        }
        else
        {
            out << std::setprecision(precision) << ms << "ms";
        }

        return out.str();
    }

    // ---------------- timestamp banner -----------------

    inline std::string formatTime(const std::string &label)
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        std::ostringstream oss;
        oss << "################################ " << label << " on: "
            << std::put_time(&tm, "%d/%m/%Y at %H:%M:%S")
            << " ################################\n";
        return oss.str();
    }

    // ---------------- format val ~ 2^{log2(val)} -----------------
    inline std::string formatRealPow2(double v,
                                      int &auto_width,
                                      bool ten_power = false,
                                      bool dec = true,
                                      int prec = 5)
    {
        std::ostringstream ss;
        ss.setf(std::ios::fixed, std::ios::floatfield);

        if (v == 0.0)
        {
            if (dec)
                ss << std::setprecision(1) << 0.0;
            ss << " ~ 2^{-∞}";
            return ss.str();
        }

        double absv = std::fabs(v);
        double lg2 = std::log2(absv);
        double lg10 = std::log10(absv);

        if (dec)
            ss << std::setprecision(prec) << v;

        ss << " ~ 2^{" << std::setprecision(2) << lg2 << "}";

        if (ten_power)
            ss << " ~ (10^{" << std::setprecision(2) << lg10 << "})";

        return ss.str();
    }

    // ---------------- format 2^{k} ~ 10^{m} -----------------
    inline std::string formatCountPow2Pow10(u64 v)
    {
        if (v == 0)
            return "0";

        double lg2 = std::log2((double)v);
        double lg10 = std::log10((double)v);

        double k_round = std::round(lg2);
        bool exact = std::fabs(lg2 - k_round) < 1e-12;

        std::ostringstream ss;
        ss.setf(std::ios::fixed, std::ios::floatfield);

        ss << "2^{";
        if (exact)
            ss << (long long)k_round;
        else
            ss << std::setprecision(2) << lg2;
        ss << "} ~ (10^{" << std::setprecision(2) << lg10 << "})";

        return ss.str();
    }

    // ---------------- border helpers -----------------
    inline void printBorder(const BiasTableLayout &lay,
                            std::ostream &out = std::cout)
    {
        out << "+";
        if (lay.show_label_1)
            out << std::string(lay.width_label_1, '-') << "+";
        if (lay.show_label_2)
            out << std::string(lay.width_label_2, '-') << "+";
        if (lay.show_label_3)
            out << std::string(lay.width_label_3, '-') << "+";
        if (lay.show_label_4)
            out << std::string(lay.width_label_4, '-') << "+";
        if (lay.show_label_5)
            out << std::string(lay.width_label_5, '-') << "+";
        if (lay.show_label_6)
            out << std::string(lay.width_label_6, '-') << "+";
        if (lay.show_label_7)
            out << std::string(lay.width_label_7, '-') << "+";
        if (lay.show_time)
            out << std::string(lay.width_time, '-') << "+";
        out << "\n";
    }

    // ---------------- header -----------------
    inline void printHeader(const BiasTableLayout &lay,
                            std::ostream &out = std::cout)
    {
        printBorder(lay, out);
        out << "|";

        if (lay.show_label_1)
            out << center(lay.label_1, lay.width_label_1) << "|";
        if (lay.show_label_2)
            out << center(lay.label_2, lay.width_label_2) << "|";
        if (lay.show_label_3)
            out << center(lay.label_3, lay.width_label_3) << "|";
        if (lay.show_label_4)
            out << center(lay.label_4, lay.width_label_4) << "|";
        if (lay.show_label_5)
            out << center(lay.label_5, lay.width_label_5) << "|";
        if (lay.show_label_6)
            out << center(lay.label_6, lay.width_label_6) << "|";
        if (lay.show_label_7)
            out << center(lay.label_7, lay.width_label_7) << "|";
        if (lay.show_time)
            out << center("Exec. Time", lay.width_time) << "|";

        out << "\n";
        printBorder(lay, out);
    }

    // ---------------- output row -----------------
    inline void outputResult(u64 samples_so_far,
                             double prob,
                             double bias,
                             double corr,
                             u32 ms,
                             BiasTableLayout &lay,
                             std::ostream &out = std::cout,
                             int rem_count = 0,
                             bool rem_flag = false,
                             bool hide_bias_corr = false)
    {
        auto cell = [&](std::string s, int w)
        {
            out << center(s, w) << "|";
        };

        std::string remark = rem_flag ? "✓ (" + std::to_string(rem_count) + ")" : "x";
        std::string t = formatMSduration(ms);

        out << "|";

        // samples generally
        if (lay.show_label_1)
        {
            std::string s = formatRealPow2(samples_so_far,
                                           lay.width_label_1,
                                           false, false,
                                           lay.precision_label_1);
            cell(s, lay.width_label_1);
        }

        // probability
        if (lay.show_label_2)
        {
            std::string s = formatRealPow2(prob,
                                           lay.width_label_2,
                                           false, true,
                                           lay.precision_label_2);
            cell(s, lay.width_label_2);
        }

        // bias
        if (lay.show_label_3)
        {
            std::string s = formatRealPow2(bias,
                                           lay.width_label_3,
                                           false, true,
                                           lay.precision_label_3);
            cell(s, lay.width_label_3);
        }

        // correlation
        if (lay.show_label_4)
        {
            std::string s = formatRealPow2(corr,
                                           lay.width_label_4,
                                           false, true,
                                           lay.precision_label_4);
            cell(s, lay.width_label_4);
        }

        //
        if (lay.show_label_5)
        {
            std::string s = formatRealPow2(corr,
                                           lay.width_label_4,
                                           false, true,
                                           lay.precision_label_4);
            cell(s, lay.width_label_5);
        }

        //
        if (lay.show_label_6)
        {
            std::string s = formatRealPow2(corr,
                                           lay.width_label_4,
                                           false, true,
                                           lay.precision_label_4);
            cell(s, lay.width_label_6);
        }

        // string
        if (lay.show_label_7)
        {
            std::string s = remark;
            cell(s, lay.width_label_7);
        }

        // time
        if (lay.show_time)
            cell(t, lay.width_time);

        out << "\n";
    }

    // ---------------- run-info printer -----------------
    inline void showInfo(const config::CipherInfo *cipher,
                         const config::DLInfo *diff = nullptr,
                         const config::SamplesInfo *samples = nullptr,
                         std::ostream &out = std::cout)
    {
        auto F = [&](const std::string &lb, auto &&val)
        {
            printField(out, lb, std::forward<decltype(val)>(val));
        };

        if (!cipher)
            return;

        {
            std::ostringstream s;
            s << cipher->cipher_name;
            if (cipher->key_size > 0)
                s << "-" << cipher->key_size;
            F("Cipher", s.str());
        }

        F("Word size", cipher->word_size_bits);
        if (!cipher->mode.empty())
            F("Mode", cipher->mode);

        if (!cipher->comment.empty())
            F("Comment", cipher->comment);

        if (cipher->total_rounds > 0.0)
            F("# of encrypting rounds", cipher->total_rounds);

        if (diff)
        {
            if (diff->distinguishing_round > 0.0)
                F("Distinguishing round", diff->distinguishing_round);

            if (diff->chosen_iv_flag)
                F("Chosen IV mode", "enabled");

            bool idflag{false}, odflag{false};

            if (!diff->id.empty())
            {
                std::ostringstream s;
                s << "{";
                for (size_t i{0}; i < diff->id.size(); ++i)
                {
                    auto [w, b] = diff->id[i];
                    s << "(" << w << "," << b << ")";
                    if (i + 1 != diff->id.size())
                        s << ", ";
                }
                s << "}";
                F("Input difference (word,bit)", s.str());
                idflag = true;
            }

            if (!diff->od.empty())
            {
                std::ostringstream s;
                s << "{";
                for (size_t i{0}; i < diff->od.size(); ++i)
                {
                    auto [w, b] = diff->od[i];
                    s << "(" << w << "," << b << ")";
                    if (i + 1 != diff->od.size())
                        s << ", ";
                }
                s << "}";
                F("Output difference (word,bit)", s.str());
                odflag = true;
            }

            if (!diff->mask.empty())
            {
                std::ostringstream s;
                s << "{";
                for (size_t i{0}; i < diff->mask.size(); i++)
                {
                    auto [w, b] = diff->mask[i];
                    s << "(" << w << "," << b << ")";
                    if (i + 1 != diff->mask.size())
                        s << ", ";
                }
                s << "}";
                F("Output mask (word,bit)", s.str());
            }

            // ---------------- print input difference ----------------
            if (!diff->input_diff_str.empty() && !idflag)
            {
                std::string s = diff->input_diff_str;
                if (s.size() > 128) // heuristic: anything >128 bits → hex
                    s = "0x" + ops::bin_to_hex(s);

                F("input difference", s);
            }

            // ---------------- print output difference ----------------
            if (!diff->output_diff_str.empty() && !odflag)
            {
                std::string s = diff->output_diff_str;
                if (s.size() > 128)
                    s = "0x" + ops::bin_to_hex(s);

                F("output difference", s);
            }
        }

        if (samples)
        {
            F("Compiler info", samples->compiler_info);
            F("C++ standard", samples->cpp_standard);

            F("# of threads", samples->max_num_threads);

            if (samples->samples_per_thread)
                F("Samples per thread", formatCountPow2Pow10(samples->samples_per_thread));

            if (samples->samples_per_batch)
                F("Samples per batch ", formatCountPow2Pow10(samples->samples_per_batch));

            if (samples->num_batches)
            {
                F("# of batches", formatCountPow2Pow10(samples->num_batches));
                F("# of samples", formatCountPow2Pow10(samples->total_samples()));
            }
        }

        out << cipher->percent_sep;
    }

    // ---------------- print cipher state -----------------
    template <typename T>
    void printState(const config::OutputStateInfo<T> &info,
                    std::ostream &out = std::cout)
    {
        if (!info.ok())
        {
            out << "[printState] empty\n";
            return;
        }

        if (!info.label.empty())
            out << info.label << ":";

        const int bits = sizeof(T) * 8;
        using PrintType = std::conditional_t<sizeof(T) == 1, u16, T>;

        for (size_t i{0}; i < info.count; i++)
        {
            if (info.matrix_layout && (i % info.words_per_row == 0))
                out << "\n";

            if (info.format == config::OutputStateInfo<T>::Format::Hex)
            {
                out << "0x"
                    << std::hex << std::right << std::setfill('0') << std::setw(bits / 4) << static_cast<PrintType>(info.state[i])
                    << std::dec << std::setfill(' ');
            }
            else
            {
                for (int b{bits - 1}; b >= 0; b--)
                    out << (((info.state[i] >> b) & 1) ? '1' : '0');
            }

            out << "  ";
        }

        out << "\n";
    }
}
