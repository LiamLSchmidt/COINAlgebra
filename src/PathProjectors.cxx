#include "COINAlgebra/PathProjectors.h"

#include "COINAlgebra/DecayLevel.h"
#include "COINAlgebra/DecayPath.h"

#include <stdexcept>


// ============================================================
// Constructor
// ============================================================

PathProjectors::PathProjectors()
{
}


// ============================================================
// Source projector
// ============================================================
//
// Selects only length-one paths whose source is the specified
// level.
//
// Importantly, the coefficient is copied directly from the
// input vector. It is NOT multiplied by a probability or by
// the value of a bilinear form.
//

DecayVector PathProjectors::SourceProjector(
    const DecayVector& vector,
    DecayLevel* source
) const
{
    if(source == nullptr)
    {
        throw std::invalid_argument(
            "PathProjectors::SourceProjector: "
            "source cannot be nullptr"
        );
    }

    DecayVector result;

    for(const auto& term : vector.GetTerms())
    {
        const DecayPath& path =
            term.path;

        // ----------------------------------------------------
        // Non-vertex projector acts only on P_1.
        // ----------------------------------------------------

        if(path.Length() != 1)
        {
            continue;
        }

        // ----------------------------------------------------
        // Source condition.
        // ----------------------------------------------------

        if(path.GetSource() != source)
        {
            continue;
        }

        // ----------------------------------------------------
        // Preserve the original coefficient.
        // ----------------------------------------------------

        result.AddTerm(
            path,
            term.coefficient
        );
    }

    return result;
}


// ============================================================
// Target projector
// ============================================================
//
// Selects only length-one paths whose target is the specified
// level.
//
// The original coefficients are preserved.
//

DecayVector PathProjectors::TargetProjector(
    const DecayVector& vector,
    DecayLevel* target
) const
{
    if(target == nullptr)
    {
        throw std::invalid_argument(
            "PathProjectors::TargetProjector: "
            "target cannot be nullptr"
        );
    }

    DecayVector result;

    for(const auto& term : vector.GetTerms())
    {
        const DecayPath& path =
            term.path;

        // ----------------------------------------------------
        // Non-vertex projector acts only on P_1.
        // ----------------------------------------------------

        if(path.Length() != 1)
        {
            continue;
        }

        // ----------------------------------------------------
        // Target condition.
        // ----------------------------------------------------

        if(path.GetTarget() != target)
        {
            continue;
        }

        // ----------------------------------------------------
        // Preserve the original coefficient.
        // ----------------------------------------------------

        result.AddTerm(
            path,
            term.coefficient
        );
    }

    return result;
}


// ============================================================
// Source vertex projector
// ============================================================
//
// Collects the coefficients of ALL paths having the specified
// source and places their sum onto the corresponding
// stationary path.
//

DecayVector PathProjectors::SourceVertexProjector(
    const DecayVector& vector,
    DecayLevel* source
) const
{
    if(source == nullptr)
    {
        throw std::invalid_argument(
            "PathProjectors::SourceVertexProjector: "
            "source cannot be nullptr"
        );
    }

    double coefficient = 0.0;

    for(const auto& term : vector.GetTerms())
    {
        const DecayPath& path =
            term.path;

        if(path.GetSource() == source)
        {
            coefficient += term.coefficient;
        }
    }

    if(coefficient == 0.0)
    {
        return DecayVector();
    }

    // --------------------------------------------------------
    // Construct the stationary path e_source.
    // --------------------------------------------------------

    DecayPath stationary(source);

    return DecayVector(
        stationary,
        coefficient
    );
}


// ============================================================
// Target vertex projector
// ============================================================
//
// Collects the coefficients of ALL paths having the specified
// target and places their sum onto the corresponding
// stationary path.
//

DecayVector PathProjectors::TargetVertexProjector(
    const DecayVector& vector,
    DecayLevel* target
) const
{
    if(target == nullptr)
    {
        throw std::invalid_argument(
            "PathProjectors::TargetVertexProjector: "
            "target cannot be nullptr"
        );
    }

    double coefficient = 0.0;

    for(const auto& term : vector.GetTerms())
    {
        const DecayPath& path =
            term.path;

        if(path.GetTarget() == target)
        {
            coefficient += term.coefficient;
        }
    }

    if(coefficient == 0.0)
    {
        return DecayVector();
    }

    // --------------------------------------------------------
    // Construct the stationary path e_target.
    // --------------------------------------------------------

    DecayPath stationary(target);

    return DecayVector(
        stationary,
        coefficient
    );
}


// ============================================================
// Branching projector
// ============================================================
//
// Selects the stationary paths already present in the input
// vector and preserves their coefficients.
//

DecayVector PathProjectors::BranchingProjector(
    const DecayVector& vector
) const
{
    DecayVector result;

    for(const auto& term : vector.GetTerms())
    {
        const DecayPath& path =
            term.path;

        // ----------------------------------------------------
        // Only stationary paths belong to the vertex
        // subspace selected by B.
        // ----------------------------------------------------

        if(!path.IsStationary())
        {
            continue;
        }

        // ----------------------------------------------------
        // Preserve the original coefficient.
        // ----------------------------------------------------

        result.AddTerm(
            path,
            term.coefficient
        );
    }

    return result;
}

