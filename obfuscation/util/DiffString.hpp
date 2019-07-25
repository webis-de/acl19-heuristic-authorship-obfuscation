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

#ifndef OBFUSCATION_SEARCH_DIFFSTRING_HPP
#define OBFUSCATION_SEARCH_DIFFSTRING_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

/**
 * String representation that stores its edit history and re-applies it on the fly to the original
 * text instead of storing the full new text in order to save memory.
 */
class DiffString {
public:
    /**
     * String edit representation
     */
    struct Edit {
        Edit(std::uint32_t editPos, std::uint8_t charsToDelete, std::string insertion)
                : editPos(editPos), charsToDelete(charsToDelete), insertion(std::move(insertion)) {}
        Edit(Edit const& other) = default;
        Edit(Edit&& other) = default;
        Edit& operator=(Edit const& other) = default;
        Edit& operator=(Edit&& other) = default;

        /**
         * Edit start position
         */
        std::uint32_t editPos;

        /**
         * Number of characters to delete from editPos;
         */
        std::uint8_t charsToDelete;

        /**
         * New string to insert instead.
         */
        std::string insertion;
    };

    explicit DiffString(std::shared_ptr<std::string> originalString);
    explicit DiffString(std::string const& string);
    DiffString(DiffString const& other) = default;
    DiffString(DiffString&& other) = default;
    DiffString& operator=(DiffString const& other) = default;
    DiffString& operator=(DiffString&& other) = default;
    DiffString& operator=(std::string const& string);
    DiffString& operator=(std::string&& string);

    std::string hashValue() const;
    bool operator==(DiffString const& rhs) const;

    std::string string() const;
    std::shared_ptr<std::string> source() const;
    std::size_t logSize() const;
    void reset(std::string const& newString);
    void reset(std::string&& newString);
    void reset(std::shared_ptr<std::string> newString);
    void edit(Edit const& edit);
    void edit(Edit const& edit, std::string const& text);
    void apply();

private:
    void updateHash(std::string const& text);

    std::shared_ptr<std::string> m_sourceString;
    std::vector<Edit> m_edits;
    std::string m_hashValue;
};


namespace {
std::hash<std::string> diffStringHash;
}

namespace std {

template<>
struct hash<DiffString> {
    std::size_t operator()(DiffString const& state) const
    {
        return diffStringHash(state.hashValue());
    }
};

} // namespace std

#endif //OBFUSCATION_SEARCH_DIFFSTRING_HPP
