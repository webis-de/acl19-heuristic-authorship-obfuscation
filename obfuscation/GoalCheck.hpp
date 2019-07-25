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

#ifndef OBFUSCATION_SEARCH_GOALCHECK_HPP
#define OBFUSCATION_SEARCH_GOALCHECK_HPP

#include "Context.hpp"
#include "State.hpp"
#include "search/generic/Node.hpp"
#include <cmath>

class State;
struct Context;

/**
 * Functor for performing goal checks for the heuristic search.
 */
template<typename CostFunc>
class GoalCheck {
public:
    GoalCheck()
        : m_costFunc() {}

    double operator()(search::generic::Node<State> const& node, Context const& context) const
    {
        auto const& state = node.state();

        if (!state.mutableMetaData()->jsd || !context.mutableMetaData->goalJSDist) {
            return false;
        }

        double jsDist = std::sqrt(2.0 * state.mutableMetaData()->jsd.get());
        return node.depth() > 0 && jsDist >= context.mutableMetaData->goalJSDist.get();
    }

private:
    CostFunc const m_costFunc;
};

#endif //OBFUSCATION_SEARCH_GOALCHECK_HPP
