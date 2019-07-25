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

#include "NgramProfile.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/locale.hpp>
#include <boost/regex.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>

#include <fstream>

/**
 * Construct n-gram profile from serialization in given file.
 *
 * @param filename input filename
 */
NgramProfile::NgramProfile(std::string const& filename)
{
    load(filename);
}

/**
 * @return number of n-grams contained in this profile
 */
std::size_t NgramProfile::n() const
{
    return m_n;
}

/**
 * @return absolute n-gram frequency count for <tt>ngram</tt>
 */
std::size_t NgramProfile::freq(Ngram ngram) const
{
    auto pos = m_updates.find(ngram);
    if (pos != m_updates.end()) {
        return pos->second;
    }

    pos = m_ngrams->find(ngram);
    return pos != m_ngrams->end() ? pos->second : static_cast<std::size_t>(0);
}

/**
 * @return normalized n-gram frequency for <tt>ngram</tt>
 */
float NgramProfile::normFreq(Ngram ngram) const
{
    return static_cast<float>(freq(ngram)) / m_n;
}

/**
 * Update a series of n-grams in this profile with relative (positive or negative) occurrence count changes.
 * If the n-gram didn't exist in the profile before, it will be inserted into it.
 *
 * @param updates set of updates
 */
void NgramProfile::update(std::vector<NgramUpdate> const& updates)
{
    m_lastNgramUpdates.clear();

    for (auto const& update: updates) {
        auto ngramPos = m_ngrams->find(update.first);
        if (ngramPos != m_ngrams->end() && m_updates.find(update.first) == m_updates.end()) {
            m_updates[update.first] = ngramPos->second;
        }
        auto updateVal = (m_updates[update.first] += update.second);
        assert(updateVal >= 0);

        if (ngramPos == m_ngrams->end() && updateVal != 0) {
            ++m_size;
        } else if (ngramPos != m_ngrams->end() && updateVal == 0) {
            assert(m_size > 0);
            --m_size;
        }

        m_n += update.second;
        m_lastNgramUpdates.push_back(update);
    }

    assert(m_n >= 0);
    assert(m_size >= 0);

    // TODO: define this threshold somewhere else
    if (m_updates.size() > 150) {
        apply();
    }
}

/**
 * Apply update history to n-gram map to trade memory for performance
 * and clear list of recent updates.
 */
void NgramProfile::apply()
{
    auto tmp = m_ngrams;
    m_ngrams = std::make_shared<NgramMap>(tmp->begin(), tmp->end());
    for (auto const& ngramPair: m_updates) {
        if (ngramPair.second == 0) {
            m_ngrams->erase(ngramPair.first);
        } else {
            (*m_ngrams)[ngramPair.first] = ngramPair.second;
        };
    }
    m_updates.clear();
}

/**
 * @return current edit log (history) size
 */
std::size_t NgramProfile::logSize() const
{
    return m_updates.size();
}

/**
 * Generate a vector of \link Ngram objects from a given string range.
 * The vector will be empty if the string range is smaller than \link NgramProfile::ORDER.
 *
 * @param begin range start
 * @param end range end
 * @return generated n-grams
 */
std::vector<NgramProfile::Ngram> ngramsFromStringRange(std::string::const_iterator begin, std::string::const_iterator end)
{
    std::vector<NgramProfile::Ngram> ngrams;

    if (end - begin < NgramProfile::ORDER) {
        return ngrams;
    }

    auto const textEnd = end - (NgramProfile::ORDER - 1);
    for (auto it = begin; it != textEnd; ++it) {
        ngrams.push_back(ngramFromStringRange(it, it + NgramProfile::ORDER));
    }

    return ngrams;
}

/**
 * Clear list of recent updates.
 */
void NgramProfile::clearRecentUpdates()
{
    m_lastNgramUpdates.clear();
}

/**
 * Generate an n-gram profile from the given string.
 * The string will be normalized in-place before an n-gram profile is created from it.
 *
 * @param text text to parse
 * @param flags bit flags for profile generation
 * @return true on success
 */
bool NgramProfile::generateFromString(std::shared_ptr<std::string> text, unsigned int flags)
{
    if (flags & STRIP_POS_ANNOTATIONS)
        stripPosAnnotationsFromText(*text);

    if (!(flags & SKIP_NORMALIZATION))
        normalizeText(*text);

    if (ORDER > text->size()) {
        std::cerr << "Order must be smaller or equal text size" << std::endl;
        return false;
    }

    m_n = 0;
    m_ngrams = std::make_shared<NgramMap>();
    m_updates.clear();
    m_lastNgramUpdates.clear();

    // generate n-grams
    auto const textEnd = text->end() - (ORDER - 1);
    std::array<char, sizeof(Ngram)> ngramBuffer{};
    for (auto it = text->begin(); it != textEnd; ++it) {
        std::copy(it, it + ORDER, ngramBuffer.begin());
        Ngram ngram = char2Ngram(ngramBuffer.data());
        (*m_ngrams)[ngram] += 1;
        ++m_n;
    }

    m_size = m_ngrams->size();

    return true;
}

/**
 * Generate an n-gram profile from the given text file.
 *
 * @param filename input text file
 * @param flags bit flags for profile generation
 * @return true on success
 */
bool NgramProfile::generate(std::string const& filename, unsigned int flags)
{
    return generate(std::vector<std::string>{filename}, flags);
}

/**
 * Generate an n-gram profile from the given text files.
 *
 * @param filename input text files
 * @param flags bit flags for profile generation
 * @return true on success
 */
bool NgramProfile::generate(std::vector<std::string> const& filenames, unsigned int flags)
{
    std::string fullText;

    for (auto const& filename: filenames) {
        std::ifstream file;
        file.open(filename);
        if (!file) {
            std::cerr << "Could not open file '" << filename << "'" << std::endl;
            return false;
        }

        std::stringstream tmpBuffer;
        tmpBuffer << file.rdbuf();
        std::string text = tmpBuffer.str();
        fullText.append(tmpBuffer.str());
    }

    return generateFromString(std::make_shared<std::string>(std::move(fullText)), flags);
}

/**
 * @return number of unique n-grams in this profile
 */
std::size_t NgramProfile::size() const
{
    return m_size;
}

/**
 * @return cloned n-gram distribution
 */
std::unique_ptr<NgramProfile> NgramProfile::clone() const
{
    auto ptr = std::make_unique<NgramProfile>();
    ptr->m_n = m_n;
    ptr->m_ngrams = m_ngrams;
    ptr->m_updates = m_updates;
    ptr->m_size = m_size;
    return ptr;
}

/**
 * Serialize n-gram profile to file.
 *
 * @param filename output filename
 */
void NgramProfile::save(std::string const& filename) const
{
    std::ofstream ofs(filename);
    boost::archive::text_oarchive archive(ofs);
    auto copy = clone();
    copy->apply();
    archive << copy->m_n << *(copy->m_ngrams);
}

/**
 * Load pre-computed n-gram profile serialization from file.
 *
 * @param filename input profile file
 */
void NgramProfile::load(std::string const& filename)
{
    m_ngrams = std::make_shared<NgramMap>();
    m_updates.clear();
    m_lastNgramUpdates.clear();

    std::ifstream ifs(filename);
    boost::archive::text_iarchive archive(ifs);
    archive >> m_n >> (*m_ngrams);
    m_size = m_ngrams->size();
}

/**
 * @return const iterator to first element of the n-gram profile map
 */
NgramProfile::Iterator NgramProfile::begin() const
{
    return {m_ngrams->cbegin(), m_ngrams->cend(), m_updates.cbegin(), m_updates.cend()};
}

/**
 * @return const iterator pointing past the last element of the n-gram profile map
 */
NgramProfile::Iterator NgramProfile::end() const
{
    return {m_ngrams->cend(), m_ngrams->cend(), m_updates.cend(), m_updates.cend()};
}

/**
 * @return const iterator to first element of the n-gram profile map
 */
NgramProfile::Iterator NgramProfile::cbegin() const
{
    return {m_ngrams->cbegin(), m_ngrams->cend(), m_updates.cbegin(), m_updates.cend()};
}

/**
 * @return const iterator pointing past the last element of the n-gram profile map
 */
NgramProfile::Iterator NgramProfile::cend() const
{
    return {m_ngrams->cend(), m_ngrams->cend(), m_updates.cend(), m_updates.cend()};
}

/**
 * Update n-gram distribution from two string ranges.
 * The first range is a window of the unmodified text from which this profile was originally generated,
 * the second range is the same window, but on a modified version of the text. The string differences
 * between the two windows will be used to update this n-gram distribution.
 *
 * @param oldBegin iterator to first element of the unmodified text
 * @param oldEnd iterator past the last element of the unmodified text
 * @param newBegin iterator to first element of the updated text
 * @param newEnd iterator past the last element of the updated text
 */
void NgramProfile::updateFromStringRange(NgramProfile::StrIt oldBegin, NgramProfile::StrIt oldEnd,
        NgramProfile::StrIt newBegin, NgramProfile::StrIt newEnd)
{
    std::vector<NgramProfile::NgramUpdate> updates;

    auto oldNgrams = ngramsFromStringRange(oldBegin, oldEnd);
    auto newNgrams = ngramsFromStringRange(newBegin, newEnd);

    std::transform(oldNgrams.begin(), oldNgrams.end(), std::back_inserter(updates), [](NgramProfile::Ngram ngram) {
        return std::make_pair(ngram, -1);
    });
    std::transform(newNgrams.begin(), newNgrams.end(), std::back_inserter(updates), [](NgramProfile::Ngram ngram) {
        return std::make_pair(ngram, 1);
    });

    update(updates);
}

/**
 * Convert a string range to an n-gram.
 * NOTE: the range must be of size \link NgramProfile::ORDER
 *
 * @param begin range start
 * @param end range end
 * @return converted Ngram representation
 */
NgramProfile::Ngram ngramFromStringRange(std::string::const_iterator begin, std::string::const_iterator end)
{
    assert(end - begin == NgramProfile::ORDER);
    static_assert(NgramProfile::ORDER <= sizeof(NgramProfile::Ngram), "N-gram size range check failed");

    std::array<char, sizeof(NgramProfile::Ngram)> charBuffer{};
    std::copy(begin, end, charBuffer.begin());
    std::replace(charBuffer.begin(), charBuffer.end(), '\n', ' ');
    return char2Ngram(charBuffer.data());
}

struct NgramProfile::Iterator::Private {
    Private(NgramMapIterator ngramsIt, NgramMapIterator ngramsEnd,
            NgramMapIterator updatesIt, NgramMapIterator updatesEnd)
            : ngramsIt(ngramsIt), ngramsEnd(ngramsEnd), updatesIt(updatesIt), updatesEnd(updatesEnd) {}
    NgramMapIterator ngramsIt;
    NgramMapIterator const ngramsEnd;
    NgramMapIterator updatesIt;
    NgramMapIterator const updatesEnd;
};

NgramProfile::Iterator::Iterator(NgramMapIterator ngramsIt, NgramMapIterator ngramsEnd,
        NgramMapIterator updatesIt, NgramMapIterator updatesEnd)
        : m_d(std::make_shared<Private>(ngramsIt, ngramsEnd, updatesIt, updatesEnd))
{
}

NgramProfile::Iterator& NgramProfile::Iterator::operator++()
{
    do {
        auto ngramsVal = m_d->ngramsIt != m_d->ngramsEnd ? m_d->ngramsIt->first : std::numeric_limits<Ngram>::max();
        auto updatesVal = m_d->updatesIt != m_d->updatesEnd ? m_d->updatesIt->first : std::numeric_limits<Ngram>::max();

        if (m_d->ngramsIt != m_d->ngramsEnd && (ngramsVal <= updatesVal || m_d->updatesIt == m_d->updatesEnd)) {
            ++m_d->ngramsIt;
        }
        if (m_d->updatesIt != m_d->updatesEnd && (updatesVal <= ngramsVal || m_d->ngramsIt == m_d->ngramsEnd)) {
            ++m_d->updatesIt;
        }

    } while (m_d->ngramsIt != m_d->ngramsEnd && m_d->updatesIt != m_d->updatesEnd &&
            m_d->updatesIt->first == m_d->ngramsIt->first && m_d->updatesIt->second == 0);

    return *this;
}

NgramProfile::Iterator NgramProfile::Iterator::operator++(int)
{
    auto oldVal = *this;
    ++(*this);
    return oldVal;
}

bool NgramProfile::Iterator::operator==(Iterator const& other) const
{
    return m_d->ngramsIt == other.m_d->ngramsIt && m_d->updatesIt == other.m_d->updatesIt;
}

bool NgramProfile::Iterator::operator!=(Iterator const& other) const
{
    return !(*this == other);
}

NgramProfile::NgramPair NgramProfile::Iterator::operator*() const
{
    if (m_d->updatesIt != m_d->updatesEnd
        && (m_d->updatesIt->first <= m_d->ngramsIt->first || m_d->ngramsIt == m_d->ngramsEnd)) {
        return *m_d->updatesIt;
    }

    assert(m_d->ngramsIt != m_d->ngramsEnd);
    return *m_d->ngramsIt;
}

/**
 * Normalize characters in a text.
 *
 * @param text text to normalize
 */
void normalizeText(std::string& text)
{
    // unicode folding / normalization
    boost::locale::generator gen;
    std::locale::global(gen("en_US.UTF-8"));
    text = boost::locale::normalize(text);

    // Remove UTF-8 BOM
    if (text.size() > 3 && text.substr(0, 3) == "\xEF\xBB\xBF") {
        text = text.substr(3);
    }

    // normalize quotes
    boost::regex quoteRegex(u8"(?:''|``|\"|„|“|”|‘|’|«|»)");
    text = boost::regex_replace(text, quoteRegex, "'");

    // normalize dashes
    boost::regex dashRegex(u8"(?:(?:‒|–|—|―)+|-{2,})");
    text = boost::regex_replace(text, dashRegex, "--");

    // normalize ellipses
    boost::regex ellipsisRegex(u8"(?:…|\\.{3,})");
    text = boost::regex_replace(text, ellipsisRegex, "...");

    // normalize line endings
    boost::regex whitespaceRegex("\\r\\n");
    text = boost::regex_replace(text, whitespaceRegex, "\n");
}

/**
 * Strip part-of-speech annotations from text.
 *
 * @param text to strip POS tags from
 */
void stripPosAnnotationsFromText(std::string& text)
{
    boost::regex wordPosRegex(R"(/[\w+\-\$\*]+(?=\s|$))");
    text = boost::regex_replace(text, wordPosRegex, "");

    boost::regex openQuotePosRegex(R"((?<=\s)(.{1,2})/``\s)");
    text = boost::regex_replace(text, openQuotePosRegex, "$1");

    boost::regex closeQuotePosRegex(R"(\s(.{1,2})/''(?=\s|$))");
    text = boost::regex_replace(text, closeQuotePosRegex, "$1");

    boost::regex openBracketPosRegex(R"((?<=\s)(.)/\((?:-\w\w)?\s)");
    text = boost::regex_replace(text, openBracketPosRegex, "$1");

    boost::regex closeBracketPosRegex(R"(\s(.)/\)(?:-\w\w)?(?=\s|$))");
    text = boost::regex_replace(text, closeBracketPosRegex, "$1");

    boost::regex punctPosRegex(R"(\s(.)/[\.,:'](?:-\w\w)?(?=\s|$))");
    text = boost::regex_replace(text, punctPosRegex, "$1");
}
