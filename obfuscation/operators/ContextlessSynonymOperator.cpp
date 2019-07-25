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

#include "ContextlessSynonymOperator.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_set>

ContextlessSynonymOperator::ContextlessSynonymOperator(std::string const& name, double cost, std::string const& description)
        : NetspeakOperator(name, cost, description)
{
    std::cout << "Loading synonym dictionary..." << std::endl;
    m_dict = AbstractWordOperator::loadDictionary("assets/synonym-dictionary.tsv");
}

std::unique_ptr<search::generic::Operator<State, Context>> ContextlessSynonymOperator::clone() const
{
    return std::make_unique<ContextlessSynonymOperator>(name(), cost(), description());
}

std::unordered_set<State> ContextlessSynonymOperator::applyImpl(const FocusPoint& focusPoint, State const& state, Context& context) const
{
    if (!m_dict) {
        return {};
    }

    auto const& text = *focusPoint.text;
    auto const focusIt = text.begin() + focusPoint.ngramOffset;

    auto bounds = parseWordBounds(focusPoint, 0, 0).second[0];
    std::string word = boost::to_lower_copy(std::string(bounds.first, bounds.second));

    auto const orderOffset = NgramProfile::ORDER;
    auto ngram = std::string(focusIt, focusIt + orderOffset);

    auto synonyms = m_dict->find(word);
    if (synonyms == m_dict->end()) {
        return {};
    }

    std::unordered_set<State> successorStates;
    for (auto const& synonym: synonyms->second) {
        State successor(*state.mutableMetaData());
        if (updateSuccessor(state, successor, focusPoint, bounds.first, bounds.second, synonym)) {
            successorStates.emplace(std::move(successor));
        }
    }

    return successorStates;
}
