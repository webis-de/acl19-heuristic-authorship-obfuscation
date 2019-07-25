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

#include "NetspeakOperator.hpp"
#include "util/netspeak.hpp"

/**
 * Netspeak request cache mutex.
 */
std::mutex NetspeakOperator::s_netspeakCacheMutex{};

/**
 * Netspeak request cache.
 */
bd::lru_cache<std::string, NetspeakOperator::NetspeakResponse> NetspeakOperator::s_netspeakRequestCache(1000);

NetspeakOperator::NetspeakOperator(std::string const& name, double cost, std::string const& description)
        : AbstractWordOperator(name, cost, description)
{
}

/**
 * Perform a Netspeak request and cache the result.
 * This method is thread-safe.
 *
 * @param request Netspeak request to run
 * @return Netspeak answer
 */
std::shared_ptr<netspeak::generated::Response> NetspeakOperator::netspeakRequest(std::string const& request, std::uint32_t maxResults)
{
    std::lock_guard<std::mutex> lock(s_netspeakCacheMutex);
    auto responseOptional = s_netspeakRequestCache.get(request);
    if (responseOptional) {
        return responseOptional.get();
    }

    netspeak::generated::Request req;
    req.set_query(request);
    req.set_max_phrase_count(maxResults);
    auto const response = netspeak_util::instance()->search(req);

    s_netspeakRequestCache.insert(request, response);
    return response;
}
