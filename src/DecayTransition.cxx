#include "COINAlgebra/DecayTransition.h"
#include "COINAlgebra/DecayLevel.h"

#include <iostream>
#include <stdexcept>


DecayTransition::DecayTransition()
    : fName(""),
      fSource(nullptr),
      fTarget(nullptr),
      fProbability(1.0)
{
}


DecayTransition::DecayTransition(
    const std::string& name,
    DecayLevel* source,
    DecayLevel* target,
    double probability
)
    : fName(name),
      fSource(source),
      fTarget(target),
      fProbability(1.0)
{
    SetProbability(probability);
}


// ============================================================
// Accessors
// ============================================================

const std::string& DecayTransition::GetName() const
{
    return fName;
}


DecayLevel* DecayTransition::GetSource() const
{
    return fSource;
}


DecayLevel* DecayTransition::GetTarget() const
{
    return fTarget;
}


double DecayTransition::GetProbability() const
{
    return fProbability;
}


// ============================================================
// Mutators
// ============================================================

void DecayTransition::SetName(
    const std::string& name
)
{
    fName = name;
}


void DecayTransition::SetSource(
    DecayLevel* source
)
{
    fSource = source;
}


void DecayTransition::SetTarget(
    DecayLevel* target
)
{
    fTarget = target;
}


void DecayTransition::SetProbability(
    double probability
)
{
    if(probability < 0.0 || probability > 1.0)
    {
        throw std::invalid_argument(
            "DecayTransition probability must satisfy "
            "0 <= probability <= 1."
        );
    }

    fProbability = probability;
}


// ============================================================
// Printing
// ============================================================

void DecayTransition::Print() const
{
    std::cout
        << "Arrow(name='"
        << fName
        << "', source=";

    if(fSource != nullptr)
    {
        std::cout
            << "Vertex(name='"
            << fSource->GetName()
            << "')";
    }
    else
    {
        std::cout << "nullptr";
    }

    std::cout
        << ", target=";

    if(fTarget != nullptr)
    {
        std::cout
            << "Vertex(name='"
            << fTarget->GetName()
            << "')";
    }
    else
    {
        std::cout << "nullptr";
    }

    std::cout
        << ", probability="
        << fProbability
        << ")"
        << std::endl;
}
