#include "COINAlgebra/DecayQuiver.h"
#include "COINAlgebra/DecayPath.h"
#include "COINAlgebra/DecayVector.h"

#include <cassert>
#include <cmath>
#include <iostream>

void test_vector()
{
    std::cout
        << "========================================"
        << std::endl;

    std::cout
        << "Testing DecayVector"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl;


    // --------------------------------------------------------
    // Construct decay quiver
    // --------------------------------------------------------

    DecayQuiver quiver;

    DecayLevel* d0 =
        quiver.AddLevel("d0");

    DecayLevel* d1 =
        quiver.AddLevel("d1");

    DecayLevel* d2 =
        quiver.AddLevel("d2");


    DecayTransition* gamma1 =
        quiver.AddTransition(
            "gamma1",
            d2,
            d1,
            1.0
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
            1.0
        );


    // --------------------------------------------------------
    // Construct paths
    // --------------------------------------------------------

    DecayPath p1(*gamma1);

    DecayPath p2(*gamma2);

    DecayPath p3 =
        p1.Compose(p2);


    std::cout
        << "p1 = "
        << p1.ToString()
        << std::endl;

    std::cout
        << "p2 = "
        << p2.ToString()
        << std::endl;

    std::cout
        << "p3 = "
        << p3.ToString()
        << std::endl;


    // --------------------------------------------------------
    // Construct vectors
    // --------------------------------------------------------

    DecayVector v1(
        p1,
        2.0
    );

    DecayVector v2(
        p3,
        3.0
    );


    std::cout
        << std::endl
        << "v1 = "
        << v1.ToString()
        << std::endl;

    std::cout
        << "v2 = "
        << v2.ToString()
        << std::endl;


    // --------------------------------------------------------
    // Addition
    // --------------------------------------------------------

    DecayVector sum =
        v1 + v2;

    std::cout
        << "v1 + v2 = "
        << sum.ToString()
        << std::endl;


    // --------------------------------------------------------
    // Scalar multiplication
    // --------------------------------------------------------

    DecayVector scaled =
        2.0 * sum;

    std::cout
        << "2(v1 + v2) = "
        << scaled.ToString()
        << std::endl;


    // --------------------------------------------------------
    // Subtraction
    // --------------------------------------------------------

    DecayVector difference =
        v1 - v2;

    std::cout
        << "v1 - v2 = "
        << difference.ToString()
        << std::endl;


    // --------------------------------------------------------
    // v - v = 0
    // --------------------------------------------------------

    DecayVector zero =
        v1 - v1;

    std::cout
        << "v1 - v1 = "
        << zero.ToString()
        << std::endl;

    assert(zero.Empty());


    // --------------------------------------------------------
    // v + 0 = v
    // --------------------------------------------------------

    DecayVector recovered =
        v1 + zero;

    assert(
        recovered.Size() ==
        v1.Size()
    );

    assert(
        std::abs(
            recovered.GetTerms()[0].coefficient -
            2.0
        ) < 1e-12
    );


    // --------------------------------------------------------
    // (v1 + v2) - v2 = v1
    // --------------------------------------------------------

    DecayVector cancellation =
        (v1 + v2) - v2;

    assert(
        cancellation.Size() ==
        v1.Size()
    );

    assert(
        std::abs(
            cancellation.GetTerms()[0].coefficient -
            2.0
        ) < 1e-12
    );


    // --------------------------------------------------------
    // Scalar distributivity
    //
    // 2(v1 + v2) = 2v1 + 2v2
    // --------------------------------------------------------

    DecayVector left =
        2.0 * (v1 + v2);

    DecayVector right =
        (2.0 * v1) + (2.0 * v2);

    assert(
        left.Size() ==
        right.Size()
    );


    // --------------------------------------------------------
    // Negative scalar
    // --------------------------------------------------------

    DecayVector negative =
        -1.0 * v1;

    std::cout
        << "-v1 = "
        << negative.ToString()
        << std::endl;

    assert(
        std::abs(
            negative.GetTerms()[0].coefficient +
            2.0
        ) < 1e-12
    );


    std::cout
        << std::endl
        << "DecayVector tests passed."
        << std::endl;
}
