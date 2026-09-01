#include "COINAlgebra/DecayLevel.h"
#include "COINAlgebra/DecayTransition.h"
#include "COINAlgebra/DecayPath.h"
#include "COINAlgebra/DecayQuiver.h"
#include "COINAlgebra/DecayVector.h"
#include "COINAlgebra/PathAlgebra.h"

#include <cassert>
#include <cmath>
#include <iostream>

void test_pathalgebra()
{
std::cout
<< "============================================\n"
<< "        COINAlgebra Path Algebra Test\n"
<< "============================================\n\n";

// ========================================================
// 1. Construct levels
// ========================================================

DecayLevel d0("d0");
DecayLevel d1("d1");
DecayLevel d2("d2");
DecayLevel d3("d3");


// ========================================================
// 2. Construct transitions
// ========================================================

DecayTransition gamma20(
    "gamma20",
    &d2,
    &d0,
    0.3
);

DecayTransition gamma21(
    "gamma21",
    &d2,
    &d1,
    0.7
);

DecayTransition gamma10(
    "gamma10",
    &d1,
    &d0,
    1.0
);

DecayTransition gamma32(
    "gamma32",
    &d3,
    &d2,
    0.5
);


// ========================================================
// 3. Construct paths
// ========================================================

DecayPath p20(gamma20);
DecayPath p21(gamma21);
DecayPath p10(gamma10);
DecayPath p32(gamma32);


// Stationary paths

DecayPath e0(&d0);
DecayPath e1(&d1);
DecayPath e2(&d2);
DecayPath e3(&d3);


// ========================================================
// 4. Construct path algebra
// ========================================================

PathAlgebra pathAlgebra;


// ========================================================
// TEST 1
//
// Multiplication of composable paths.
//
//     p21 : d2 -> d1
//     p10 : d1 -> d0
//
// Therefore
//
//     p21 * p10
//
// gives the path
//
//     d2 -> d1 -> d0.
// ========================================================

std::cout
    << "Test 1: Composable path multiplication\n";

DecayVector v21(
    p21,
    2.0
);

DecayVector v10(
    p10,
    3.0
);

DecayVector product1 =
    pathAlgebra.Multiply(
        v21,
        v10
    );

assert(product1.Size() == 1);

const auto& term1 =
    product1.GetTerms()[0];

assert(
    std::abs(
        term1.coefficient - 6.0
    ) < 1.0e-12
);

assert(
    term1.path.Length() == 2
);

assert(
    term1.path.GetSource() == &d2
);

assert(
    term1.path.GetTarget() == &d0
);

assert(
    std::abs(
        term1.path.GetProbability() - 0.7
    ) < 1.0e-12
);

std::cout
    << "  PASS\n"
    << "  "
    << product1.ToString()
    << "\n\n";


// ========================================================
// TEST 2
//
// Non-composable paths produce zero.
//
//     p20 : d2 -> d0
//     p10 : d1 -> d0
//
// Since
//
//     d0 != d1,
//
// they cannot be composed.
// ========================================================

std::cout
    << "Test 2: Non-composable paths\n";

DecayVector v20(
    p20,
    2.0
);

DecayVector product2 =
    pathAlgebra.Multiply(
        v20,
        v10
    );

assert(product2.Empty());

std::cout
    << "  PASS\n"
    << "  Non-composable product = 0\n\n";


// ========================================================
// TEST 3
//
// Stationary path as a right identity.
//
//     p21 : d2 -> d1
//     e1  : d1 -> d1
//
// Therefore
//
//     p21 * e1 = p21.
// ========================================================

std::cout
    << "Test 3: Stationary right identity\n";

DecayVector ve1(
    e1,
    4.0
);

DecayVector product3 =
    pathAlgebra.Multiply(
        v21,
        ve1
    );

assert(product3.Size() == 1);

const auto& term3 =
    product3.GetTerms()[0];

assert(
    std::abs(
        term3.coefficient - 8.0
    ) < 1.0e-12
);

assert(
    term3.path == p21
);

std::cout
    << "  PASS\n"
    << "  "
    << product3.ToString()
    << "\n\n";


// ========================================================
// TEST 4
//
// Stationary path as a left identity.
//
//     e2  : d2 -> d2
//     p21 : d2 -> d1
//
// Therefore
//
//     e2 * p21 = p21.
// ========================================================

std::cout
    << "Test 4: Stationary left identity\n";

DecayVector ve2(
    e2,
    5.0
);

DecayVector product4 =
    pathAlgebra.Multiply(
        ve2,
        v21
    );

assert(product4.Size() == 1);

const auto& term4 =
    product4.GetTerms()[0];

assert(
    std::abs(
        term4.coefficient - 10.0
    ) < 1.0e-12
);

assert(
    term4.path == p21
);

std::cout
    << "  PASS\n"
    << "  "
    << product4.ToString()
    << "\n\n";


// ========================================================
// TEST 5
//
// Stationary paths at different levels do not compose.
//
//     e2 : d2 -> d2
//     p10: d1 -> d0
//
// Therefore e2 * p10 = 0.
// ========================================================

std::cout
    << "Test 5: Incompatible stationary path\n";

DecayVector product5 =
    pathAlgebra.Multiply(
        ve2,
        v10
    );

assert(product5.Empty());

std::cout
    << "  PASS\n"
    << "  Product = 0\n\n";


// ========================================================
// TEST 6
//
// Multiplication of vectors.
//
//     v = 2 p21 + 4 p20
//     w = 3 p10
//
// Only p21 * p10 is composable.
//
// Therefore
//
//     v*w = 6 (p21 p10).
// ========================================================

std::cout
    << "Test 6: Bilinear vector multiplication\n";

DecayVector v;

v.AddTerm(
    p21,
    2.0
);

v.AddTerm(
    p20,
    4.0
);

DecayVector w;

w.AddTerm(
    p10,
    3.0
);

DecayVector product6 =
    pathAlgebra.Multiply(
        v,
        w
    );

assert(product6.Size() == 1);

const auto& term6 =
    product6.GetTerms()[0];

assert(
    std::abs(
        term6.coefficient - 6.0
    ) < 1.0e-12
);

assert(
    term6.path.Length() == 2
);

assert(
    term6.path.GetSource() == &d2
);

assert(
    term6.path.GetTarget() == &d0
);

std::cout
    << "  PASS\n"
    << "  "
    << product6.ToString()
    << "\n\n";


// ========================================================
// TEST 7
//
// Longer path.
//
//     p32 : d3 -> d2
//     p21 : d2 -> d1
//     p10 : d1 -> d0
//
// First construct
//
//     (p32 * p21) * p10.
//
// The final path should be
//
//     d3 -> d2 -> d1 -> d0.
// ========================================================

std::cout
    << "Test 7: Longer path multiplication\n";

DecayVector v32(
    p32,
    2.0
);

DecayVector firstProduct =
    pathAlgebra.Multiply(
        v32,
        v21
    );

DecayVector secondProduct =
    pathAlgebra.Multiply(
        firstProduct,
        v10
    );

assert(secondProduct.Size() == 1);

const auto& term7 =
    secondProduct.GetTerms()[0];

assert(
    std::abs(
        term7.coefficient - 12.0
    ) < 1.0e-12
);

assert(
    term7.path.Length() == 3
);

assert(
    term7.path.GetSource() == &d3
);

assert(
    term7.path.GetTarget() == &d0
);

assert(
    std::abs(
        term7.path.GetProbability() -
        (0.5 * 0.7 * 1.0)
    ) < 1.0e-12
);

std::cout
    << "  PASS\n"
    << "  "
    << secondProduct.ToString()
    << "\n\n";


// ========================================================
// TEST 8
//
// Stationary paths have intrinsic probability one.
// Their VECTOR coefficients can nevertheless be arbitrary.
//
// Here:
//
//     7 e0
//
// has coefficient 7 but path probability 1.
// ========================================================

std::cout
    << "Test 8: Stationary path coefficient vs probability\n";

DecayVector stationaryVector(
    e0,
    7.0
);

assert(stationaryVector.Size() == 1);

const auto& stationaryTerm =
    stationaryVector.GetTerms()[0];

assert(
    std::abs(
        stationaryTerm.coefficient - 7.0
    ) < 1.0e-12
);

assert(
    std::abs(
        stationaryTerm.path.GetProbability() - 1.0
    ) < 1.0e-12
);

std::cout
    << "  PASS\n"
    << "  "
    << stationaryVector.ToString()
    << "\n";

std::cout
    << "  Path probability = "
    << stationaryTerm.path.GetProbability()
    << "\n";

std::cout
    << "  Vector coefficient = "
    << stationaryTerm.coefficient
    << "\n\n";


// ========================================================
// Final result
// ========================================================

std::cout
    << "============================================\n"
    << "       ALL PATH ALGEBRA TESTS PASSED\n"
    << "============================================\n";

}
