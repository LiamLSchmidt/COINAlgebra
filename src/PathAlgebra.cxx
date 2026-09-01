#include "COINAlgebra/PathAlgebra.h"


// ============================================================
// Constructor
// ============================================================

PathAlgebra::PathAlgebra()
{
}


// ============================================================
// Path-algebra multiplication
// ============================================================
//
// For two vectors
//
//     v = sum_i a_i p_i
//
//     w = sum_j b_j q_j
//
// the product is defined by bilinearity:
//
//     v * w
//       = sum_{i,j} a_i b_j (p_i * q_j).
//
// For individual paths:
//
//     p * q = p.Compose(q)
//
// whenever p and q are composable.
//
// Non-composable pairs contribute zero.
//

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
            // Coefficients multiply.
            // ------------------------------------------------

            const double coefficient =
                a * b;


            // ------------------------------------------------
            // Add the product to the result.
            //
            // AddTerm() combines coefficients when the same
            // path occurs more than once.
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
// Source form
// ============================================================
//
// For basis paths p and q:
//
//     <p,q>_S = 1
//
// if
//
//     s(p) = s(q),
//
// and zero otherwise.
//
// For vectors
//
//     v = sum_i a_i p_i
//
//     w = sum_j b_j q_j,
//
// bilinearity gives
//
//     <v,w>_S
//       = sum_{i,j}
//           a_i b_j
//           delta_{s(p_i),s(q_j)}.
//
// Thus matching paths contribute the product of their
// coefficients.
//

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


            // ------------------------------------------------
            // Source comparison
            // ------------------------------------------------

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
// For basis paths p and q:
//
//     <p,q>_T = 1
//
// if
//
//     t(p) = t(q),
//
// and zero otherwise.
//
// Extended bilinearly:
//
//     <v,w>_T
//       = sum_{i,j}
//           a_i b_j
//           delta_{t(p_i),t(q_j)}.
//

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


            // ------------------------------------------------
            // Target comparison
            // ------------------------------------------------

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
// For basis paths p and q:
//
//     <p,q>_P = 1
//
// if
//
//     s(p) = s(q)
//
// and
//
//     t(p) = t(q).
//
// Otherwise:
//
//     <p,q>_P = 0.
//
// Extended bilinearly:
//
//     <v,w>_P
//       = sum_{i,j}
//           a_i b_j
//           delta_{s(p_i),s(q_j)}
//           delta_{t(p_i),t(q_j)}.
//
// This form therefore identifies paths by their endpoints,
// independently of their lengths or intermediate transitions.
//

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


            // ------------------------------------------------
            // Endpoint comparison
            // ------------------------------------------------

            if(p.HasSameEndpoints(q))
            {
                result += a * b;
            }
        }
    }


    return result;
}
