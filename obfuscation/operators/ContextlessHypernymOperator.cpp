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

#include "ContextlessHypernymOperator.hpp"

ContextlessHypernymOperator::ContextlessHypernymOperator(std::string const& name, double cost, std::string const& description)
        : ContextlessSynonymOperator(name, cost, description)
{
    std::cout << "Loading hypernym dictionary..." << std::endl;
    m_dict = AbstractWordOperator::loadDictionary("assets/hypernym-dictionary.tsv");
}

std::unique_ptr<search::generic::Operator<State, Context>> ContextlessHypernymOperator::clone() const
{
    return std::make_unique<ContextlessHypernymOperator>(name(), cost(), description());
}
