#include "pocket_engineer/engine.hpp"

#include <cstdlib>
#include <iostream>

namespace { int failures{}; void expect(bool value, const char* message){if(!value){++failures;std::cerr<<"FAILED: "<<message<<'\n';}} }
int main() {
    pocket_engineer::Engine e;
    auto identified=e.identify("Use a K-map to simplify F(A,B,C,D) = Σm(0,1,2,3)");expect(identified.candidates.front().topic=="kmap_minimization","K-map identification before solving");
    auto nodal=e.identify("Find Vout using nodal analysis and KCL");expect(nodal.candidates.front().topic=="dc_nodal_analysis","circuit identification before solving");
    auto identified_bernoulli=e.identify("Solve this Bernoulli differential equation");expect(identified_bernoulli.candidates.front().topic=="bernoulli","Bernoulli identification");
    auto identified_superposition=e.identify("Use superposition to find Vout");expect(identified_superposition.candidates.front().topic=="superposition","superposition identification");
    auto identified_loop=e.identify("Trace this for loop");expect(identified_loop.candidates.front().topic=="loops","loop identification");
    auto algebra=e.solve({"algebra","simplify","(x^2 - 1)/(x - 1)"}); expect(algebra.answer=="x + 1, with x ≠ 1","domain-aware cancellation"); expect(algebra.verification.status==pocket_engineer::VerificationStatus::verified_exact,"algebra verifier");
    auto numeric=e.solve({"algebra","numeric_evaluation","2*(3+4)^2"});expect(numeric.answer=="98","arithmetic precedence");
    auto quadratic=e.solve({"algebra","quadratic_equation","x^2-5x+6=0"});expect(quadratic.answer=="x₁ = 3, x₂ = 2","quadratic formula");
    auto derivative=e.solve({"calculus","differentiation","3x^4"});expect(derivative.answer=="12x^3","power-rule derivative");
    auto integral=e.solve({"calculus","integration","integrate 4x^3 dx"});expect(integral.answer=="x^4 + C","reverse power rule");
    auto tangent=e.solve({"calculus","tangent_line","3x^2;at=2"});expect(tangent.answer=="y = 12x + -12","tangent line");
    auto area=e.solve({"calculus","definite_integral","integrate 3x^2 from 0 to 2"});expect(area.answer=="∫ = 8","definite integral");
    auto curve=e.solve({"calculus","curve_analysis","3x^2"});expect(curve.answer=="critical point: (0, 0); local/global minimum","monomial curve analysis");
    auto matrix=e.solve({"linear_algebra","rref","1,2,5;3,4,11"});expect(matrix.status=="success"&&matrix.answer.find("1")!=std::string::npos,"matrix rref");
    auto determinant=e.solve({"linear_algebra","determinant","1,2;3,4"});expect(determinant.answer=="det(A) = -2","matrix determinant");
    auto inverse=e.solve({"linear_algebra","inverse","1,2;3,4"});expect(inverse.status=="success"&&inverse.answer.find("-2")!=std::string::npos,"matrix inverse");
    auto dot=e.solve({"linear_algebra","vectors","dot:1,2,3|4,5,6"});expect(dot.answer=="v·w = 32","vector dot product");
    auto system=e.solve({"linear_algebra","linear_system","1,1,3;2,-1,0"});expect(system.answer=="x1 = 1, x2 = 2","linear system");
    auto product=e.solve({"linear_algebra","multiply","1,2;3,4|2,0;1,2"});expect(product.answer=="[[4, 4]; [10, 8]]","matrix multiplication");
    auto rank=e.solve({"linear_algebra","rank","1,2;2,4"});expect(rank.answer=="rank(A) = 1","matrix rank");
    auto eigen=e.solve({"linear_algebra","eigenvalues","2,0;0,3"});expect(eigen.answer=="λ₁ = 3, λ₂ = 2","2x2 eigenvalues");
    auto logic=e.solve({"logic","truth_table","A & !B"});expect(logic.verification.status==pocket_engineer::VerificationStatus::verified_exhaustive,"logic exhaustive verification");
    auto kmap=e.solve({"logic","kmap_minimization","vars=3; minterms=1,3,5,7"});expect(kmap.answer=="F = C","K-map minimization");
    auto full_adder=e.solve({"logic","combinational_logic","full_adder 1 1 1"});expect(full_adder.answer=="Sum = 1; Cout = 1","full adder");
    auto mux=e.solve({"logic","combinational_logic","mux4 1 0 0 1 1 0"});expect(mux.answer=="Y = D2 = 1","4:1 multiplexer");
    auto comparator=e.solve({"logic","combinational_logic","comparator 17 9"});expect(comparator.answer=="A > B","comparator");
    auto conversion=e.solve({"logic","number_systems","FF hex decimal"});expect(conversion.answer.find("255")!=std::string::npos,"base conversion");
    auto signed_sum=e.solve({"logic","signed_arithmetic","twos_add 4 0111 0001"});expect(signed_sum.answer.find("1000 (-8); overflow = 1")!=std::string::npos,"two's-complement arithmetic");
    auto pos=e.solve({"logic","canonical_pos","A & B"});const bool pos_valid=pos.answer.starts_with("F = ")&&pos.answer.ends_with("M(0, 1, 2)");if(!pos_valid)std::cerr<<"Canonical POS actual answer: ["<<pos.answer<<"] status: "<<pos.status<<'\n';expect(pos_valid,"canonical POS");
    auto tff=e.solve({"logic","sequential_logic","tff 1 1"});expect(tff.answer=="Qnext = 0","T flip-flop");
    auto jkff=e.solve({"logic","sequential_logic","jkff 1 1 0"});expect(jkff.answer=="Qnext = 1","JK flip-flop");
    auto circuit=e.solve({"circuit","voltage_divider","12 1000 2000"});expect(circuit.answer.find("8 V")!=std::string::npos,"voltage divider");
    auto nodal_solution=e.solve({"circuit","dc_nodal_analysis","V V1 vin 0 12; R R1 vin vout 1000; R R2 vout 0 2000"});expect(nodal_solution.answer.find("V(vout) = 8 V")!=std::string::npos,"modified nodal analysis");
    auto transient=e.solve({"circuit","first_order_transient","RC 10 1000 0.000001 0.001"});expect(transient.answer.find("6.32120558829")!=std::string::npos,"RC transient");
    auto thevenin=e.solve({"circuit","network_theorems","thevenin 12 1000 2000"});expect(thevenin.answer.find("Vload = 8 V")!=std::string::npos,"Thevenin load");
    auto maximum_power=e.solve({"circuit","network_theorems","maximum_power 12 1000"});expect(maximum_power.answer=="Rload = 1000 Ω; Pmax = 0.036 W","maximum power transfer");
    auto mesh=e.solve({"circuit","mesh","mesh 12 6 1000 1000 1000"});expect(mesh.answer=="I1 = 0.01 A; I2 = 0.008 A","two-mesh KVL analysis");
    auto transform=e.solve({"circuit","source_transform","thevenin 12 1000"});expect(transform.answer=="In = 0.012 A; Rn = 1000 Ω","source transformation");
    auto superposition=e.solve({"circuit","superposition","superposition 10 1000 5 1000 1000"});expect(superposition.answer.find("Vload = 5 V")!=std::string::npos,"superposition");
    auto unit=e.solve({"engineering","unit_conversion","2.2 kOhm Ohm"});expect(unit.answer.find("2200")!=std::string::npos,"engineering prefixes");
    auto ode=e.solve({"differential_equations","separable","dy/dx = 2*x*y"});expect(ode.answer=="y = C·exp(x^2)","separable ODE");
    auto linear_ode=e.solve({"differential_equations","first_order_linear","dy/dx + 2*y = 4"});expect(linear_ode.answer=="y = 2 + C·exp(-2x)","linear ODE");
    auto second_ode=e.solve({"differential_equations","second_order_constant_coefficient","y'' + 3*y' + 2*y = 0"});expect(second_ode.answer=="y = C1·exp(-x) + C2·exp(-2x)","second-order ODE");
    auto rk4=e.solve({"differential_equations","rk4","1 1 0.1 10"});expect(rk4.answer.find("2.71827974414")!=std::string::npos,"RK4 initial-value ODE");
    auto exact=e.solve({"differential_equations","exact","exact 1 3 2"});expect(exact.answer.find("x^2y")!=std::string::npos,"exact differential equation");
    auto bernoulli=e.solve({"differential_equations","bernoulli","bernoulli 2 4 2"});expect(bernoulli.answer=="y = (2 + C·exp(2x))^(-1)","Bernoulli differential equation");
    auto homogeneous=e.solve({"differential_equations","homogeneous","homogeneous 3"});expect(homogeneous.answer=="y = C·x^3","homogeneous differential equation");
    auto trace=e.solve({"programming","cpp_trace","int x=4; x += 9;"});expect(trace.answer=="x = 13","restricted C++ trace");
    auto array=e.solve({"programming","arrays","sum [1,2,3,4]"});expect(array.answer=="sum = 10","array traversal");
    auto function=e.solve({"programming","functions","int f(int x){ return x*3; } f(4)"});expect(function.answer=="f(4) = 12","restricted function call");
    auto branch=e.solve({"programming","branches","int x=4; if (x > 2) x += 3; else x += 9;"});expect(branch.answer=="x = 7","restricted branch trace");
    auto loop=e.solve({"programming","loops","int sum=0; for (int i=1; i<=5; ++i) sum += i;"});expect(loop.answer=="sum = 15","restricted loop trace");
    auto recursion=e.solve({"programming","recursion","fact(5)"});expect(recursion.answer=="fact(5) = 120","restricted recursion trace");
    auto bad_kmap=e.solve({"logic","kmap","vars=5; minterms=1,2"});expect(bad_kmap.status=="error","unsupported K-map size rejected");
    auto bad_bernoulli=e.solve({"differential_equations","bernoulli","bernoulli 2 4 1"});expect(bad_bernoulli.status=="error","invalid Bernoulli exponent rejected");
    auto bad_loop=e.solve({"programming","loops","int sum=0; for (int i=1; i<=100001; ++i) sum += i;"});expect(bad_loop.status=="error","unbounded teaching loop rejected");
    if(failures){std::cerr<<failures<<" test(s) failed\n";return EXIT_FAILURE;}std::cout<<"All Pocket Engineer tests passed\n";
}
