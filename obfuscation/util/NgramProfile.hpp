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

#ifndef OBFUSCATION_SEARCH_NGRAMPROFILE_HPP
#define OBFUSCATION_SEARCH_NGRAMPROFILE_HPP

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iterator>

class NgramProfile {
public:
    /**
     * N-Gram order.
     */
    static std::size_t constexpr ORDER = 3;

    /**
     * N-Gram profile generation bit flags.
     */
    enum Flags {
        SKIP_NORMALIZATION = 2,        // skip normalization altogether.
        STRIP_POS_ANNOTATIONS = 4      // strip POS tags from the text.
    };

    typedef std::uint32_t Ngram;
    typedef std::pair<Ngram, std::size_t> NgramPair;
    typedef std::map<Ngram, std::size_t> NgramMap;
    typedef NgramMap::const_iterator NgramMapIterator;
    typedef std::pair<Ngram, int> NgramUpdate;
    typedef std::string::const_iterator StrIt;

    class Iterator: public std::iterator<std::forward_iterator_tag, NgramPair> {
    public:
        Iterator(NgramMapIterator ngramsIt, NgramMapIterator ngramsEnd,
                NgramMapIterator updatesIt, NgramMapIterator updatesEnd);
        Iterator& operator++();
        Iterator operator++(int);
        bool operator==(Iterator const& other) const;
        bool operator!=(Iterator const& other) const;
        NgramPair operator*() const;

    private:
        struct Private;
        std::shared_ptr<Private> m_d;
    };

    NgramProfile() = default;
    explicit NgramProfile(std::string const& filename);

    std::size_t n() const;
    std::size_t freq(Ngram ngram) const;
    float normFreq(Ngram ngram) const;

    std::size_t size() const;
    std::unique_ptr<NgramProfile> clone() const;

    void update(std::vector<NgramUpdate> const& updates);
    void updateFromStringRange(StrIt oldBegin, StrIt oldEnd, StrIt newBegin, StrIt newEnd);
    void apply();
    std::size_t logSize() const;

    /**
     * @return list of most recent n-gram updates
     */
    inline std::vector<NgramUpdate> const& lastUpdates() const
    {
        return m_lastNgramUpdates;
    }

    void clearRecentUpdates();

    bool generateFromString(std::shared_ptr<std::string> text, unsigned int flags = 0);
    bool generate(std::string const& filename, unsigned int flags = 0);
    bool generate(std::vector<std::string> const& filenames, unsigned int flags = 0);
    void save(std::string const& filename) const;
    void load(std::string const& filename);

    Iterator begin() const;
    Iterator end() const;
    Iterator cbegin() const;
    Iterator cend() const;
private:
    std::size_t m_n = 0;
    std::size_t m_size = 0;
    std::shared_ptr<NgramMap> m_ngrams;
    NgramMap m_updates;
    std::vector<NgramUpdate> m_lastNgramUpdates;
};


NgramProfile::Ngram ngramFromStringRange(std::string::const_iterator begin, std::string::const_iterator end);
std::vector<NgramProfile::Ngram> ngramsFromStringRange(std::string::const_iterator begin, std::string::const_iterator end);
void normalizeText(std::string& text);
void stripPosAnnotationsFromText(std::string& text);


/**
 * Cast a char array pointer to an \link Ngram.
 * The input buffer is expected to have the size of \link Ngram.
 * NO BOUNDS CHECKS ARE PERFORMED!
 *
 * @param ngramBuf input buffer
 * @return cast result type
 */
inline NgramProfile::Ngram const& char2Ngram(char const* ngramBuf)
{
    return *(reinterpret_cast<NgramProfile::Ngram const*>(ngramBuf));
}

/**
 * Cast an \Ngram to a char array pointer where the first \link NgramProfile::ORDER elements
 * are the characters of the n-gram.
 *
 * @param ngram input n-gram
 * @return pointer to char buffer of size sizeof(\link Ngram)
 */
inline char const* ngram2Char(NgramProfile::Ngram const& ngram)
{
    return reinterpret_cast<char const*>(&ngram);
}


#endif //OBFUSCATION_SEARCH_NGRAMPROFILE_HPP
