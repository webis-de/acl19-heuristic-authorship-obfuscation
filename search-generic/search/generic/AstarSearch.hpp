// AstarSearch.hpp -*- C++ -*-
// Copyright (C) 2014-2015 Martin Trenkmann
#ifndef SEARCH_GENERIC_ASTAR_SEARCH_HPP
#define SEARCH_GENERIC_ASTAR_SEARCH_HPP

#ifdef PROFILING_ENABLED
#include <iostream>
#include <google/profiler.h>
#endif

#include <cassert>
#include <thread>
#include <vector>
#include <thread>
#include <future>
#include "thread_pool/thread_pool.hpp"
#include "search/generic/Operator.hpp"
#include "search/generic/Status.hpp"

namespace search {
namespace generic {

// Options for a call to AstarSearch.
struct Options {

    Options()
            : status_update_interval(100), free_memory_limit_in_mbytes(1000)
    {
    }

    // Update the non-atomic member of the status variable every n-th goal check.
    // (status_update_interval defines this n)
    std::size_t status_update_interval;

    // Abort computation if the systems free memory falls below this limit.
    std::size_t free_memory_limit_in_mbytes;
};

// Applies a number of operators to a given node/state and returns the
// generated new nodes/states. The returned nodes/states may contain
// duplicates, therefore duplicate detection and removal must be handled
// by the caller, i.e. within the AstarSearch function.
template<typename State, typename Context>
std::vector<std::shared_ptr<search::generic::Node<State>>> GenerateSuccessorNodes(
        tp::ThreadPool& thread_pool,
        const std::shared_ptr<search::generic::Node<State>>& node, Context& context,
        const std::vector<std::unique_ptr<search::generic::Operator<State, Context>>>& operators,
        std::vector<OperatorStats>& operator_stats)
{
    assert(operators.size() == operator_stats.size());
    std::vector<std::shared_ptr<search::generic::Node<State>>> new_nodes;

    const auto node_r = std::ref(node);
    const auto context_r = std::ref(context);
    const auto operators_r = std::ref(operators);
    const auto operator_stats_r = std::ref(operator_stats);

    std::vector<std::future<std::unordered_set<State>>> futures;

    for (std::size_t i = 0; i < operators.size(); ++i) {
        auto task = std::packaged_task<std::unordered_set<State>()>([i, &node_r, &context_r, &operator_stats_r, &operators_r]() {
            const auto t0 = std::chrono::high_resolution_clock::now();
            const auto new_states = operators_r.get()[i]->apply(node_r.get()->state(), context_r);
            const auto t1 = std::chrono::high_resolution_clock::now();

            std::vector<OperatorStats>& stats = operator_stats_r;
            stats[i].runtime_in_micros += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            stats[i].num_generated_states += new_states.size();
            ++stats[i].num_applications;

            return new_states;
        });
        auto future = task.get_future();

        thread_pool.post(task);
        futures.emplace_back(std::move(future));
    }

    std::size_t i = 0;
    for (auto& future: futures) {
        for (const auto& state : future.get()) {
            new_nodes.push_back(std::make_shared<Node<State>>(state, node, i, operators[i]->cost()));
        }

        ++i;
    }

    return new_nodes;
}

// A function that does nothing and can be used as the callback parameter
// for the AstarSearch function if no callback is needed.
template<typename State, typename Context>
void NullCallback(const Status <State, Context>& /* status */)
{
}

// A function that runs the A* search algorithm.
//
// TEMPLATE PARAMETERS
//
// State. A type that represents a state in the search space.
//
// Context. A type that keeps shared knowledge or data during the search.
// An instance of this type is passed into AstarSearch via the status argument.
// The context object is then passed to all operator invocations. An example of
// a context might be a global dictionary all operators need access to.
//
// ARGUMENTS
//
// status. The status object serves as input and output parameter. It contains
// all information to run a heuristic search problem. If AstarSearch was called
// asynchronuosly by another thread it can be used to poll into the running
// computation to get state information. Therefore, Status' interface has been
// designed to be thread-safe. It also implements a mechanism to abort the
// running computation or wait for its completion.
//
// callback. A function that is invoked when the non-atomic member of status
// have been updated, i.e. every n-th loop, where n is specified via
// options.status_update_interval.
//
// options. Parameters to control some aspects of the computation.
template<typename State, typename Context>
void AstarSearch(const std::shared_ptr<search::generic::Status<State, Context>>& status,
                 std::function<void(const search::generic::Status<State, Context>&)> callback,
                 const Options& options = Options())
{
    try {
        assert(status->operators.size() == status->operator_stats.size());
        assert(status->init_memory_in_kbytes != 0);
        assert(status->compute_hash);
        assert(status->compute_cost_h);
        assert(status->is_goal_state);

        const auto t0 = std::chrono::high_resolution_clock::now();

#ifdef PROFILING_ENABLED
        const auto filename = "search-generic-AstarSearch.prof";
        ProfilerStart(filename);
#endif

        OpenList<State> open(status->compute_hash);
        ClosedList<State> closed(status->compute_hash);

        const auto initial_node_and_context = status->getCurrentNodeAndContext();
        auto node = std::make_shared<Node<State>>(initial_node_and_context.first);
        auto context = initial_node_and_context.second;

        node->setCostH(static_cast<float>(status->compute_cost_h(*node, context)));
        open.pushOrUpdate(node);

        tp::ThreadPool thread_pool;

        while (!open.empty()) {
            node = open.pop();
            closed.put(node);

            status->size_of_open = open.size();
            status->size_of_closed = closed.size();

            if (status->num_goal_checks % options.status_update_interval == 0) {
                status->setCurrentNodeAndContext(*node, context);
                status->recordMemoryUsage();

                status->recordRuntime(t0);
                callback(*status);

                const auto free_memory_limit_in_kbytes = options.free_memory_limit_in_mbytes * 1024;
                if (status->free_memory_in_kbytes < free_memory_limit_in_kbytes) {
                    status->aborted_by_memguard = true;
                }
            }

            ++status->num_goal_checks;
            if (status->is_goal_state(*node, context)) {
                status->has_goal_state = true;
                break;
            }

            if (status->aborted_by_memguard || status->aborted_by_caller) {
                break;
            }

            const auto new_nodes = GenerateSuccessorNodes(thread_pool, node, context,
                    status->operators, status->operator_stats);
            status->recordBranching(new_nodes.size());
            for (const auto& new_node : new_nodes) {
                if (closed.contains(new_node->state())) {
                    auto closedNode = closed.get(new_node->state());
                    if (new_node->costG() < closedNode->costG()) {
                        closed.pop(closedNode);
                        open.pushOrUpdate(new_node);
                        ++status->num_reopened_states;
                    } else {
                        ++status->num_duplicated_states;
                    }
                } else {
                    new_node->setCostH(status->compute_cost_h(*new_node, context));
                    if (!open.pushOrUpdate(new_node)) {
                        ++status->num_duplicated_states;
                    } else if (open.size() > 40000) {
                        // TODO: don't hardcode this
                        open.clear(10);
                        closed.clear(open.begin(), open.end());
                    }
                }
            }
        }

#ifdef PROFILING_ENABLED
        ProfilerStop();
        std::cout << "Now you can display the profiled data via\n"
                  << "google-pprof --evince <executable> " << filename << std::endl;
#endif

        status->open_list = std::move(open);
        status->closed_list = std::move(closed);
        status->size_of_open = status->open_list.size();
        status->size_of_closed = status->closed_list.size();
        status->setCurrentNodeAndContext(*node, context);
        status->recordMemoryUsage();

        status->recordRuntime(t0);
    } catch (std::exception& error) {
        status->error_message = error.what();
    } catch (...) {
        status->error_message = "Caught something not derived from std::exception";
    }

    status->finished = true;
    status->notifyOne();
}

// A function that runs the AstarSearch function asynchroniously,
// i.e. the call does not block, but returns immediately.
// The status object can then be used to poll into the running computation.
// The status object and all its internal data is kept until the last shared
// pointer gets deleted.
template<typename State, typename Context>
void AstarSearchAsync(const std::shared_ptr<search::generic::Status<State, Context>>& status,
                      std::function<void(const search::generic::Status<State, Context>&)> callback,
                      const Options& options = Options())
{
    std::thread thread(AstarSearch<State, Context>, status, callback, options);
    thread.detach();
}

}  // namespace generic
}  // namespace search

#endif  // SEARCH_GENERIC_ASTAR_SEARCH_HPP
