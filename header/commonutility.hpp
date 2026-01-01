/*
 * REFERENCE IMPLEMENTATION OF some common utility functions
 *
 *
 * created: 13/10/25
 * updated: 21/12/25
 *
 * by Hiren
 * Research Fellow
 * NTU Singapore
 *
 * Synopsis:
 * This file contains some common functions used in different scheme
 */
#include <algorithm>
#include <bit>
#include <bitset>
#include <iomanip>
#include <iostream> // cin cout, unsigned integers
#include <filesystem>
#include <fstream> // files
#include <random>  // mt19937
#include <sstream> //
#include <span>    // for outputstateinfo
#include <thread>  // thread
#include <type_traits>
#include <vector>

using ull = unsigned long long; // 32 - 64 bits in integer
using longd = long double;      // 64 bits in double

using u8 = std::uint8_t;   // positive integer of 8 bits
using u16 = std::uint16_t; // positive integer of 16 bits
using u32 = std::uint32_t; // positive integer of 32 bits
using u64 = std::uint64_t; // positive integer of 64 bits
using u128 = __uint128_t;  // positive integer of 128 bits

// #define GET_BIT(word, bit) (((word) >> (bit)) & 0x1)
// #define SET_BIT(word, bit) ((word) |= (1ULL << (bit)))
// #define UNSET_BIT(word, bit) ((word) &= ~(1ULL << (bit)))
// #define TOGGLE_BIT(word, bit) ((word) ^= (1ULL << (bit)))

// helper alias (C++20+). For C++17, replace with remove_cv + remove_reference.
template <class T>
using rm_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// 0/1 extractor (works even if `word` is an lvalue like DiffState[d.first])
#define GET_BIT(word, bit) \
    (((word) >> (bit)) & rm_cvref_t<decltype(word)>(1))

// type-correct masks (no more 1ULL forcing 64-bit)
#define SET_BIT(word, bit) \
    ((word) |= (rm_cvref_t<decltype(word)>(1) << (bit)))

#define UNSET_BIT(word, bit) \
    ((word) &= ~(rm_cvref_t<decltype(word)>(1) << (bit)))

#define TOGGLE_BIT(word, bit) \
    ((word) ^= (rm_cvref_t<decltype(word)>(1) << (bit)))

// #define ROTATE_LEFT(x, n) (((x) << ((n) % (sizeof(x) * 8))) | ((x) >> ((sizeof(x) * 8) - ((n) % (sizeof(x) * 8)))))
// #define ROTATE_RIGHT(x, n) (((x) >> ((n) % (sizeof(x) * 8))) | ((x) << ((sizeof(x) * 8) - ((n) % (sizeof(x) * 8)))))

#define ROTATE_LEFT(x, n) std::rotl(x, n)
#define ROTATE_RIGHT(x, n) std::rotr(x, n)

inline thread_local std::mt19937 gen{std::random_device{}()};

template <typename T>
T RandomNumber(T min = 0, T max = std::numeric_limits<T>::max())
{
    std::uniform_int_distribution<T> dis(min, max);
    return dis(gen);
}

// a function which generates a random boolean value
bool RandomBoolean()
{
    std::bernoulli_distribution dis(0.5);
    return dis(gen);
}

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

    RoundGranularity detectGranularity(double r)
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

namespace ops
{
    /**
     * @brief Copy a slice of the state array.
     *
     * Copies words in the half-open range [start, end) from src[] to dst[].
     * Assumes the two arrays do NOT overlap.
     *
     * Example:
     *   u32 a[16], b[16];
     *   copyState(b, a, 4, 8);   // copies a[4..7] into b[4..7]
     */
    template <typename T>
    void copyState(T *dst, const T *src,
                   std::size_t start = 0,
                   std::size_t end = 16)
    {
        if (!dst || !src)
            throw std::invalid_argument("copyState: null pointer");
        if (start > end)
            throw std::out_of_range("copyState: start > end");

        std::memcpy(dst + start, src + start, (end - start) * sizeof(T));
    }

    // XOR two state slices: z[i] = x[i] ^ x1[i]  for i in [start, end)
    // Preconditions: pointers must be valid; T must be unsigned.
    template <typename T>
    void xorState(const T *x, const T *x1, T *output,
                  std::size_t start = 0, std::size_t end = 16)
    {
        static_assert(std::is_unsigned_v<T>,
                      "xorState() requires an unsigned integral type");

        if (!output || !x || !x1)
            throw std::invalid_argument("xorState: null pointer detected");

        if (start >= end)
            throw std::out_of_range("xorState: start must be < end");

        for (std::size_t i{start}; i < end; ++i)
            output[i] = x[i] ^ x1[i];
    }

    // Add two states : z[i] = x[i] + x1[i]  for i in [start, end)
    // Preconditions: pointers must be valid; T must be unsigned.
    template <typename T>
    void addState(const T *x, const T *x1, T *z, size_t start = 0, size_t end = 16)
    {
        static_assert(std::is_unsigned_v<T>,
                      "addState() requires an unsigned integeral type.");

        if (!z || !x || !x1)
            throw std::invalid_argument("addState: null pointer detected.");

        if (start >= end)
            throw std::out_of_range("addState: start must be < end.");

        for (size_t i{start}; i < end; ++i)
            z[i] = x[i] + x1[i];
    }

    // Subtract two states: z[i] = x[i] - x1[i]  for i in [start, end)
    // Preconditions: pointers must be valid; T must be unsigned.
    template <typename T>
    void subtractState(const T *x, const T *x1, T *z,
                       size_t start = 0, size_t end = 16)
    {
        static_assert(std::is_unsigned_v<T>,
                      "subtractState() requires an unsigned integeral type.");

        if (!z || !x || !x1)
            throw std::invalid_argument("subtractState: null pointer detected.");
        if (start >= end)
            throw std::out_of_range("subtractState: start must be < end.");

        for (size_t i{start}; i < end; ++i)
            z[i] = x[i] - x1[i];
    }

    // Fill the slice [start, end) of the state array with `value`.
    // Example: setState(x, 0, 16, 0) clears a 16-word ChaCha state.
    template <typename T>
    void setState(T *x, std::size_t start = 0, std::size_t end = 16, T value = 0)
    {
        if (!x)
            throw std::invalid_argument("setState: null pointer");
        if (start >= end)
            throw std::out_of_range("setState: start must be < end");

        std::fill(x + start, x + end, value);
    }

    // Extract bits in the inclusive range [start, end] from `word`.
    // Example: bitSegment(0b11100100, 2, 5) → 0b1001 (i.e., 9)
    template <typename T>
    constexpr T bitSegment(T word, int start, int end)
    {
        static_assert(std::is_unsigned_v<T>,
                      "bitSegment requires an unsigned integer type");

        const int width = end - start + 1;
        const int bit_size = std::numeric_limits<T>::digits;

        // Runtime sanity check (remove if speed-critical)
        if (start < 0 || end >= bit_size || start > end)
            throw std::out_of_range("bitSegment: invalid bit range");

        // Handle full-width extraction safely
        const T mask = (width == bit_size)
                           ? std::numeric_limits<T>::max()
                           : ((T(1) << width) - 1);

        return (word >> start) & mask;
    }

    // Replace bits in dst on the inclusive range [start, end]
    // with the corresponding bits taken from src.
    //
    // Example:
    //   u32 a = 0b11110000;
    //   u32 b = 0b01011010;
    //   replaceBitSegment(a, b, 2, 5);
    //   // a becomes: 0b11011000 (bits 2..5 copied from b)
    template <typename T>
    constexpr void replaceBitSegment(T &dst, T src, int start, int end)
    {
        static_assert(std::is_unsigned_v<T>,
                      "replaceBitSegment requires an unsigned integer type");

        const int width = end - start + 1;                   // number of bits in the segment
        const int bit_size = std::numeric_limits<T>::digits; // e.g. 32 for u32

        // Basic sanity checks (you can remove for raw speed)
        if (start < 0 || end >= bit_size || start > end)
            throw std::out_of_range("replaceBitSegment: invalid bit range");

        // Base mask for width bits: 0b111...1 of length `width`
        const T base_mask = (width == bit_size)
                                ? std::numeric_limits<T>::max() // all bits
                                : (T(1) << width) - 1;

        // Shifted mask to the target position in dst
        const T mask = base_mask << start;

        // Extract the source segment (aligned to bit 0)
        const T segment = (src >> start) & base_mask;

        // Clear that segment in dst, then insert the new bits
        dst = (dst & ~mask) | (segment << start);
    }

    // Write a message to console and/or file.
    // Uses stream references (safe): no nullptrs, no is_open() checks needed.
    // If 'file' is not open, the write simply sets failbit (no crash).
    inline void writeMesaage(std::ostream &out_console,
                             std::ostream &out_file,
                             bool write_cout,
                             bool write_file,
                             const std::string &msg)
    {
        if (write_cout)
            out_console << msg;
        if (write_file)
            out_file << msg;
    }

    // Convert a hex or binary string into an array of N unsigned state words.
    // Works for 32-bit, 64-bit, and even 128-bit (or wider) custom unsigned types.
    //
    // str:       input string ("0x..." or "0b..." allowed)
    // out[N]:    output state words
    // hexflag:   true = parse hex, false = parse binary
    //
    // Requirements:
    //   1. T must be an unsigned integer type
    //   2. str length must match exactly (word_size * N)
    //   3. Supports manual parsing when T > 64 bits
    //
    // Example:
    //   u32 s[4];
    //   stringToState("0x00010203AABBCCDD11223344FFEEDDCC", s);
    //
    //   unsigned u128 s128[1];
    //   stringToState("0x1234567890abcdef1234567890abcdef", s128);
    template <typename T, std::size_t N>
    void stringToState(const std::string &str, T (&out)[N], bool hexflag = true)
    {
        static_assert(std::is_unsigned_v<T>, "T must be unsigned");

        constexpr std::size_t BITS = sizeof(T) * 8; // bits per word
        constexpr std::size_t HEX_CH = BITS / 4;    // hex chars per word (1 hex = 4 bits)

        std::string s = str;

        // ---------------------------------------------------------------------
        // HEX MODE
        // ---------------------------------------------------------------------
        if (hexflag)
        {
            // Remove 0x or 0X prefix if present
            if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
                s.erase(0, 2);

            // Must exactly match (#words * chars_per_word)
            if (s.size() != HEX_CH * N)
                throw std::runtime_error("Hex string length does not match state size");

            for (std::size_t i = 0; i < N; i++)
            {
                // Extract chunk belonging to word i
                std::string chunk = s.substr(i * HEX_CH, HEX_CH);

                // -------------------------------------------------------------
                // Fast path: 32-bit or 64-bit words → safe to use stoull
                // -------------------------------------------------------------
                if constexpr (sizeof(T) <= 8)
                {
                    out[i] = static_cast<T>(std::stoull(chunk, nullptr, 16));
                }
                else
                {
                    // ---------------------------------------------------------
                    // Slow path: 128-bit (or wider) words → manual parse needed
                    // ---------------------------------------------------------
                    T value = 0;
                    for (char c : chunk)
                    {
                        value <<= 4; // shift by 4 bits for next hex nibble
                        if (c >= '0' && c <= '9')
                            value |= (c - '0');
                        else if (c >= 'a' && c <= 'f')
                            value |= (c - 'a' + 10);
                        else if (c >= 'A' && c <= 'F')
                            value |= (c - 'A' + 10);
                        else
                            throw std::runtime_error("Invalid hex character in input");
                    }
                    out[i] = value;
                }
            }
            return;
        }

        // ---------------------------------------------------------------------
        // BINARY MODE
        // ---------------------------------------------------------------------

        // Remove 0b or 0B prefix if present
        if (s.rfind("0b", 0) == 0 || s.rfind("0B", 0) == 0)
            s.erase(0, 2);

        // Must exactly match (#words * bits_per_word)
        if (s.size() != BITS * N)
            throw std::runtime_error("Binary string length does not match state size");

        for (std::size_t i{0}; i < N; i++)
        {
            // Extract chunk for one word
            std::string chunk = s.substr(i * BITS, BITS);

            // Use bitset for 32-bit and 64-bit words
            if constexpr (sizeof(T) <= 8)
            {
                std::bitset<BITS> bits(chunk);
                out[i] = static_cast<T>(bits.to_ullong());
            }
            else
            {
                // Manual parsing for 128-bit words
                T value = 0;
                for (char c : chunk)
                {
                    value <<= 1; // shift by 1
                    if (c == '1')
                        value |= 1;
                    else if (c != '0')
                        throw std::runtime_error("Invalid binary char");
                }
                out[i] = value;
            }
        }
    }

    template <typename T, std::size_t N>
    void stringToStateAuto(const std::string &str, T (&out)[N])
    {
        static_assert(std::is_unsigned_v<T>, "T must be unsigned");

        std::string s = str;

        // -------------------------------
        // Detect prefix
        // -------------------------------
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
            return stringToState(s, out, /*hexflag=*/true);

        if (s.rfind("0b", 0) == 0 || s.rfind("0B", 0) == 0)
            return stringToState(s, out, /*hexflag=*/false);

        // ---------------------------------------------
        // No prefix: infer from allowed character set
        // ---------------------------------------------
        bool is_hex = true;
        bool is_bin = true;

        for (char c : s)
        {
            if (!(c == '0' || c == '1'))
                is_bin = false;

            if (!((c >= '0' && c <= '9') ||
                  (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F')))
                is_hex = false;
        }

        if (is_hex && !is_bin)
            return stringToState(s, out, /*hexflag=*/true);

        if (is_bin && !is_hex)
            return stringToState(s, out, /*hexflag=*/false);

        throw std::runtime_error(
            "stringToStateAuto: cannot infer hex/binary from input string");
    }

    // Convert an array of unsigned words into one hex or binary string.
    //
    // Usage example:
    //     uint32_t s[4] = {0x11223344, 0xaabbccdd, 0x01020304, 0xdeadbeef};
    //     std::string H = stateToString(s);          // → "0x11223344aabbccdd01020304deadbeef"
    //     std::string B = stateToString(s, false);   // → long binary string
    //
    // hexflag = true  → return hex string
    // hexflag = false → return binary string
    // count = number of words to include
    template <typename T>
    std::string stateToString(const T *x, bool hexflag = true, std::size_t count = 16)
    {
        static_assert(std::is_unsigned_v<T>, "T must be unsigned");
        if (!x)
            throw std::invalid_argument("stateToString: null pointer");

        constexpr std::size_t BITS = sizeof(T) * 8;
        constexpr std::size_t HEX_PER = BITS / 4;

        // Binary path
        if (!hexflag)
        {
            std::string out;
            out.reserve(2 + BITS * count);
            out += "0b";
            for (std::size_t i = 0; i < count; ++i)
                out += std::bitset<BITS>(x[i]).to_string();
            return out;
        }

        // Hex path
        std::ostringstream oss;
        oss << "0x";
        for (std::size_t i{0}; i < count; ++i)
        {
            oss << std::hex << std::nouppercase
                << std::setw(static_cast<int>(HEX_PER))
                << std::setfill('0')
                << static_cast<unsigned long long>(x[i]);
        }
        return oss.str();
    }

    /**
     * matchBitsWithWildcard<N>
     *
     * Compares two bitstrings (hex or binary) of length N, where the pattern
     * may contain a wildcard character (default '*') that matches ANY bit.
     *
     * Supports:
     *   - "0x..." or "0b..." prefixes (optional)
     *   - Hex patterns: wildcard expands to 4 wildcard bits
     *   - Binary patterns: wildcard stays single-bit
     *   - Strict length enforcement after conversion (must be exactly N bits)
     *
     * Example:
     *     matchBitsWithWildcard<128>("0xdeadbeef...", "0x****beef...", '*');
     *     matchBitsWithWildcard<256>("0b1010...", "0b10*0...", '*');
     */
    template <std::size_t N>
    bool matchBitsWithWildcard(const std::string &diff,
                               const std::string &pat,
                               char wildcard = '*')
    {
        std::string d = diff;
        std::string p = pat;

        // Check if hex mode is needed
        bool hex_mode =
            (d.rfind("0x", 0) == 0 || d.rfind("0X", 0) == 0) ||
            (p.rfind("0x", 0) == 0 || p.rfind("0X", 0) == 0);

        // Remove 0x/0b prefix if present
        auto strip_prefix = [](std::string &s)
        {
            if (s.size() >= 2 &&
                (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0 ||
                 s.rfind("0b", 0) == 0 || s.rfind("0B", 0) == 0))
            {
                s = s.substr(2);
            }
        };

        strip_prefix(d);
        strip_prefix(p);

        // Convert hex → binary; propagate wildcard nibble → 4 wildcard bits
        if (hex_mode)
        {
            auto hexToBin = [wildcard](const std::string &hex)
            {
                std::string bin;
                bin.reserve(hex.size() * 4);

                for (char c : hex)
                {
                    if (c == wildcard)
                    {
                        bin.append(4, wildcard);
                        continue;
                    }

                    unsigned v;
                    if (c >= '0' && c <= '9')
                        v = c - '0';
                    else if (c >= 'a' && c <= 'f')
                        v = c - 'a' + 10;
                    else if (c >= 'A' && c <= 'F')
                        v = c - 'A' + 10;
                    else
                        throw std::runtime_error("Invalid hex character in pattern");

                    for (int b = 3; b >= 0; --b)
                        bin.push_back(((v >> b) & 1) ? '1' : '0');
                }
                return bin;
            };

            d = hexToBin(d);
            p = hexToBin(p);
        }
        // else: already binary (wildcards unchanged)

        // Must match exact bit-length N
        if (d.size() != N || p.size() != N)
            throw std::runtime_error("Strings must be exactly " + std::to_string(N) + " bits after conversion.");

        // Bit compare with wildcard tolerance
        for (std::size_t i{0}; i < N; ++i)
        {
            char pc = p[i];

            if (pc == wildcard)
                continue;

            if (pc != '0' && pc != '1')
                throw std::runtime_error("Pattern has invalid character; allowed: 0/1/" +
                                         std::string(1, wildcard));

            if (d[i] != pc)
                return false;
        }

        return true;
    }

    /**
     * hammingWeight(x)
     *
     * Returns the number of 1-bits in the unsigned integer x.
     *
     * Example:
     *     std::uint32_t v = 0b1011u;   // 3 one-bits
     *     int hw = hammingWeight(v);   // hw == 3
     */
    template <class T>
    inline int hammingWeight(T x)
    {
        static_assert(std::is_unsigned_v<T>,
                      "hammingWeight: T must be an unsigned integer type");

        // For types up to 64 bits, just use std::popcount directly.
        if constexpr (sizeof(T) <= sizeof(unsigned long long))
        {
            return std::popcount(static_cast<unsigned long long>(x));
        }
        else
        {
            // For wider integer types (e.g. 128-bit), process in 64-bit chunks.
            int hw = 0;
            constexpr int bits = std::numeric_limits<T>::digits;

            T tmp = x;
            for (int processed{0}; processed < bits; processed += 64)
            {
                unsigned long long chunk =
                    static_cast<unsigned long long>(tmp & T{0xFFFFFFFFFFFFFFFFULL});
                hw += std::popcount(chunk);
                tmp >>= 64;
            }
            return hw;
        }
    }

    /**
     * hammingWeight(arr)
     *
     * Computes the total Hamming weight of an array of unsigned integers.
     *
     * Example:
     *     uint32_t x[3] = {0b1010, 0b1111, 0b0001};
     *     int hw = hammingWeight(x);   // hw = 2 + 4 + 1 = 7
     */
    template <class T, std::size_t N>
    inline int hammingWeight(const T (&arr)[N])
    {
        static_assert(std::is_unsigned_v<T>,
                      "hammingWeight(array): T must be unsigned integer type");

        int hw = 0;
        for (std::size_t i{0}; i < N; ++i)
            hw += hammingWeight(arr[i]); // uses the scalar version

        return hw;
    }
    /**
     * Build a 512-bit binary string (MSB→LSB per word) from diff.od,
     * which stores (word,bit) pairs indicating output difference bits.
     * If diff.od is empty → return all-zero 512-bit string.
     **/
    inline void build_output_diff_str(config::DLInfo &diff, const config::CipherInfo &cipher)
    {
        const std::size_t WORDS = cipher.words_in_state;     // typically 16
        const std::size_t word_size = cipher.word_size_bits; // 8, 32, 64, ...
        const std::size_t BITS = WORDS * word_size;

        std::string out(BITS, '0');

        if (diff.od.empty())
        {
            diff.output_diff_str = out;
            return;
        }

        for (const auto &[w, b] : diff.od)
        {
            if (w >= WORDS || b >= word_size)
                throw std::runtime_error("Mask out of range in build_output_diff_str()");

            // MSB-first inside each word:
            // b = 0 means MSB, b = word_size-1 means LSB
            const std::size_t pos = w * word_size + (word_size - 1 - b);

            out[pos] = '1';
        }

        diff.output_diff_str = out;
    }
    /**
     * ---------------------- bin_to_hex ----------------------
     * Convert a binary string ("0" / "1") into a hex string.
     *
     * Length must be a multiple of 4 (one nibble).
     * No whitespace, no separators allowed.
     * Output is uppercase hex, no "0x" prefix.
     * Used to shorten very long bitstrings (e.g., 512-bit diff)
     *  when printing input/output differences in run-config.
     */
    inline std::string bin_to_hex(const std::string &bin)
    {
        if (bin.size() % 4 != 0)
            throw std::runtime_error("Binary string length must be a multiple of 4 to convert to hex.");

        static const char *hexmap = "0123456789ABCDEF";
        std::string out;
        out.reserve(bin.size() / 4);

        for (size_t i{0}; i < bin.size(); i += 4)
        {
            int v = (bin[i] - '0') << 3 |
                    (bin[i + 1] - '0') << 2 |
                    (bin[i + 2] - '0') << 1 |
                    (bin[i + 3] - '0');
            out.push_back(hexmap[v & 0xF]);
        }
        return out;
    }
}

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

    // inline std::string formatValueInTwoPower(double v,
    //                                          int &auto_width,
    //                                          bool ten_power = false,
    //                                          bool dec = true,
    //                                          int prec = 5)
    // {
    //     std::ostringstream ss;
    //     double lg2 = std::log2(std::fabs(v));
    //     double lg10 = std::log10((std::fabs(v)));

    //     if (!v)
    //         ss << std::setprecision(prec) << 0.0;

    //     else
    //     {
    //         if (dec)
    //             ss << std::setprecision(prec) << v;
    //         ss << " ~ 2^{" << std::setprecision(2) << lg2 << "}";

    //         if (ten_power)
    //         {
    //             ss << " ~ (10^{" << std::setprecision(2) << lg10 << "})";
    //         }
    //     }
    //     return ss.str();
    // }

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

    /// Pretty end banner including wall & CPU duration since start/reset.
    std::string end_message() const
    {
        const long long wall_ms = elapsed_ms();
        const double cpu = cpu_ms();

        std::ostringstream ss;
        ss << std::left << std::setw(35) << "Wall time elapsed " << " : " << display::formatMSduration(wall_ms) << ".\n";
        ss << std::left << std::setw(35) << "CPU time used     " << " : " << std::fixed << std::setprecision(3) << cpu << " ms.\n";
        ss << display::formatTime("Execution ended");
        return ss.str();
    }
};

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