#include "COINAlgebra/Core/DecayQuiver.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>


void test_quiver()
{
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "       COINAlgebra Quiver Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;


    // --------------------------------------------------------
    // Construct the decay quiver
    //
    //              gamma1
    //         d2 ----------> d1
    //          |              |
    //       gamma3          gamma2
    //          |              |
    //          v              v
    //         d0 <------------
    //
    // gamma1 = 0.7
    // gamma2 = 1.0
    // gamma3 = 0.3
    // --------------------------------------------------------

    // Energy is optional, validated and independent of names and graph identity.
    DecayLevel unknown("unknown");
    assert(!unknown.HasEnergy());
    bool unsetThrows=false;
    try { unknown.GetEnergy(); } catch(const std::logic_error&) { unsetThrows=true; }
    assert(unsetThrows);
    DecayLevel ground("ground",0.0);assert(ground.HasEnergy() && ground.GetEnergy()==0.0);
    ground.SetEnergy(123.456789);ground.SetName("renamed");assert(ground.GetEnergy()==123.456789);
    auto copy=ground;assert(copy.HasEnergy() && copy.GetEnergy()==ground.GetEnergy());
    for(double bad:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        bool rejected=false;try{ground.SetEnergy(bad);}catch(const std::invalid_argument&){rejected=true;}
        assert(rejected && ground.GetEnergy()==123.456789);
    }
    ground.ClearEnergy();assert(!ground.HasEnergy());
    DecayQuiver energyQuiver;
    auto* excited=energyQuiver.AddLevel("excited",200.5);assert(excited->GetEnergy()==200.5);
    bool rejected=false;try{energyQuiver.AddLevel("invalid",-1);}catch(const std::invalid_argument&){rejected=true;}
    assert(rejected && energyQuiver.GetLevels().size()==1);

    DecayQuiver quiver;

    DecayLevel* d2 = quiver.AddLevel("d2");
    DecayLevel* d1 = quiver.AddLevel("d1");
    DecayLevel* d0 = quiver.AddLevel("d0");

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
    // Print
    // --------------------------------------------------------

    quiver.Print();


    // --------------------------------------------------------
    // Check vertices
    // --------------------------------------------------------

    assert(quiver.GetLevel("d2") == d2);
    assert(quiver.GetLevel("d1") == d1);
    assert(quiver.GetLevel("d0") == d0);

    assert(quiver.GetLevels().size() == 3);


    // --------------------------------------------------------
    // Check transitions
    // --------------------------------------------------------

    assert(quiver.GetTransition("gamma1") == gamma1);
    assert(quiver.GetTransition("gamma2") == gamma2);
    assert(quiver.GetTransition("gamma3") == gamma3);

    assert(quiver.GetTransitions().size() == 3);


    // --------------------------------------------------------
    // Check source and target maps
    // --------------------------------------------------------

    assert(gamma1->GetSource() == d2);
    assert(gamma1->GetTarget() == d1);

    assert(gamma2->GetSource() == d1);
    assert(gamma2->GetTarget() == d0);

    assert(gamma3->GetSource() == d2);
    assert(gamma3->GetTarget() == d0);


    // --------------------------------------------------------
    // Check probabilities
    // --------------------------------------------------------

    assert(
        std::abs(gamma1->GetProbability() - 0.7)
        < 1.0e-12
    );

    assert(
        std::abs(gamma2->GetProbability() - 1.0)
        < 1.0e-12
    );

    assert(
        std::abs(gamma3->GetProbability() - 0.3)
        < 1.0e-12
    );


    // --------------------------------------------------------
    // Check branching normalization
    // --------------------------------------------------------

    assert(
        std::abs(
            quiver.GetOutgoingProbability(d2) - 1.0
        ) < 1.0e-12
    );

    assert(
        std::abs(
            quiver.GetOutgoingProbability(d1) - 1.0
        ) < 1.0e-12
    );

    assert(quiver.IsNormalized(d2));
    assert(quiver.IsNormalized(d1));


    // --------------------------------------------------------
    // Check composability
    //
    // gamma1 : d2 -> d1
    // gamma2 : d1 -> d0
    //
    // Therefore gamma1 followed by gamma2 is composable.
    // --------------------------------------------------------

    assert(
        quiver.IsComposable(gamma1, gamma2)
    );

    // gamma3 : d2 -> d0
    //
    // gamma1 followed by gamma3 is not composable.

    assert(
        !quiver.IsComposable(gamma1, gamma3)
    );


    // --------------------------------------------------------
    // Check direct transitions
    // --------------------------------------------------------

    assert(
        quiver.HasDirectTransition(d2, d1)
    );

    assert(
        quiver.HasDirectTransition(d1, d0)
    );

    assert(
        quiver.HasDirectTransition(d2, d0)
    );


    std::cout << std::endl;
    std::cout << "All quiver tests passed." << std::endl;
    std::cout << std::endl;
}
