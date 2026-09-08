#include "COINAlgebra/Probability/DecayProbability.h"

#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"


// ============================================================
// Constructor
// ============================================================

DecayProbability::DecayProbability(
    const PathAlgebra& algebra,
    const PathProjectors& projectors
)
    : fAlgebra(&algebra),
      fProjectors(&projectors)
{
}


// ============================================================
// Feeding vector
// ============================================================
//
// Given a decay vector:
//
//     d = b + t
//
// where:
//
//     b = branching component
//     t = transition component
//
// construct:
//
//     1 + t + t^2 + ... + t^N
//
// where:
//
//     N = PathAlgebra::MaxPower().
//
// The branching vector is then propagated through this
// transition expansion, projected onto target vertices, and
// multiplied by the transition vector.
//
// ============================================================

DecayVector DecayProbability::FeedingVector(
    const DecayVector& decay
) const
{
    // --------------------------------------------------------
    // Extract branching component.
    // --------------------------------------------------------

    DecayVector branching =
        fProjectors->BranchingProjector(
            decay
        );


    // --------------------------------------------------------
    // Extract transition component.
    //
    //     t = d - b
    // --------------------------------------------------------

    DecayVector transition =
        decay - branching;


    // --------------------------------------------------------
    // Construct the complete finite power expansion.
    //
    //     1 + t + t^2 + ... + t^N
    //
    // where N is determined by the quiver.
    // --------------------------------------------------------

    DecayVector transitionExpansion =
        fAlgebra->PowerExpand(
            transition,
            fAlgebra->MaxPower()
        );


    // --------------------------------------------------------
    // Propagate branching through the transition expansion.
    // --------------------------------------------------------

    DecayVector propagated =
        fAlgebra->Multiply(
            branching,
            transitionExpansion
        );


    // --------------------------------------------------------
    // Project onto target vertices.
    // --------------------------------------------------------

    DecayVector targetVertices =
        fProjectors->TargetVertexProjector(
            propagated
        );


    // --------------------------------------------------------
    // Multiply by the transition vector.
    // --------------------------------------------------------

    DecayVector result =
        fAlgebra->Multiply(
            targetVertices,
            transition
        );


    return result;
}


// ============================================================
// Feeding probability
// ============================================================
//
// Converts the specified path into a basis vector:
//
//     p -> 1*p
//
// and evaluates the path form:
//
//     <FeedingVector(d), p>_P.
//
// ============================================================

double DecayProbability::FeedingProbability(
    const DecayVector& decay,
    const DecayPath& path
) const
{
    // --------------------------------------------------------
    // Calculate feeding vector.
    // --------------------------------------------------------

    DecayVector feeding =
        FeedingVector(
            decay
        );


    // --------------------------------------------------------
    // Embed the path into the vector space.
    // --------------------------------------------------------

    DecayVector pathVector;

    pathVector.AddTerm(
        path,
        1.0
    );


    // --------------------------------------------------------
    // Evaluate the path form.
    // --------------------------------------------------------

    return fAlgebra->PathForm(
        feeding,
        pathVector
    );
}

// ============================================================
// Path connection
// ============================================================
//
// Calculates the second factor in the coincidence probability:
//
//     < V_t(epsilon_{t(p_i)} * tau_bar^n) * tau,
//       p_j >_P
//
// The expansion begins at the target vertex of p_i.
//
// ============================================================

double DecayProbability::PathConnection(
    const DecayVector& decay,
    const DecayPath& path_i,
    const DecayPath& path_j
) const
{
    // --------------------------------------------------------
    // Get the target vertex of path_i.
    // --------------------------------------------------------

    DecayLevel* target =
        path_i.GetTarget();


    // --------------------------------------------------------
    // Construct the stationary path
    //
    //     epsilon_{t(p_i)}.
    // --------------------------------------------------------

    DecayPath stationaryTarget(
        target
    );


    // --------------------------------------------------------
    // Embed the stationary path into the vector space:
    //
    //     epsilon_{t(p_i)} -> 1 * epsilon_{t(p_i)}
    // --------------------------------------------------------

    DecayVector targetBasis(
        stationaryTarget,
        1.0
    );


    // --------------------------------------------------------
    // Extract the branching component.
    // --------------------------------------------------------

    DecayVector branching =
        fProjectors->BranchingProjector(
            decay
        );


    // --------------------------------------------------------
    // Extract the transition component:
    //
    //     tau = d - b.
    // --------------------------------------------------------

    DecayVector transition =
        decay - branching;


    // --------------------------------------------------------
    // Construct
    //
    //     1 + tau + tau^2 + ... + tau^N
    //
    // where N is determined by the quiver.
    // --------------------------------------------------------

    DecayVector transitionExpansion =
        fAlgebra->PowerExpand(
            transition,
            fAlgebra->MaxPower()
        );


    // --------------------------------------------------------
    // Propagate from the target vertex:
    //
    //     epsilon_{t(p_i)}
    //          * tau_bar^n
    // --------------------------------------------------------

    DecayVector propagated =
        fAlgebra->Multiply(
            targetBasis,
            transitionExpansion
        );


    // --------------------------------------------------------
    // Project onto target vertices.
    // --------------------------------------------------------

    DecayVector targetVertices =
        fProjectors->TargetVertexProjector(
            propagated
        );


    // --------------------------------------------------------
    // Apply one transition:
    //
    //     V_t(epsilon_{t(p_i)}
    //          * tau_bar^n) * tau
    // --------------------------------------------------------

    DecayVector connectionVector =
        fAlgebra->Multiply(
            targetVertices,
            transition
        );


    // --------------------------------------------------------
    // Embed p_j into the vector space.
    // --------------------------------------------------------

    DecayVector pathVector_j;

    pathVector_j.AddTerm(
        path_j,
        1.0
    );


    // --------------------------------------------------------
    // Evaluate the path form:
    //
    //     <connectionVector, p_j>_P.
    // --------------------------------------------------------

    return fAlgebra->PathForm(
        connectionVector,
        pathVector_j
    );
}

// ============================================================
// Coincidence probability
// ============================================================
//
//     P(p_i cap p_j)
//       = P(p_i) P(p_j | t(p_i))
//
// where
//
//     P(p_i)
//       = FeedingProbability(decay, path_i)
//
// and
//
//     P(p_j | t(p_i))
//       = PathConnection(decay, path_i, path_j).
//
// ============================================================

double DecayProbability::CoincidenceProbability(
    const DecayVector& decay,
    const DecayPath& path_i,
    const DecayPath& path_j
) const
{
    const double feedingProbability =
        FeedingProbability(
            decay,
            path_i
        );

    const double pathConnection =
        PathConnection(
            decay,
            path_i,
            path_j
        );

    return feedingProbability * pathConnection;
}
