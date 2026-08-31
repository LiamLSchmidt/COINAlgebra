#ifndef COINALGEBRA_DECAYQUIVER_H
#define COINALGEBRA_DECAYQUIVER_H

#include "COINAlgebra/DecayLevel.h"
#include "COINAlgebra/DecayTransition.h"

#include <string>
#include <vector>


class DecayQuiver
{
public:

    DecayQuiver();

    ~DecayQuiver();

    // --------------------------------------------------------
    // Vertices
    // --------------------------------------------------------

    DecayLevel* AddLevel(
        const std::string& name
    );

    DecayLevel* GetLevel(
        const std::string& name
    ) const;

    const std::vector<DecayLevel*>& GetLevels() const;

    // --------------------------------------------------------
    // Transitions
    // --------------------------------------------------------

    DecayTransition* AddTransition(
        const std::string& name,
        DecayLevel* source,
        DecayLevel* target,
        double probability = 1.0
    );

    DecayTransition* GetTransition(
        const std::string& name
    ) const;

    const std::vector<DecayTransition*>& GetTransitions() const;

    // --------------------------------------------------------
    // Quiver structure
    // --------------------------------------------------------

    bool IsComposable(
        const DecayTransition* first,
        const DecayTransition* second
    ) const;

    bool HasDirectTransition(
        const DecayLevel* source,
        const DecayLevel* target
    ) const;

    // --------------------------------------------------------
    // Probabilities
    // --------------------------------------------------------

    double GetOutgoingProbability(
        const DecayLevel* level
    ) const;

    bool IsNormalized(
        const DecayLevel* level,
        double tolerance = 1.0e-12
    ) const;

    // --------------------------------------------------------
    // Printing
    // --------------------------------------------------------

    void Print() const;

private:

    std::vector<DecayLevel*> fLevels;

    std::vector<DecayTransition*> fTransitions;
};

#endif
