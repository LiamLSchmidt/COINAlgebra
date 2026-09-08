#ifndef COINALGEBRA_DECAYPATH_H
#define COINALGEBRA_DECAYPATH_H

#include "COINAlgebra/Core/DecayTransition.h"

#include <cstddef>
#include <string>
#include <vector>


class DecayLevel;


class DecayPath
{
public:

    // --------------------------------------------------------
    // Constructors
    // --------------------------------------------------------

    // Constructs an empty path.
    DecayPath();

    // Constructs the stationary path
    //
    //     e_d : d -> d
    //
    // of length zero.
    explicit DecayPath(
        DecayLevel* level
    );

    // Constructs a path of length one.
    explicit DecayPath(
        const DecayTransition& transition
    );

    // Constructs a path from a sequence of transitions.
    explicit DecayPath(
        const std::vector<DecayTransition>& transitions
    );


    // --------------------------------------------------------
    // Path properties
    // --------------------------------------------------------

    // True only for the default-constructed empty path.
    bool Empty() const;

    // True for a stationary path e_d : d -> d.
    bool IsStationary() const;

    // Number of transitions in the path.
    //
    // A stationary path has length zero.
    std::size_t Length() const;

    const std::vector<DecayTransition>&
    GetTransitions() const;

    const DecayTransition&
    GetTransition(
        std::size_t index
    ) const;


    // --------------------------------------------------------
    // Source and target
    // --------------------------------------------------------

    DecayLevel* GetSource() const;

    DecayLevel* GetTarget() const;


    // --------------------------------------------------------
    // Probability
    // --------------------------------------------------------

    // Product of transition probabilities.
    //
    // For a stationary path, the empty product is one.
    double GetProbability() const;


    // --------------------------------------------------------
    // Composition
    //
    // If
    //
    //     p : d0 -> d1
    //     q : d1 -> d2
    //
    // then
    //
    //     p.Compose(q)
    //
    // represents the path q p.
    //
    // Stationary paths act as identity paths.
    // --------------------------------------------------------

    bool IsComposableWith(
        const DecayPath& other
    ) const;

    DecayPath Compose(
        const DecayPath& other
    ) const;


    // ========================================================
    // Equality
    // ========================================================

    // Exact path equality.
    bool operator==(
        const DecayPath& other
    ) const;

    bool operator!=(
        const DecayPath& other
    ) const;


    // --------------------------------------------------------
    // Endpoint equivalence
    // --------------------------------------------------------
    // True if both paths have the same source.
    bool HasSameSource(
        const DecayPath& other
    ) const;

// True if both paths have the same target.
    bool HasSameTarget(
        const DecayPath& other
    ) const;

    // True if both paths have the same source and target,
    // regardless of their lengths or intermediate transitions.
    bool HasSameEndpoints(
        const DecayPath& other
    ) const;


    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    std::string ToString() const;


private:

    // --------------------------------------------------------
    // Path data
    // --------------------------------------------------------

    // Sequence of transitions.
    //
    // Empty for both an empty path and a stationary path.
    std::vector<DecayTransition> fTransitions;

    // Explicit endpoints.
    //
    // Empty path:
    //     fSource == nullptr
    //     fTarget == nullptr
    //
    // Stationary path:
    //     fSource == fTarget
    //
    // Non-stationary path:
    //     determined by the first and last transitions.
    DecayLevel* fSource;

    DecayLevel* fTarget;


    // --------------------------------------------------------
    // Validation
    // --------------------------------------------------------

    void Validate() const;
};

#endif
