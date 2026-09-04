#ifndef COINALGEBRA_PATHALGEBRA_H
#define COINALGEBRA_PATHALGEBRA_H

#include "COINAlgebra/DecayVector.h"

#include <cstddef>


class DecayQuiver;


class PathAlgebra
{
public:

    // ========================================================
    // Constructor
    // ========================================================

    // Constructs the path algebra associated with a decay
    // quiver.
    //
    // The quiver is not owned by PathAlgebra and must remain
    // alive for the lifetime of this object.
    //
    PathAlgebra(
        const DecayQuiver& quiver
    );


    // ========================================================
    // Identity
    // ========================================================

    // Returns the global identity of the path algebra:
    //
    //     1 = sum_{v in Q_0} e_v
    //
    // where e_v is the stationary path associated with
    // vertex v.
    //
    DecayVector Identity() const;


    // ========================================================
    // Maximum path power
    // ========================================================

    // Returns the maximum meaningful power for the quiver.
    //
    // For an acyclic quiver with |Q_0| vertices, the maximum
    // length of a non-stationary path is
    //
    //     |Q_0| - 1.
    //
    // This is therefore the maximum power required for a
    // complete finite power expansion.
    //
    std::size_t MaxPower() const;


    // ========================================================
    // Quiver
    // ========================================================

    // Returns the quiver associated with this path algebra.
    //
    const DecayQuiver& GetQuiver() const;


    // ========================================================
    // Multiplication
    // ========================================================

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
    // The vector product is extended bilinearly.
    //
    DecayVector Multiply(
        const DecayVector& first,
        const DecayVector& second
    ) const;


    // ========================================================
    // Powers
    // ========================================================

    // Returns the n-th power of a decay vector:
    //
    //     d^n.
    //
    // In particular:
    //
    //     d^0 = 1
    //     d^1 = d
    //     d^2 = d*d
    //     ...
    //
    // where 1 is the global identity of the path algebra.
    //
    DecayVector Power(
        const DecayVector& d,
        std::size_t n
    ) const;


    // ========================================================
    // Power expansion
    // ========================================================

    // Returns the finite power expansion
    //
    //     1 + d + d^2 + ... + d^N
    //
    // where N = maxPower.
    //
    DecayVector PowerExpand(
        const DecayVector& d,
        std::size_t maxPower
    ) const;


    // ========================================================
    // Bilinear forms
    // ========================================================

    // --------------------------------------------------------
    // Source form
    // --------------------------------------------------------

    double SourceForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;


    // --------------------------------------------------------
    // Target form
    // --------------------------------------------------------

    double TargetForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;


    // --------------------------------------------------------
    // Path form
    // --------------------------------------------------------

    double PathForm(
        const DecayVector& first,
        const DecayVector& second
    ) const;


private:

    // ========================================================
    // Associated quiver
    // ========================================================

    // Non-owning pointer.
    //
    // The quiver must outlive the PathAlgebra object.
    //
    const DecayQuiver* fQuiver;
};

#endif

