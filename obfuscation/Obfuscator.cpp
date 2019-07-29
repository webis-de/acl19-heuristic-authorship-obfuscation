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

#include "Obfuscator.hpp"
#include "ComputeCostH.hpp"
#include "GoalCheck.hpp"
#include "operators/NgramRemovalOperator.hpp"
#include "operators/CharMapOperator.hpp"
#include "operators/ContextlessHypernymOperator.hpp"
#include "operators/CharacterFlipOperator.hpp"
#include "operators/ContextlessSynonymOperator.hpp"
//#include "operators/WordReplacementOperator.hpp"
//#include "operators/WordRemovalOperator.hpp"
#include "search/generic/AstarSearch.hpp"
#include <iomanip>

/**
 * Run obfuscation.
 *
 * @param input input text as stream
 * @param output output stream for the obfuscated text
 * @param targetDist target n-gram distribution to imitate
 * @param flags bit flags for n-gram profile generation
 */
void Obfuscator::obfuscate(std::stringstream& input, LayeredOStream& output, Context::NgramPtr targetDist, unsigned int flags)
{
    auto sourceText = std::make_shared<std::string>(input.str());

    auto const status = std::make_shared<Status>();
    status->init_memory_in_kbytes = search::generic::GetUsedMemoryInKilobytes();
    status->compute_cost_h = ComputeCostH();
    status->is_goal_state = GoalCheck<ComputeCostH>();
    status->compute_hash = [](State const& s) { return s.hashValue(); };

    // define search context
    Context context(targetDist);
    context.mutableMetaData->originalTextLength = sourceText->size();

    // JSD obfuscation thresholds calculated on various corpora

    // Gutenberg training corpus (e_0.5)
//    context.mutableMetaData->goalJSDist = -0.10347 * std::log2(sourceText->size()) + 2.0555;

    // Gutenberg training corpus (e_0.7)
    context.mutableMetaData->goalJSDist = -0.10437 * std::log2(sourceText->size()) + 2.0831;

    // PAN 15 training corpus (e_0.7)
//    context.mutableMetaData->goalJSDist = -0.092848 * std::log2(sourceText->size()) + 1.9863;

    // PAN 14 Essays training corpus (e_0.7)
//    context.mutableMetaData->goalJSDist = -0.082107 * std::log2(sourceText->size()) + 1.8435;

    // PAN 14 Novels training corpus (e_0.7)
//    context.mutableMetaData->goalJSDist = -0.1 * std::log2(sourceText->size()) + 2.0283;

    // PAN 13 training corpus (e_0.7)
//    context.mutableMetaData->goalJSDist = -0.092108 * std::log2(sourceText->size()) + 1.9916;

    // define initial state
    State initialState;
    initialState.setText(std::move(sourceText), flags);
    search::generic::Node<State> const initialNode(initialState);
    status->setCurrentNodeAndContext(initialNode, context);

    // load operators
    std::vector<std::unique_ptr<Operator>> operators;
    operators.push_back(std::make_unique<NgramRemovalOperator>(
            "N-Gram removal", 40, "Delete n-grams from the text"));
    operators.push_back(std::make_unique<CharacterFlipOperator>(
            "Character flips", 30, "Flip two neighboring character"));
    operators.push_back(std::make_unique<SentenceSplitAndRunOnOperator>(
            "Character mapping", 3, "Map characters to other characters (e.g. dots to commas)"));
    operators.push_back(std::make_unique<ContextlessSynonymOperator>(
            "Context-less synonyms", 10, "Replace synonyms without context consideration"));
    operators.push_back(std::make_unique<ContextlessHypernymOperator>(
            "Context-less hypernyms", 6, "Replace hypernyms without context consideration"));
//    operators.push_back(std::make_unique<WordReplacementOperator>(
//            "Word replacement", 4, "Replace a word when the replacement commonly appears in that context"));
//    operators.push_back(std::make_unique<WordRemovalOperator>(
//            "Word removal", 2, "Delete a word from the text if it's not strictly needed in its context"));
    status->setOperators(std::move(operators));

    // configure other options
    search::generic::Options options;
    options.free_memory_limit_in_mbytes = 2000;
    options.status_update_interval = 500;

    double bestJsd = 0.0;

    // define status callback
    std::function<void(Status const&)> callback = [&context, &output, &bestJsd](Status const& s) {
        auto const& node = s.getCurrentNodeAndContext().first;
        auto const& state = node.state();
        std::string text = state.text().string();

        double jsd = state.mutableMetaData()->jsd.value_or(0.0);
        if (s.has_goal_state || jsd > bestJsd) {
            output << text;
            output.flushBase(true);
            bestJsd = jsd;
        }

        double parentH = 0.0;
        double parentG = 0.0;
        double parentF = 0.0;
        double parentJsd = 0.0;
        if (node.parent()) {
            parentH = node.parent()->costH();
            parentG = node.parent()->costG();
            parentF = node.parent()->costF();
            parentJsd = node.parent()->state().mutableMetaData()->jsd.get();
        }

        std::cout << std::setprecision(5) << std::fixed
                  << "Used Memory: " << (s.used_memory_in_kbytes / 1024) << " MiB\n"
                  << "Closed States: " << s.size_of_closed << "\n"
                  << "Open States: " << s.size_of_open << "\n"
                  << "Closed States/s: " << (1000.0 * s.size_of_closed / s.runtime_in_millis) << "\n"
                  << "Reopened States/s: " << s.num_reopened_states << "\n"
                  << "Duplicate States/s: " << s.num_duplicated_states << "\n"
                  << "States/s: " << (1000.0 * (s.size_of_open + s.size_of_closed + s.num_duplicated_states) / s.runtime_in_millis) << "\n"
                  << "New States/s: " << (1000.0 * (s.size_of_open + s.size_of_closed) / s.runtime_in_millis) << "\n"
                  << "Runtime: " << s.runtime_in_millis / 1000 << "s" << "\n"
                  << "Depth: " << node.depth() << "\n"
                  << "Branching factor (min / max): " << s.branching_factor_min << " / " << s.branching_factor_max << "\n"
                  << "h(x): " << std::setw(15) << node.costH()
                        << ",     h(x-1): " << std::setw(15) << parentH
                        << ",     diff: " << std::setw(10) << (node.costH() - parentH) << "\n"
                  << "g(x): " << std::setw(15) << node.costG()
                        << ",     g(x-1): " << std::setw(15) << parentG
                        << ",     diff: " << std::setw(10) << (node.costG() - parentG) << "\n"
                  << "f(x): " << std::setw(15) << node.costF()
                        << ",     f(x-1): " << std::setw(15) << parentF
                        << ",     diff: " << std::setw(10) << (node.costF() - parentF) << "\n"
                  << "jsd(x): " << std::setw(13) << jsd
                        << ",     jsd(x-1): " << std::setw(13) << parentJsd
                        << ",     diff: " << std::setw(10) << (jsd - parentJsd) << "\n"
                  << "Monotone h(x-1) <= c(x-1, x) + h(x):  " << (parentH <= (node.costG() - parentG) + node.costH()) << "\n"
                  << "Text Length Ratio: " << static_cast<double>(text.length()) / context.mutableMetaData->originalTextLength.get() << "\n"
                  << "Target JSDist: " << context.mutableMetaData->goalJSDist.get() << "\n"
                  << "Best JSDist: " << std::sqrt(2.0 * parentJsd) << "\n"
                  << std::endl;
    };

    // run A* search
    search::generic::AstarSearch(status, callback, options);
    std::cout << "y3, y2, y1 = np.reshape([";
    auto node = status->getCurrentNodeAndContext().first;
    double jsd = node.state().mutableMetaData()->jsd.value_or(0.0);
    int i = 0;
    while (node.parent()) {
        node = *node.parent();
        double prevJsd = node.state().mutableMetaData()->jsd.value_or(0.0);
        std::cout << (jsd - prevJsd) << "," << (node.costG()) << "," << (node.costH()) << ",";
        jsd = prevJsd;
        ++i;
    }
    std::cout << "][::-1], (3, " << i << "), 'F')" << std::endl;

    std::cout << "==== GOAL STATE: ====" << std::endl;
    callback(*status);
}
