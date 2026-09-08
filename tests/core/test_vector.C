#include "COINAlgebra/Core/DecayLevel.h"
#include "COINAlgebra/Core/DecayTransition.h"
#include "COINAlgebra/Core/DecayPath.h"
#include "COINAlgebra/Core/DecayVector.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>


void test_vector()
{
    std::cout << "========================================\n";
    std::cout << "      COINAlgebra DecayVector Tests\n";
    std::cout << "========================================\n\n";


    // ========================================================
    // Levels
    // ========================================================

    DecayLevel d0("d0");
    DecayLevel d1("d1");
    DecayLevel d2("d2");


    // ========================================================
    // Transitions
    // ========================================================

    DecayTransition gamma1(
        "gamma1",
        &d0,
        &d1,
        0.4
    );

    DecayTransition gamma2(
        "gamma2",
        &d1,
        &d2,
        0.6
    );


    // ========================================================
    // Paths
    // ========================================================

    DecayPath e0(&d0);
    DecayPath e1(&d1);

    DecayPath p1(gamma1);

    DecayPath p2(
        std::vector<DecayTransition>{
            gamma1,
            gamma2
        }
    );


    // ========================================================
    // Test 1: Empty vector
    // ========================================================

    std::cout << "Test 1: Empty vector...\n";

    DecayVector empty;

    assert(empty.Empty());
    assert(empty.Size() == 0);
    assert(empty.GetTerms().empty());

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 2: Vector containing a stationary path
    // ========================================================

    std::cout
        << "Test 2: Stationary path component...\n";

    DecayVector stationary(
        e0,
        0.7
    );

    assert(!stationary.Empty());
    assert(stationary.Size() == 1);

    const auto& stationary_term =
        stationary.GetTerms()[0];

    assert(stationary_term.path.IsStationary());

    assert(
        stationary_term.path.GetSource() == &d0
    );

    assert(
        stationary_term.path.GetTarget() == &d0
    );

    assert(
        stationary_term.path.Length() == 0
    );

    // Intrinsic probability of stationary path.
    assert(
        std::abs(
            stationary_term.path.GetProbability() - 1.0
        ) < 1.0e-12
    );

    // Coefficient of stationary path in vector.
    assert(
        std::abs(
            stationary_term.coefficient - 0.7
        ) < 1.0e-12
    );

    std::cout
        << "  Path probability: "
        << stationary_term.path.GetProbability()
        << "\n";

    std::cout
        << "  Vector coefficient: "
        << stationary_term.coefficient
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 3: Ordinary path probability
    // ========================================================

    std::cout
        << "Test 3: Ordinary path probability...\n";

    assert(
        std::abs(
            p2.GetProbability() - 0.24
        ) < 1.0e-12
    );

    std::cout
        << "  Path: "
        << p2.ToString()
        << "\n";

    std::cout
        << "  Path probability: "
        << p2.GetProbability()
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 4: Explicit probability coefficient
    // ========================================================

    std::cout
        << "Test 4: Path coefficient equal to probability...\n";

    DecayVector probability_vector(
        p2,
        p2.GetProbability()
    );

    assert(probability_vector.Size() == 1);

    const auto& probability_term =
        probability_vector.GetTerms()[0];

    assert(
        std::abs(
            probability_term.coefficient -
            p2.GetProbability()
        ) < 1.0e-12
    );

    assert(
        std::abs(
            probability_term.coefficient - 0.24
        ) < 1.0e-12
    );

    std::cout
        << "  Path probability: "
        << probability_term.path.GetProbability()
        << "\n";

    std::cout
        << "  Vector coefficient: "
        << probability_term.coefficient
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 5: Default coefficient is 1
    // ========================================================

    std::cout
        << "Test 5: Default coefficient...\n";

    DecayVector default_coefficient(
        p1
    );

    assert(default_coefficient.Size() == 1);

    const auto& default_term =
        default_coefficient.GetTerms()[0];

    assert(
        std::abs(
            default_term.coefficient - 1.0
        ) < 1.0e-12
    );

    // The path probability is different from the
    // vector coefficient in this case.
    assert(
        std::abs(
            default_term.path.GetProbability() - 0.4
        ) < 1.0e-12
    );

    assert(
        std::abs(
            default_term.coefficient -
            default_term.path.GetProbability()
        ) > 1.0e-12
    );

    std::cout
        << "  Path probability: "
        << default_term.path.GetProbability()
        << "\n";

    std::cout
        << "  Vector coefficient: "
        << default_term.coefficient
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 6: Multiple terms
    // ========================================================

    std::cout
        << "Test 6: Multiple vector components...\n";

    DecayVector v;

    v.AddTerm(
        e0,
        0.7
    );

    v.AddTerm(
        p1,
        0.2
    );

    v.AddTerm(
        p2,
        0.1
    );

    assert(v.Size() == 3);

    const auto& terms =
        v.GetTerms();

    assert(terms[0].path == e0);
    assert(terms[1].path == p1);
    assert(terms[2].path == p2);

    assert(
        std::abs(terms[0].coefficient - 0.7)
        < 1.0e-12
    );

    assert(
        std::abs(terms[1].coefficient - 0.2)
        < 1.0e-12
    );

    assert(
        std::abs(terms[2].coefficient - 0.1)
        < 1.0e-12
    );

    std::cout
        << "  Vector: "
        << v.ToString()
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 7: Multiple stationary paths
    // ========================================================

    std::cout
        << "Test 7: Multiple stationary paths...\n";

    DecayVector stationary_vector;

    stationary_vector.AddTerm(
        e0,
        0.7
    );

    stationary_vector.AddTerm(
        e1,
        0.3
    );

    assert(stationary_vector.Size() == 2);

    const auto& stationary_terms =
        stationary_vector.GetTerms();

    assert(
        stationary_terms[0].path.IsStationary()
    );

    assert(
        stationary_terms[1].path.IsStationary()
    );

    assert(
        stationary_terms[0].path.GetSource() == &d0
    );

    assert(
        stationary_terms[1].path.GetSource() == &d1
    );

    assert(
        std::abs(
            stationary_terms[0].coefficient - 0.7
        ) < 1.0e-12
    );

    assert(
        std::abs(
            stationary_terms[1].coefficient - 0.3
        ) < 1.0e-12
    );

    std::cout
        << "  Vector: "
        << stationary_vector.ToString()
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 8: Addition
    // ========================================================

    std::cout
        << "Test 8: Vector addition...\n";

    DecayVector a(
        e0,
        0.4
    );

    DecayVector b(
        e1,
        0.6
    );

    DecayVector sum = a + b;

    assert(sum.Size() == 2);

    std::cout
        << "  Sum: "
        << sum.ToString()
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 9: Simplification of identical stationary paths
    // ========================================================

    std::cout
        << "Test 9: Stationary-path simplification...\n";

    DecayVector simplified;

    simplified.AddTerm(
        e0,
        0.2
    );

    simplified.AddTerm(
        e0,
        0.3
    );

    assert(simplified.Size() == 1);

    const auto& simplified_term =
        simplified.GetTerms()[0];

    assert(
        simplified_term.path == e0
    );

    assert(
        std::abs(
            simplified_term.coefficient - 0.5
        ) < 1.0e-12
    );

    std::cout
        << "  Simplified vector: "
        << simplified.ToString()
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 10: Different stationary paths are not equal
    // ========================================================

    std::cout
        << "Test 10: Distinct stationary paths...\n";

    assert(e0 != e1);
    assert(!e0.HasSameEndpoints(e1));

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 11: Scalar multiplication
    // ========================================================

    std::cout
        << "Test 11: Scalar multiplication...\n";

    DecayVector scaled =
        stationary * 2.0;

    assert(scaled.Size() == 1);

    assert(
        std::abs(
            scaled.GetTerms()[0].coefficient - 1.4
        ) < 1.0e-12
    );

    // The path probability remains unchanged.
    assert(
        std::abs(
            scaled.GetTerms()[0].path.GetProbability()
            - 1.0
        ) < 1.0e-12
    );

    std::cout
        << "  Scaled vector: "
        << scaled.ToString()
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Test 12: Vector subtraction
    // ========================================================

    std::cout
        << "Test 12: Vector subtraction...\n";

    DecayVector c(
        e0,
        0.7
    );

    DecayVector d(
        e0,
        0.2
    );

    DecayVector difference =
        c - d;

    assert(difference.Size() == 1);

    assert(
        std::abs(
            difference.GetTerms()[0].coefficient - 0.5
        ) < 1.0e-12
    );

    std::cout
        << "  Difference: "
        << difference.ToString()
        << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Final result
    // ========================================================

    std::cout << "========================================\n";
    std::cout << "          ALL TESTS PASSED\n";
    std::cout << "========================================\n";
}