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

#ifndef OBFUSCATION_SEARCH_COMPUTECOSTH_HPP
#define OBFUSCATION_SEARCH_COMPUTECOSTH_HPP

#include "Context.hpp"
#include <search/generic/Node.hpp>

class State;

/**
 * Functor for computing the h(n) cost function of the heuristic search.
 */
class ComputeCostH {
public:
    ComputeCostH() = default;
    double operator()(search::generic::Node<State> const& node, Context const& context, bool allowUpdate = true) const;

private:
    double calculateJsd(Context::ConstNgramPtr const& sourceProfile, Context::ConstNgramPtr const& targetProfile) const;
    double calculateJsdUpdate(double previousJsd, std::vector<NgramProfile::NgramUpdate> const& updates,
            Context::ConstNgramPtr const& sourceProfile, Context::ConstNgramPtr const& targetProfile) const;
};

#endif //OBFUSCATION_SEARCH_COMPUTECOSTH_HPP
