/*
 * Copyright 2017-2019 Janek Bevendorff, Webis Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OBFUSCATION_SEARCH_LOGPROB_HPP
#define OBFUSCATION_SEARCH_LOGPROB_HPP

#include <cmath>
#include <cassert>
#include <algorithm>

/**
 * High-precision arithmetic in log space.
 *
 * Based on cpp-logprobs by donovanr <https://github.com/donovanr/cpp-logprobs>
 */
namespace logprob
{

/**
 * Convert normal probability to log space.
 */
template<typename T>
inline double toLog(T x)
{
    return std::log(x);
}

/**
 * Convert from log space to normal probability.
 */
template<typename T>
inline T fromLog(T x)
{
    return std::exp(x);
}

/**
 * Multiply two log probabilities in log space.
 */
template<typename T>
inline T multiply(T x, T y)
{
    return x + y;
}

/**
 * Divide on log probability by another in log space.
 */
template<typename T>
inline T divide(T x, T y)
{
    return x - y;
}

namespace {

/**
 * @return threshold at which additions / subtractions actually matter at the given precision
 */
template<typename T>
constexpr T threshold()
{
    return std::log(static_cast<T>(2.0)) * sizeof(T) * 8.0 + 1.0;
}

}   // namespace

/**
 * Add two log probabilities in log space.
 */
template<typename T>
T add(T x, T y)
{
    // make sure x > y, so that exp(y - x) < 1
    if (y > x) {
        std::swap(x, y);
    }

    if (x - y > threshold<T>()) {
        return x;
    }

    return x + std::log1p(std::exp(y - x));
}

/**
 * Subtract a log probability from another in log space.
 */
template<typename T>
T subtract(T x, T y)
{
    assert(x > y);

    if (x - y > threshold<T>()) {
        return x;
    }

    return x + std::log1p(-std::exp(y - x));
}

}   // namespace logprob

#endif // OBFUSCATION_SEARCH_LOGPROB_HPP
