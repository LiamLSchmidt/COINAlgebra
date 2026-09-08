#include "COINAlgebra/Core/DecayLevel.h"
#include "COINAlgebra/Core/DecayTransition.h"
#include "COINAlgebra/Core/DecayPath.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>


void test_path()
{
    std::cout << "========================================\n";
    std::cout << "       COINAlgebra DecayPath Tests\n";
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
    // Empty path
    // ========================================================

    std::cout << "Test 1: Empty path...\n";

    DecayPath empty;

    assert(empty.Empty());
    assert(!empty.IsStationary());
    assert(empty.Length() == 0);
    assert(empty.GetSource() == nullptr);
    assert(empty.GetTarget() == nullptr);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Stationary path
    // ========================================================

    std::cout << "Test 2: Stationary path...\n";

    DecayPath e0(&d0);

    assert(!e0.Empty());
    assert(e0.IsStationary());
    assert(e0.Length() == 0);

    assert(e0.GetSource() == &d0);
    assert(e0.GetTarget() == &d0);

    assert(
        std::abs(e0.GetProbability() - 1.0)
        < 1.0e-12
    );

    std::cout << "  Path: "
              << e0.ToString()
              << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Second stationary path
    // ========================================================

    std::cout << "Test 3: Second stationary path...\n";

    DecayPath e1(&d1);

    assert(!e1.Empty());
    assert(e1.IsStationary());
    assert(e1.Length() == 0);

    assert(e1.GetSource() == &d1);
    assert(e1.GetTarget() == &d1);

    assert(
        std::abs(e1.GetProbability() - 1.0)
        < 1.0e-12
    );

    std::cout << "  Path: "
              << e1.ToString()
              << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Empty != stationary
    // ========================================================

    std::cout
        << "Test 4: Empty vs stationary...\n";

    assert(empty.Length() == e0.Length());

    assert(empty != e0);
    assert(!(empty == e0));

    std::cout << "  PASS\n\n";


    // ========================================================
    // Stationary equality
    // ========================================================

    std::cout
        << "Test 5: Stationary equality...\n";

    DecayPath e0_copy(&d0);

    assert(e0 == e0_copy);
    assert(e0 != e1);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Single-transition path
    // ========================================================

    std::cout
        << "Test 6: Single-transition path...\n";

    DecayPath p(gamma1);

    assert(!p.Empty());
    assert(!p.IsStationary());
    assert(p.Length() == 1);

    assert(p.GetSource() == &d0);
    assert(p.GetTarget() == &d1);

    assert(
        std::abs(p.GetProbability() - 0.4)
        < 1.0e-12
    );

    std::cout << "  Path: "
              << p.ToString()
              << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Two-transition path
    // ========================================================

    std::cout
        << "Test 7: Two-transition path...\n";

    std::vector<DecayTransition> transitions{
        gamma1,
        gamma2
    };

    DecayPath p02(transitions);

    assert(p02.Length() == 2);
    assert(p02.GetSource() == &d0);
    assert(p02.GetTarget() == &d2);

    assert(
        std::abs(p02.GetProbability() - 0.24)
        < 1.0e-12
    );

    std::cout << "  Path: "
              << p02.ToString()
              << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Transition access
    // ========================================================

    std::cout
        << "Test 8: Transition access...\n";

    assert(
        p02.GetTransition(0).GetName()
        == "gamma1"
    );

    assert(
        p02.GetTransition(1).GetName()
        == "gamma2"
    );

    bool caught = false;

    try
    {
        p02.GetTransition(2);
    }
    catch(const std::out_of_range&)
    {
        caught = true;
    }

    assert(caught);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Endpoint equivalence
    // ========================================================

    std::cout
        << "Test 9: Endpoint equivalence...\n";

    assert(
        e0.HasSameEndpoints(e0_copy)
    );

    assert(
        p02.HasSameEndpoints(p02)
    );

    assert(
        !e0.HasSameEndpoints(p)
    );

    assert(
        !e0.HasSameEndpoints(e1)
    );

    std::cout << "  PASS\n\n";


    // ========================================================
    // Stationary identity at source
    // ========================================================

    std::cout
        << "Test 10: Identity at source...\n";

    assert(e0.IsComposableWith(p));

    DecayPath left_identity =
        e0.Compose(p);

    assert(left_identity == p);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Stationary identity at target
    // ========================================================

    std::cout
        << "Test 11: Identity at target...\n";

    assert(p.IsComposableWith(e1));

    DecayPath right_identity =
        p.Compose(e1);

    assert(right_identity == p);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Stationary self-composition
    // ========================================================

    std::cout
        << "Test 12: Stationary self-composition...\n";

    assert(e0.IsComposableWith(e0));

    DecayPath e0_squared =
        e0.Compose(e0);

    assert(e0_squared == e0);
    assert(e0_squared.IsStationary());
    assert(e0_squared.GetSource() == &d0);
    assert(e0_squared.GetTarget() == &d0);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Ordinary composition
    // ========================================================

    std::cout
        << "Test 13: Ordinary composition...\n";

    DecayPath q(gamma2);

    assert(p.IsComposableWith(q));

    DecayPath composed =
        p.Compose(q);

    assert(composed.Length() == 2);
    assert(composed.GetSource() == &d0);
    assert(composed.GetTarget() == &d2);

    assert(
        std::abs(composed.GetProbability() - 0.24)
        < 1.0e-12
    );

    std::cout << "  Path: "
              << composed.ToString()
              << "\n";

    std::cout << "  PASS\n\n";


    // ========================================================
    // Probability under composition
    // ========================================================

    std::cout
        << "Test 14: Composition probability...\n";

    double expected =
        p.GetProbability() *
        q.GetProbability();

    assert(
        std::abs(
            composed.GetProbability() - expected
        ) < 1.0e-12
    );

    std::cout << "  PASS\n\n";


    // ========================================================
    // Non-composable paths
    // ========================================================

    std::cout
        << "Test 15: Non-composable paths...\n";

    assert(!p.IsComposableWith(e0));

    bool caught_invalid = false;

    try
    {
        p.Compose(e0);
    }
    catch(const std::invalid_argument&)
    {
        caught_invalid = true;
    }

    assert(caught_invalid);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Invalid transition sequence
    // ========================================================

    std::cout
        << "Test 16: Invalid path validation...\n";

    DecayTransition gamma_bad(
        "gamma_bad",
        &d2,
        &d0,
        0.5
    );

    bool caught_bad_path = false;

    try
    {
        std::vector<DecayTransition> bad{
            gamma1,
            gamma_bad
        };

        DecayPath invalid(bad);
    }
    catch(const std::invalid_argument&)
    {
        caught_bad_path = true;
    }

    assert(caught_bad_path);

    std::cout << "  PASS\n\n";


    // ========================================================
    // Final result
    // ========================================================

    std::cout << "========================================\n";
    std::cout << "          ALL TESTS PASSED\n";
    std::cout << "========================================\n";
}
