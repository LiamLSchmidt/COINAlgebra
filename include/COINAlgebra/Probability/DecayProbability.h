#ifndef COINALGEBRA_DECAYPROBABILITY_H
#define COINALGEBRA_DECAYPROBABILITY_H

#include "COINAlgebra/Core/DecayVector.h"
#include "COINAlgebra/Core/DecayPath.h"


class PathAlgebra;
class PathProjectors;


class DecayProbability
{
public:

    // ========================================================
    // Constructor
    // ========================================================

    // Constructs a probability calculator from an existing
    // PathAlgebra and PathProjectors.
    //
    // Neither object is owned by DecayProbability.
    //
    DecayProbability(
        const PathAlgebra& algebra,
        const PathProjectors& projectors
    );


    // ========================================================
    // Feeding vector
    // ========================================================

    // Calculates the feeding vector associated with a decay
    // vector.
    //
    // The maximum power is determined automatically by the
    // associated path algebra.
    //
    DecayVector FeedingVector(
        const DecayVector& decay
    ) const;


    // ========================================================
    // Feeding probability
    // ========================================================

    // Calculates the feeding probability associated with a
    // specified path.
    //
    // The path is embedded into the decay-vector space with
    // coefficient one and paired with the feeding vector using
    // the path form.
    //
    double FeedingProbability(
        const DecayVector& decay,
        const DecayPath& path
    ) const;

    double PathConnection( 
         const DecayVector& decay, 
         const DecayPath& path_i, 
         const DecayPath& path_j 
    ) const;

    double CoincidenceProbability(
         const DecayVector& decay, 
         const DecayPath& path_i, 
         const DecayPath& path_j 
    ) const;

private:

    // ========================================================
    // Dependencies
    // ========================================================

    // Non-owning pointers.
    //
    const PathAlgebra* fAlgebra;
    const PathProjectors* fProjectors;
};

#endif
