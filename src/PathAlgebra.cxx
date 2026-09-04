#include "COINAlgebra/PathAlgebra.h"

#include "COINAlgebra/DecayQuiver.h"
#include "COINAlgebra/DecayPath.h"


// ============================================================
// Constructor
// ============================================================

PathAlgebra::PathAlgebra(
    const DecayQuiver& quiver
)
    : fQuiver(&quiver)
{
}


// ============================================================
// Global identity
// ============================================================
//
// The global identity is the sum of all stationary paths:
//
//     1 = sum_{v in Q_0} e_v.
//
// Each stationary path satisfies:
//
//     e_v * e_v = e_v
//
// and for distinct vertices:
//
//     e_v * e_w = 0.
//
// Therefore the sum acts as the identity on every path.
//
// ============================================================

DecayVector PathAlgebra::Identity() const
{
    DecayVector identity;

    for(auto* level : fQuiver->GetLevels())
    {
        if(level == nullptr)
        {
            continue;
        }

        DecayPath stationary(level);

        identity.AddTerm(
            stationary,
            1.0
        );
    }

    return identity;
}


// ============================================================
// Maximum path power
// ============================================================
//
// For an acyclic quiver with N vertices, a non-stationary path
// can visit at most N distinct vertices.
//
// Therefore its maximum length is:
//
//     N - 1.
//
// ============================================================

std::size_t PathAlgebra::MaxPower() const
{
    const std::size_t numberOfLevels =
        fQuiver->GetLevels().size();

    if(numberOfLevels == 0)
    {
        return 0;
    }

    return numberOfLevels - 1;
}


// ============================================================
// Get quiver
// ============================================================

const DecayQuiver& PathAlgebra::GetQuiver() const
{
    return *fQuiver;
}


// ============================================================
// Path-algebra multiplication
// ============================================================
//
// For vectors
//
//     v = sum_i a_i p_i
//
//     w = sum_j b_j q_j
//
// the product is
//
//     v*w = sum_{i,j} a_i b_j (p_i*q_j).
//
// Non-composable path pairs contribute zero.
//
// ============================================================

DecayVector PathAlgebra::Multiply(
    const DecayVector& first,
    const DecayVector& second
) const
{
    DecayVector result;


    // --------------------------------------------------------
    // Zero-vector cases
    // --------------------------------------------------------

    if(first.Empty() || second.Empty())
    {
        return result;
    }


    // --------------------------------------------------------
    // Bilinear product
    // --------------------------------------------------------

    for(const auto& firstTerm :
        first.GetTerms())
    {
        const DecayPath& p =
            firstTerm.path;

        const double a =
            firstTerm.coefficient;


        for(const auto& secondTerm :
            second.GetTerms())
        {
            const DecayPath& q =
                secondTerm.path;

            const double b =
                secondTerm.coefficient;


            // ------------------------------------------------
            // Non-composable paths contribute zero.
            // ------------------------------------------------

            if(!p.IsComposableWith(q))
            {
                continue;
            }


            // ------------------------------------------------
            // Compose the paths.
            // ------------------------------------------------

            DecayPath product =
                p.Compose(q);


            // ------------------------------------------------
            // Multiply coefficients.
            // ------------------------------------------------

            const double coefficient =
                a * b;


            // ------------------------------------------------
            // Add product to result.
            // ------------------------------------------------

            result.AddTerm(
                product,
                coefficient
            );
        }
    }


    return result;
}


// ============================================================
// Path-algebra power
// ============================================================
//
//     d^0 = 1
//
//     d^n = d^(n-1) * d
//
// ============================================================

DecayVector PathAlgebra::Power(
    const DecayVector& d,
    std::size_t n
) const
{
    // --------------------------------------------------------
    // Zeroth power
    // --------------------------------------------------------

    if(n == 0)
    {
        return Identity();
    }


    // --------------------------------------------------------
    // First power
    // --------------------------------------------------------

    DecayVector result = d;


    // --------------------------------------------------------
    // Higher powers
    // --------------------------------------------------------

    for(std::size_t i = 1;
        i < n;
        ++i)
    {
        result =
            Multiply(
                result,
                d
            );
    }


    return result;
}


// ============================================================
// Power expansion
// ============================================================
//
//     PowerExpand(d,N)
//
// returns
//
//     1 + d + d^2 + ... + d^N.
//
// ============================================================

DecayVector PathAlgebra::PowerExpand(
    const DecayVector& d,
    std::size_t maxPower
) const
{
    DecayVector result;

    for(std::size_t n = 0;
        n <= maxPower;
        ++n)
    {
        result += Power(
            d,
            n
        );
    }

    return result;
}


// ============================================================
// Source form
// ============================================================
//
//     <v,w>_S
//       = sum_{s(p)=s(q)} a_p b_q.
//
// ============================================================

double PathAlgebra::SourceForm(
    const DecayVector& first,
    const DecayVector& second
) const
{
    double result = 0.0;


    for(const auto& firstTerm :
        first.GetTerms())
    {
        const DecayPath& p =
            firstTerm.path;

        const double a =
            firstTerm.coefficient;


        for(const auto& secondTerm :
            second.GetTerms())
        {
            const DecayPath& q =
                secondTerm.path;

            const double b =
                secondTerm.coefficient;


            if(p.HasSameSource(q))
            {
                result += a * b;
            }
        }
    }


    return result;
}


// ============================================================
// Target form
// ============================================================
//
//     <v,w>_T
//       = sum_{t(p)=t(q)} a_p b_q.
//
// ============================================================

double PathAlgebra::TargetForm(
    const DecayVector& first,
    const DecayVector& second
) const
{
    double result = 0.0;


    for(const auto& firstTerm :
        first.GetTerms())
    {
        const DecayPath& p =
            firstTerm.path;

        const double a =
            firstTerm.coefficient;


        for(const auto& secondTerm :
            second.GetTerms())
        {
            const DecayPath& q =
                secondTerm.path;

            const double b =
                secondTerm.coefficient;


            if(p.HasSameTarget(q))
            {
                result += a * b;
            }
        }
    }


    return result;
}


// ============================================================
// Path form
// ============================================================
//
//     <v,w>_P
//       = sum_{s(p)=s(q), t(p)=t(q)} a_p b_q.
//
// This identifies paths by their endpoints.
//
// ============================================================

double PathAlgebra::PathForm(
    const DecayVector& first,
    const DecayVector& second
) const
{
    double result = 0.0;


    for(const auto& firstTerm :
        first.GetTerms())
    {
        const DecayPath& p =
            firstTerm.path;

        const double a =
            firstTerm.coefficient;


        for(const auto& secondTerm :
            second.GetTerms())
        {
            const DecayPath& q =
                secondTerm.path;

            const double b =
                secondTerm.coefficient;


            if(p.HasSameEndpoints(q))
            {
                result += a * b;
            }
        }
    }


    return result;
}
