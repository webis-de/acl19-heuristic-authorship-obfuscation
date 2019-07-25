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

#include "NgramRemovalOperator.hpp"

#include <algorithm>

NgramRemovalOperator::NgramRemovalOperator(std::string const& name, double cost, std::string const& description)
        :ObfuscationOperator(name, cost, description)
{
}

std::unique_ptr<search::generic::Operator<State, Context>> NgramRemovalOperator::clone() const
{
    return std::make_unique<NgramRemovalOperator>(name(), cost(), description());
}

std::unordered_set<State> NgramRemovalOperator::applyImpl(FocusPoint const& focusPoint, State const& state, Context& context) const
{
    auto const& text = *focusPoint.text;
    auto const pos = text.begin() + focusPoint.ngramOffset;

    State successor(*state.mutableMetaData());
    if (updateSuccessor(state, successor, focusPoint, pos, pos + NgramProfile::ORDER, "")) {
        return {std::move(successor)};
    }

    return {};
}
