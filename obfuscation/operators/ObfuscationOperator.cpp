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

#include "ObfuscationOperator.hpp"

#include <algorithm>
#include <random>
#include <chrono>

/**
 * Mutex for static cache access.
 */
std::mutex ObfuscationOperator::s_cacheMutex{};

/**
 * Cached operator working data.
 */
bd::lru_cache<std::string, ObfuscationOperator::CacheData> ObfuscationOperator::s_cachedData{200};

ObfuscationOperator::ObfuscationOperator(std::string const& name, double cost, std::string const& description)
        : Operator(name, cost, description)
{
}

/**
 * Select n-grams and cache them for the given state.
 * If a previous n-gram selection for this state is already cached,
 * the cached version is returned instead.
 */
boost::optional<ObfuscationOperator::CacheData>
        ObfuscationOperator::getCachedNgramSelection(State const& state,Context const& context) const
{
    auto const hash = state.hashValue();

    {
        std::lock_guard<std::mutex> lock(s_cacheMutex);
        auto cached = s_cachedData.get(hash);
        if (cached) {
            return cached;
        }
    }

    // random seed
    long seed = std::chrono::system_clock::now().time_since_epoch().count();

    CacheData data{std::make_shared<std::vector<std::string::const_iterator>>(),
                   std::make_shared<std::string>(state.text().string())};

    auto const sourceProfile = state.ngramProfile();
    auto rankedNgrams = rankNgrams(sourceProfile, context.targetNgramProfile);
//    std::shuffle(rankedNgrams.begin(), rankedNgrams.end(), std::default_random_engine(seed));
    if (rankedNgrams.empty()) {
        return {};
    }

    if (state.text().logSize() > 150) {
        state.text().apply();
    }

    std::vector<NgramProfile::Ngram> selectedNgrams;
    while (!rankedNgrams.empty() && selectedNgrams.size() < MAX_NGRAM_RANK) {
        std::pop_heap(rankedNgrams.begin(), rankedNgrams.end());
        selectedNgrams.emplace_back(rankedNgrams.back().ngram);
        rankedNgrams.pop_back();
    }

    if (selectedNgrams.empty()) {
        return {};
    }

    // determine n-gram positions in the text
    std::vector<std::string::const_iterator> ngramPositions;
    for (auto ngram: selectedNgrams) {
        std::string ngramStr(ngram2Char(ngram), NgramProfile::ORDER);
        std::size_t lastPos = 0;
        std::size_t strPos = 0;

        std::vector<std::string::const_iterator> candidates;
        while ((strPos = data.sourceText->find(ngramStr, lastPos)) != std::string::npos) {
            lastPos = strPos + 1;
            candidates.emplace_back(data.sourceText->begin() + strPos);
        }

        // shuffle candidate positions randomly and take the first `MAX_OCCURRENCES`
        std::shuffle(candidates.begin(), candidates.end(), std::default_random_engine(seed));
        if (candidates.size() > MAX_OCCURRENCES) {
            candidates.erase(candidates.begin() + MAX_OCCURRENCES, candidates.end());
        }
        std::copy(candidates.begin(), candidates.end(), std::back_inserter(*data.ngramPositions));
    }

    std::lock_guard<std::mutex> lock(s_cacheMutex);
    s_cachedData.insert(hash, data);
    return boost::optional<CacheData>(data);
}

std::unordered_set<State> ObfuscationOperator::apply(State const& state, Context& context) const
{
    auto dataOpt = getCachedNgramSelection(state, context);
    if (!dataOpt) {
        return {};
    }
    auto& data = dataOpt.get();

    std::vector<State> successorStates;
    for (auto const& ngramPosIt: *data.ngramPositions) {
        FocusPoint fp{ngramPosIt - data.sourceText->begin(), data.sourceText.get()};
        auto states = applyImpl(fp, state, context);
        std::copy(std::make_move_iterator(states.begin()),
                  std::make_move_iterator(states.end()),
                  std::back_inserter(successorStates));
    }

    if (successorStates.size() > MAX_SUCCESSORS) {
        // reduce total number of successors
        long seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(successorStates.begin(), successorStates.end(), std::default_random_engine(seed));
        successorStates.erase(successorStates.begin() + MAX_SUCCESSORS, successorStates.end());
    }

    return {std::make_move_iterator(successorStates.begin()), std::make_move_iterator(successorStates.end())};
}

/**
 * Helper method for ranking n-grams according to their KLD impact.
 * N-grams with less than two occurrences or a rank of 0 are discarded.
 *
 * @param sourceProfile source n-gram profile
 * @param targetProfile target n-gram profile
 * @return heap-ordered vector of n-gram ranks
 */
std::vector<ObfuscationOperator::NgramRank> ObfuscationOperator::rankNgrams(Context::ConstNgramPtr sourceProfile,
                                                                            Context::ConstNgramPtr targetProfile) const
{
    std::vector<NgramRank> ngrams;
    ngrams.reserve(sourceProfile->size() / 2);

    double n = sourceProfile->n();
    for (auto const& ngram: *sourceProfile) {

        // we only need to rank n-grams which appear at least twice
        if (ngram.second < 2) {
            continue;
        }

        double normQ = ngram.second / n;
        double normP = targetProfile->normFreq(ngram.first);

        // don't rank n-grams which are not part of the intersection between both texts
        if (normP == 0) {
            continue;
        }

        double rank = normP / normQ;

        // never consider n-grams whose reduction would make the texts more similar
        if (rank < 1.0) {
            continue;
        }

        ngrams.emplace_back(ngram.first, rank);
    }

    std::make_heap(ngrams.begin(), ngrams.end());

    return ngrams;
}

/**
 * Update a successor state (node) with a text edit.
 *
 * @param origState original state
 * @param successor successor which is to be updated
 * @param focusPoint operator focus point on the original text
 * @param editStart edit start position on the original text
 * @param editEnd edit end position on the original text
 * @param update replacement string to insert between edit positions
 * @return true on successful update, false if edit would re-introduce the original n-gram
 */
bool ObfuscationOperator::updateSuccessor(State const& origState, State& successor,
        ObfuscationOperator::FocusPoint const& focusPoint, ObfuscationOperator::StrPos const& editStart,
        ObfuscationOperator::StrPos const& editEnd, std::string const& update) const
{
    auto const& text = *focusPoint.text;
    auto const focusPos = text.begin() + focusPoint.ngramOffset;
    auto const& origNgram = std::string(focusPos, focusPos + NgramProfile::ORDER);

    auto newText = std::make_shared<std::string>(text.begin(), editStart);
    newText->append(update);
    newText->append(editEnd, text.end());

    auto const newTextPos = newText->begin() + (editStart - text.begin());
    auto const newBegin = std::max(newTextPos - NgramProfile::ORDER, newText->begin());
    auto const newEnd = std::min(newTextPos + update.length() + NgramProfile::ORDER, newText->end());

    // don't update successor if edit would re-introduce the same n-gram
    if (std::string(newBegin, newEnd).find(origNgram) != std::string::npos) {
        return false;
    }

    auto const oldBegin = std::max(editStart - NgramProfile::ORDER, text.begin());
    auto const oldEnd = std::min(editEnd + NgramProfile::ORDER, text.end());

    Context::NgramPtr newProfile(origState.ngramProfile()->clone().release());
    newProfile->updateFromStringRange(oldBegin, oldEnd, newBegin, newEnd);

    DiffString newDiff = origState.text();
    newDiff.edit(DiffString::Edit(
            static_cast<uint32_t>(oldBegin - text.begin()),
            static_cast<uint8_t>(oldEnd - oldBegin),
            std::string(newBegin, newEnd)), *newText);
    successor.setNgramProfile(std::move(newDiff), newProfile);

    return true;
}

ObfuscationOperator::NgramRank::NgramRank(NgramProfile::Ngram ngram, float rank)
        : ngram(ngram)
        , rank(rank)
{
}

bool ObfuscationOperator::NgramRank::operator<(ObfuscationOperator::NgramRank rhs) const
{
    return rank < rhs.rank;
}
