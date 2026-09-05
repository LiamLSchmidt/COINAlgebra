#ifndef COINALGEBRA_DECAYTRANSITION_H
#define COINALGEBRA_DECAYTRANSITION_H

#include <string>

class DecayLevel;

class DecayTransition
{
public:

    DecayTransition();

    DecayTransition(
        const std::string& name,
        DecayLevel* source,
        DecayLevel* target,
        double probability = 1.0
    );

    // --------------------------------------------------------
    // Accessors
    // --------------------------------------------------------

    const std::string& GetName() const;

    DecayLevel* GetSource() const;

    DecayLevel* GetTarget() const;

    double GetProbability() const;

    // --------------------------------------------------------
    // Mutators
    // --------------------------------------------------------

    void SetName(
        const std::string& name
    );

    void SetSource(
        DecayLevel* source
    );

    void SetTarget(
        DecayLevel* target
    );

    void SetProbability(
        double probability
    );

    // --------------------------------------------------------
    // Utility
    // --------------------------------------------------------

    void Print() const;

private:

    std::string fName;

    DecayLevel* fSource;

    DecayLevel* fTarget;

    double fProbability;
};

#endif
