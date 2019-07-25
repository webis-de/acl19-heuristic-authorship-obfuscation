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

#include "CharacterFlipOperator.hpp"

CharacterFlipOperator::CharacterFlipOperator(std::string const& name, double cost, std::string const& description)
        : ObfuscationOperator(name, cost, description)
{
}

std::unique_ptr<search::generic::Operator<State, Context>> CharacterFlipOperator::clone() const
{
    return std::make_unique<CharacterFlipOperator>(name(), cost(), description());
}


std::unordered_set<State> CharacterFlipOperator::applyImpl(const ObfuscationOperator::FocusPoint& focusPoint,
                                                           State const& state, Context& context) const
{
    std::unordered_set<State> successors;
    auto const& text = *focusPoint.text;
    auto const origPos = text.begin() + focusPoint.ngramOffset;

    for (std::size_t  i = 0; i < NgramProfile::ORDER - 1; ++i) {
        auto const startPos = origPos + i;
        auto const endPos = startPos + 2;
        if (endPos >= focusPoint.text->end()) {
            break;
        }

        std::string const orig = std::string(startPos, endPos);
        std::string perm = orig;
        std::swap(perm[0], perm[1]);

        if (perm == orig) {
            continue;
        }

        State successor(*state.mutableMetaData());
        if (updateSuccessor(state, successor, focusPoint, startPos, endPos, perm)) {
            successors.emplace(std::move(successor));
        }
    }

    return successors;
}
