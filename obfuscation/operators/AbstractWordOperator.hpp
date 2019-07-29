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

#ifndef OBFUSCATION_SEARCH_ABSTRACTWORD_HPP
#define OBFUSCATION_SEARCH_ABSTRACTWORD_HPP

#include "ObfuscationOperator.hpp"

#include <string>
#include <mutex>
#include <deque>
//#include <netspeak/NetspeakRS3.hpp>
#include <boost/compute/detail/lru_cache.hpp>

namespace bd = boost::compute::detail;

/**
 * Abstract base class for word-based operators.
 */
class AbstractWordOperator: public ObfuscationOperator {
public:
    AbstractWordOperator(std::string const& name, double cost, std::string const& description);

protected:
//    typedef std::shared_ptr<netspeak::generated::Response> NetspeakResponse;
    typedef std::pair<StrPos, StrPos> WordBounds;
    typedef std::vector<WordBounds> WordBoundsList;
    typedef std::pair<WordBoundsList, WordBoundsList> WordBoundsListPair;
    typedef std::unordered_map<std::string, std::vector<std::string>> Dictionary;

    static WordBoundsListPair parseWordBounds(FocusPoint const& focusPoint, std::size_t wordsBefore, std::size_t wordsAfter);
    static inline bool isWordBoundary(char c);
    static inline StrPos parseWordStart(std::string const& text, StrPos pos);
    static inline StrPos parseWordEnd(std::string const& text, StrPos pos);
    static std::shared_ptr<Dictionary const> loadDictionary(std::string const& dictFile, char separator = '\t');

private:
    static std::mutex s_dictMutex;
    static std::unordered_map<std::string, std::shared_ptr<Dictionary>> s_dictionaries;
    static std::mutex s_boundsCacheMutex;
    static bd::lru_cache<std::string, WordBoundsListPair> s_cachedWordBounds;
};

#endif //OBFUSCATION_SEARCH_ABSTRACTWORD_HPP
