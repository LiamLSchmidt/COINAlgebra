#include "COINAlgebra/DecayPath.h"

#include "COINAlgebra/DecayLevel.h"

#include <sstream>
#include <stdexcept>


// ============================================================
// Constructors
// ============================================================

DecayPath::DecayPath()
    : fTransitions{},
      fSource(nullptr),
      fTarget(nullptr)
{
}


// ------------------------------------------------------------
// Stationary path
// ------------------------------------------------------------
//
// Constructs
//
//     e_d : d -> d
//
// with length zero.
//

DecayPath::DecayPath(
    DecayLevel* level
)
    : fTransitions{},
      fSource(level),
      fTarget(level)
{
    if(level == nullptr)
    {
        throw std::invalid_argument(
            "DecayPath: "
            "stationary path requires a valid level"
        );
    }

    Validate();
}


// ------------------------------------------------------------
// Single-transition path
// ------------------------------------------------------------

DecayPath::DecayPath(
    const DecayTransition& transition
)
    : fTransitions{transition},
      fSource(transition.GetSource()),
      fTarget(transition.GetTarget())
{
    Validate();
}


// ------------------------------------------------------------
// General path
// ------------------------------------------------------------

DecayPath::DecayPath(
    const std::vector<DecayTransition>& transitions
)
    : fTransitions(transitions),
      fSource(nullptr),
      fTarget(nullptr)
{
    if(!fTransitions.empty())
    {
        fSource =
            fTransitions.front().GetSource();

        fTarget =
            fTransitions.back().GetTarget();
    }

    Validate();
}


// ============================================================
// Path properties
// ============================================================

bool DecayPath::Empty() const
{
    return fTransitions.empty() &&
           fSource == nullptr &&
           fTarget == nullptr;
}


bool DecayPath::IsStationary() const
{
    return fTransitions.empty() &&
           fSource != nullptr &&
           fSource == fTarget;
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
    return fSource;
}


DecayLevel* DecayPath::GetTarget() const
{
    return fTarget;
}


// ============================================================
// Probability
// ============================================================

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


    // --------------------------------------------------------
    // Stationary identity
    // --------------------------------------------------------

    if(IsStationary())
    {
        return other;
    }

    if(other.IsStationary())
    {
        return *this;
    }


    // --------------------------------------------------------
    // General composition
    // --------------------------------------------------------

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
    // --------------------------------------------------------
    // Empty paths
    // --------------------------------------------------------

    if(Empty() && other.Empty())
    {
        return true;
    }

    if(Empty() || other.Empty())
    {
        return false;
    }


    // --------------------------------------------------------
    // Stationary paths
    // --------------------------------------------------------

    if(IsStationary() || other.IsStationary())
    {
        return IsStationary() &&
               other.IsStationary() &&
               fSource == other.fSource;
    }


    // --------------------------------------------------------
    // Non-stationary paths
    // --------------------------------------------------------

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
// Endpoint equivalence
// ============================================================

bool DecayPath::HasSameSource(
    const DecayPath& other
) const
{
    if(Empty() || other.Empty())
    {
        return false;
    }

    return GetSource() == other.GetSource();
}


bool DecayPath::HasSameTarget(
    const DecayPath& other
) const
{
    if(Empty() || other.Empty())
    {
        return false;
    }

    return GetTarget() == other.GetTarget();
}


bool DecayPath::HasSameEndpoints(
    const DecayPath& other
) const
{
    if(Empty() || other.Empty())
    {
        return false;
    }

    return HasSameSource(other) &&
           HasSameTarget(other);
}
// ============================================================
// Display
// ============================================================

std::string DecayPath::ToString() const
{
    if(Empty())
    {
        return "<empty path>";
    }


    // --------------------------------------------------------
    // Stationary path
    // --------------------------------------------------------

    if(IsStationary())
    {
        std::ostringstream stream;

        stream
            << GetSource()->GetName()
            << " -> "
            << GetTarget()->GetName();

        return stream.str();
    }


    // --------------------------------------------------------
    // Non-stationary path
    // --------------------------------------------------------

    std::ostringstream stream;

    stream << GetSource()->GetName();

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
    // --------------------------------------------------------
    // Empty or stationary path
    // --------------------------------------------------------

    if(fTransitions.empty())
    {
        // Empty path.
        if(fSource == nullptr &&
           fTarget == nullptr)
        {
            return;
        }

        // Stationary path.
        if(fSource != nullptr &&
           fSource == fTarget)
        {
            return;
        }

        throw std::invalid_argument(
            "DecayPath: "
            "invalid length-zero path"
        );
    }


    // --------------------------------------------------------
    // Non-stationary path
    // --------------------------------------------------------

    if(fSource == nullptr ||
       fTarget == nullptr)
    {
        throw std::invalid_argument(
            "DecayPath: "
            "path has null source or target"
        );
    }


    // --------------------------------------------------------
    // Check composability of transitions
    // --------------------------------------------------------

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
                "DecayPath: "
                "transitions are not composable"
            );
        }
    }


    // --------------------------------------------------------
    // Check stored endpoints
    // --------------------------------------------------------

    if(fSource !=
       fTransitions.front().GetSource())
    {
        throw std::invalid_argument(
            "DecayPath: "
            "invalid source"
        );
    }

    if(fTarget !=
       fTransitions.back().GetTarget())
    {
        throw std::invalid_argument(
            "DecayPath: "
            "invalid target"
        );
    }
}
