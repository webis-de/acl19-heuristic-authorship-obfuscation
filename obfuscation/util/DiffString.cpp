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

#include "DiffString.hpp"
#include <openssl/md5.h>

#include <cassert>

DiffString::DiffString(std::shared_ptr<std::string> originalString)
    : m_sourceString(std::move(originalString))
{
    updateHash(*m_sourceString);
}

DiffString::DiffString(std::string const& string)
{
    reset(std::make_shared<std::string>(string));
}

DiffString& DiffString::operator=(std::string const& string)
{
    reset(std::make_shared<std::string>(string));
    return *this;
}

DiffString& DiffString::operator=(std::string&& string)
{
    reset(std::make_shared<std::string>(std::move(string)));
    return *this;
}

std::string DiffString::hashValue() const
{
    return m_hashValue;
}

void DiffString::updateHash(std::string const& text)
{
    m_hashValue = std::string(MD5_DIGEST_LENGTH, 0);
    MD5(reinterpret_cast<const unsigned char*>(&text[0]),
        text.size(), reinterpret_cast<unsigned char*>(&m_hashValue[0]));
    m_hashValue.shrink_to_fit();
}

bool DiffString::operator==(DiffString const& rhs) const
{
    return string() == rhs.string();
}

/**
 * @return current string with applied edit history
 */
std::string DiffString::string() const
{
    std::string edited = *m_sourceString;
    for (auto const& edit: m_edits) {
        assert(edit.editPos < edited.length() && edit.editPos + edit.charsToDelete <= edited.length());
        edited.erase(edit.editPos, edit.charsToDelete);
        edited.insert(edit.editPos, edit.insertion);
    }

    return edited;
}

/**
 * @return pointer to the original source string
 */
std::shared_ptr<std::string> DiffString::source() const
{
    return m_sourceString;
}

/**
 * @return edit log size
 */
std::size_t DiffString::logSize() const
{
    return m_edits.size();
}

/**
 * Forget all edits and reset source string to <tt>newString</tt>.
 *
 * @param newString new source string
 */
void DiffString::reset(std::string const& newString)
{
    reset(std::make_shared<std::string>(newString));
}

/**
 * Forget all edits and reset source string to <tt>newString</tt>.
 *
 * @param newString new source string
 */
void DiffString::reset(std::string&& newString)
{
    reset(std::make_shared<std::string>(std::move(newString)));
}

/**
 * Forget all edits and reset source string to <tt>newString</tt>.
 *
 * @param newString new source string
 */
void DiffString::reset(std::shared_ptr<std::string> newString)
{
    m_sourceString = std::move(newString);
    m_edits.clear();
    updateHash(*m_sourceString);
}

/**
 * Add an edit to the string edit history.
 * WARNING: this method can be expensive, because it needs to calculate a temporary version of the full
 * text to calculate the hash. If you already have a full version of the new text,
 * use \link edit(DiffString::Edit const&, std::size_t, int) instead.
 *
 * No bounds checks are done. You are responsible for making sure edit positions point to valid
 * positions in the string.
 *
 * @param edit new edit to apply
 */
void DiffString::edit(DiffString::Edit const& edit)
{
    m_edits.emplace_back(edit);
    updateHash(string());
}

/**
 * Add an edit to the string edit history and update the DiffString's hash value
 * from the given text.
 *
 * No bounds checks are done. You are responsible for making sure edit positions point to valid
 * positions in the string.
 *
 * @param edit new edit to apply
 * @param text new text for calculating the new hash
 * @param newHash text to calculate the hash value from
 */
void DiffString::edit(DiffString::Edit const& edit, std::string const& text)
{
    m_edits.emplace_back(edit);
    updateHash(text);
}

/**
 * Apply all previous edits, generate a new source string from it and clear the edit history.
 * Use this to trade memory for performance if the edit history becomes too long.
 */
void DiffString::apply()
{
    m_sourceString = std::make_shared<std::string>(string());
    m_edits.clear();
    m_edits.shrink_to_fit();
}
