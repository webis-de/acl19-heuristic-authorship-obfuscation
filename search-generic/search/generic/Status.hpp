// Status.hpp -*- C++ -*-
// Copyright (C) 2014-2015 Martin Trenkmann
#ifndef SEARCH_GENERIC_STATUS_HPP
#define SEARCH_GENERIC_STATUS_HPP

#include <cassert>
#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "search/generic/Node.hpp"
#include "search/generic/OpenList.hpp"
#include "search/generic/ClosedList.hpp"
#include "search/generic/Operator.hpp"

namespace search {
namespace generic {

// Returns the amount of the systems free memory in kilobytes as shown by the
// tool gnome-system-monitor. Accordingly, the actual available memory contains
// 1. Unused memory (MemFree)
// 2. File IO buffer (Buffers)
// 3. Cache memory (Cached)
inline std::size_t GetFreeMemoryInKilobytes()
{
    std::ifstream stream("/proc/meminfo");
    assert(stream.is_open());
    std::string name;
    std::size_t value = 0;
    std::size_t kbytes = 0;
    while (stream >> name) {
        if (name == "MemFree:") {
            stream >> value;
            kbytes += value;
            continue;
        }
        if (name == "Buffers:") {
            stream >> value;
            kbytes += value;
            continue;
        }
        if (name == "Cached:") {
            stream >> value;
            kbytes += value;
            break;  // Assumes that this line comes at last.
        }
    }
    return kbytes;
}

// Returns the amount of memory in kilobytes used by the current process.
inline std::size_t GetUsedMemoryInKilobytes()
{
    std::ifstream stream("/proc/self/status");
    assert(stream.is_open());
    std::string token;
    std::size_t kbytes = 0;
    while (stream >> token) {
        if (token == "VmRSS:") {
            stream >> kbytes;
            break;
        }
    }
    return kbytes;
}

// A class to record statistics about the usage of a certain operator.
// When running AStarSearch each operator is associated with an instance of
// this type and its members will be updated when an operator was applied.
// Since instances might be read and written asynchronously all data members
// are of atomic (thread-safe) types.
struct OperatorStats {

    OperatorStats()
            : num_applications(0), num_generated_states(0), runtime_in_micros(0)
    {
    }

    OperatorStats(const OperatorStats& other)
            : num_applications(other.num_applications.load()),
              num_generated_states(other.num_generated_states.load()),
              runtime_in_micros(other.runtime_in_micros.load())
    {
    }

    OperatorStats& operator=(const OperatorStats& other)
    {
        if (&other != this) {
            num_applications = other.num_applications.load();
            num_generated_states = other.num_generated_states.load();
            runtime_in_micros = other.runtime_in_micros.load();
        }
        return *this;
    }

    std::atomic_uint_fast64_t num_applications;
    std::atomic_uint_fast64_t num_generated_states;
    std::atomic_uint_fast64_t runtime_in_micros;
};

// A class that serves as the central input and output parameter to the
// AStarSearch function. In multithreaded scenarios, when AStarSearch is called
// asynchronously via AStarSearchAsync, an instance of this type is shared
// between the calling and the processing thread. Therefore, most of the
// public data member are of atomic type, and some member functions are made
// thread-safe via locking a mutex. Those parts of the interface not made
// thread-safe are not allowed to be accessed concurrently by design.
//
// In addition to gain access to the data in process, the status object also
// acts as a handle for the calling thread to abort the computation (and to
// stop the running thread) or to wait for the computation/thread to complete.
//
// The information/data hold by this type is:
// * Various status values (see atomic data members)
// * The list of operators (see class Operator).
// * The list of operator statistics (see class OperatorStats)
// * The Open and Closed lists.
// * A hash function to compute a state's hash value (used in Open and Closed).
// * The cost h function (see A* search theory).
// * The goal check function (see A* search theory).
// * The initial/current node and its context.
template<typename State, typename Context>
struct Status {

    // These members are thread-safe, because operations on them are atomic.
    std::atomic_bool finished;
    std::atomic_bool has_goal_state;
    std::atomic_bool aborted_by_caller;
    std::atomic_bool aborted_by_memguard;
    std::atomic_uint_fast32_t runtime_in_millis;
    std::atomic_uint_fast32_t branching_factor_min;
    std::atomic_uint_fast32_t branching_factor_max;
    std::atomic_uint_fast32_t init_memory_in_kbytes;
    std::atomic_uint_fast32_t used_memory_in_kbytes;
    std::atomic_uint_fast32_t free_memory_in_kbytes;
    std::atomic_uint_fast32_t num_duplicated_states;
    std::atomic_uint_fast32_t num_reopened_states;
    std::atomic_uint_fast32_t num_goal_checks;
    std::atomic_uint_fast32_t size_of_closed;
    std::atomic_uint_fast32_t size_of_open;

    // These members will not be mutated during a run of AstarSearch. Therefore
    // they can be accessed by multiple threads without additional locking.
    typedef std::unique_ptr<Operator<State, Context> > UniqueOperator;
    std::vector<UniqueOperator> operators;
    std::vector<OperatorStats> operator_stats;

    // These members are not thread-safe and therefore not used directly during
    // a run of AstarSearch in order to avoid race conditions. Instead they are
    // set or move-assigned at the end of the computation. To request the size of
    // OPEN and/or CLOSED during execution use the atomic member size_of_open and
    // size_of_closed or wait until this->finished == true.
    std::string error_message;
    OpenList<State> open_list;
    ClosedList<State> closed_list;

    // Whether access to these members from multiple threads is safe depends on
    // the actual wrapped function object. Free functions without side effects
    // have no race conditions. Function objects, however, may have an internal
    // state that might not be thread-safe.

    // Function that computes state's hash value.
    std::function<std::string(const State&)> compute_hash;

    // Function that computes an estimated cost of a state to reach a goal state.
    // Represents the h-function defined in heuristic search theory.
    std::function<double(const Node<State>&, const Context&)> compute_cost_h;

    // Function that checks if a state is a goal state.
    std::function<bool(const Node<State>&, const Context&)> is_goal_state;

    Status()
            : finished(false),
              has_goal_state(false),
              aborted_by_caller(false),
              aborted_by_memguard(false),
              runtime_in_millis(0),
              branching_factor_min(std::numeric_limits<std::uint64_t>::max()),
              branching_factor_max(std::numeric_limits<std::uint64_t>::min()),
              init_memory_in_kbytes(0),
              used_memory_in_kbytes(0),
              free_memory_in_kbytes(0),
              num_duplicated_states(0),
              num_reopened_states(0),
              num_goal_checks(0),
              size_of_closed(0),
              size_of_open(0)
    {
    }

    Status(const Status&) = delete;

    Status& operator=(const Status& other) = delete;

    void setOperators(std::vector<UniqueOperator>&& operators)
    {
        this->operator_stats.assign(operators.size(), OperatorStats());
        this->operators = std::move(operators);
    }

    std::pair<Node<State>, Context> getCurrentNodeAndContext() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::make_pair(current_node_, context_);
    }

    void setCurrentNodeAndContext(const Node<State>& node,
                                  const Context& context)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        current_node_ = node;
        context_ = context;
    }

    std::size_t getNumGeneratedStates() const
    {
        std::size_t num = 0;
        for (const auto& stats : operator_stats) {
            num += stats.num_generated_states;
        }
        return num;
    }

    std::size_t getNumOperatorApplications() const
    {
        std::size_t num = 0;
        for (const auto& stats : operator_stats) {
            num += stats.num_applications;
        }
        return num;
    }

    void recordBranching(std::size_t num_branches)
    {
        branching_factor_min = std::min(branching_factor_min.load(), num_branches);
        branching_factor_max = std::max(branching_factor_max.load(), num_branches);
    }

    void recordMemoryUsage()
    {
        used_memory_in_kbytes = GetUsedMemoryInKilobytes();
        free_memory_in_kbytes = GetFreeMemoryInKilobytes();
    }

    void recordRuntime(const std::chrono::high_resolution_clock::time_point& t0)
    {
        runtime_in_millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - t0).count();
    }

    void notifyOne()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        condition_.notify_one();
    }

    void waitForCompletion()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return finished.load(); });
        lock.unlock();
    }

    void print() const
    {
        std::cout << "\nfinished                  " << finished
                  << "\nhas_goal_state            " << has_goal_state
                  << "\naborted_by_caller         " << aborted_by_caller
                  << "\naborted_by_memory_guard   " << aborted_by_memguard
                  << "\nruntime_in_millis         " << runtime_in_millis
                  << "\nbranching_factor_min      " << branching_factor_min
                  << "\nbranching_factor_max      " << branching_factor_max
                  << "\ninit_memory_in_kbytes     " << init_memory_in_kbytes
                  << "\nused_memory_in_kbytes     " << used_memory_in_kbytes
                  << "\nfree_memory_in_kbytes     " << free_memory_in_kbytes
                  << "\nnum_operator_applications " << getNumOperatorApplications()
                  << "\nnum_generated_states      " << getNumGeneratedStates()
                  << "\nnum_duplicated_states     " << num_duplicated_states
                  << "\nnum_goal_checks           " << num_goal_checks
                  << "\nsize_of_closed            " << size_of_closed
                  << "\nsize_of_open              " << size_of_open << std::endl;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;

    // These members are not thread-safe by itself, so that access is protected
    // via synchronized getter/setter methods. During a run of AstarSearch these
    // members are updated/set before each invocation of the callback function.
    Node<State> current_node_;
    Context context_;
};

}  // namespace generic
}  // namespace search

#endif  // SEARCH_GENERIC_STATUS_HPP
