/*
 * REFERENCE IMPLEMENTATION OF pnb header file
 *
 *
 * created: 20/11/23
 * updated: 21/12/25
 *
 * by Hiren
 * Researcher
 *
 *
 * Synopsis:
 * This file contains functions that implement the bare minimum of PNB usecases
 */

#pragma once
#include "common/common.hpp"
#include <filesystem>
#include <fstream>
#include <set>

namespace pnbinfo
{
    struct PNBdetails
    {
        std::string pnb_file;
        double neutrality_measure = 0.0; // threshold for neutrality

        // bool pnb_search_flag = false;  // run a PNB search
        bool pnb_pattern_flag = false; // use pattern filtering

        bool logfile = false;

        std::size_t potential_pnb_count = 0; // number of potential PNBs found

        std::vector<u16> pnbs; // list of discovered PNB bit positions
        std::vector<u16> pnbs_in_pattern;
        std::vector<u16> pnbs_in_border;
        std::vector<u16> rest_pnbs;
    };

    config::CipherInfo config;
    // PNB-specific tiny header (optional, but nice)
    inline void showPNBConfig(const PNBdetails &pnb,
                              std::ostream &out = std::cout)
    {
        if (pnb.neutrality_measure > 0)
            out << std::left << std::setw(35) << "Threshold"
                << " : " << pnb.neutrality_measure << "\n";

        if (pnb.pnbs.size())
        {
            out << std::left << std::setw(35) << "PNB count"
                << " : " << pnb.pnbs.size() << "\n";
            out << std::left << std::setw(35) << "The PNB list is " << " : ";
            bool first = true;
            for (auto &i : pnb.pnbs)
            {
                if (!first)
                    out << ", ";
                out << i;
                first = false;
            }
            out << "\n";
        }

        if (pnb.pnb_pattern_flag)
            out << std::left << std::setw(35) << "Pattern flag"
                << " : " << std::boolalpha << pnb.pnb_pattern_flag << std::noboolalpha << "\n";
        out << config.star_sep;
    }

    template <class T>
    std::tuple<std::vector<T>, std::vector<T>, std::vector<T>>
    splitConsecutive(const std::vector<T> &elems)
    {
        static_assert(std::is_integral_v<T>, "T must be integral");

        if (elems.empty())
            return {{}, {}, {}};

        std::vector<T> first, second, third;
        std::vector<T> cur;
        cur.reserve(elems.size());

        cur.push_back(elems[0]);
        for (std::size_t i = 1; i < elems.size(); ++i)
        {
            if (elems[i] == cur.back() + 1)
            {
                cur.push_back(elems[i]);
            }
            else
            {
                if (cur.size() >= 2)
                {
                    first.insert(first.end(), cur.begin(), cur.end() - 1);
                    second.push_back(cur.back());
                }
                else
                {
                    third.push_back(cur[0]);
                }
                cur.clear();
                cur.push_back(elems[i]);
            }
        }
        // flush last block
        if (cur.size() >= 2)
        {
            first.insert(first.end(), cur.begin(), cur.end() - 1);
            second.push_back(cur.back());
        }
        else
        {
            third.push_back(cur[0]);
        }

        return {first, second, third};
    }

    // PNB loading / processing
    //
    // Usage:
    //
    // 1) Load PNBs from a text file (whitespace- or comma-separated integers):
    //
    //      pnbinfo::PNBdetails cfg;
    //      cfg.pnb_pattern_flag = true;      // if you want pattern/border/rest
    //      bool ok = openPNBFile("pnbs.txt", cfg);
    //
    //      // After this:
    //      //   cfg.pnbs            = all PNB indices (sorted, deduplicated)
    //      //   cfg.pnbs_in_pattern = first block from splitConsecutive()
    //      //   cfg.pnbs_in_border  = second block ...
    //      //   cfg.rest_pnbs       = third block ...
    //
    // 2) Provide PNBs directly in code, *without* a file:
    //
    //      pnbinfo::PNBdetails cfg;
    //      cfg.pnbs = {148,149,150,156,157,158,159,191,196};
    //      cfg.pnb_pattern_flag = true;
    //      bool ok = preparePNBFromVector(cfg);   // same post-processing as file
    //
    // File format:
    //   - Any mix of spaces/newlines/commas is accepted.
    //   - E.g. "1 2 3 7 8 21" or "1,2,3,7,8,21"
    //   - Values must be in [0,256).
    // -----------------------------------------------------------------------------

    // Helper: take a vector of PNB indices, sort/dedup, and fill cfg.{pnbs, ...}
    inline bool finalizePNBValues(PNBdetails &cfg, std::vector<u16> vals)
    {
        if (vals.empty())
        {
            cfg.pnbs.clear();
            cfg.pnbs_in_pattern.clear();
            cfg.pnbs_in_border.clear();
            cfg.rest_pnbs.clear();
            return false; // nothing to do
        }

        // sort + dedup
        std::sort(vals.begin(), vals.end());
        vals.erase(std::unique(vals.begin(), vals.end()), vals.end());

        // full list
        cfg.pnbs = vals;

        if (!cfg.pnb_pattern_flag)
        {
            cfg.pnbs_in_pattern.clear();
            cfg.pnbs_in_border.clear();
            cfg.rest_pnbs.clear();
            return true;
        }

        // split into 3 sets using your consecutive-run logic
        auto [first, second, third] = splitConsecutive(vals);

        cfg.pnbs_in_pattern = std::move(first);
        cfg.pnbs_in_border = std::move(second);
        cfg.rest_pnbs = std::move(third);
        return true;
    }

    // Case 1: read from file, then call finalizePNBValues()
    inline bool preparePNBFromFile(const std::string &filename, PNBdetails &cfg, const config::CipherInfo *cipher)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "⚠ Could not open PNB file: " << filename << "\n";
            return false;
        }

        std::vector<u16> vals;
        std::string line;

        while (std::getline(file, line))
        {
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream iss(line);
            int v;
            while (iss >> v)
            {
                if (v < 0 || v > cipher->key_size)
                {
                    std::cerr << "⚠ Invalid PNB value: " << v
                              << " (must be in [0,256))\n";
                    return false;
                }
                vals.push_back(static_cast<u16>(v));
            }
        }
        file.close();

        if (vals.empty())
        {
            std::cerr << "⚠ PNB file is empty.\n";
            return false;
        }

        return finalizePNBValues(cfg, std::move(vals));
    }

    // Case 2: cfg.pnbs is already filled in code; reuse the same pipeline.
    inline bool preparePNBFromVector(PNBdetails &cfg)
    {
        // copy because finalizePNBValues sorts and mutates the vector
        std::vector<u16> vals = cfg.pnbs;
        return finalizePNBValues(cfg, std::move(vals));
    }

    // Helper: print a vector in { a, b, c } format
    template <class T>
    void print_braced_list(const std::vector<T> &v, std::ostream &out)
    {
        out << "{";
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            out << v[i];
            if (i + 1 != v.size())
                out << ", ";
        }
        out << "}\n";
    }

    // Helper: is index in PNB set (use std::set for O(log n))
    inline bool is_pnb(u16 idx, const std::set<u16> &pnb_set)
    {
        return pnb_set.find(idx) != pnb_set.end();
    }

    // 1. Print PNB / Non-PNB counts and sets
    void print_basic_pnb_sets(const std::vector<u16> &pnbs_sorted_by_index,
                              const std::vector<u16> &pnbs_sorted_by_bias,
                              const std::vector<u16> &nonpnbs_sorted_by_index,
                              std::ostream &out)
    {
        const std::size_t count_pnb = pnbs_sorted_by_index.size();
        const std::size_t count_nonpnb = nonpnbs_sorted_by_index.size();

        out << "-----------------------------------------------------------------------\n";
        // out << "PNB count           : " << count_pnb << "\n";
        out << std::left << std::setw(35) << "PNB count" << " : " << count_pnb << "\n";
        out << std::left << std::setw(35) << "non-PNB count" << " : " << count_nonpnb << "\n";

        // out << "Non-PNB count       : " << count_nonpnb << "\n";
        out << "-----------------------------------------------------------------------\n\n";

        out << count_pnb << " PNBs in set (sorted by index)\n";
        print_braced_list(pnbs_sorted_by_index, out);
        out << "\n";

        out << count_pnb << " PNBs in set (sorted by decresing order of bias)\n";
        print_braced_list(pnbs_sorted_by_bias, out);
        out << "\n";

        out << "=============================================================================================================\n";
        out << "################################ EXTRA INFO ################################\n";
        out << "=============================================================================================================\n\n";

        out << count_nonpnb << " Non-PNBs in set (sorted by index)\n";
        print_braced_list(nonpnbs_sorted_by_index, out);
        // out << "\n";
    }

    // 2. Bias list grouped by word
    //    bias_per_bit[i]   = bias of bit i (0..key_bits-1)
    //    pnbs_sorted_by_index = used to mark P/N
    void print_bias_list_by_word(const std::vector<double> &bias_per_bit,
                                 const std::vector<u16> &pnbs_sorted_by_index,
                                 const config::CipherInfo *cipher,
                                 std::ostream &out)
    {
        // const std::size_t key_bits = bias_per_bit.size();

        int word_size = cipher->word_size_bits;

        const std::size_t num_words = cipher->key_size / cipher->word_size_bits;

        // std::cout << "word_size: " << word_size << "\n";

        std::set<u16> pnb_set(pnbs_sorted_by_index.begin(), pnbs_sorted_by_index.end());

        out << "------------------------------------------------------------------------------\n";
        out << "Bias list of all " << cipher->key_size << " key-bits\n";
        out << "Format:bit_index  bias_value  flag\n";

        out << "(P = PNB, N = non-PNB)\n";
        out << std::noshowpos;

        // formatting
        out << std::fixed << std::setprecision(3);

        for (std::size_t w{0}; w < num_words; ++w)
        {
            const int start_idx = (w * word_size);
            const int end_idx = ((w + 1) * word_size - 1);

            out << "--- Keyword " << w << " ("
                << start_idx << "-" << end_idx << ") ---\n";

            for (int bit_idx{start_idx}; bit_idx <= end_idx; ++bit_idx)
            {
                double b = bias_per_bit[bit_idx];
                char flag = is_pnb(static_cast<u16>(bit_idx), pnb_set) ? 'P' : 'N';

                out << std::noshowpos
                    << std::setw(6) << bit_idx << "  "
                    << std::setw(12) << b << "  "
                    << flag << "\n";
            }
            // out << "\n";
        }
    }

    // 3. Per-keyword PNB segments
    //    Example: Keyword 4 (128–159): [31:28], [22], [20], [16:10], [3:1]
    static std::string compress_segments_desc(const std::vector<int> &bits_desc)
    {
        // bits_desc must be sorted in descending order
        if (bits_desc.empty())
            return "";

        std::string s;
        int seg_start = bits_desc[0];
        int seg_end = bits_desc[0];

        auto flush_segment = [&](int start, int end)
        {
            if (!s.empty())
                s += ", ";
            if (start == end)
                s += "[" + std::to_string(start) + "]";
            else
                s += "[" + std::to_string(start) + ":" + std::to_string(end) + "]";
        };

        for (std::size_t i{1}; i < bits_desc.size(); ++i)
        {
            int b = bits_desc[i];
            if (b == seg_end - 1) // consecutive in descending order
            {
                seg_end = b;
            }
            else
            {
                flush_segment(seg_start, seg_end);
                seg_start = seg_end = b;
            }
        }
        flush_segment(seg_start, seg_end);
        return s;
    }

    inline void print_per_keyword_segments(const std::vector<u16> &bits_sorted_by_index,
                                           const config::CipherInfo *cipher,
                                           const std::string &title,
                                           std::ostream &out)
    {
        int word_size = cipher->word_size_bits;
        if (bits_sorted_by_index.empty())
        {
            out << "------------------------------------------------------------------------------\n";
            out << title << "\n";
            out << "(none)\n\n";
            return;
        }

        const std::size_t max_idx = bits_sorted_by_index.back();
        const std::size_t num_words = max_idx / word_size + 1;

        out << "------------------------------------------------------------------------------\n";
        out << title << ":\n";

        for (std::size_t w{0}; w < num_words; ++w)
        {
            std::vector<int> bits_in_word;
            bits_in_word.reserve(cipher->word_size_bits);

            // collect all PNB bit positions in this word
            for (u16 idx : bits_sorted_by_index)
            {
                if (idx / word_size == w)
                    bits_in_word.push_back(static_cast<int>(idx % word_size));
            }

            if (bits_in_word.empty())
                continue;

            // sort descending so we get [31:28] style
            std::sort(bits_in_word.begin(), bits_in_word.end(), std::greater<int>());

            const int start_idx = static_cast<int>(w * word_size);
            const int end_idx = static_cast<int>((w + 1) * word_size - 1);

            std::string segments = compress_segments_desc(bits_in_word);

            // build label once, then align it
            std::ostringstream label;
            label << "Keyword " << w << " (" << start_idx << "-" << end_idx << ")";

            out << std::left << std::setw(22) << label.str()
                << " : " << segments << "\n";
        }

        // out << "\n";
    }

    inline void print_per_keyword_pnb_segments(const std::vector<u16> &pnbs_sorted_by_index,
                                               const config::CipherInfo *cipher,
                                               std::ostream &out)
    {
        print_per_keyword_segments(pnbs_sorted_by_index, cipher, "Per-keyword PNB segments", out);
    }

    inline void print_per_keyword_nonpnb_segments(const std::vector<u16> &nonpnbs_sorted_by_index,
                                                  const config::CipherInfo *cipher,
                                                  std::ostream &out)
    {
        print_per_keyword_segments(nonpnbs_sorted_by_index, cipher, "Per-keyword non-PNB segments", out);
    }

    inline void print_per_keyword_ps_map(const std::vector<u16> &pnbs_sorted_by_index,
                                         const std::vector<u16> &nonpnbs_sorted_by_index,
                                         const config::CipherInfo *cipher,
                                         std::ostream &out)
    {
        const std::size_t total_bits = cipher->key_size;
        const std::size_t word_size = cipher->word_size_bits;
        const std::size_t num_words = (total_bits + word_size - 1) / word_size;

        std::vector<char> flags(total_bits, '.');
        for (u16 idx : nonpnbs_sorted_by_index)
        {
            if (idx < total_bits)
                flags[idx] = 's';
        }
        for (u16 idx : pnbs_sorted_by_index)
        {
            if (idx < total_bits)
                flags[idx] = 'p';
        }

        out << "------------------------------------------------------------------------------\n";
        out << "Per-keyword P/S map (bit " << (word_size - 1) << " .. 0):\n";

        for (std::size_t w{0}; w < num_words; ++w)
        {
            const std::size_t start = w * word_size;
            if (start >= total_bits)
                break;
            const std::size_t end = std::min(start + word_size - 1, total_bits - 1);

            std::string line;
            line.reserve(end - start + 1);
            for (std::size_t b = end + 1; b-- > start;)
                line.push_back(flags[b]);

            std::ostringstream label;
            label << "Keyword " << w << " (" << start << "-" << end << ")";

            out << std::left << std::setw(22) << label.str()
                << " : " << line << "\n";
        }
    }

    // 4. Biases as –log2(|bias|) for all PNBs
    void print_neglog2_biases_all(const std::vector<double> &bias_per_bit,
                                  const config::CipherInfo *cipher,
                                  std::ostream &out)
    {
        const std::size_t total_bits = cipher->key_size;

        out << "------------------------------------------------------------------------------\n";
        out << "Biases as -log2(|bias|) for ALL key bits (0 to " << (total_bits - 1) << ")\n";
        out << "Note: value = -log2(|bias|);  larger value = weaker bias.\n";
        out << "{";

        out << std::fixed << std::setprecision(2);

        for (std::size_t i{0}; i < total_bits; ++i)
        {
            double b = bias_per_bit[i];
            double ab = std::fabs(b);

            double v;
            if (ab == 0.0)
                v = std::numeric_limits<double>::infinity(); // infinite uncertainty
            else if (ab == 1.0)
                v = 0.0; // perfect correlation
            else
                v = -std::log2(ab);

            out << v;
            if (i + 1 != total_bits)
                out << ", ";
            else
                out << "";
        }

        out << "}\n";
        out << "=============================================================================================================\n";
        out << "################################ END OF REPORT ################################\n";
        out << "=============================================================================================================\n\n";
    }

    // 5. One convenience wrapper to print the whole "extra" part
    void print_full_pnb_report_tail(const std::vector<u16> &pnbs_sorted_by_index,
                                    const std::vector<u16> &pnbs_sorted_by_bias,
                                    const std::vector<u16> &nonpnbs_sorted_by_index,
                                    const std::vector<double> &bias_per_bit,
                                    const config::CipherInfo cipher,
                                    std::ostream &out)
    {
        print_basic_pnb_sets(pnbs_sorted_by_index,
                             pnbs_sorted_by_bias,
                             nonpnbs_sorted_by_index,
                             out);

        print_bias_list_by_word(bias_per_bit, pnbs_sorted_by_index, &cipher, out);
        print_per_keyword_pnb_segments(pnbs_sorted_by_index, &cipher, out);
        print_neglog2_biases_all(bias_per_bit, &cipher, out);
    }

    inline std::string makeLogFilename(const config::CipherInfo &cipher,
                                       const config::DLInfo &diff,
                                       const PNBdetails *pnb_cfg, const std::string &folder)
    {
        namespace fs = std::filesystem;

        // ensure folder exists
        // std::string folder = "logs_pnb";
        fs::create_directories(folder);

        // mask[0] handling
        std::string mask_str = "nomask";
        if (!diff.mask.empty())
        {
            auto [w, b] = diff.mask[0];
            std::ostringstream ms;
            ms << w << "_" << b;
            mask_str = ms.str();
        }

        // timestamp
        std::time_t t = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream name;

        name << cipher.cipher_name << "_"
             << cipher.mode
             << cipher.total_rounds << "_mask_"
             << mask_str << "_"
             << tm.tm_hour << "_" << tm.tm_min << "_"
             << tm.tm_mday << "_" << (tm.tm_mon + 1) << "_"
             << (tm.tm_year + 1900);

        if (pnb_cfg)
            name << "_nm_" << pnb_cfg->neutrality_measure;
        name << ".txt";

        return folder + "/" + name.str();
    }
}

namespace salcharo
{
    struct QuarterSchedule
    {
        int total_qr; // total number of quarter rounds
        int dist_qr;  // quarter index at which we check parity (1-based)
                      // = 0 means "no eval" or "before any forward", your choice
    };

    inline bool isMultipleOfQuarter(double r)
    {
        if (r < 0.0)
            return false;
        double scaled = r * 4.0; // quarter units
        double rounded = std::round(scaled);
        return std::fabs(scaled - rounded) < 1e-9;
    }

    // Build schedule from generic config structs
    inline QuarterSchedule buildQuarterSchedule(const config::CipherInfo &ci,
                                                const config::DLInfo &dl)
    {
        QuarterSchedule qs{};

        // total rounds: must be k * 0.25
        if (!isMultipleOfQuarter(ci.total_rounds))
            throw std::invalid_argument("total_rounds must be multiple of 0.25");

        qs.total_qr = static_cast<int>(std::lround(ci.total_rounds * 4.0));
        if (qs.total_qr <= 0)
            throw std::invalid_argument("total_rounds must be > 0");

        // eval round (can be zero or negative meaning "no eval")
        if (dl.distinguishing_round <= 0.0)
        {
            qs.dist_qr = 0; // convention: 0 = no eval
            return qs;
        }

        if (!isMultipleOfQuarter(dl.distinguishing_round))
            throw std::invalid_argument("dsit_round must be multiple of 0.25");

        if (dl.distinguishing_round > ci.total_rounds)
            throw std::invalid_argument("dist_round cannot exceed total_rounds");

        qs.dist_qr = static_cast<int>(std::lround(dl.distinguishing_round * 4.0)); // 3.0 -> 12 etc.

        return qs;
    }

} // namespace salcharo
