#ifndef COINALGEBRA_PATHPROJECTORS_H
#define COINALGEBRA_PATHPROJECTORS_H

#include "COINAlgebra/DecayVector.h"

class DecayLevel;


class PathProjectors
{
public:

    // ========================================================
    // Constructor
    // ========================================================

    PathProjectors();


    // ========================================================
    // Source projector
    // ========================================================

    // Projects a decay vector onto the subspace of
    // length-one paths having the specified source.
    //
    // If
    //
    //     d = sum_i a_i p_i,
    //
    // then
    //
    //     S_s(d)
    //       = sum_{p_i in P_1}
    //           delta_{s(p_i),s}
    //           a_i p_i.
    //
    // The coefficients of the retained paths are copied
    // unchanged from the input vector.
    //
    // Only paths of length one are included.
    DecayVector SourceProjector(
        const DecayVector& vector,
        DecayLevel* source
    ) const;


    // ========================================================
    // Target projector
    // ========================================================

    // Projects a decay vector onto the subspace of
    // length-one paths having the specified target.
    //
    //     T_t(d)
    //       = sum_{p_i in P_1}
    //           delta_{t(p_i),t}
    //           a_i p_i.
    //
    // The coefficients of the retained paths are copied
    // unchanged from the input vector.
    //
    // Only paths of length one are included.
    DecayVector TargetProjector(
        const DecayVector& vector,
        DecayLevel* target
    ) const;


    // ========================================================
    // Source vertex projector
    // ========================================================

    // Projects a decay vector onto the stationary path
    // associated with the specified source vertex.
    //
    // The coefficient is obtained by summing the coefficients
    // of all paths whose source is the specified vertex.
    //
    //     V_s,i(d)
    //       = sum_p delta_{s(p),i} a_p e_i
    //
    // where e_i is the stationary path at vertex i.
    DecayVector SourceVertexProjector(
        const DecayVector& vector,
        DecayLevel* source
    ) const;


    // ========================================================
    // Target vertex projector
    // ========================================================

    // Projects a decay vector onto the stationary path
    // associated with the specified target vertex.
    //
    // The coefficient is obtained by summing the coefficients
    // of all paths whose target is the specified vertex.
    //
    //     V_t,i(d)
    //       = sum_p delta_{t(p),i} a_p e_i
    //
    // where e_i is the stationary path at vertex i.
    DecayVector TargetVertexProjector(
        const DecayVector& vector,
        DecayLevel* target
    ) const;


    // ========================================================
    // Branching projector
    // ========================================================

    // Projects a decay vector onto its stationary-path
    // subspace.
    //
    // For each stationary path e_i already contained in the
    // vector, its coefficient is retained.
    //
    // Equivalently, this selects the length-zero stationary
    // paths from the vector.
    DecayVector BranchingProjector(
        const DecayVector& vector
    ) const;
};

#endif

