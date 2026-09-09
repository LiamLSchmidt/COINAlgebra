#ifndef COINALGEBRA_DECAYPROBABILITY_H
#define COINALGEBRA_DECAYPROBABILITY_H

#include "COINAlgebra/Core/DecayVector.h"
#include "COINAlgebra/Core/DecayPath.h"


class PathAlgebra;
class PathProjectors;
class DetectionMaps;


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
    // vector, counting both gamma and conversion when decay contains
    // total physical transition probabilities.
    //
    // The maximum power is determined automatically by the
    // associated path algebra.
    //
    DecayVector FeedingVector(
        const DecayVector& decay
    ) const;


    // ========================================================
    // Detection feeding vector
    // ========================================================

    // Apply detection efficiencies to FeedingVector(decay), after physical
    // population propagation. For a single transition g, the coefficient is
    // P(g) * efficiency(g); earlier transitions need not be detected.
    // Longer output paths receive the product of their edge efficiencies.
    // The map is keyed by transition name; required entries must be finite
    // and in [0, 1], as enforced by DecayVector::ApplyDetectionMap.
    DecayVector DetectionFeedingVector(
        const DecayVector& decay,
        const std::unordered_map<std::string, double>& efficiencies
    ) const;


    // Gamma emission from total physical feeding; conversion still feeds
    // daughter levels. Supply total (gamma + IC) branches in decay.
    DecayVector EmissionFeedingVector(
        const DecayVector& decay,
        const std::unordered_map<std::string, double>& conversionCoefficients
    ) const;

    // Gamma detection: emission feeding times detector efficiency.
    // The two-argument overload applies only the supplied efficiency map;
    // use this overload to include internal conversion explicitly.
    DecayVector DetectionFeedingVector(
        const DecayVector& decay,
        const std::unordered_map<std::string, double>& efficiencies,
        const std::unordered_map<std::string, double>& conversionCoefficients
    ) const;

    // ========================================================
    // Feeding probability
    // ========================================================

    // Connected-path summing, Eqs. (32)-(33), with terminal-complete
    // propagation below the observed path and whole-array N^(k-1) scaling.
    // These conventions differ from the literal printed Eqs. (31)-(32);
    // see docs/mathematics/detection-summing.md.
    // Use PathForm(result, DecayVector(path)) to collect endpoint-equivalent
    // direct and summed paths. Individual path contributions remain separate.
    // Requires a DAG and normalized, nonnegative total physical edge branches.
    // Every quiver level with outgoing edges must have outgoing sum one in decay.
    DecayVector SummingFeedingVector(
        const DecayVector& decay, const DetectionMaps& maps
    ) const;

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
