// Run: bin/coinalgebra -l -b -q examples/calculations/coincidence.C
#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Detection/DetectionMaps.h"
#include <iomanip>
#include <iostream>

void coincidence() {
    DecayQuiver q;
    auto* a=q.AddLevel("A",600); auto* b=q.AddLevel("B",400);
    auto* c=q.AddLevel("C",100); auto* g=q.AddLevel("G",0);
    auto* ab=q.AddTransition("AB",a,b);
    auto* bc=q.AddTransition("BC",b,c);
    auto* bg=q.AddTransition("BG",b,g);
    auto* cg=q.AddTransition("CG",c,g);
    std::vector<DecayLevel*> order{a,b,c,g};
    DecayVector tau;
    tau.AddTerm(DecayPath(*ab),1); tau.AddTerm(DecayPath(*bc),.75);
    tau.AddTerm(DecayPath(*bg),.25); tau.AddTerm(DecayPath(*cg),1);
    DecayVector branch(DecayPath(a),1), terminal(DecayPath(g),1);
    // Deliberately synthetic calibration; replace these with measured values.
    DetectionMaps maps({{"AB",.4},{"BC",.3},{"BG",.35},{"CG",.5}},
                       {{"AB",.6},{"BC",.5},{"BG",.55},{"CG",.7}},4,
                       {{"AB",0},{"BC",1},{"BG",0},{"CG",.25}});
    CAlgebra physical(q,order,branch+tau);
    const auto pairs=physical.Multiply(physical.Embed(branch),physical.Power(physical.Embed(tau),2));
    CAlgebra out(q,order,maps.SummingOut(tau));
    const auto spectrum=out.Multiply(out.Multiply(out.Embed(branch),
        maps.SummingInExpansion(out,tau)),out.Embed(terminal));
    std::cout << std::setprecision(12);
    auto print=[](const char* title,const CAlgebra::Vector& vector) {
        std::cout << "\n" << title << "\n";
        for (const auto& term : vector)
            std::cout << term.coefficient << "  " << term.coin.ToString() << "\n";
    };
    print("Physical transition pairs per decay (Eq. 61)",pairs);
    print("Independent full-energy pair probabilities, without summing corrections",
          maps.FullEnergyHit(pairs));
    CAlgebra twoOut(q,order,maps.AvoidDetectors(tau,2));
    auto resolved=twoOut.Multiply(twoOut.Multiply(twoOut.Embed(branch),
        twoOut.Power(twoOut.Embed(maps.FullEnergyHit(tau)),2)),twoOut.Embed(terminal));
    for (auto& term : resolved) term.coefficient*=3.0/4.0;
    print("Two distinct clean detector peaks per decay, no summed-in groups",resolved);
    print("Expected peaks per decay including disconnected sums (Eq. 70)",spectrum);
    std::cout << "\nFor AB and CG: physical=.75; independent detection=.12;"
                 " resolved distinct-detector pair=.07875; same-detector sum with BC avoiding that detector=.028125.\n";
}
