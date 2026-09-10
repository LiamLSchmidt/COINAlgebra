#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Detection/DetectionMaps.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
void requireCoin(bool ok) { if (!ok) throw std::runtime_error("coincidence detection mismatch"); }
void nearCoin(double a, double b) { requireCoin(std::abs(a-b) < 1e-12); }
void collectCoin(CAlgebra::Vector& v, const DecayCoin& c, double w) {
    if (w == 0) return;
    for (auto& t : v) if (t.coin == c) { t.coefficient += w; return; }
    v.push_back({c,w});
}
void compareCoin(const CAlgebra::Vector& a, const CAlgebra::Vector& b) {
    for (const auto* v : {&a,&b}) for (const auto& t : *v) {
        double x=0,y=0;
        for (const auto& u : a) if (u.coin==t.coin) x+=u.coefficient;
        for (const auto& u : b) if (u.coin==t.coin) y+=u.coefficient;
        nearCoin(x,y);
    }
}
}

void test_coincidence_detection() {
    DecayQuiver q;
    auto* a=q.AddLevel("a",400); auto* b=q.AddLevel("b",300);
    auto* c=q.AddLevel("c",100); auto* d=q.AddLevel("d",0);
    auto* e=q.AddLevel("e",0); // second terminal branch
    std::vector<DecayLevel*> order{a,b,c,d,e};
    std::vector<DecayTransition*> edges{
        q.AddTransition("ab",a,b),q.AddTransition("ab2",a,b),
        q.AddTransition("ac",a,c),q.AddTransition("bc",b,c),
        q.AddTransition("bd",b,d),q.AddTransition("cd",c,d),
        q.AddTransition("ce",c,e)};
    const std::vector<double> weights{.2,.3,.5,.7,.3,.6,.4};
    const std::vector<double> populations{.6,.3,.1,0,0};
    DecayVector tau,branch,terminal;
    for (size_t i=0;i<edges.size();++i) tau.AddTerm(DecayPath(*edges[i]),weights[i]);
    for (size_t i=0;i<order.size();++i) branch.AddTerm(DecayPath(order[i]),populations[i]);
    terminal.AddTerm(DecayPath(d),1); terminal.AddTerm(DecayPath(e),1);
    CAlgebra physical(q,order,branch+tau);
    PathAlgebra paths(q);
    // Independent complete-cascade recursion, then enumerate every nonempty
    // observed subset. No CAlgebra connection/product is used for weights.
    for (size_t N : {1u,2u,4u}) for (int scenario=0;scenario<3;++scenario) {
        DetectionMaps::EfficiencyMap peak,total,alpha;
        for (size_t i=0;i<edges.size();++i) {
            const auto name=edges[i]->GetName();
            peak[name]=scenario==0 ? 1.0 : (scenario==2 && i==0 ? 0.0 : .2+.05*i);
            total[name]=scenario==0 ? 1.0 : .8;
            alpha[name]=scenario==0 ? 0.0 : .15*i;
        }
        DetectionMaps maps(peak,total,N,alpha);
        CAlgebra out(q,order,maps.SummingOut(tau));
        auto actual=out.Multiply(out.Multiply(out.Embed(branch),
            maps.SummingInExpansion(out,tau)),out.Embed(terminal));
        CAlgebra::Vector expected, physicalPairs, resolvedPairs;
        std::vector<size_t> cascade;
        std::function<void(DecayLevel*,DecayLevel*,double)> walk;
        walk=[&](DecayLevel* initial,DecayLevel* level,double probability) {
            bool sink=true;
            for (size_t i=0;i<edges.size();++i) if (edges[i]->GetSource()==level) {
                sink=false; cascade.push_back(i);
                walk(initial,edges[i]->GetTarget(),probability*weights[i]);
                cascade.pop_back();
            }
            if (!sink) return;
            for (size_t mask=1;mask<(size_t(1)<<cascade.size());++mask) {
                std::vector<DecayPath> factors{DecayPath(initial)};
                double w=probability, resolved=probability; size_t count=0;
                for (size_t j=0;j<cascade.size();++j) {
                    const auto* edge=edges[cascade[j]];
                    const auto name=edge->GetName();
                    const double gamma=1/(1+alpha[name]);
                    if (mask&(size_t(1)<<j)) {
                        factors.emplace_back(*edge); ++count;
                        w*=gamma*peak[name]; resolved*=gamma*peak[name];
                    } else {
                        w*=1-gamma*total[name]/double(N);
                        if (N>=2) resolved*=1-2*gamma*total[name]/double(N);
                    }
                }
                if (count==2) collectCoin(physicalPairs,physical.Coin(factors),probability);
                factors.emplace_back(level);
                if (count==2 && N>=2) collectCoin(resolvedPairs,physical.Coin(factors),
                    resolved*double(N-1)/double(N));
                w/=std::pow(double(N),double(count-1));
                collectCoin(expected,physical.Coin(factors),w);
            }
        };
        for (size_t i=0;i<order.size();++i) walk(order[i],order[i],populations[i]);
        compareCoin(actual,expected);
        if (N>=2) {
            CAlgebra twoOut(q,order,maps.AvoidDetectors(tau,2));
            auto resolved=twoOut.Multiply(twoOut.Multiply(twoOut.Embed(branch),
                twoOut.Power(twoOut.Embed(maps.FullEnergyHit(tau)),2)),twoOut.Embed(terminal));
            for (auto& t : resolved) t.coefficient*=double(N-1)/double(N);
            compareCoin(resolved,resolvedPairs);
        }
        compareCoin(physical.Embed(maps.AvoidDetectors(tau,0)),physical.Embed(tau));
        compareCoin(physical.Embed(maps.AvoidDetectors(tau,1)),physical.Embed(maps.SummingOut(tau)));
        auto pairs=physical.Multiply(physical.Embed(branch),physical.Power(physical.Embed(tau),2));
        compareCoin(pairs,physicalPairs);
        // Maps commute with embedding and with composable path multiplication.
        auto path=paths.Multiply(DecayVector(DecayPath(*edges[0])),DecayVector(DecayPath(*edges[3])));
        compareCoin(maps.FullEnergyHit(physical.Embed(path)),physical.Embed(maps.FullEnergyHit(path)));
        compareCoin(maps.TotalHit(physical.Embed(path)),physical.Embed(maps.TotalHit(path)));
        compareCoin(maps.SummingOut(physical.Embed(path)),physical.Embed(maps.SummingOut(path)));
        compareCoin(physical.Embed(maps.FullEnergyHit(path)),physical.Multiply(
            physical.Embed(maps.FullEnergyHit(DecayVector(DecayPath(*edges[0])))),
            physical.Embed(maps.FullEnergyHit(DecayVector(DecayPath(*edges[3]))))));
        // Mapping observed factors in a fixed fiber is distinct from mapping
        // connecting edges: a--b [unobserved b--c] c--d.
        auto upper=physical.Embed(DecayVector(DecayPath(*edges[0]),.2));
        auto lower=physical.Embed(DecayVector(DecayPath(*edges[5]),.6));
        auto fixed=maps.FullEnergyHit(physical.Multiply(upper,lower));
        auto changed=out.Multiply(maps.FullEnergyHit(upper),maps.FullEnergyHit(lower));
        if (!fixed.empty()) nearCoin(changed.empty()?0:changed[0].coefficient,
            fixed[0].coefficient*(1-total["bc"]/(1+alpha["bc"])/double(N)));
    }
    DetectionMaps maps({{"ab",.5}},{{"ab",.6}},2);
    bool rejected=false;
    try { maps.SummingInExpansion(physical,branch); }
    catch (const std::invalid_argument&) { rejected=true; }
    requireCoin(rejected);
    rejected=false;
    try { maps.FullEnergyHit(physical.Embed(tau)); }
    catch (const std::invalid_argument&) { rejected=true; }
    requireCoin(rejected);
    rejected=false;
    try { maps.AvoidDetectors(tau,3); }
    catch (const std::invalid_argument&) { rejected=true; }
    requireCoin(rejected);
    // Exact detector assignments: three perfect photons, first two required
    // as separate clean peaks. The third must avoid BOTH occupied detectors.
    for (int N : {2,4}) {
        int accepted=0;
        for (int i=0;i<N;++i) for (int j=0;j<N;++j) for (int k=0;k<N;++k)
            if (i!=j && k!=i && k!=j) ++accepted;
        const double exact=double(accepted)/(N*N*N);
        nearCoin(exact,double(N-1)*(N-2)/(N*N));
        const double printed=double(N-1)*(N-1)/(N*N);
        requireCoin(std::abs(exact-printed)>1e-12); // documented Eq. 72 limitation
    }
    std::cout << "PASS: physical coincidences and Eq. 70 match independent cascade/subset enumeration"
                 " for N=1,2,4, IC, zero/perfect efficiency, parallel edges, multiple populations and sinks.\n";
}
