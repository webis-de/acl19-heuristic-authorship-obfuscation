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

#include "State.hpp"

State::State()
        : State(MetaData())
{
}

/**
 * @param metaData state meta data
 */
State::State(MetaData const& metaData)
        : m_text(std::make_shared<std::string>())
        , m_ngramProfile(std::make_shared<NgramProfile>())
        , m_mutableMetaData(std::make_shared<MetaData>(metaData))
{
}

/**
 * @param metaData state meta data
 * @param text normalized source text
 */
State::State(MetaData const& metaData, DiffString text)
        : m_text(std::move(text))
        , m_ngramProfile(std::make_shared<NgramProfile>())
        , m_mutableMetaData(std::make_shared<MetaData>(metaData))
{
    m_ngramProfile->generateFromString(std::make_shared<std::string>(m_text.string()), NgramProfile::SKIP_NORMALIZATION);
}

/**
 * @param metaData state meta data
 * @param text normalized source text
 * @param ngramProfile source n-gram profile generated from text
 */
State::State(MetaData const& metaData, StringPtr text, Context::NgramPtr ngramProfile)
        : m_text(std::move(text))
        , m_ngramProfile(std::move(ngramProfile))
        , m_mutableMetaData(std::make_shared<MetaData>(metaData))
{
}

std::string State::hashValue() const
{
    return m_text.hashValue();
}

bool State::operator==(State const& other) const
{
    return m_text == other.m_text;
}

/**
 * @return raw text as string
 */
DiffString State::text() const
{
    return m_text;
}

/**
 * Set the source text to obfuscate and generate an n-gram profile from it.
 * The text will be normalized in-place before n-grams are created from it
 *
 * @param text raw text as string
 * @param flags bit flags for n-gram profile generation
 */
void State::setText(StringPtr text, unsigned int flags)
{
    DiffString diffStr(text);
    m_ngramProfile->generateFromString(text, flags);
    m_text = std::move(diffStr);
}

/**
 * @return pointer to current n-gram profile
 */
Context::NgramPtr State::ngramProfile() const
{
    return m_ngramProfile;
}

/**
 * @param text raw (normalized) text as string
 * @param profile pre-calculated source n-gram distribution of <tt>text</tt>
 */
void State::setNgramProfile(StringPtr text, Context::NgramPtr profile)
{
    m_text = DiffString(std::move(text));
    m_ngramProfile = std::move(profile);
}

/**
 * @param text raw (normalized) text as DiffString rvalue
 * @param profile pre-calculated source n-gram distribution of <tt>text</tt>
 */
void State::setNgramProfile(DiffString&& text, Context::NgramPtr profile)
{
    m_text = std::move(text);
    m_ngramProfile = std::move(profile);
}
