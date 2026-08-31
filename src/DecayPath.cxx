#include "COINAlgebra/DecayPath.h"

#include "COINAlgebra/DecayLevel.h"

#include <sstream>
#include <stdexcept>


DecayPath::DecayPath()
{
}


DecayPath::DecayPath(
    const DecayTransition& transition
)
    : fTransitions{transition}
{
    Validate();
}


DecayPath::DecayPath(
    const std::vector<DecayTransition>& transitions
)
    : fTransitions(transitions)
{
    Validate();
}


// ============================================================
// Path properties
// ============================================================

bool DecayPath::Empty() const
{
    return fTransitions.empty();
}


std::size_t DecayPath::Length() const
{
    return fTransitions.size();
}


const std::vector<DecayTransition>&
DecayPath::GetTransitions() const
{
    return fTransitions;
}


const DecayTransition&
DecayPath::GetTransition(
    std::size_t index
) const
{
    if(index >= fTransitions.size())
    {
        throw std::out_of_range(
            "DecayPath::GetTransition: "
            "index out of range"
        );
    }

    return fTransitions[index];
}


// ============================================================
// Source and target
// ============================================================

DecayLevel* DecayPath::GetSource() const
{
    if(fTransitions.empty())
    {
        return nullptr;
    }

    return fTransitions.front().GetSource();
}


DecayLevel* DecayPath::GetTarget() const
{
    if(fTransitions.empty())
    {
        return nullptr;
    }

    return fTransitions.back().GetTarget();
}


// ============================================================
// Probability
// ============================================================
//
// For a path
//
//     p = gamma_n ... gamma_2 gamma_1
//
// the path probability is
//
//     P(p) = P(gamma_1) ... P(gamma_n).
//
// Since multiplication of real numbers is commutative,
// the order of multiplication is irrelevant numerically,
// although the ordering of transitions in the path remains
// important for source, target, and composition.
//

double DecayPath::GetProbability() const
{
    double probability = 1.0;

    for(const auto& transition : fTransitions)
    {
        probability *= transition.GetProbability();
    }

    return probability;
}


// ============================================================
// Composition
// ============================================================
//
// If
//
//     this  = gamma_n ... gamma_2 gamma_1
//
// and
//
//     other = delta_m ... delta_2 delta_1,
//
// with
//
//     target(this) = source(other),
//
// then the composed path is
//
//     other * this
//
// with the transition sequence
//
//     gamma_1, ..., gamma_n,
//     delta_1, ..., delta_m.
//
// The probability therefore satisfies
//
//     P(other * this)
//         = P(this) P(other).
//

bool DecayPath::IsComposableWith(
    const DecayPath& other
) const
{
    if(Empty() || other.Empty())
    {
        return false;
    }

    return GetTarget() == other.GetSource();
}


DecayPath DecayPath::Compose(
    const DecayPath& other
) const
{
    if(!IsComposableWith(other))
    {
        throw std::invalid_argument(
            "DecayPath::Compose: "
            "paths are not composable"
        );
    }

    std::vector<DecayTransition> composed;

    composed.reserve(
        fTransitions.size() +
        other.fTransitions.size()
    );

    composed.insert(
        composed.end(),
        fTransitions.begin(),
        fTransitions.end()
    );

    composed.insert(
        composed.end(),
        other.fTransitions.begin(),
        other.fTransitions.end()
    );

    return DecayPath(composed);
}
// ============================================================
// Equality
// ============================================================

bool DecayPath::operator==(
    const DecayPath& other
) const
{
    if(fTransitions.size() !=
       other.fTransitions.size())
    {
        return false;
    }

    for(std::size_t i = 0;
        i < fTransitions.size();
        ++i)
    {
        const DecayTransition& a =
            fTransitions[i];

        const DecayTransition& b =
            other.fTransitions[i];

        if(a.GetName() != b.GetName())
        {
            return false;
        }

        if(a.GetSource() != b.GetSource())
        {
            return false;
        }

        if(a.GetTarget() != b.GetTarget())
        {
            return false;
        }
    }

    return true;
}


bool DecayPath::operator!=(
    const DecayPath& other
) const
{
    return !(*this == other);
}

// ============================================================
// Display
// ============================================================

std::string DecayPath::ToString() const
{
    if(fTransitions.empty())
    {
        return "<empty path>";
    }

    std::ostringstream stream;

    DecayLevel* source = GetSource();

    if(source != nullptr)
    {
        stream << source->GetName();
    }
    else
    {
        stream << "nullptr";
    }

    for(const auto& transition : fTransitions)
    {
        stream
            << " -["
            << transition.GetName()
            << "]-> ";

        DecayLevel* target =
            transition.GetTarget();

        if(target != nullptr)
        {
            stream << target->GetName();
        }
        else
        {
            stream << "nullptr";
        }
    }

    return stream.str();
}


// ============================================================
// Validation
// ============================================================

void DecayPath::Validate() const
{
    for(std::size_t i = 1;
        i < fTransitions.size();
        ++i)
    {
        DecayLevel* previousTarget =
            fTransitions[i - 1].GetTarget();

        DecayLevel* currentSource =
            fTransitions[i].GetSource();

        if(previousTarget != currentSource)
        {
            throw std::invalid_argument(
                "DecayPath: transitions are not composable"
            );
        }
    }
}
