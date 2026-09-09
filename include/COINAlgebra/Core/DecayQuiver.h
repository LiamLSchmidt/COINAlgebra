#ifndef COINALGEBRA_DECAYQUIVER_H
#define COINALGEBRA_DECAYQUIVER_H

#include "COINAlgebra/Core/DecayLevel.h"
#include "COINAlgebra/Core/DecayTransition.h"
#include "COINAlgebra/Core/DecayVector.h"

#include <string>
#include <vector>
#include <unordered_map>


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

    DecayLevel* AddLevel(const std::string& name, double energy_keV);

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
    // Mutators
    // --------------------------------------------------------

    bool RemoveLevel(const DecayLevel* level);

    bool RemoveTransition(const DecayTransition* transition);

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

    // Write DQStudio JSON, optionally including vectors and a display title.
    // Optional IC coefficients cover every transition (total alpha >= 0).
    // Optional initial populations align with GetLevels() and sum to one.
    // IC inputs require total gamma+IC physical transition branches.
    // Overwrites filename. Parent directories must already exist.
    // Throws invalid_argument for data that cannot round-trip through Studio,
    // or runtime_error for file errors. No Qt or ROOT runtime is required.
    void ExportJson(
        const std::string& filename,
        const std::vector<DecayVector>& vectors = {},
        const std::string& title = "",
        const std::unordered_map<std::string, double>& conversionCoefficients = {},
        const std::vector<double>& initialPopulations = {}
    ) const;

private:

    std::vector<DecayLevel*> fLevels;

    std::vector<DecayTransition*> fTransitions;
};

#endif
