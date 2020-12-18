// Operator.hpp -*- C++ -*-
// Copyright (C) 2014-2015 Martin Trenkmann
#ifndef SEARCH_GENERIC_OPERATOR_HPP
#define SEARCH_GENERIC_OPERATOR_HPP

#include <cstdio>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

namespace search {
namespace generic {

// An interface for operators used in heuristic search algorithms.
// Each operator has a name, cost, and an optional description.
//
// The two basic operations are:
// 1. Apply the operator to a state to generate a set of successor states.
// 2. Clone the operator. For more details see Operator::clone().
template<typename State, typename Context>
class Operator {
public:
    Operator(std::string name, double cost,
             std::string description = "")
            : name_(std::move(name)), description_(std::move(description))
    {
        setCost(cost);
    }

    virtual ~Operator() = default;

    // Generates and returns a set of successor states from a given state.
    // The context object can be used to gain access to data/information that
    // is shared between all states, e.g. a global dictionary.
    //
    // We use std::unordered_set instead of std::vector in order to enforce the
    // set criterion because it turned out during development that unsound/lazy
    // implementations of operators have the potential to generate lots of
    // duplicate states. We want to avoid that in the first place.
    virtual std::unordered_set<State> apply(const State& state,
                                            Context& context) const = 0;

    // Creates and returns a deep copy of the operator.
    // The purpose is that in multithreaded scenarios multiple users want to use
    // the same operator but with different settings of the operators cost value.
    // This is only possible if each user/thread has its own operator copy.
    virtual std::unique_ptr<Operator> clone() const = 0;

    const std::string& name() const
    {
        return name_;
    }

    const std::string& description() const
    {
        return description_;
    }

    double cost() const
    {
        return cost_;
    }

    // Sets the cost that are associated with the application of the operator.
    // In multithreaded scenarios it is common practice to first create a copy of
    // the operator via clone() and then set its cost to a user-supplied value.
    void setCost(double cost)
    {
        if (cost < 0) {
            std::printf("[WARN] Operator::cost has negative value (%f)\n", cost);
            // It is uncommon to allow negative costs, but for certain experiments
            // it might be useful. However, we emit a warning to the user.
        }
        cost_ = cost;
    }

private:
    const std::string name_;
    const std::string description_;
    double cost_ = 0.0;
};

}  // namespace generic
}  // namespace search

#endif  // SEARCH_GENERIC_OPERATOR_HPP
