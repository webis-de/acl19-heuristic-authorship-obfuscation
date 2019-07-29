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

#ifndef OBFUSCATION_SEARCH_NETSPEAKOPERATOR_HPP
#define OBFUSCATION_SEARCH_NETSPEAKOPERATOR_HPP

#include "AbstractWordOperator.hpp"

/**
 * Abstract base class for Netspeak-based operators
 */
class NetspeakOperator : public AbstractWordOperator {
public:
    NetspeakOperator(std::string const& name, double cost, std::string const& description);

protected:
//    static std::shared_ptr<netspeak::generated::Response> netspeakRequest(std::string const& request, std::uint32_t maxResults);

private:
//    static std::mutex s_netspeakCacheMutex;
//    static bd::lru_cache<std::string, NetspeakResponse> s_netspeakRequestCache;
};

#endif //OBFUSCATION_SEARCH_NETSPEAKOPERATOR_HPP
