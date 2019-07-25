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

#include "ComputeCostH.hpp"
#include "State.hpp"
#include "util/dekker.hpp"

#include <cassert>
#include <numeric>
#include <boost/optional.hpp>

/**
 * Compute h(n) heuristic function based on the Jensen-Shannon divergence
 * between two n-gram distributions.
 *
 * @param node node to calculate h(n) for
 * @param context search context
 * @param whether to allow approximate cost updates (faster) or only exact recalculations
 * @return computed cost
 */
double ComputeCostH::operator()(search::generic::Node<State> const& node, Context const& context, bool allowUpdate) const
{
    auto const& state = node.state();

    auto const sourceProfile = state.ngramProfile();
    auto const targetProfile = context.targetNgramProfile;

    double jsd;
//    if (!allowUpdate || !state.mutableMetaData()->jsd || node.depth() % 5 == 0 || state.ngramProfile()->lastUpdates().empty()) {
        jsd = calculateJsd(sourceProfile, targetProfile);

//    } else {
//        jsd = calculateJsdUpdate(state.mutableMetaData()->jsd.get(), state.ngramProfile()->lastUpdates(), sourceProfile, targetProfile);
//    }

    if (jsd > 1.0) {
        std::cerr << "ERROR: Numerical underflow, jsd = " << jsd << std::endl;
    }

    assert(jsd <= 1.0);
    state.mutableMetaData()->jsd = jsd;

    double origJsd;
    if (!context.mutableMetaData->originalJsd) {
        origJsd = std::max(0.0, jsd - 1.0e-10);
        context.mutableMetaData->originalJsd = origJsd;
    } else {
        origJsd = context.mutableMetaData->originalJsd.get();
    }
    double const jsDistance = std::sqrt(2.0 * jsd);
    double const goal = context.mutableMetaData->goalJSDist.get();

    double p = node.costG() / std::max(1.0e-6, jsDistance - std::sqrt(2.0 * origJsd));
    double r = std::max(0.0, goal - jsDistance);
    double h = r * p;

    return h;
}

namespace {
inline double logAdd(double s1, double s2)
{
    return s1 + std::log(1.0 + std::exp(s2 - s1));
}
}

/**
 * Calculate the Jensen-Shannon divergence between two n-gram profiles.
 */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
double ComputeCostH::calculateJsd(Context::ConstNgramPtr const& sourceProfile, Context::ConstNgramPtr const& targetProfile) const
{
    auto pNorm = static_cast<double>(targetProfile->n());
    auto qNorm = static_cast<double>(sourceProfile->n());
    auto pIt = targetProfile->cbegin();
    auto qIt = sourceProfile->cbegin();
    auto const pEnd = targetProfile->cend();
    auto const qEnd = sourceProfile->cend();

    dekker::Double<double> jsdP = 0.0;
    dekker::Double<double> jsdQ = 0.0;

    double const logHalf = std::log(0.5);

    boost::optional<NgramProfile::NgramPair> pDeref;
    boost::optional<NgramProfile::NgramPair> qDeref;
    while (pIt != pEnd || qIt != qEnd) {
        double p = 1.0;
        double q = 1.0;

        pDeref = boost::none;
        if (pIt != pEnd) {
            pDeref = *pIt;
        }

        qDeref = boost::none;
        if (qIt != qEnd) {
            qDeref = *qIt;
        }

        if ((pDeref && !qDeref) || (pDeref && pDeref->first < qDeref->first)) {
            p = pDeref->second != 0 ? std::log(pDeref->second) - std::log(pNorm) : 1.0;
           ++pIt;
        } else if ((qDeref && !pDeref) || (qDeref && pDeref->first > qDeref->first)) {
            q = qDeref->second != 0 ? std::log(qDeref->second) - std::log(qNorm) : 1.0;
            ++qIt;
        } else if (pDeref && qDeref && pDeref->first == qDeref->first) {
            p = pDeref->second != 0 ? std::log(pDeref->second) - std::log(pNorm) : 1.0;
            q = qDeref->second != 0 ? std::log(qDeref->second) - std::log(qNorm) : 1.0;

            ++pIt;
            ++qIt;
        } else {
            assert(false);
        }

        double m;
        if (p <= 0.0 && q <= 0.0) {
            m = logHalf + logAdd(p, q);
        } else {
            m = logHalf + std::min(p, q);
        }

        if (p <= 0.0) {
            jsdP += std::exp(p) * std::log2(std::exp(p - m));
        }

        if (q <= 0.0) {
            jsdQ += std::exp(q) * std::log2(std::exp(q - m));
        }
    }

    return 0.5 * static_cast<double>(jsdP + jsdQ);
}

/**
 * Update the previous Jensen-Shannon divergence from a difference vector.
 * The updated value is only approximate and needs to be corrected after a few iterations.
 *
 * @param previousJsd previous JSD value
 * @param updates vector of n-gram count updates
 * @param sourceProfile new source profile
 * @param targetProfile target profile
 * @return approximate new JSD
 */
double ComputeCostH::calculateJsdUpdate(double previousJsd, std::vector<NgramProfile::NgramUpdate> const& updates,
        Context::ConstNgramPtr const& sourceProfile, Context::ConstNgramPtr const& targetProfile) const
{
    std::size_t const oldQN = sourceProfile->n();
    int newQN = 0;
    std::unordered_map<NgramProfile::Ngram, int> updatesMap;
    for (auto& update: updates) {
        newQN += update.second;
        updatesMap[update.first] += update.second;
    }
    newQN = oldQN + newQN;
    assert(newQN > 0);

    double const logHalf = std::log(0.5);
    double const newQNLog = std::log(newQN);
    double const oldQNLog = std::log(oldQN);

    dekker::Double<double> oldJsdDiff = 0.0;
    dekker::Double<double> newJsdDiff = 0.0;
    for (auto const& update: updatesMap) {
        double pNorm = targetProfile->normFreq(update.first);
        double oldQ = sourceProfile->freq(update.first);
        double newQ = oldQ + update.second;
        assert(newQ >= 0);

        pNorm = pNorm != 0.0 ? std::log(pNorm) : 1.0;
        double newQNorm = newQ > 0 ? std::log(newQ) - newQNLog : 1.0;
        double oldQNorm = oldQ > 0 ? std::log(oldQ) - oldQNLog : 1.0;

        double newMNorm = 1.0;
        if (pNorm <= 0.0 && newQNorm <= 0.0) {
            newMNorm = logHalf + logAdd(pNorm, newQNorm);
        } else if (pNorm <= 0.0 || newQNorm <= 0.0) {
            newMNorm = logHalf + std::min(pNorm, newQNorm);
        }

        double oldMNorm = 1.0;
        if (pNorm <= 0.0 && oldQNorm <= 0.0) {
            oldMNorm = logHalf + logAdd(pNorm, oldQNorm);
        } else if (pNorm <= 0.0 || oldQNorm <= 0.0) {
            oldMNorm = logHalf + std::min(pNorm, oldQNorm);
        }

        // new diff
        if (newMNorm <= 0.0) {
            if (pNorm <= 0.0)
                newJsdDiff += std::exp(pNorm) * std::log2(std::exp(pNorm - newMNorm));

            if (newQNorm <= 0.0)
                newJsdDiff += std::exp(newQNorm) * std::log2(std::exp(newQNorm - newMNorm));
        }

        // old diff
        if (oldMNorm <= 0.0) {
            if (pNorm <= 0.0)
                oldJsdDiff -= std::exp(pNorm) * std::log2(std::exp(pNorm - oldMNorm));

            if (oldQNorm <= 0.0)
                oldJsdDiff -= std::exp(oldQNorm) * std::log2(std::exp(oldQNorm - oldMNorm));
        }
    }

    return previousJsd + 0.5 * static_cast<double>(oldJsdDiff + newJsdDiff);
}
