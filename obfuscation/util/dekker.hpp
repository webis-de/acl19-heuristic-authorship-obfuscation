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

#ifndef OBFUSCATION_SEARCH_DEKKER_HPP
#define OBFUSCATION_SEARCH_DEKKER_HPP

#include <cmath>

/**
 * Dekker (1971) accurate summation.
 */
namespace dekker
{

template<typename T>
struct Double
{
    T hi;
    T lo;

    Double()
            : hi(), lo() {}

    Double(T const& x)
            : hi(x), lo() {}

    Double(T const& x, T const& y)
            : hi(x), lo(y) {}

    Double(Double<T> const& other) = default;

    inline explicit operator T() const
    {
        return hi;
    }

    inline Double<T>& operator+=(Double<T> const& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    inline Double<T>& operator-=(Double<T> const& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    inline Double<T> operator+(Double<T> const& rhs) const
    {
        T z, zz, r, s;
        r = hi + rhs.hi;
        s = std::abs(hi) > std::abs(rhs.hi) ?
            hi - r + rhs.hi + rhs.lo + lo :
            rhs.hi - r + hi + lo + rhs.lo;
        z = r + s;
        zz = r - z + s;

        return Double<T>(z, zz);
    }

    inline Double<T> operator-() const
    {
        return Double<T>(-hi, -lo);
    }

    inline Double<T> operator-(Double<T> const& rhs) const
    {
        return *this + (-rhs);
    }
};

}   // namespace dekker

#endif //OBFUSCATION_SEARCH_DEKKER_HPP
