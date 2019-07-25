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

#ifndef OBFUSCATION_SEARCH_OBFUSCATOR_HPP
#define OBFUSCATION_SEARCH_OBFUSCATOR_HPP

#include "State.hpp"
#include "Context.hpp"
#include "util/LayeredOStream.hpp"

#include <search/generic/Operator.hpp>
#include <search/generic/Status.hpp>
#include <sstream>

class Obfuscator {
public:
    typedef search::generic::Operator<State, Context> Operator;
    typedef search::generic::Status<State, Context> Status;

    void obfuscate(std::stringstream& input, LayeredOStream& output, Context::NgramPtr targetDist, unsigned int flags = 0);
};

#endif //OBFUSCATION_SEARCH_OBFUSCATOR_HPP
