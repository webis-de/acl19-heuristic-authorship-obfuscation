// Node.hpp -*- C++ -*-
// Copyright (C) 2014 Martin Trenkmann
#ifndef SEARCH_GENERIC_NODE_HPP
#define SEARCH_GENERIC_NODE_HPP

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <vector>

namespace search {
namespace generic {

// A class that represents a node in the heuristic search space/graph.
// A node keeps a single state plus metadata to drive the A* search algorithm.
// The metadata contains:
// * A parent node containing the previous state.
// * An operator code (opcode) that identifies the operator that was applied
//   to the parent state and that generated this state.
// * The cost g (actual cost for generating this state)
// * The cost h (the cost needed to reach a goal state)
//
// Once a node is created it cannot be modified to avoid inconsistencies.
// An exception is the value for cost h that may be updated running A*.
// For more information take a look at the details of A* search.
template<typename State>
class Node {
public:
    Node()
            : costG_(0), costH_(0), opcode_(0)
    {
    }

    Node(const State& state)
            : state_(state), costG_(0), costH_(0), opcode_(0)
    {
    }

    Node(const State& state, const std::shared_ptr<Node<State>>& parent,
         std::uint8_t opcode, float opcost)
            : state_(state),
              costG_(parent->costG() + opcost),
              costH_(0),
              opcode_(opcode),
              parent_(parent)
    {
    }

    const std::shared_ptr<Node<State>> parent() const
    {
        return parent_;
    }

    std::uint8_t opcode() const
    {
        return opcode_;
    }

    const State& state() const
    {
        return state_;
    }

    inline float costF() const
    {
        return costG() + costH();
    }

    inline float costG() const
    {
        return costG_;
    }

    inline float costH() const
    {
        return costH_;
    }

    void setCostH(float cost)
    {
        costH_ = cost;
    }

    std::size_t depth() const
    {
        std::size_t depth = 0;
        auto parent = parent_;
        while (parent) {
            parent = parent->parent_;
            ++depth;
        }
        return depth;
    }

    void clear()
    {
        state_ = State();
        costG_ = 0;
        costH_ = 0;
        opcode_ = 0;
        parent_.reset();
    }

    // Debugging methods.

    void printDebugString() const
    {
        printDebugStringTo(std::cout);
    }

    void printDebugStringTo(std::ostream& os) const
    {
        os << "node: { "
           << "f: " << costF() << ", g: " << costG() << ", h: " << costH()
           << ", state: " << state() << " }\n";
    }

    void printDebugStringRecursively() const
    {
        printDebugStringRecursivelyTo(std::cout);
    }

    void printDebugStringRecursivelyTo(std::ostream& os) const
    {
        os << "node: { "
           << "f: " << costF() << ", g: " << costG() << ", h: " << costH();
        std::vector<const Node<State> *> nodes = {this};
        Node<State> *parent = this->parent().get();
        while (parent) {
            nodes.push_back(parent);
            parent = parent->parent().get();
        }
        std::reverse(nodes.begin(), nodes.end());
        auto iter = nodes.begin();
        if (iter != nodes.end()) {
            os << ", path: ";
            (*iter)->state().printShortDebugStringTo(os);
            ++iter;
        }
        while (iter != nodes.end()) {
            //      os << ' ' << (*iter)->opcode() << ' '
            os << " > ";
            (*iter)->state().printShortDebugStringTo(os);
            ++iter;
        }
        --iter;
        os << " #" << (*iter)->state().hashCode() << " }";
    }

    std::vector<std::uint8_t> getOpcodesStartingFromRoot() const
    {
        std::vector<std::uint8_t> opcodes = {opcode()};
        Node<State> *parent = this->parent().get();
        while (parent) {
            opcodes.push_back(parent->opcode());
            parent = parent->parent().get();
        }
        // Last node is start state -> remove opcode from list.
        opcodes.pop_back();
        std::reverse(opcodes.begin(), opcodes.end());
        return opcodes;
    }

private:
    State state_;
    float costG_;
    float costH_;
    std::uint8_t opcode_;
    std::shared_ptr<Node<State>> parent_;
};

template<typename State>
std::ostream& operator<<(std::ostream& os, const Node<State>& node)
{
    node.printDebugStringRecursivelyTo(os);
    return os;
}

template<typename State>
void printPathStartingFromRoot(const std::shared_ptr<Node<State> >& node)
{
    std::vector<std::shared_ptr<Node<State>>> nodes = {node};
    std::shared_ptr<Node<State> > parent = node->parent();
    while (parent) {
        nodes.push_back(parent);
        parent = parent->parent();
    }
    std::reverse(nodes.begin(), nodes.end());
    auto iter = nodes.begin();
    if (iter != nodes.end()) {
        std::cout << "State: " << (*iter)->state().DebugString() << '\n';
        ++iter;
    }
    while (iter != nodes.end()) {
        std::cout << "Apply: " << static_cast<std::uint16_t>((*iter)->opcode())
                  << "\nState: " << (*iter)->state().DebugString() << '\n';
        ++iter;
    }
}

}  // namespace generic
}  // namespace search

#endif  // SEARCH_GENERIC_NODE_HPP
