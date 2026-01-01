/*
 * REFERENCE IMPLEMENTATION OF Salsa.h header file
 *
 * Filename: salsa.h
 *
 * created: 23/9/23
 * updated: 01/12/25
 *
 * by Hiren
 * Researcher
 *
 *
 * Synopsis:
 * This file contains functions that implement the bare minimum of SALSA cipher
 */

#pragma once
#include "pnbutility.hpp"

constexpr size_t KEYWORD_COUNT = 8;
constexpr size_t WORD_SIZE = 32;
constexpr size_t STATEWORD_COUNT = 16; // state is formed by sixteen 32-bit words

constexpr size_t SALSA_IV_START = 6;
constexpr size_t SALSA_IV_END = 9;

// ---------------------------QR-----------------------------------
#define QR_7(a, b, c, d, X)                     \
    do                                          \
    {                                           \
        if ((X))                                \
            (b) ^= ROTATE_LEFT(((a) ^ (d)), 7); \
        else                                    \
            (b) ^= ROTATE_LEFT(((a) + (d)), 7); \
    } while (0)

#define QR_9(a, b, c, d, X)                     \
    do                                          \
    {                                           \
        if ((X))                                \
            (c) ^= ROTATE_LEFT(((b) ^ (a)), 9); \
        else                                    \
            (c) ^= ROTATE_LEFT(((b) + (a)), 9); \
    } while (0)

#define QR_13(a, b, c, d, X)                     \
    do                                           \
    {                                            \
        if ((X))                                 \
            (d) ^= ROTATE_LEFT(((c) ^ (b)), 13); \
        else                                     \
            (d) ^= ROTATE_LEFT(((c) + (b)), 13); \
    } while (0)


#define QR_18(a, b, c, d, X)                     \
    do                                           \
    {                                            \
        if ((X))                                 \
            (a) ^= ROTATE_LEFT(((d) ^ (c)), 18); \
        else                                     \
            (a) ^= ROTATE_LEFT(((d) + (c)), 18); \
    } while (0)

#define UQR_18(a, b, c, d, X)                    \

#define QR_7_9(a, b, c, d, X)          \
    do                                 \
    {                                  \
        QR_7((a), (b), (c), (d), (X)); \
        QR_9((a), (b), (c), (d), (X)); \
    } while (0)

#define QR_9_7(a, b, c, d, X)          \
    do                                 \
    {                                  \
        QR_9((a), (b), (c), (d), (X)); \
        QR_7((a), (b), (c), (d), (X)); \
    } while (0)
#define QR_13_18(a, b, c, d, X)         \
    do                                  \
    {                                   \
        QR_13((a), (b), (c), (d), (X)); \
        QR_18((a), (b), (c), (d), (X)); \
    } while (0)

#define QR_18_13(a, b, c, d, X)         \
    do                                  \
    {                                   \
        QR_18((a), (b), (c), (d), (X)); \
        QR_13((a), (b), (c), (d), (X)); \
    } while (0)

#define QR_7_9_13_18(a, b, c, d, X)        \
    do                                     \
    {                                      \
        QR_7_9((a), (b), (c), (d), (X));   \
        QR_13_18((a), (b), (c), (d), (X)); \
    } while (0)

#define QR_18_13_9_7(a, b, c, d, X)        \
    do                                     \
    {                                      \
        QR_18_13((a), (b), (c), (d), (X));   \
        QR_9_7((a), (b), (c), (d), (X)); \
    } while (0)

class QR
{
public:
    void ODDARX_7(u32 *x)
    {
        QR_7(x[0], x[4], x[8], x[12], false);
        QR_7(x[5], x[9], x[13], x[1], false);
        QR_7(x[10], x[14], x[2], x[6], false);
        QR_7(x[15], x[3], x[7], x[11], false);
    }

    void EVENARX_7(u32 *x)
    {
        QR_7(x[0], x[1], x[2], x[3], false);
        QR_7(x[5], x[6], x[7], x[4], false);
        QR_7(x[10], x[11], x[8], x[9], false);
        QR_7(x[15], x[12], x[13], x[14], false);
    }
    void ODDARX_9(u32 *x)
    {
        QR_9(x[0], x[4], x[8], x[12], false);
        QR_9(x[5], x[9], x[13], x[1], false);
        QR_9(x[10], x[14], x[2], x[6], false);
        QR_9(x[15], x[3], x[7], x[11], false);
    }
    void EVENARX_9(u32 *x)
    {
        QR_9(x[0], x[1], x[2], x[3], false);
        QR_9(x[5], x[6], x[7], x[4], false);
        QR_9(x[10], x[11], x[8], x[9], false);
        QR_9(x[15], x[12], x[13], x[14], false);
    }

    void ODDARX_13(u32 *x)
    {
        QR_13(x[0], x[4], x[8], x[12], false);
        QR_13(x[5], x[9], x[13], x[1], false);
        QR_13(x[10], x[14], x[2], x[6], false);
        QR_13(x[15], x[3], x[7], x[11], false);
    }

    void EVENARX_13(u32 *x)
    {
        QR_13(x[0], x[1], x[2], x[3], false);
        QR_13(x[5], x[6], x[7], x[4], false);
        QR_13(x[10], x[11], x[8], x[9], false);
        QR_13(x[15], x[12], x[13], x[14], false);
    }

    void ODDARX_18(u32 *x)
    {
        QR_18(x[0], x[4], x[8], x[12], false);
        QR_18(x[5], x[9], x[13], x[1], false);
        QR_18(x[10], x[14], x[2], x[6], false);
        QR_18(x[15], x[3], x[7], x[11], false);
    }

    void EVENARX_18(u32 *x)
    {
        QR_18(x[0], x[1], x[2], x[3], false);
        QR_18(x[5], x[6], x[7], x[4], false);
        QR_18(x[10], x[11], x[8], x[9], false);
        QR_18(x[15], x[12], x[13], x[14], false);
    }

    void UEVENARX_18(u32 *x)
    {
        UQR_18(x[0], x[1], x[2], x[3], false);
        UQR_18(x[5], x[6], x[7], x[4], false);
        UQR_18(x[10], x[11], x[8], x[9], false);
        UQR_18(x[15], x[12], x[13], x[14], false);
    }

} qr;

// -------------------------------------- RoundFunctionDefinition --------------------------------------
// forward round function of Salsa
class FORWARD
{
public:
    // XOR version of full round functions, round means even or odd round
    void XRoundFunction(u32 *x, u32 round)
    {
        if (round & 1)
        {
            QR_7_9_13_18(x[0], x[4], x[8], x[12], true);  // column 1
            QR_7_9_13_18(x[5], x[9], x[13], x[1], true);  // column 2
            QR_7_9_13_18(x[10], x[14], x[2], x[6], true); // column 3
            QR_7_9_13_18(x[15], x[3], x[7], x[11], true); // column 4
        }
        else
        {
            QR_7_9_13_18(x[0], x[1], x[2], x[3], true);     // row 1
            QR_7_9_13_18(x[5], x[6], x[7], x[4], true);     // row 2
            QR_7_9_13_18(x[10], x[11], x[8], x[9], true);   // row 3
            QR_7_9_13_18(x[15], x[12], x[13], x[14], true); // row 4
        }
    }
    void Half_1_EvenRF(u32 *x)
    {
        qr.EVENARX_7(x);
        qr.EVENARX_9(x);
    }
    void Half_1_OddRF(u32 *x)
    {
        qr.ODDARX_7(x);
        qr.ODDARX_9(x);
    }

    void Half_2_EvenRF(u32 *x)
    {
        qr.EVENARX_13(x);
        qr.EVENARX_18(x);
    }

    void Half_2_OddRF(u32 *x)
    {
        qr.ODDARX_13(x);
        qr.ODDARX_18(x);
    }
    // full round function, round means even or odd round
    void RoundFunction(u32 *x, u32 round)
    {
        if (round & 1)
        {
            Half_1_OddRF(x);
            Half_2_OddRF(x);
        }
        else
        {
            Half_1_EvenRF(x);
            Half_2_EvenRF(x);
        }
    }
} frward;

/* bw rounds 18 13 9 7 */
// backward round function of Salsa 
class BACKWARD
{
public:
    // XOR version of full round functions, round means even or odd round
    void XRoundFunction(u32 *x, u32 round)
    {
        if (round & 1)
        {
            QR_18_13_9_7(x[0], x[4], x[8], x[12], true);  // column 1
            QR_18_13_9_7(x[5], x[9], x[13], x[1], true);  // column 2
            QR_18_13_9_7(x[10], x[14], x[2], x[6], true); // column 3
            QR_18_13_9_7(x[15], x[3], x[7], x[11], true); // column 4
        }
        else
        {
            QR_18_13_9_7(x[0], x[1], x[2], x[3], true);     // row 1
            QR_18_13_9_7(x[5], x[6], x[7], x[4], true);     // row 2
            QR_18_13_9_7(x[10], x[11], x[8], x[9], true);   // row 3
            QR_18_13_9_7(x[15], x[12], x[13], x[14], true); // row 4
        }
    }
    void Half_1_EvenRF(u32 *x)
    {
        qr.EVENARX_18(x);
        qr.EVENARX_13(x);
    }
    void Half_1_OddRF(u32 *x)
    {
        qr.ODDARX_18(x);
        qr.ODDARX_13(x);
    }

    void Half_2_EvenRF(u32 *x)
    {
        qr.EVENARX_9(x);
        qr.EVENARX_7(x);
    }

    void Half_2_OddRF(u32 *x)
    {
        qr.ODDARX_9(x);
        qr.ODDARX_7(x);
    }
    // full round function, round means even or odd round
    void RoundFunction(u32 *x, u32 round)
    {
        if (round & 1)
        {
            Half_1_OddRF(x);
            Half_2_OddRF(x);
        }
        else
        {
            Half_1_EvenRF(x);
            Half_2_EvenRF(x);
        }
    }
} bckward;

namespace salsa
{
    u16 column[4][4] = {
        {0, 4, 8, 12}, {5, 9, 13, 1}, {10, 14, 2, 6}, {15, 3, 7, 11}};
    u16 row[4][4] = {{0, 1, 2, 3}, {5, 6, 7, 4}, {10, 11, 8, 9}, {15, 12, 13, 14}};
    void init_iv_const(u32 *x, bool randflag = true, u32 value = 0)
    {
        x[0] = 0x61707865;
        x[5] = 0x3120646e;
        x[10] = 0x79622d36;
        x[15] = 0x6b206574;
        if (randflag)
        {
            for (size_t index{SALSA_IV_START}; index <= SALSA_IV_END; ++index)
                x[index] = RandomNumber<u32>(); // IV
        }
        else
        {
            for (size_t index{SALSA_IV_START}; index <= SALSA_IV_END; ++index)
                x[index] = value;
        }
    }
    void insert_key(u32 *x, u32 *k)
    {
        for (size_t index{1}; index <= 4; ++index)
            x[index] = k[index - 1];
        for (size_t index{11}; index <= 14; ++index)
            x[index] = k[index - 7];
    }
    // calculates the position of the index in the state matrix
    void calculate_word_bit(u16 index, u16 &WORD, u16 &BIT)
    {
        if ((index / WORD_SIZE) > 3)
        {
            WORD = (index / WORD_SIZE) + 7;
        }
        else
        {
            WORD = (index / WORD_SIZE) + 1;
        }
    }
    template <typename T>
    struct HW_Config
    {
        static_assert(std::is_unsigned_v<T>, "State type must be an unsigned integer type");

        const T *state = nullptr;           // pointer to full state (e.g., 16 words)
        const u16 (*column)[4] = nullptr;   // 4x4 mapping: column[col][i] -> state index
        const u16 (*diagonal)[4] = nullptr; // 4x4 mapping: diagonal[d][i] -> state index
        const u16 (*row)[4] = nullptr;      // 4x4 mapping: row[r][i] -> state index

        u16 column_no = 0; // 0..3
        u16 diag_no = 0;   // 0..3
        u16 row_no = 0;    // 0..3
    };

    // --- internal helper (sum 4 indices from a mapping table) ---
    // template <typename T>
    // static inline int hw_sum4(const T *state, const u16 (*table)[4], u16 which)
    // {
    //   if (!state || !table || which >= 4)
    //     return 0;
    //   int hw = 0;
    //   for (int i = 0; i < 4; ++i)
    //   {
    //     const u16 idx = table[which][i];
    //     hw += ops::hammingWeight(state[idx]);
    //   }
    //   return hw;
    // }

    // --- public helpers you’ll actually call ---
    template <typename T>
    int computeHammingWeight(const HW_Config<T> &cfg)
    {
        static_assert(std::is_unsigned_v<T>, "State type must be an unsigned integer type");

        if (!cfg.state)
            throw std::invalid_argument("HW_Config: state pointer is null.");

        int hw = 0;

        if (cfg.column && cfg.column_no < 4)
        {
            for (int i = 0; i < 4; ++i)
                hw += ops::hammingWeight(cfg.state[cfg.column[cfg.column_no][i]]);
        }
        else if (cfg.diagonal && cfg.diag_no < 4)
        {
            for (int i = 0; i < 4; ++i)
                hw += ops::hammingWeight(cfg.state[cfg.diagonal[cfg.diag_no][i]]);
        }
        else if (cfg.row && cfg.row_no < 4)
        {
            for (int i = 0; i < 4; ++i)
                hw += ops::hammingWeight(cfg.state[cfg.row[cfg.row_no][i]]);
        }
        else
        {
            throw std::invalid_argument("HW_Config: No valid mapping provided.");
        }

        return hw;
    }

    struct InitKey
    {
        // randflag = true, means random key values, otherwise key = value
        void key_256bit(u32 *k, bool random_flag = true, u32 value = 0)
        {
            if (random_flag)
            {
                for (size_t index{0}; index < KEYWORD_COUNT; ++index)
                    k[index] = RandomNumber<u32>();
            }
            else
            {
                for (size_t index{0}; index < KEYWORD_COUNT; ++index)
                    k[index] = value;
            }
        }
        void key_128bit(u32 *k, bool random_flag = true, u32 value = 1)
        {
            if (random_flag)
            {
                for (size_t index{0}; index < KEYWORD_COUNT / 2; ++index)
                {
                    k[index] = RandomNumber<u32>();
                    k[index + 4] = k[index];
                }
            }
            else
            {
                for (size_t index{0}; index < KEYWORD_COUNT / 2; ++index)
                {
                    k[index] = value;
                    k[index + 4] = k[index];
                }
            }
        }
    };
}
// namespace Salsa
