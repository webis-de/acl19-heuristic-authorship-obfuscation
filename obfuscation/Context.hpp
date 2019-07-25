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

#ifndef OBFUSCATION_CONTEXT_HPP
#define OBFUSCATION_CONTEXT_HPP

#include "util/NgramProfile.hpp"

#include <boost/optional.hpp>

/**
 * Global search context.
 */
struct Context {
    /**
     * DTO for context meta data.
     */
    struct MetaData
    {
        boost::optional<std::size_t> originalTextLength;
        boost::optional<double> originalJsd;
        boost::optional<double> goalJSDist;
    };

    typedef std::shared_ptr<NgramProfile> NgramPtr;
    typedef std::shared_ptr<NgramProfile const> ConstNgramPtr;

    Context() = default;
    explicit Context(ConstNgramPtr targetProfile);
    Context(Context const& other) = default;
    Context(Context&& other) noexcept = default;
    Context& operator=(Context const& other) = default;
    Context& operator=(Context&& other) noexcept = default;

    ConstNgramPtr targetNgramProfile;

    /**
     * Pointer to mutable meta data for this context.
     * The target object may be modified during execution.
     */
    std::shared_ptr<MetaData> mutableMetaData = std::make_shared<MetaData>();
};

#endif // OBFUSCATION_CONTEXT_HPP
