#include "COINAlgebra/DecayLevel.h"
#include "COINAlgebra/DecayTransition.h"
#include "COINAlgebra/DecayPath.h"
#include "COINAlgebra/DecayVector.h"
#include "COINAlgebra/PathAlgebra.h"
#include "COINAlgebra/PathProjectors.h"

#include <cassert>
#include <cmath>
#include <iostream>


// ============================================================
// Helper
// ============================================================

bool AlmostEqual(
    double a,
    double b,
    double tolerance = 1.0e-12
)
{
    return std::abs(a - b) < tolerance;
}


// ============================================================
// Test
// ============================================================

void test_forms_projectors()
{
    std::cout
        << "============================================\n"
        << "     COINAlgebra Forms and Projectors Test\n"
        << "============================================\n"
        << std::endl;


    // ========================================================
    // Construct levels
    // ========================================================

    DecayLevel d0("d0");
    DecayLevel d1("d1");
    DecayLevel d2("d2");


    // ========================================================
    // Construct transitions
    // ========================================================

    DecayTransition gamma20(
        "gamma20",
        &d2,
        &d0,
        0.4
    );

    DecayTransition gamma21(
        "gamma21",
        &d2,
        &d1,
        0.5
    );

    DecayTransition gamma10(
        "gamma10",
        &d1,
        &d0,
        0.6
    );


    // ========================================================
    // Construct stationary paths
    // ========================================================

    DecayPath e0(&d0);
    DecayPath e1(&d1);
    DecayPath e2(&d2);


    // ========================================================
    // Construct length-one paths
    // ========================================================

    DecayPath p20(gamma20);
    DecayPath p21(gamma21);
    DecayPath p10(gamma10);


    // ========================================================
    // Construct input vector
    // ========================================================
    //
    //     d =
    //       0.1 e0
    //     + 0.2 e1
    //     + 0.3 e2
    //     + 0.4 p20
    //     + 0.5 p21
    //     + 0.6 p10
    //
    // The coefficients are deliberately different from the
    // transition probabilities in some cases so that we can
    // verify that the projectors preserve vector coefficients
    // rather than introducing path probabilities.
    //

    DecayVector d;

    d.AddTerm(e0, 0.1);
    d.AddTerm(e1, 0.2);
    d.AddTerm(e2, 0.3);

    d.AddTerm(p20, 0.4);
    d.AddTerm(p21, 0.5);
    d.AddTerm(p10, 0.6);


    std::cout
        << "Input vector:\n"
        << d.ToString()
        << "\n\n";


    PathAlgebra algebra;
    PathProjectors projectors;


    // ========================================================
    // SOURCE FORM
    // ========================================================

    std::cout
        << "============================================\n"
        << "              SOURCE FORM\n"
        << "============================================\n";

    //
    // Test the form against itself.
    //
    // Sources:
    //
    // d0: e0                  coefficient 0.1
    // d1: e1 + p10            coefficients 0.2, 0.6
    // d2: e2 + p20 + p21      coefficients 0.3, 0.4, 0.5
    //
    // Therefore
    //
    // <d,d>_S =
    //
    //   (0.1)^2
    //
    // + (0.2 + 0.6)^2
    //
    // + (0.3 + 0.4 + 0.5)^2
    //
    // = 0.01 + 0.64 + 1.44
    //
    // = 2.09
    //

    double sourceForm =
        algebra.SourceForm(d, d);

    std::cout
        << "<d,d>_S = "
        << sourceForm
        << "\n";

    assert(
        AlmostEqual(
            sourceForm,
            2.09
        )
    );


    // ========================================================
    // TARGET FORM
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "              TARGET FORM\n"
        << "============================================\n";

    //
    // Targets:
    //
    // d0: e0 + p20 + p10
    //     coefficients 0.1, 0.4, 0.6
    //
    // d1: e1 + p21
    //     coefficients 0.2, 0.5
    //
    // d2: e2
    //     coefficient 0.3
    //
    // Therefore
    //
    // <d,d>_T =
    //
    //   (0.1 + 0.4 + 0.6)^2
    // + (0.2 + 0.5)^2
    // + (0.3)^2
    //
    // = 1.21 + 0.49 + 0.09
    //
    // = 1.79
    //

    double targetForm =
        algebra.TargetForm(d, d);

    std::cout
        << "<d,d>_T = "
        << targetForm
        << "\n";

    assert(
        AlmostEqual(
            targetForm,
            1.79
        )
    );


    // ========================================================
    // PATH FORM
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "               PATH FORM\n"
        << "============================================\n";

    //
    // Endpoint classes:
    //
    // d0 -> d0 : e0
    // d1 -> d1 : e1
    // d2 -> d2 : e2
    // d2 -> d0 : p20
    // d2 -> d1 : p21
    // d1 -> d0 : p10
    //
    // Every path has a unique pair of endpoints here, so
    //
    // <d,d>_P
    //
    // is simply the sum of the squared coefficients:
    //
    // 0.1^2 + 0.2^2 + 0.3^2
    // + 0.4^2 + 0.5^2 + 0.6^2
    //
    // = 0.91
    //

    double pathForm =
        algebra.PathForm(d, d);

    std::cout
        << "<d,d>_P = "
        << pathForm
        << "\n";

    assert(
        AlmostEqual(
            pathForm,
            0.91
        )
    );


    // ========================================================
    // SOURCE PROJECTOR
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "           SOURCE PROJECTOR\n"
        << "============================================\n";

    DecayVector sourceD2 =
        projectors.SourceProjector(
            d,
            &d2
        );

    std::cout
        << "S_d2(d):\n"
        << sourceD2.ToString()
        << "\n";

    //
    // Only length-one paths are retained.
    //
    // Both p20 and p21 originate at d2.
    //
    // The stationary path e2 MUST NOT appear.
    //
    // The coefficients must remain:
    //
    //     0.4 p20 + 0.5 p21
    //

    assert(sourceD2.Size() == 2);

    assert(
        AlmostEqual(
            sourceD2.GetTerms()[0].coefficient,
            0.4
        )
    );

    assert(
        AlmostEqual(
            sourceD2.GetTerms()[1].coefficient,
            0.5
        )
    );


    // ========================================================
    // TARGET PROJECTOR
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "           TARGET PROJECTOR\n"
        << "============================================\n";

    DecayVector targetD0 =
        projectors.TargetProjector(
            d,
            &d0
        );

    std::cout
        << "T_d0(d):\n"
        << targetD0.ToString()
        << "\n";

    //
    // Only length-one paths are retained.
    //
    // p20 and p10 terminate at d0.
    //
    // The stationary path e0 MUST NOT appear.
    //
    // The coefficients must remain:
    //
    //     0.4 p20 + 0.6 p10
    //

    assert(targetD0.Size() == 2);

    assert(
        AlmostEqual(
            targetD0.GetTerms()[0].coefficient,
            0.4
        )
    );

    assert(
        AlmostEqual(
            targetD0.GetTerms()[1].coefficient,
            0.6
        )
    );


    // ========================================================
    // SOURCE VERTEX PROJECTOR
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "        SOURCE VERTEX PROJECTOR\n"
        << "============================================\n";

    DecayVector sourceVertexD2 =
        projectors.SourceVertexProjector(
            d,
            &d2
        );

    std::cout
        << "V_s,d2(d):\n"
        << sourceVertexD2.ToString()
        << "\n";

    //
    // Every path with source d2 contributes:
    //
    //     e2  -> 0.3
    //     p20 -> 0.4
    //     p21 -> 0.5
    //
    // Therefore
    //
    //     V_s,d2(d)
    //       = (0.3 + 0.4 + 0.5)e2
    //       = 1.2 e2.
    //

    assert(sourceVertexD2.Size() == 1);

    assert(
        sourceVertexD2.GetTerms()[0].path
        == e2
    );

    assert(
        AlmostEqual(
            sourceVertexD2.GetTerms()[0].coefficient,
            1.2
        )
    );


    // ========================================================
    // TARGET VERTEX PROJECTOR
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "        TARGET VERTEX PROJECTOR\n"
        << "============================================\n";

    DecayVector targetVertexD0 =
        projectors.TargetVertexProjector(
            d,
            &d0
        );

    std::cout
        << "V_t,d0(d):\n"
        << targetVertexD0.ToString()
        << "\n";

    //
    // Every path with target d0 contributes:
    //
    //     e0  -> 0.1
    //     p20 -> 0.4
    //     p10 -> 0.6
    //
    // Therefore
    //
    //     V_t,d0(d)
    //       = (0.1 + 0.4 + 0.6)e0
    //       = 1.1 e0.
    //

    assert(targetVertexD0.Size() == 1);

    assert(
        targetVertexD0.GetTerms()[0].path
        == e0
    );

    assert(
        AlmostEqual(
            targetVertexD0.GetTerms()[0].coefficient,
            1.1
        )
    );


    // ========================================================
    // BRANCHING PROJECTOR
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "          BRANCHING PROJECTOR\n"
        << "============================================\n";

    DecayVector branching =
        projectors.BranchingProjector(d);

    std::cout
        << "B(d):\n"
        << branching.ToString()
        << "\n";

    //
    // Only stationary paths are retained:
    //
    //     0.1 e0 + 0.2 e1 + 0.3 e2
    //
    // Their coefficients are unchanged.
    //

    assert(branching.Size() == 3);

    assert(
        branching.GetTerms()[0].path
        == e0
    );

    assert(
        branching.GetTerms()[1].path
        == e1
    );

    assert(
        branching.GetTerms()[2].path
        == e2
    );

    assert(
        AlmostEqual(
            branching.GetTerms()[0].coefficient,
            0.1
        )
    );

    assert(
        AlmostEqual(
            branching.GetTerms()[1].coefficient,
            0.2
        )
    );

    assert(
        AlmostEqual(
            branching.GetTerms()[2].coefficient,
            0.3
        )
    );


    // ========================================================
    // Explicit coefficient-preservation test
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "      COEFFICIENT PRESERVATION TEST\n"
        << "============================================\n";

    //
    // Give a path a coefficient deliberately different from
    // its intrinsic decay probability.
    //
    // p20 has probability 0.4, but its vector coefficient is
    // chosen to be 7.5.
    //

    DecayVector arbitrary;

    arbitrary.AddTerm(
        p20,
        7.5
    );

    DecayVector projected =
        projectors.SourceProjector(
            arbitrary,
            &d2
        );

    std::cout
        << "Input:\n"
        << arbitrary.ToString()
        << "\n\n";

    std::cout
        << "Projected:\n"
        << projected.ToString()
        << "\n";

    //
    // The projector must preserve 7.5 rather than replacing it
    // with the intrinsic path probability 0.4.
    //

    assert(projected.Size() == 1);

    assert(
        AlmostEqual(
            projected.GetTerms()[0].coefficient,
            7.5
        )
    );


    // ========================================================
    // Finished
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "             ALL TESTS PASSED\n"
        << "============================================\n"
        << std::endl;
}

