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

#ifndef OBFUSCATION_OPERATORS_OBFUSCATIONOPERATOR_HPP
#define OBFUSCATION_OPERATORS_OBFUSCATIONOPERATOR_HPP

#include "State.hpp"
#include "Context.hpp"

#include <search/generic/Operator.hpp>
#include <mutex>
#include <vector>
#include <string>
#include <utility>
#include <boost/optional.hpp>
#include <boost/compute/detail/lru_cache.hpp>

namespace bd = boost::compute::detail;

/**
 * Pure virtual obfuscation operator base class.
 */
class ObfuscationOperator : public search::generic::Operator<State, Context> {
public:
    typedef std::string::const_iterator StrPos;

    ObfuscationOperator(std::string const& name, double cost, std::string const& description);
    std::unordered_set<State> apply(State const& state, Context& context) const final;

    /**
     * Maximum n-gram rank to consider for producing successors.
     */
    static std::size_t constexpr MAX_NGRAM_RANK = 10;

    /**
     * Maximum number of n-gram occurrences that an operator will be applied on.
     */
    static std::size_t constexpr MAX_OCCURRENCES = 2;

    /**
     * Maximum number successors an operator will generate.
     */
    static std::size_t constexpr MAX_SUCCESSORS = 6;

protected:
    /**
     * Focus point inside a text to run an operator on.
     */
    struct FocusPoint {
        /**
         * Integer offset of the n-gram of interest.
         */
        long ngramOffset = 0;

        /**
         * Source text.
         */
        std::string const* text = nullptr;
    };

    /**
     * Concrete operator implementation.
     *
     * @param focusPoint operator focus point inside the text
     * @param state current search node state
     * @param context search context
     * @return generated successor states
     */
    virtual std::unordered_set<State> applyImpl(FocusPoint const& focusPoint,
            State const& state, Context& context) const = 0;

    virtual bool updateSuccessor(State const& origState, State& successor, FocusPoint const& focusPoint,
            StrPos const& editStart, StrPos const& editEnd, std::string const& update) const;

private:
    /**
     * DTO for n-grams ranked by a specific criterion
     */
    struct NgramRank
    {
        NgramProfile::Ngram ngram;
        float rank;

        NgramRank(NgramProfile::Ngram ngram, float rank);
        bool operator<(NgramRank rhs) const;
    };

    /**
     * Cache object for operator working data.
     */
    struct CacheData
    {
        std::shared_ptr<std::vector<std::string::const_iterator>> ngramPositions;
        std::shared_ptr<std::string> sourceText;
    };

    boost::optional<CacheData> getCachedNgramSelection(State const& state, Context const& context) const;
    std::vector<NgramRank> rankNgrams(Context::ConstNgramPtr sourceProfile,Context::ConstNgramPtr targetProfile) const;

    static std::mutex s_cacheMutex;
    static bd::lru_cache<std::string, CacheData> s_cachedData;
};

#endif // OBFUSCATION_OPERATORS_OBFUSCATIONOPERATOR_HPP
