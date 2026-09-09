#include "COINAlgebra/Probability/DecayProbability.h"

#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Detection/DetectionMaps.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <stdexcept>

namespace {
struct SummingContext {
    std::size_t visited = 0;
    DecayVector terminals;
};

SummingContext ValidateSummingDecay(const DecayQuiver& quiver, const DecayVector& decay)
{
    SummingContext context;
    std::map<DecayLevel*, std::size_t> indegree;
    std::map<DecayLevel*, std::vector<DecayLevel*>> outgoing;
    std::map<DecayLevel*, double> total;
    for (auto* level : quiver.GetLevels()) indegree[level] = 0;
    for (const auto* edge : quiver.GetTransitions()) {
        outgoing[edge->GetSource()].push_back(edge->GetTarget());
        ++indegree[edge->GetTarget()];
    }
    for (const auto& term : decay.GetTerms()) {
        if (term.path.Empty() || term.path.Length() > 1 ||
            !indegree.count(term.path.GetSource()) || !indegree.count(term.path.GetTarget()) ||
            !std::isfinite(term.coefficient) || term.coefficient < 0.0)
            throw std::invalid_argument("DecayProbability: summing requires physical stationary/single-edge terms");
        if (!term.path.IsStationary()) {
            const auto& edges = quiver.GetTransitions();
            if (std::none_of(edges.begin(), edges.end(), [&](const auto* edge) {
                return DecayPath(*edge) == term.path;
            })) throw std::invalid_argument("DecayProbability: summing transition outside quiver");
            total[term.path.GetSource()] += term.coefficient;
        }
    }
    std::queue<DecayLevel*> ready;
    for (auto* level : quiver.GetLevels()) {
        if (outgoing[level].empty()) context.terminals.AddTerm(DecayPath(level), 1.0);
        else if (std::abs(total[level] - 1.0) > 1e-10)
            throw std::invalid_argument("DecayProbability: summing requires normalized total outgoing branches");
        if (indegree[level] == 0) ready.push(level);
    }
    while (!ready.empty()) {
        auto* level = ready.front(); ready.pop();
        ++context.visited;
        for (auto* target : outgoing[level])
            if (--indegree[target] == 0) ready.push(target);
    }
    if (context.visited != quiver.GetLevels().size())
        throw std::invalid_argument("DecayProbability: summing requires an acyclic quiver");
    return context;
}
}


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
    // For the usual decay vector (stationary populations plus single edges),
    // sum population flow in topological order instead of enumerating paths.
    // This is the same finite path expansion, with shared suffixes combined.
    const auto& levels = fAlgebra->GetQuiver().GetLevels();
    std::map<DecayLevel*, double> population;
    std::map<DecayLevel*, int> indegree;
    std::map<DecayLevel*, std::vector<const DecayVector::Term*>> outgoing;
    for (auto* level : levels) { population[level] = 0; indegree[level] = 0; }
    bool singleEdges = true;
    for (const auto& term : decay.GetTerms()) {
        if (!population.count(term.path.GetSource()) || !population.count(term.path.GetTarget())) {
            singleEdges = false; break;
        }
        if (term.path.IsStationary()) population[term.path.GetSource()] += term.coefficient;
        else if (term.path.Length() == 1) {
            outgoing[term.path.GetSource()].push_back(&term);
            ++indegree[term.path.GetTarget()];
        } else { singleEdges = false; break; }
    }
    if (singleEdges) {
        std::queue<DecayLevel*> ready;
        for (auto* level : levels) if (indegree[level] == 0) ready.push(level);
        std::size_t visited = 0;
        while (!ready.empty()) {
            auto* level = ready.front(); ready.pop(); ++visited;
            for (const auto* term : outgoing[level]) {
                auto* target = term->path.GetTarget();
                population[target] += population[level] * term->coefficient;
                if (--indegree[target] == 0) ready.push(target);
            }
        }
        if (visited == levels.size()) {
            DecayVector feeding;
            for (const auto& term : decay.GetTerms())
                if (!term.path.IsStationary())
                    feeding.AddTerm(term.path, population[term.path.GetSource()] * term.coefficient);
            return feeding;
        }
    }
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

DecayVector DecayProbability::SummingFeedingVector(
    const DecayVector& decay, const DetectionMaps& maps
) const
{
    const auto context = ValidateSummingDecay(fAlgebra->GetQuiver(), decay);
    const auto branching = fProjectors->BranchingProjector(decay);
    const auto transition = decay - branching;
    const auto avoidance = fAlgebra->PowerExpand(maps.SummingOut(transition), fAlgebra->MaxPower());
    const auto above = fProjectors->TargetVertexProjector(fAlgebra->Multiply(branching, avoidance));
    // Restrict the suffix to completed cascades. Summing all prefixes would
    // count a downstream cascade once for each intermediate stopping point.
    const auto below = fProjectors->SourceVertexProjector(fAlgebra->Multiply(avoidance, context.terminals));
    const auto observed = maps.SummingInExpansion(*fAlgebra, transition);
    return fAlgebra->Multiply(fAlgebra->Multiply(above, observed), below);
}

// Detection weights apply only to the observed path. Population flow uses
// physical transition probabilities, including undetected transitions.
DecayVector DecayProbability::DetectionFeedingVector(
    const DecayVector& decay,
    const std::unordered_map<std::string, double>& efficiencies
) const
{
    return FeedingVector(decay).ApplyDetectionMap(efficiencies);
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

DecayVector DecayProbability::EmissionFeedingVector(
    const DecayVector& decay,
    const std::unordered_map<std::string, double>& conversionCoefficients
) const
{
    return FeedingVector(decay).ApplyConversionMap(conversionCoefficients);
}

DecayVector DecayProbability::DetectionFeedingVector(
    const DecayVector& decay,
    const std::unordered_map<std::string, double>& efficiencies,
    const std::unordered_map<std::string, double>& conversionCoefficients
) const
{
    return EmissionFeedingVector(decay, conversionCoefficients).ApplyDetectionMap(efficiencies);
}
