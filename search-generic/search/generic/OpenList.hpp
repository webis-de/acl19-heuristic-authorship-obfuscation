// OpenList.hpp -*- C++ -*-
// Copyright (C) 2014-2015 Martin Trenkmann
#ifndef SEARCH_GENERIC_OPEN_LIST_HPP
#define SEARCH_GENERIC_OPEN_LIST_HPP

#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "search/generic/Node.hpp"
#include "search/generic/debug.hpp"

namespace search {
namespace generic {

// A class that represents an open list used in heuristic search algorithms.
// Its purpose is to store states that are left to be processed in an ordered
// manner, similar to a priority queue. Because order criterion is the cost f
// of a state the list actually stores entire nodes rather than pure states.
//
// The three basic operations are:
// 1. Pop (remove and return) the node with the lowest cost f from the list.
// 2. Push (insert) or update a node into/in the list.
// 3. Check if a given state (!) is contained.
template<typename State>
class OpenList {

    typedef std::shared_ptr<search::generic::Node<State>> SharedNode;
    typedef std::unordered_map<std::string, SharedNode> NodeMap;
    typedef std::vector<SharedNode> NodeHeap;

    // Q: Why NodeHeap and std::make_heap instead of std::priority_queue?
    // A: Running the A* algorithm requires to update a nodes cost g value
    //    while it is already inserted in OPEN. After this update operation
    //    the priority queue needs to be resorted. However, the interface of
    //    std::priority_queue does not allow access to nodes other than 'top'
    //    for good reason. Therefore, this implementation manages the queue
    //    manually by storing all nodes in a vector (NodeHeap) that is indexed
    //    by a map (NodeMap) to random-access individual nodes/states by its
    //    hash value, and to be able to reorder the queue via std::make_heap.

    struct HigherCost {
        bool operator()(const SharedNode& lhs, const SharedNode& rhs) const
        {
            return lhs->costF() > rhs->costF();
        }
    };

public:
    // A default instance is not usable due to the empty compute_hash_ member.
    OpenList() = default;

    // Move construction and assignment are allowed.
    OpenList(OpenList&&) noexcept = default;

    OpenList& operator=(OpenList&&) noexcept = default;

    // Copy construction and assignment are not allowed due to memory usage.
    OpenList(const OpenList&) = delete;

    OpenList& operator=(const OpenList&) = delete;

    explicit OpenList(std::function<std::string(const State&)> compute_hash)
            : compute_hash_(compute_hash)
    {
    }

    typename NodeHeap::const_iterator begin() const
    {
        return nodes_heap_.begin();
    }

    typename NodeHeap::const_iterator end() const
    {
        return nodes_heap_.end();
    }

    // Removes and returns the top node from the list.
    SharedNode pop()
    {
        std::pop_heap(nodes_heap_.begin(), nodes_heap_.end(), higher_cost_);
        SharedNode popped_node = nodes_heap_.back();
        nodes_heap_.pop_back();
        nodes_map_.erase(compute_hash_(popped_node->state()));
        return popped_node;
    }

    // Inserts a node into the list. If a node with the same state is already
    // present then the new node will not be inserted, but the cost g value of
    // the present node will be updated if the new nodes cost g value is lower.
    // Returns true if the node could be inserted, false if it was updated.
    bool pushOrUpdate(const SharedNode& node)
    {
        const auto hashcode = compute_hash_(node->state());
        auto insert_result = nodes_map_.insert(std::make_pair(hashcode, node));
        if (insert_result.second) {
            nodes_heap_.emplace_back(node);
            std::push_heap(nodes_heap_.begin(), nodes_heap_.end(), higher_cost_);
        } else if (insert_result.first->second->costG() > node->costG()) {
            *insert_result.first->second = *node;
            std::make_heap(nodes_heap_.begin(), nodes_heap_.end(), higher_cost_);
        }
        return insert_result.second;
    }

    bool contains(const State& state) const
    {
        return nodes_map_.find(compute_hash_(state)) != nodes_map_.end();
    }

    std::size_t size() const
    {
        return nodes_heap_.size();
    }

    bool empty() const
    {
        return nodes_map_.empty();
    }

    /**
     * Clear the OPEN list.
     *
     * @param keepNodes number of most promising nodes to keep
     */
    void clear(std::size_t keepNodes = 0)
    {
        if (nodes_heap_.empty()) {
            return;
        }

        NodeHeap newHeap;
        NodeMap newMap;
        for (std::size_t i = 0; i < keepNodes; ++i) {
            const auto node = pop();
            newHeap.emplace_back(node);
            newMap.insert(std::make_pair(compute_hash_(node->state()), node));
        }
        std::make_heap(newHeap.begin(), newHeap.end(), higher_cost_);
        nodes_heap_ = newHeap;
        nodes_map_ = newMap;
    }

private:
    std::function<std::string(const State&)> compute_hash_;
    HigherCost higher_cost_;
    NodeHeap nodes_heap_;
    NodeMap nodes_map_;
};

}  // namespace generic
}  // namespace search

#endif  // SEARCH_GENERIC_OPEN_LIST_HPP
