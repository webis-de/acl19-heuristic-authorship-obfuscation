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

#include "AbstractWordOperator.hpp"

/**
 * Mutex for static bounds cache access.
 */
std::mutex AbstractWordOperator::s_boundsCacheMutex{};

/**
 * Cached operator working data.
 */
bd::lru_cache<std::string, AbstractWordOperator::WordBoundsListPair> AbstractWordOperator::s_cachedWordBounds{500};

/**
 * Mutex for static dictionary access.
 */
std::mutex AbstractWordOperator::s_dictMutex{};

/**
 * Loaded word lists.
 */
std::unordered_map<std::string, std::shared_ptr<AbstractWordOperator::Dictionary>> AbstractWordOperator::s_dictionaries{};


AbstractWordOperator::AbstractWordOperator(std::string const& name, double cost, std::string const& description)
        : ObfuscationOperator(name, cost, description)
{
}

/**
 * Check if a character represents a word boundary.
 *
 * @param c character
 * @return true if character is a non-word character
 */
bool AbstractWordOperator::isWordBoundary(char c)
{
    return std::isspace(c) || std::ispunct(c);
}

/**
 * Get an iterator to the beginning of the current word.
 * If <tt>pos</tt> points at a word boundary, the beginning of the next word is returned
 * and if there is no next word, the iterator is returned unchanged.
 *
 * The returned position can be less than, equal to, or greater than the original position.
 *
 * @param text input text
 * @param pos iterator to the current position
 * @param numWords number of previous words to skip
 * @return iterator
 */
AbstractWordOperator::StrPos AbstractWordOperator::parseWordStart(std::string const& text, std::string::const_iterator pos)
{
    if (pos >= text.end() || pos <= text.begin()) {
        return pos;
    }

    // navigate to the beginning of the next word if we
    // started on a word boundary character
    bool isBoundary = isWordBoundary(*pos);
    auto origPos = pos;
    while (isBoundary) {
        ++pos;
        if (pos >= text.end()) {
            return origPos;
        }
        isBoundary = isWordBoundary(*pos);
        if (!isBoundary) {
            return pos;
        }
    }

    // navigate to the beginning of the current word or the beginning of the text
    while (pos > text.begin()) {
        --pos;
        if (isWordBoundary(*pos)) {
            // step one forward again to point at actual word beginning
            ++pos;
            break;
        }
    }

    return pos;
}

/**
 * Get an iterator past the end of the current word.
 * If <tt>pos</tt> points at a word boundary, an iterator past the end of the previous word is returned
 * and if there is no previous word, the iterator is returned unchanged.
 *
 * The returned position can be less than, equal to, or greater than the original position.
 *
 * @param text input text
 * @param pos iterator to the current position
 * @return iterator
 */
AbstractWordOperator::StrPos AbstractWordOperator::parseWordEnd(std::string const& text, std::string::const_iterator pos)
{
    if (pos >= text.end() || pos <= text.begin()) {
        return pos;
    }

    // navigate to the end of the previous word if we
    // started on a word boundary character
    bool isBoundary = isWordBoundary(*pos);
    auto origPos = pos;
    while (isBoundary) {
        --pos;
        if (pos <= text.begin()) {
            return origPos;
        }
        isBoundary = isWordBoundary(*pos);
        if (!isBoundary) {
            return pos + 1;
        }
    }

    // navigate to the end of the current word or the end of the text
    while (pos < text.end()) {
        ++pos;
        if (isWordBoundary(*pos)) {
            break;
        }
    }

    return pos;
}

/**
 * Get two vectors containing \link WordBounds for the <tt>wordsBefore</tt> and <tt>wordsAfter</tt> next words
 * parsed from the text around the given \link FocusPoint.
 * The first element of the second vector is always the current word. The first vector can be empty if
 * <tt>wordsBefore</tt> is 0.
 * Results are cached, so successive calls with identical parameters may be served faster.
 *
 * @param focusPoint operator focus point
 * @param wordsBefore number of words before the current one
 * @param wordsAfter number of words after the current one
 * @return pair of vectors containing \link WordBounds
 */
AbstractWordOperator::WordBoundsListPair AbstractWordOperator::parseWordBounds(
        FocusPoint const& focusPoint, std::size_t wordsBefore, std::size_t wordsAfter)
{
    auto const& text = *focusPoint.text;
    auto pos = text.begin() + focusPoint.ngramOffset;

    auto const cacheKey = std::to_string(reinterpret_cast<std::size_t>(&*pos))
            .append(":")
            .append(std::to_string(wordsBefore))
            .append(":")
            .append(std::to_string(wordsAfter));
    {
        std::lock_guard<std::mutex> lock(s_boundsCacheMutex);
        auto cached = s_cachedWordBounds.get(cacheKey);
        if (cached) {
            return cached.get();
        }
    }

    std::vector<WordBounds> boundsBefore;
    boundsBefore.reserve(wordsBefore);
    std::vector<WordBounds> boundsAfter;
    boundsAfter.reserve(wordsAfter + 1);

    auto start = parseWordStart(text, pos);
    auto end = parseWordEnd(text, start);
    boundsAfter.emplace_back(start, end);

    while (wordsAfter > 0 && end < text.end()) {
        auto nextStart = parseWordStart(text, end + 1);
        auto nextEnd = parseWordEnd(text, nextStart);
        if (nextEnd <= nextStart || start == nextStart) {
            break;
        }
        start = nextStart;
        end = nextEnd;
        boundsAfter.emplace_back(start, end);
        --wordsAfter;
    }

    start = boundsAfter[0].first;
    while (wordsBefore > 0 && start > text.begin()) {
        auto prevEnd = parseWordEnd(text, start - 1);
        auto prevStart = parseWordStart(text, prevEnd - 1);
        if (prevEnd <= prevStart || start == prevStart) {
            break;
        }
        start = prevStart;
        end = prevEnd;
        boundsBefore.emplace_back(start, end);
        --wordsBefore;
    }
    std::reverse(boundsBefore.begin(), boundsBefore.end());

    auto pair = std::make_pair(boundsBefore, boundsAfter);
    std::lock_guard<std::mutex> lock(s_boundsCacheMutex);
    s_cachedWordBounds.insert(cacheKey, pair);
    return pair;
}

/**
 * Load and cache a dictionary. A dictionary maps a specific word to a list of alternative words.
 * Successive calls to this function will return the same dictionary instance.
 * This method is thread-safe.
 *
 * @param dictFile path to input file
 * @param separator entry separator inside the input file
 * @return shared pointer to dictionary or nullptr if dictionary could not be loaded
 */
std::shared_ptr<AbstractWordOperator::Dictionary const> AbstractWordOperator::loadDictionary(std::string const& dictFile, char separator)
{
    std::lock_guard<std::mutex> lock(s_dictMutex);

    auto dictIt = s_dictionaries.find(dictFile);
    if (dictIt != s_dictionaries.end()) {
        return dictIt->second;
    }

    std::ifstream stream;
    stream.open(dictFile);
    if (!stream) {
        std::cerr << "Could not open file '" << dictFile << "'" << std::endl;
        return nullptr;
    }

    auto dictPtr = std::make_shared<Dictionary>();
    auto& dict = *dictPtr;

    std::string line;
    while (std::getline(stream, line)) {
        std::deque<std::string> tokens;
        boost::split(tokens, line, [separator](char c) { return c == separator; });

        if (tokens.size() < 2) {
            continue;
        }

        std::string key = boost::to_lower_copy(tokens[0]);
        tokens.pop_front();
        dict[key] = {std::make_move_iterator(tokens.begin()), std::make_move_iterator(tokens.end())};
    }

    s_dictionaries[dictFile] = dictPtr;
    return dictPtr;
}
