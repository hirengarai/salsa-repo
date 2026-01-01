#pragma once

#include <limits>
#include <random>

inline std::mt19937 &thread_rng()
{
    // Lazy init to avoid any startup stall if random_device blocks.
    thread_local std::mt19937 gen{std::random_device{}()};
    return gen;
}

template <typename T>
T RandomNumber(T min = 0, T max = std::numeric_limits<T>::max())
{
    std::uniform_int_distribution<T> dis(min, max);
    return dis(thread_rng());
}

// a function which generates a random boolean value
inline bool RandomBoolean()
{
    std::bernoulli_distribution dis(0.5);
    return dis(thread_rng());
}
