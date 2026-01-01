#pragma once

#include "config.hpp"
#include "types.hpp"

#include <algorithm>
#include <bit>
#include <bitset>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

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
