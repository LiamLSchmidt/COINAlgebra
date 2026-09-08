#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/DecayCoin.h"
#include "COINAlgebra/CAlgebra.h"
#include <iostream>
#include <string>
#include <cmath>
#include <stdexcept>

// Run with: bin/coinalgebra -l -b -q tests/core/test_coin.C
// Or run scripts/test_coincidences.sh for both native and ROOT checks.
namespace {
void coin_check(bool condition, const char* expression) {
    if (!condition) throw std::runtime_error(std::string("FAIL: ") + expression);
}
#define COIN_CHECK(expression) coin_check((expression), #expression)

template<class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::invalid_argument&) { rejected = true; }
    COIN_CHECK(rejected);
}
void close(double a, double b) { COIN_CHECK(std::abs(a-b) < 1e-12); }
}
void test_coin() {
    std::cout << "Testing DecayCoin and CAlgebra\n";
    DecayQuiver q;
    auto* a=q.AddLevel("a"); auto* b=q.AddLevel("b");
    auto* c=q.AddLevel("c"); auto* d=q.AddLevel("d");
    auto* e=q.AddLevel("e"); auto* f=q.AddLevel("f");
    auto* ab=q.AddTransition("ab",a,b);
    auto* bc=q.AddTransition("bc",b,c);
    auto* cd=q.AddTransition("cd",c,d);
    auto* bd=q.AddTransition("bd",b,d);
    auto* de=q.AddTransition("de",d,e);
    auto* ef=q.AddTransition("ef",e,f);
    auto* ac=q.AddTransition("ac",a,c);
    auto* parallel=q.AddTransition("ab2",a,b);
    std::vector<DecayLevel*> order{a,b,c,d,e,f};
    DecayVector decay;
    decay.AddTerm(DecayPath(*bc),0.5);
    decay.AddTerm(DecayPath(*cd),0.4);
    decay.AddTerm(DecayPath(*bd),0.3);
    decay.AddTerm(DecayPath(a),100.0); // feeding is excluded from c_d
    CAlgebra algebra(q,order,decay);
    auto A=algebra.Coin(std::vector<DecayTransition>{*ab});
    auto D=algebra.Coin(std::vector<DecayTransition>{*de});
    auto E=algebra.Coin(std::vector<DecayTransition>{*ef});
    auto gap=algebra.Coin(std::vector<DecayTransition>{*de,*ab});
    COIN_CHECK(gap.Degree()==2 && gap.GetSource()==a && gap.GetTarget()==e);
    COIN_CHECK(gap==A.Compose(D));
    std::cout << "PASS: unordered gapped coincidence, degree and endpoints (Eq. 40)\n";
    close(algebra.ScalarConnection(A,D),0.5); // 0.3 + 0.5*0.4
    close(algebra.ScalarConnection(D,E),1.0);
    COIN_CHECK(algebra.Multiply(D,A).empty());
    COIN_CHECK(algebra.Multiply(A,A).empty());
    rejects([&]{ algebra.Coin(std::vector<DecayTransition>{*ab,*ab}); });
    rejects([&]{ algebra.Coin(std::vector<DecayTransition>{*ab,*parallel}); });
    rejects([&]{ algebra.Coin(std::vector<DecayTransition>{*ac,*bd}); });
    rejects([&]{ algebra.Coin(std::vector<DecayPath>{DecayPath(*ac),DecayPath(b)}); });
    std::cout << "PASS: connections, reversed products, repeated/parallel/crossing transitions\n";
    auto source=algebra.Coin(std::vector<DecayPath>{DecayPath(a)});
    auto target=algebra.Coin(std::vector<DecayPath>{DecayPath(b)});
    COIN_CHECK(source.Degree()==0 && source.IsStationary());
    COIN_CHECK(source.Compose(A)==A && A.Compose(target)==A);
    COIN_CHECK(source.Compose(source)==source);
    auto marked=algebra.Coin(std::vector<DecayPath>{DecayPath(a),DecayPath(*de)});
    COIN_CHECK(marked.GetFactors().size()==2 && marked.Degree()==1);
    auto path=DecayPath(std::vector<DecayTransition>{*de,*ef});
    COIN_CHECK(algebra.Coin(std::vector<DecayPath>{path})==D.Compose(E));
    std::cout << "PASS: stationary identities, separated stationary factors and path embedding\n";
    auto left=algebra.Multiply(algebra.Multiply(A,D),CAlgebra::Vector{{E,1.0}});
    auto right=algebra.Multiply(CAlgebra::Vector{{A,1.0}},algebra.Multiply(D,E));
    COIN_CHECK(left.size()==1 && right.size()==1 && left[0].coin==right[0].coin);
    close(left[0].coefficient,right[0].coefficient);
    CAlgebra zero(q,order,DecayVector{});
    COIN_CHECK(zero.Multiply(A,D).empty()); // valid basis, zero connection
    close(zero.ScalarConnection(D,E),1.0);
    auto squared=algebra.Power({{A,2.0},{D,3.0}},2);
    COIN_CHECK(squared.size()==1 && squared[0].coin==gap);
    close(squared[0].coefficient,3.0);
    rejects([&]{ algebra.Power({},0); });
    rejects([&]{ CAlgebra bad(q,{a,b,c},decay); });
    rejects([&]{ CAlgebra bad(q,{f,e,d,c,b,a},decay); });
    DecayTransition foreign("foreign",a,b);
    rejects([&]{ algebra.Coin(std::vector<DecayTransition>{foreign}); });
    // Bilinearity, term collection, cancellation and scalar coefficients.
    auto distributed=algebra.Multiply(CAlgebra::Vector{{A,2.0},{A,1.0}},
                                      CAlgebra::Vector{{D,4.0}});
    COIN_CHECK(distributed.size()==1 && distributed[0].coin==gap);
    close(distributed[0].coefficient,6.0);
    COIN_CHECK(algebra.Multiply(CAlgebra::Vector{{A,1.0},{A,-1.0}},
                               CAlgebra::Vector{{D,1.0}}).empty());
    auto embedded=algebra.Embed(DecayVector(path,0.25));
    COIN_CHECK(embedded.size()==1 && embedded[0].coin==D.Compose(E));
    close(embedded[0].coefficient,0.25);
    auto cubed=algebra.Power({{A,2.0},{D,3.0},{E,4.0}},3);
    COIN_CHECK(cubed.size()==1 && cubed[0].coin==gap.Compose(E));
    close(cubed[0].coefficient,12.0);
    std::cout << "PASS: weighted vector products, cancellation, powers and invalid inputs\n";
    // Associativity across transitions, stationary factors and gapped tensors.
    std::vector<DecayCoin> basis{A,D,E,gap,marked};
    for (auto* level : order)
        basis.push_back(algebra.Coin(std::vector<DecayPath>{DecayPath(level)}));
    for (auto* edge : q.GetTransitions())
        basis.push_back(algebra.Coin(std::vector<DecayTransition>{*edge}));
    for (const auto& x : basis) for (const auto& y : basis) for (const auto& z : basis) {
        auto lhs=algebra.Multiply(algebra.Multiply(x,y),CAlgebra::Vector{{z,1.0}});
        auto rhs=algebra.Multiply(CAlgebra::Vector{{x,1.0}},algebra.Multiply(y,z));
        COIN_CHECK(lhs.size()==rhs.size());
        if (!lhs.empty()) {
            COIN_CHECK(lhs.front().coin==rhs.front().coin);
            close(lhs.front().coefficient,rhs.front().coefficient);
        }
    }
    std::cout << "PASS: associativity for " << basis.size()*basis.size()*basis.size()
              << " basis triples\n";
    COIN_CHECK(DecayCoin{}.Empty());
    COIN_CHECK(algebra.Multiply(DecayCoin{},A).empty());
    std::cout << "PASS: empty sentinel and zero products\nAll coincidence tests passed.\n";
}
#undef COIN_CHECK
