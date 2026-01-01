#pragma once

#include "types.hpp"

#include <bitset>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace config
{
    enum class RoundGranularity : u8
    {
        Full = 1,
        Half = 2,
        Quarter = 4
    };

    inline bool is_valid_round(double r, RoundGranularity g)
    {
        const int step = static_cast<int>(g); // 1, 2, 4
        const double scaled = r * step;       // e.g. 7.5*2 = 15
        return std::fabs(scaled - std::round(scaled)) < 1e-9;
    }

    inline RoundGranularity detectGranularity(double r)
    {
        double frac = r - std::floor(r);
        if (std::fabs(frac * 4 - std::round(frac * 4)) < 1e-9)
            return RoundGranularity::Quarter;
        if (std::fabs(frac * 2 - std::round(frac * 2)) < 1e-9)
            return RoundGranularity::Half;
        return RoundGranularity::Full;
    }

    struct CipherInfo
    {
        std::string cipher_name;
        std::string mode;
        std::string comment;

        int key_size = 256;

        int nonce_bits = 96;
        int block_bits = 512;
        std::size_t words_in_state = 16;
        std::size_t word_size_bits = 8;

        bool logfile_flag = false;

        double total_rounds = 0.0;
        RoundGranularity run_granularity = detectGranularity(total_rounds);

        //  Helpers
        bool totalRoundsAreFractional() const
        {
            double int_part;
            return std::modf(total_rounds, &int_part) > 0.0;
        }

        int roundedTotalRounds() const
        {
            return static_cast<int>(total_rounds);
        }

        bool isValidRoundCount() const { return is_valid_round(total_rounds, run_granularity); }

        std::string eq_dash_sep = "====------------------------------------------------------------------====\n";
        std::string eql_sep = "==========================================================================\n";
        std::string dash_sep = "--------------------------------------------------------------------------\n";
        std::string star_sep = "**************************************************************************\n";
        std::string hash_sep4 = "##########################################################################\n";
        std::string percent_sep = "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
        std::string col_sep = "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n";
        std::string arr_sep = "<<<-------------------------------------------------------------------->>>\n";
        std::string inv_arr_sep = ">>>--------------------------------------------------------------------<<<\n";

        std::string box1 = "+-----------------------------------------------------------------------+\n";
        std::string box2 = "+==========================================================+";
        std::string box3 = "+**********************************************************+";
        std::string box4 = "+###############################+###########################";

        std::string mid1 = "---------------- LOOP ----------------";
        std::string mid2 = "============ LOOP ============";
        std::string mid3 = "----- 512-bit Bias Report -----";
        std::string mid4 = ":::::: Forward Round Analysis ::::::";

        std::string dbl1 = "||==============================================||";
        std::string dbl2 = "||----------------------------------------------||";
        std::string dbl3 = "||*************** END OF LOOP ******************||";

        std::string slim1 = "----------------------------------------";
        std::string slim2 = "........................................";

        std::string slim4 = "........................................";

        std::string frame3 = "[[[======================================================]]]";
        std::string frame4 = "(((------------------------------------------------------)))";

        std::string block_start = "====[ BLOCK START ]====";
        std::string block_end = "====[ BLOCK END ]====";
    };

    struct DLInfo
    {
        double dl_start_round = 0.0;
        double distinguishing_round = 0.0;     // total forward rounds (can be fractional)
        std::vector<std::pair<u16, u16>> id;   // input differences (input of E1)
        std::vector<std::pair<u16, u16>> od;   // output differences (output of E1, input of Em)
        std::vector<std::pair<u16, u16>> mask; // output masks (output of Em)

        std::string input_diff_str = ""; // string format for some of ciphers
        std::string output_diff_str = "";

        std::size_t output_precision = 0; // digits to print
        bool chosen_iv_flag = false;      // whether chosen IV is used
        RoundGranularity diff_granularity = detectGranularity(distinguishing_round);

        // Generic helpers
        bool fwdRoundsAreFractional() const
        {
            double ip;
            return std::modf(distinguishing_round, &ip) != 0.0;
        }
        int roundedFwdRounds() const { return static_cast<int>(distinguishing_round); }

        bool isValidForCipher(const CipherInfo &) const
        {
            return is_valid_round(distinguishing_round, diff_granularity);
        }
    };

    struct SamplesInfo
    {
        std::size_t samples_per_thread = 0; // per-thread experiments
        std::size_t samples_per_batch = 0;  // = samples_per_thread * max_num_threads
        std::size_t num_batches = 0;        // how many batches (outer iterations)

        // Number of threads to actually use
        std::size_t max_num_threads = []()
        {
            unsigned hw = std::thread::hardware_concurrency();
            return hw > 1 ? hw - 1 : 1; // leave 1 core free
        }();

        // Compiler information
        std::string compiler_info = __VERSION__;
        std::string cpp_standard =
            (__cplusplus == 201703L) ? "C++17" : (__cplusplus == 202002L) ? "C++20"
                                             : (__cplusplus == 202302L)   ? "C++23"
                                                                          : "Unknown C++ Standard";

        // Total samples executed
        std::size_t total_samples() const
        {
            if (samples_per_batch == 0 || num_batches == 0)
                return 0;

            u128 tmp = static_cast<u128>(samples_per_batch) * static_cast<u128>(num_batches);

            if (tmp > std::numeric_limits<std::size_t>::max())
                return std::numeric_limits<std::size_t>::max();

            return static_cast<std::size_t>(tmp);
        }
    };

    /**
     * @brief Lightweight state-printing helper for cipher debugging.
     *
     * This struct lets you easily print a raw uint32_t (or uint64_t) state
     * in either hex or binary form, optionally formatted as a matrix.
     *
     * ------------------ Example Usage ------------------
     *
     * @code
     * u32 x0[16] = {
     *     0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
     *     0,0,0,0,
     *     0,0,0,0,
     *     0,0,0,0
     * };
     *
     * OutputStateInfo<u32> st;
     * st.state         = x0;     // pointer to array
     * st.count         = 16;     // how many 32-bit words
     * st.label         = "Initial ChaCha State";
     * st.matrix_layout = true;   // print in rows
     * st.words_per_row = 4;
     * st.format        = OutputStateInfo<u32>::Format::Hex;   // or Binary
     *
     * printState(st, std::cout); // prints a clean hex matrix
     * @endcode
     *
     * This is intentionally barebones and works with raw arrays.
     */
    template <typename T>
    struct OutputStateInfo
    {
        static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>,
                      "State words must be unsigned integral type");

        enum class Format
        {
            Hex,
            Binary
        };

        const T *state = nullptr; // pointer to the array
        std::size_t count = 16;   // number of words in the array

        Format format = Format::Hex; // hex by default

        bool matrix_layout = true; // show in rows
        std::size_t words_per_row = 4;

        std::string label = "State";

        bool ok() const noexcept
        {
            return state != nullptr && count > 0;
        }
    };

    // formatWord EXAMPLES
    //
    // formatWord(0xA5u, true, false, true)
    // → "0xA5"
    //
    // formatWord(0xA5u, true, true, true)
    // → "0xA5"                // hex grouping currently has no effect
    //
    // formatWord(0xA5u, false, false, true)
    // → "0b10100101"
    //
    // formatWord(0xA5u, false, true, true)
    // → "0b10100101"          // grouped in 8-bit blocks (but here only 8 bits)
    //
    // formatWord(0xDEADBEEFu, false, true, true)
    // → "0b11011110 10101101 10111110 11101111"
    //
    // formatWord(0xDEADBEEFu, false, false, false)
    // → "11011110101011011011111011101111"
    //
    // formatWord((u128)1 << 127, false, true, true)
    // → "0b10000000 00000000 ... 00000000"   // 128-bit value grouped every 8 bits
    template <class U>
    std::string formatWord(U v, bool hex = true, bool group = false, bool add_prefix = true)
    {
        static_assert(std::is_unsigned_v<U>, "U must be an unsigned integral type");

        const std::size_t Wbits = sizeof(U) * 8; // total bit width of the word
        std::string out;

        // -------------------------
        // Binary output path
        // -------------------------
        if (!hex)
        {
            if (add_prefix)
                out += "0b";

            // Use bitset for <= 64-bit types (fast and simple)
            if constexpr (sizeof(U) <= sizeof(unsigned long long))
            {
                std::string bits =
                    std::bitset<sizeof(U) * 8>(
                        static_cast<unsigned long long>(v))
                        .to_string();

                if (group)
                {
                    // group every 8 bits
                    for (std::size_t i = 0; i < bits.size(); ++i)
                    {
                        out.push_back(bits[i]);
                        if ((i + 1) % 8 == 0 && i + 1 != bits.size())
                            out.push_back(' ');
                    }
                }
                else
                {
                    out += bits;
                }
            }
            else
            {
                // Manual fallback for 128-bit (and future larger types)
                for (std::size_t i = 0; i < Wbits; ++i)
                {
                    std::size_t b = Wbits - 1 - i; // MSB → LSB
                    out.push_back(((v >> b) & U{1}) ? '1' : '0');

                    if (group && ((i + 1) % 8 == 0) && (i + 1 != Wbits))
                        out.push_back(' ');
                }
            }

            return out;
        }

        // -------------------------
        // Hexadecimal output path
        // -------------------------
        if (add_prefix)
            out += "0x";

        constexpr const char *digits = "0123456789abcdef";
        const std::size_t H = Wbits / 4; // number of hex digits

        for (std::size_t i = 0; i < H; ++i)
        {
            std::size_t nib = (H - 1 - i) * 4; // MS nibble → LS nibble
            unsigned d = static_cast<unsigned>((v >> nib) & U{0xF});
            out.push_back(digits[d]);

            // Group every 2 hex chars ("abcd ef01" style)
            if (group && ((i + 1) % 2 == 0) && (i + 1 != H))
                out.push_back(' ');
        }

        return out;
    }
}
