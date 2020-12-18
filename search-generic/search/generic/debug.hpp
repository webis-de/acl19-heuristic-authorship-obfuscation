// debug.hpp -*- C++ -*-
// Copyright (C) 2014 Martin Trenkmann
#ifndef SEARCH_GENERIC_DEBUG_HPP
#define SEARCH_GENERIC_DEBUG_HPP

#include <algorithm>
#include <iostream>
#include <iterator>
#include <chrono>
#include <string>

namespace search {
namespace generic {

inline void Pause()
{
    std::cout << "Press any key to continue" << std::endl;
    std::cin.get();
}

template<typename T>
void Print(const T& object)
{
    std::cout << "DEBUG " << object << '\n';
}

template<typename T>
void Print(const std::string& message, const T& object)
{
    std::cout << "DEBUG " << message << " -> " << object << '\n';
}

inline void PrintNewline()
{
    std::cout << '\n';
}

template<typename Sequence>
void PrintSequence(const std::string& message, const Sequence& sequence)
{
    std::cout << "DEBUG " << message << " -> ";
    const auto first = std::begin(sequence);
    std::copy(first, std::end(sequence),
              std::ostream_iterator<decltype(*first)>(std::cout, " "));
    std::cout << '\n';
}

inline std::size_t timestamp()
{
    return static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}

}  // namespace generic
}  // namespace search

#ifdef DEBUG_OUTPUT_ENABLED

#define DEBUG_PAUSE() search::generic::Pause()
#define DEBUG_PRINT(MESSAGE) search::generic::Print(MESSAGE)
#define DEBUG_PRINT2(MESSAGE, OBJECT) search::generic::Print(MESSAGE, OBJECT)
#define DEBUG_PRINT_NEWLINE() search::generic::PrintNewline()
#define DEBUG_PRINT_SEQUENCE(MESSAGE, SEQUENCE) \
  search::generic::PrintSequence(MESSAGE, SEQUENCE)

#else

#define DEBUG_PAUSE() ((void)0)
#define DEBUG_PRINT(MESSAGE) ((void)0)
#define DEBUG_PRINT2(MESSAGE, OBJECT) ((void)0)
#define DEBUG_PRINT_NEWLINE() ((void)0)
#define DEBUG_PRINT_SEQUENCE(MESSAGE, SEQUENCE) ((void)0)

#endif  // DEBUG_OUTPUT_ENABLED

#endif  // SEARCH_GENERIC_DEBUG_HPP
