#include "COINAlgebra/DecayQuiver.h"
#include "COINAlgebra/DecayPath.h"

#include <cassert>
#include <cmath>
#include <iostream>


void test_paths()
{
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "       COINAlgebra Path Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;


    // --------------------------------------------------------
    // Construct decay quiver
    // --------------------------------------------------------

    DecayQuiver quiver;

    DecayLevel* d2 =
        quiver.AddLevel("d2");

    DecayLevel* d1 =
        quiver.AddLevel("d1");

    DecayLevel* d0 =
        quiver.AddLevel("d0");


    DecayTransition* gamma1 =
        quiver.AddTransition(
            "gamma1",
            d2,
            d1,
            0.7
        );

    DecayTransition* gamma2 =
        quiver.AddTransition(
            "gamma2",
            d1,
            d0,
            1.0
        );

    DecayTransition* gamma3 =
        quiver.AddTransition(
            "gamma3",
            d2,
            d0,
            0.3
        );


    // --------------------------------------------------------
    // Construct elementary paths
    // --------------------------------------------------------

    DecayPath p1(*gamma1);

    DecayPath p2(*gamma2);

    DecayPath p3(*gamma3);


    // --------------------------------------------------------
    // Check elementary path probabilities
    // --------------------------------------------------------

    assert(
        std::abs(
            p1.GetProbability() - 0.7
        ) < 1.0e-12
    );

    assert(
        std::abs(
            p2.GetProbability() - 1.0
        ) < 1.0e-12
    );

    assert(
        std::abs(
            p3.GetProbability() - 0.3
        ) < 1.0e-12
    );


    // --------------------------------------------------------
    // Compose
    //
    // p1 : d2 -> d1
    // p2 : d1 -> d0
    //
    // p1.Compose(p2)
    //
    // represents
    //
    // p2 p1 : d2 -> d0
    // --------------------------------------------------------

    assert(
        p1.IsComposableWith(p2)
    );

    DecayPath p21 =
        p1.Compose(p2);


    // --------------------------------------------------------
    // Check composed path
    // --------------------------------------------------------

    assert(
        p21.GetSource() == d2
    );

    assert(
        p21.GetTarget() == d0
    );

    assert(
        p21.Length() == 2
    );


    // --------------------------------------------------------
    // Check path probability
    //
    // P(p2 p1)
    //     = P(p1) P(p2)
    //     = 0.7 * 1.0
    //     = 0.7
    // --------------------------------------------------------

    assert(
        std::abs(
            p21.GetProbability() - 0.7
        ) < 1.0e-12
    );


    // --------------------------------------------------------
    // Check path representation
    // --------------------------------------------------------

    std::cout
        << "p1  = "
        << p1.ToString()
        << std::endl;

    std::cout
        << "p2  = "
        << p2.ToString()
        << std::endl;

    std::cout
        << "p21 = "
        << p21.ToString()
        << std::endl;

    std::cout
        << "p3  = "
        << p3.ToString()
        << std::endl;


    // --------------------------------------------------------
    // The two decay histories from d2 to d0 are
    //
    //     p2 p1
    //
    // and
    //
    //     p3.
    //
    // Their probabilities sum to unity.
    // --------------------------------------------------------

    const double total =
        p21.GetProbability()
        +
        p3.GetProbability();

    assert(
        std::abs(total - 1.0)
        < 1.0e-12
    );


    // --------------------------------------------------------
    // Non-composable paths
    // --------------------------------------------------------

    assert(
        !p1.IsComposableWith(p3)
    );


    std::cout << std::endl;
    std::cout
        << "All path tests passed."
        << std::endl;
    std::cout << std::endl;
}

