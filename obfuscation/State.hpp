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

#ifndef OBFUSCATION_STATE_HPP
#define OBFUSCATION_STATE_HPP

#include "Context.hpp"
#include "ComputeCostH.hpp"
#include "util/NgramProfile.hpp"
#include "util/DiffString.hpp"

#include <boost/optional.hpp>
#include <functional>
#include <cstddef>
#include <memory>
#include <search/generic/Node.hpp>
#include <utility>

class State {
public:
    /**
     * DTO for state meta data.
     */
    struct MetaData
    {
        /**
         * Jensen-Shannon divergence of this state.
         */
        boost::optional<double> jsd;
    };

    typedef std::shared_ptr<std::string> StringPtr;
    typedef std::shared_ptr<std::string const> ConstStringPtr;

    explicit State();
    explicit State(MetaData const& metaData);
    explicit State(MetaData const& metaData, DiffString text);
    explicit State(MetaData const& metaData, StringPtr text, Context::NgramPtr ngramProfile);

    std::string hashValue() const;
    bool operator==(State const& other) const;

    DiffString text() const;

    void setText(StringPtr text, unsigned int flags = 0);
    Context::NgramPtr ngramProfile() const;
    void setNgramProfile(StringPtr text, Context::NgramPtr profile);
    void setNgramProfile(DiffString&& text, Context::NgramPtr profile);

    /**
     * Get a pointer to a mutable meta data DTO for this state.
     * The target DTO may be modified during state evaluation.
     */
    inline std::shared_ptr<MetaData> mutableMetaData() const
    {
        return m_mutableMetaData;
    }

private:
    DiffString m_text;
    Context::NgramPtr m_ngramProfile;
    std::shared_ptr<MetaData> m_mutableMetaData = nullptr;
};


namespace {
std::hash<std::string> stateHash;
}

namespace std {

template<>
struct hash<State> {
    std::size_t operator()(State const& state) const
    {
        return stateHash(state.hashValue());
    }
};

} // namespace std


#endif //OBFUSCATION_STATE_HPP
