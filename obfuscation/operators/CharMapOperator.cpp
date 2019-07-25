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

#include "CharMapOperator.hpp"

std::unordered_map<char, std::vector<char>> const SentenceSplitAndRunOnOperator::s_characterTranslationMap = {
        {',', {';', '.'}},
        {'.', {',', '!'}},
        {':', {'.', ';'}},
        {'!', {'.', ','}},
        {'?', {'.'}}
};

SentenceSplitAndRunOnOperator::SentenceSplitAndRunOnOperator(std::string const& name, double cost, std::string const& description)
        : ObfuscationOperator(name, cost, description)
{
}

std::unique_ptr<search::generic::Operator<State, Context>> SentenceSplitAndRunOnOperator::clone() const
{
    return std::make_unique<SentenceSplitAndRunOnOperator>(name(), cost(), description());
}

std::unordered_set<State> SentenceSplitAndRunOnOperator::applyImpl(const ObfuscationOperator::FocusPoint& focusPoint,
                                                                   State const& state, Context& context) const
{
    std::unordered_set<State> successors;
    auto const startPos = focusPoint.text->begin() + focusPoint.ngramOffset;

    for (std::size_t i = 0; i < NgramProfile::ORDER; ++i) {
        auto const replPos = startPos + i;
        if (replPos >= focusPoint.text->end()) {
            break;
        }

        auto const mapping = s_characterTranslationMap.find(*(startPos + i));
        if (mapping == s_characterTranslationMap.end()) {
            continue;
        }
        auto const& variants = mapping->second;
        char const repl = variants[std::rand() % variants.size()];

        State successor(*state.mutableMetaData());
        if (updateSuccessor(state, successor, focusPoint, replPos, replPos + 1, std::string(1, repl))) {
            successors.emplace(std::move(successor));
        }
    }

    return successors;
}
