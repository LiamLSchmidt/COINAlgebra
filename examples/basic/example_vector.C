#include "COINAlgebra/Core/DecayLevel.h"
#include "COINAlgebra/Core/DecayTransition.h"
#include "COINAlgebra/Core/DecayPath.h"
#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Core/DecayVector.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>


void example_vector()
{
    std::cout
        << "============================================\n"
        << "       COINAlgebra Vector Example\n"
        << "============================================\n\n";


    // ========================================================
    // 1. Construct five-level quiver
    // ========================================================

    DecayQuiver quiver;

    DecayLevel* d0 =
        quiver.AddLevel("d0");

    DecayLevel* d1 =
        quiver.AddLevel("d1");

    DecayLevel* d2 =
        quiver.AddLevel("d2");

    DecayLevel* d3 =
        quiver.AddLevel("d3");

    DecayLevel* d4 =
        quiver.AddLevel("d4");


    // ========================================================
    // 2. Construct fully connected decay quiver
    //
    // Every excited level has a transition to every lower
    // level.
    //
    // The outgoing probabilities from each level sum to one.
    // ========================================================

    // d4 -> d0, d1, d2, d3

    quiver.AddTransition(
        "gamma40",
        d4,
        d0,
        0.10
    );

    quiver.AddTransition(
        "gamma41",
        d4,
        d1,
        0.20
    );

    quiver.AddTransition(
        "gamma42",
        d4,
        d2,
        0.30
    );

    quiver.AddTransition(
        "gamma43",
        d4,
        d3,
        0.40
    );


    // d3 -> d0, d1, d2

    quiver.AddTransition(
        "gamma30",
        d3,
        d0,
        0.20
    );

    quiver.AddTransition(
        "gamma31",
        d3,
        d1,
        0.30
    );

    DecayTransition* g32 = quiver.AddTransition(
        "gamma32",
        d3,
        d2,
        0.50
    );


    // d2 -> d0, d1

    quiver.AddTransition(
        "gamma20",
        d2,
        d0,
        0.30
    );

   DecayTransition* g21 =  quiver.AddTransition(
        "gamma21",
        d2,
        d1,
        0.70
    );


    // d1 -> d0

    DecayTransition* g10 = quiver.AddTransition(
        "gamma10",
        d1,
        d0,
        1.00
    );


    // ========================================================
    // 3. Print quiver
    // ========================================================

    std::cout
        << "Quiver:\n\n";

    quiver.Print();


    // ========================================================
    // 4. Construct stationary paths
    //
    // e_i : d_i -> d_i
    //
    // These have:
    //
    //     Length()       = 0
    //     IsStationary() = true
    //     GetProbability() = 1
    //
    // The probability is one because the probability is the
    // empty product for a length-zero path.
    // ========================================================

    DecayPath e0(d0);
    DecayPath e1(d1);
    DecayPath e2(d2);
    DecayPath e3(d3);
    DecayPath e4(d4);


    // ========================================================
    // 5. Construct the decay vector
    //
    // For this example we explicitly choose the coefficients
    // to equal the intrinsic decay probabilities:
    //
    //     coefficient(path) = path.GetProbability()
    //
    // Thus the stationary paths receive coefficient 1.
    // ========================================================

    DecayVector decay_vector;


    // --------------------------------------------------------
    // Stationary components
    // --------------------------------------------------------

    decay_vector.AddTerm(
        e0,
        0.10 //e0.GetProbability()
    );

    decay_vector.AddTerm(
        e1,
        0.20 //e1.GetProbability()
    );

    decay_vector.AddTerm(
        e2,
        0.30 // e2.GetProbability()
    );

    decay_vector.AddTerm(
        e3,
        0.25 //e3.GetProbability()
    );

    decay_vector.AddTerm(
        e4,
        0.15 //e4.GetProbability()
    );


    // --------------------------------------------------------
    // Length-one components
    // --------------------------------------------------------

    for(const auto* transition :
        quiver.GetTransitions())
    {
        DecayPath path(*transition);

        decay_vector.AddTerm(
            path,
            path.GetProbability()
        );
    }


    // ========================================================
    // 6. Print stationary paths
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "          STATIONARY PATHS\n"
        << "============================================\n\n";

    for(const auto& level :
        quiver.GetLevels())
    {
        DecayPath path(level);

        std::cout
            << std::left
            << std::setw(12)
            << path.ToString()
            << " length = "
            << std::setw(3)
            << path.Length()
            << " probability = "
            << path.GetProbability()
            << "\n";
    }


    // ========================================================
    // 7. Print length-one paths
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "           LENGTH-ONE PATHS\n"
        << "============================================\n\n";

    for(const auto* transition :
        quiver.GetTransitions())
    {
        DecayPath path(*transition);

        std::cout
            << std::left
            << std::setw(35)
            << path.ToString()
            << " P = "
            << std::setprecision(10)
            << path.GetProbability()
            << "\n";
    }


    // ========================================================
    // 8. Print complete vector
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "       PROBABILITY-WEIGHTED VECTOR\n"
        << "============================================\n\n";

    decay_vector.Print();


    // ========================================================
    // 9. Summary
    // ========================================================

    std::cout
        << "\n============================================\n"
        << "                  SUMMARY\n"
        << "============================================\n\n";

    std::cout
        << "Number of levels: "
        << quiver.GetLevels().size()
        << "\n";

    std::cout
        << "Number of transitions: "
        << quiver.GetTransitions().size()
        << "\n";

    std::cout
        << "Number of stationary paths: 5\n";

    std::cout
        << "Number of length-one paths: "
        << quiver.GetTransitions().size()
        << "\n";

    std::cout
        << "Number of vector terms: "
        << decay_vector.Size()
        << "\n";

    std::cout
        << "Expected number of vector terms: 15\n";

    std::cout
        << "\n============================================\n";
    // Probability vector testing: 
    PathAlgebra algebra(quiver);
    PathProjectors projectors;
    DecayProbability pb(algebra,projectors);
    DecayVector feedingVector = pb.FeedingVector(decay_vector);
    feedingVector.PrintTable();
    DecayPath pathi(*g32);
    DecayPath pathj(*g10);
    double prob = pb.FeedingProbability(decay_vector,pathi);
    double coin = pb.CoincidenceProbability(decay_vector,pathi, pathj);
    std::cout << prob <<"\n";
    std::cout << coin <<"\n";
}