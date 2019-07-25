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

#include "util/netspeak.hpp"
#include "WordRemovalOperator.hpp"

WordRemovalOperator::WordRemovalOperator(std::string const& name, double cost, std::string const& description)
        : NetspeakOperator(name, cost, description)
{
}

std::unique_ptr<search::generic::Operator<State, Context>> WordRemovalOperator::clone() const
{
    return std::make_unique<WordRemovalOperator>(name(), cost(), description());
}

std::unordered_set<State> WordRemovalOperator::applyImpl(ObfuscationOperator::FocusPoint const& focusPoint,
        State const& state, Context& context) const
{
    std::unordered_set<State> successorStates;

    for (int offset = -1; offset < 2; ++offset) {
        auto wordBoundsLists = parseWordBounds(focusPoint, 2 + offset, 2 - offset);

        if (wordBoundsLists.first.empty() || wordBoundsLists.second.size() < 2) {
            continue;
        }

        // build query
        std::string query;
        for (auto const& bounds: wordBoundsLists.first) {
            query.append(bounds.first, bounds.second).append(" ");
        }
        bool first = true;
        for (auto const& bounds: wordBoundsLists.second) {
            if (first) {
                // skip original word
                first = false;
                continue;
            }
            query.append(bounds.first, bounds.second).append(" ");
        }

        // process response
        auto const response = netspeakRequest(query, 5);
        auto& delBounds = wordBoundsLists.second[0];

        for (auto const& phrase: response->phrase()) {
            if (phrase.frequency() < 50000) {
                continue;
            }
            State successor(*state.mutableMetaData());
            if (updateSuccessor(state, successor, focusPoint, delBounds.first, delBounds.second, "")) {
                successorStates.emplace(std::move(successor));
            }
        }
    }
    return successorStates;
}
