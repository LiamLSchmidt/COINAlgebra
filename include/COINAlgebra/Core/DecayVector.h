#ifndef COINALGEBRA_DECAYVECTOR_H
#define COINALGEBRA_DECAYVECTOR_H

#include "COINAlgebra/Core/DecayPath.h"

#include <cstddef>
#include <string>
#include <vector>

class DecayVector
{
public:

    struct Term
    {
        DecayPath path;
        double coefficient;
    };

    DecayVector();

    explicit DecayVector(
        const DecayPath& path,
        double coefficient = 1.0
    );

    // --------------------------------------------------------
    // Properties
    // --------------------------------------------------------

    bool Empty() const;

    std::size_t Size() const;

    const std::vector<Term>& GetTerms() const;

    // --------------------------------------------------------
    // Vector construction
    // --------------------------------------------------------

    void AddTerm(
        const DecayPath& path,
        double coefficient
    );

    // --------------------------------------------------------
    // Vector arithmetic
    // --------------------------------------------------------

    DecayVector operator+(
        const DecayVector& other
    ) const;

    DecayVector operator-(
        const DecayVector& other
    ) const;

    DecayVector& operator+=(
        const DecayVector& other
    );

    DecayVector& operator-=(
        const DecayVector& other
    );

    DecayVector operator*(
        double scalar
    ) const;

    DecayVector& operator*=(
        double scalar
    );

    friend DecayVector operator*(
        double scalar,
        const DecayVector& vector
    );

    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    std::string ToString() const;

    void Print() const;
    void PrintTable() const;
private:

    std::vector<Term> fTerms;

    static bool PathsEqual(
        const DecayPath& first,
        const DecayPath& second
    );

    void Simplify();
};

#endif
