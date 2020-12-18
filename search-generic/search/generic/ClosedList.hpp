// ClosedList.hpp -*- C++ -*-
// Copyright (C) 2014-2015 Martin Trenkmann
#ifndef SEARCH_GENERIC_CLOSED_LIST_HPP
#define SEARCH_GENERIC_CLOSED_LIST_HPP

#include <functional>
#include <memory>
#include <unordered_set>
#include "search/generic/Node.hpp"

namespace search {
namespace generic {

// A class that represents a closed list used in heuristic search algorithms.
// Its purpose is to store states that have been processed. However, this
// implementation stores entire nodes (one node wraps a state) in order to be
// able to derive graph information out of it (e.g. what was the predecessor of
// a certain node/state).
//
// The two basic operations are:
// 1. Put a node/state into the list.
// 2. Check if a given state (!) is contained.
//
// Note: If memory usage is really an issue it would be sufficient to store
// only a state's hashcode in the list rather than the entire node. However,
// in this case you are not able to derive graph information anymore.
template<typename State>
class ClosedList {

    typedef std::shared_ptr<search::generic::Node<State>> SharedNode;
    typedef std::unordered_map<std::string, SharedNode> Map;

public:
    // A default instance is not usable due to the empty compute_hash_ member.
    ClosedList() = default;

    // Move construction and assignment are allowed.
    ClosedList(ClosedList&&) noexcept = default;

    ClosedList& operator=(ClosedList&&) noexcept = default;

    // Copy construction and assignment are not allowed due to memory usage.
    ClosedList(const ClosedList&) = delete;

    ClosedList& operator=(const ClosedList&) = delete;

    explicit ClosedList(std::function<std::string(const State&)> compute_hash)
            : compute_hash_(compute_hash)
    {
    }

    typename Map::const_iterator begin() const
    {
        return nodes_.begin();
    }

    typename Map::const_iterator end() const
    {
        return nodes_.end();
    }

    bool put(const SharedNode& node)
    {
        const auto hashcode = compute_hash_(node->state());
        return nodes_.insert(std::make_pair(hashcode, node)).second;
    }

    void pop(const SharedNode& node)
    {
        auto pos = nodes_.find(compute_hash_(node->state()));
        if (pos != nodes_.end()) {
            nodes_.erase(pos);
        }
    }

    SharedNode get(const State& state) const
    {
        auto node = nodes_.find(compute_hash_(state));
        if (node == nodes_.end()) {
            return nullptr;
        }

        return node->second;
    }

    bool contains(const State& state) const
    {
        return nodes_.find(compute_hash_(state)) != nodes_.end();
    }

    std::size_t size() const
    {
        return nodes_.size();
    }

    bool empty() const
    {
        return nodes_.empty();
    }

    void clear()
    {
        nodes_.clear();
    }

    /**
     * Clear the CLOSED list, but keep parents of the SharedNodes in the given range.
     * The top-level nodes directly pointed at by the iterator range will not be kept as they
     * are expected to be on OPEN.
     *
     * @param keepBegin iterator to beginning of SharedNode range to retain
     * @param keepEnd iterator past the end of the SharedNode range
     */
    template<typename SharedNodeIterator>
    void clear(SharedNodeIterator keepBegin, SharedNodeIterator keepEnd)
    {
        Map newMap;

        for (SharedNodeIterator i = keepBegin; i != keepEnd; ++i) {
            if (*i == nullptr) {
                continue;
            }

            SharedNode keepNode = (*i)->parent();
            do {
                newMap.insert(std::make_pair(compute_hash_(keepNode->state()), keepNode));
            } while ((keepNode = keepNode->parent()) != nullptr);
        }

        nodes_ = newMap;
    }

private:
    std::function<std::string(const State&)> compute_hash_;
    Map nodes_;
};

}  // namespace generic
}  // namespace search

#endif  // SEARCH_GENERIC_CLOSED_LIST_HPP
