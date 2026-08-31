#ifndef COINALGEBRA_DECAYPATH_H
#define COINALGEBRA_DECAYPATH_H

#include "COINAlgebra/DecayTransition.h"

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

    DecayPath();

    explicit DecayPath(
        const DecayTransition& transition
    );

    explicit DecayPath(
        const std::vector<DecayTransition>& transitions
    );


    // --------------------------------------------------------
    // Path properties
    // --------------------------------------------------------

    bool Empty() const;

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

    bool operator==(
        const DecayPath& other
    ) const;

    bool operator!=(
        const DecayPath& other
    ) const;

    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    std::string ToString() const;


private:

    std::vector<DecayTransition> fTransitions;

    void Validate() const;
};

#endif
