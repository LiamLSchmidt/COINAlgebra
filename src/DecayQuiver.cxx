#include "COINAlgebra/DecayQuiver.h"

#include <cmath>
#include <iostream>
#include <stdexcept>


DecayQuiver::DecayQuiver()
{
}


DecayQuiver::~DecayQuiver()
{
    for(auto* transition : fTransitions)
    {
        delete transition;
    }

    for(auto* level : fLevels)
    {
        delete level;
    }
}


// ============================================================
// Vertices
// ============================================================

DecayLevel* DecayQuiver::AddLevel(
    const std::string& name
)
{
    if(GetLevel(name) != nullptr)
    {
        throw std::invalid_argument(
            "Decay level '" + name + "' already exists."
        );
    }

    auto* level = new DecayLevel(name);

    fLevels.push_back(level);

    return level;
}


DecayLevel* DecayQuiver::GetLevel(
    const std::string& name
) const
{
    for(auto* level : fLevels)
    {
        if(level != nullptr &&
           level->GetName() == name)
        {
            return level;
        }
    }

    return nullptr;
}


const std::vector<DecayLevel*>&
DecayQuiver::GetLevels() const
{
    return fLevels;
}


// ============================================================
// Transitions
// ============================================================

DecayTransition* DecayQuiver::AddTransition(
    const std::string& name,
    DecayLevel* source,
    DecayLevel* target,
    double probability
)
{
    if(source == nullptr)
    {
        throw std::invalid_argument(
            "DecayTransition source cannot be nullptr."
        );
    }

    if(target == nullptr)
    {
        throw std::invalid_argument(
            "DecayTransition target cannot be nullptr."
        );
    }

    if(GetTransition(name) != nullptr)
    {
        throw std::invalid_argument(
            "Decay transition '" + name + "' already exists."
        );
    }

    auto* transition =
        new DecayTransition(
            name,
            source,
            target,
            probability
        );

    fTransitions.push_back(transition);

    return transition;
}


DecayTransition* DecayQuiver::GetTransition(
    const std::string& name
) const
{
    for(auto* transition : fTransitions)
    {
        if(transition != nullptr &&
           transition->GetName() == name)
        {
            return transition;
        }
    }

    return nullptr;
}


const std::vector<DecayTransition*>&
DecayQuiver::GetTransitions() const
{
    return fTransitions;
}


// ============================================================
// Quiver structure
// ============================================================

bool DecayQuiver::IsComposable(
    const DecayTransition* first,
    const DecayTransition* second
) const
{
    if(first == nullptr || second == nullptr)
    {
        return false;
    }

    return first->GetTarget() == second->GetSource();
}


bool DecayQuiver::HasDirectTransition(
    const DecayLevel* source,
    const DecayLevel* target
) const
{
    if(source == nullptr || target == nullptr)
    {
        return false;
    }

    for(auto* transition : fTransitions)
    {
        if(transition == nullptr)
        {
            continue;
        }

        if(transition->GetSource() == source &&
           transition->GetTarget() == target)
        {
            return true;
        }
    }

    return false;
}


// ============================================================
// Probabilities
// ============================================================

double DecayQuiver::GetOutgoingProbability(
    const DecayLevel* level
) const
{
    if(level == nullptr)
    {
        throw std::invalid_argument(
            "Cannot calculate outgoing probability "
            "for nullptr level."
        );
    }

    double total = 0.0;

    for(auto* transition : fTransitions)
    {
        if(transition == nullptr)
        {
            continue;
        }

        if(transition->GetSource() == level)
        {
            total += transition->GetProbability();
        }
    }

    return total;
}


bool DecayQuiver::IsNormalized(
    const DecayLevel* level,
    double tolerance
) const
{
    if(level == nullptr)
    {
        return false;
    }

    const double total =
        GetOutgoingProbability(level);

    return std::abs(total - 1.0) <= tolerance;
}


// ============================================================
// Printing
// ============================================================

void DecayQuiver::Print() const
{
    std::cout << std::endl;

    std::cout << "Vertices:" << std::endl;

    for(auto* level : fLevels)
    {
        if(level != nullptr)
        {
            level->Print();
        }
    }

    std::cout << std::endl;

    std::cout << "Arrows:" << std::endl;

    for(auto* transition : fTransitions)
    {
        if(transition != nullptr)
        {
            transition->Print();
        }
    }

    std::cout << std::endl;
}
