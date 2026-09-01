#ifndef COINALGEBRA_PATHALGEBRA_H
#define COINALGEBRA_PATHALGEBRA_H

#include "COINAlgebra/DecayVector.h"

class PathAlgebra
{
public:

// --------------------------------------------------------
// Constructor
// --------------------------------------------------------

PathAlgebra();


// --------------------------------------------------------
// Multiplication
// --------------------------------------------------------

// Multiplies two decay vectors using the ordinary
// path-algebra product.
//
// For basis paths p and q:
//
//     p * q = p.Compose(q)
//
// when p and q are composable.
//
// Otherwise:
//
//     p * q = 0.
//
// For vectors
//
//     v = sum_i a_i p_i
//
//     w = sum_j b_j q_j
//
// bilinearity gives
//
//     v*w
//       = sum_{i,j} a_i b_j (p_i*q_j).
//
// Non-composable path pairs contribute zero.
DecayVector Multiply(
    const DecayVector& first,
    const DecayVector& second
) const;

// --------------------------------------------------------
    // Bilinear forms
    // --------------------------------------------------------

    // Source form.
    //
    // Compares the sources of every pair of paths.
    //
    //     <v,w>_S
    //       = sum_{s(p)=s(q)} a_p b_q
    double SourceForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;


    // Target form.
    //
    // Compares the targets of every pair of paths.
    //
    //     <v,w>_T
    //       = sum_{t(p)=t(q)} a_p b_q
    double TargetForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;


    // Path form.
    //
    // Compares the source and target of every pair of paths.
    //
    //     <v,w>_P
    //       = sum_{s(p)=s(q), t(p)=t(q)} a_p b_q
    double PathForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;

};

#endif