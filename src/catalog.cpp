#include "pocket_engineer/engine.hpp"

#include <sstream>

namespace pocket_engineer {
// One source of truth for the native app, browser and example smoke tests.
// Scope is deliberately explicit: a course name is not a promise to solve every
// problem in that course. Examples are original, runnable teaching inputs.
const std::vector<TopicInfo>& topic_catalog() {
    static const std::vector<TopicInfo> topics{
        {"algebra", "numeric_evaluation", "Scientific calculator", "2*(3+4)^2", "Numbers, + − * / ^, parentheses, sin, cos, tan, sqrt, ln, log, pi, e", "Real-valued arithmetic; angles in radians. Powers associate to the right."},
        {"algebra", "simplify", "Simplify an expression", "(x^2 - 1)/(x - 1)", "(x^2-a^2)/(x-a), or a numeric expression", "Difference-of-squares cancellation with the excluded point retained; not a general symbolic algebra system."},
        {"algebra", "factorisation", "Factor a quadratic", "x^2-5x+6", "x^2 + bx + c", "Real quadratic factors in the supported coefficient syntax."},
        {"algebra", "linear_equation", "Linear equations", "2x+3=11", "ax+b=c", "One variable, numeric coefficients, one equation."},
        {"algebra", "quadratic_equation", "Quadratic equations", "x^2-5x+6=0", "ax^2+bx+c=0", "Real roots using the quadratic formula; complex roots are outside this module."},
        {"algebra", "trigonometry", "Trigonometry", "sin(pi/6)", "sin(expression), cos(expression), tan(expression)", "Numeric radian evaluation and the sin(x)^2+cos(x)^2 identity."},
        {"algebra", "logarithms", "Logarithms", "log(1000)", "log(value) or ln(value)", "Positive real arguments; log is base 10 and ln is base e."},
        {"calculus", "differentiation", "Differentiate", "3x^4-2x+7", "Polynomial: sums, products, parentheses and nonnegative integer powers", "Real univariate polynomials of degree at most 32; numerical derivative cross-check and term-by-term working."},
        {"calculus", "tangent_line", "Tangent lines", "3x^2;at=2", "polynomial;at=x0", "Slope and intercept at a real point on a polynomial; degree at most 32."},
        {"calculus", "integration", "Indefinite integrals", "integrate 3x^2-2x+7 dx", "integrate polynomial dx", "Polynomial antiderivatives with an integration constant and coefficient reconstruction check."},
        {"calculus", "definite_integral", "Definite integrals", "integrate (x+1)^2 from 0 to 2", "integrate polynomial from lower to upper", "Signed area from polynomial antiderivatives; real constant bounds and expanded degree at most 32."},
        {"calculus", "limits", "Removable limits", "limit (x^2-25)/(x-5) as x -> 5", "limit (x^2-a^2)/(x-a) as x -> a", "Difference-of-squares removable discontinuities."},
        {"calculus", "curve_analysis", "Curve analysis", "-2x^4", "a*x^n with n >= 2", "Stationary point and classification for a nonzero monomial."},
        {"linear_algebra", "rref", "Row reduction", "1,2,5;3,4,11", "Comma-separated entries; semicolon-separated rows", "Gauss–Jordan elimination with partial pivoting; up to 16 rows and 17 columns."},
        {"linear_algebra", "determinant", "Determinants", "1,2;3,4", "A square matrix", "Real square matrices up to 16 by 16. Floating-point pivot tolerance applies."},
        {"linear_algebra", "inverse", "Matrix inverses", "1,2;3,4", "A nonsingular square matrix", "Gauss–Jordan inversion with an actual A times inverse residual check."},
        {"linear_algebra", "linear_system", "Linear systems", "1,1,3;2,-1,0", "Augmented matrix [A|b]: 1,1,3;2,-1,0", "Unique, inconsistent and dependent systems; coefficient rank determines whether a unique answer exists."},
        {"linear_algebra", "multiply", "Matrix multiplication", "1,2;3,4|2,0;1,2", "A|B with columns(A) = rows(B)", "Rectangular real matrix products within the matrix budget."},
        {"linear_algebra", "transpose", "Transpose", "1,2,3;4,5,6", "A rectangular matrix", "Exchange row and column indices."},
        {"linear_algebra", "rank", "Matrix rank", "1,2;2,4", "A rectangular matrix", "Numerical rank through row reduction; near-singular inputs can depend on tolerance."},
        {"linear_algebra", "eigenvalues", "Eigenvalues · 2 × 2", "2,0;0,3", "A real 2 by 2 matrix", "Real eigenvalues only. Eigenvectors and larger eigensystems are not implemented."},
        {"linear_algebra", "vectors", "Vector operations", "dot:1,2,3|4,5,6", "dot:v|w, cross:v|w, magnitude:v", "Real dot products, three-dimensional cross products and Euclidean norms."},
        {"differential_equations", "separable", "Separable ODEs", "dy/dx = 2*x*y", "dy/dx = a*x*y", "This separable family only, with arbitrary constant C."},
        {"differential_equations", "first_order_linear", "First-order linear ODEs", "dy/dx + 2*y = 4", "dy/dx + a*y = b, or b*x", "Constant nonzero a; constant or linear forcing."},
        {"differential_equations", "exact", "Exact equations", "exact 1 3 2", "exact a b c", "(2axy+b)dx + (ax^2+2cy)dy = 0; potential function and mixed-partial check."},
        {"differential_equations", "bernoulli", "Bernoulli equations", "bernoulli 2 4 2", "bernoulli p q n", "y'+py=q*y^n with constant p != 0 and integer n != 1; nonzero-solution branch."},
        {"differential_equations", "homogeneous", "Homogeneous first-order", "homogeneous 3", "homogeneous k", "y'=k*y/x on a domain excluding x=0; not every homogeneous ODE."},
        {"differential_equations", "second_order_constant_coefficient", "Second-order ODEs", "y'' + 3*y' + 2*y = 0", "y'' + a*y' + b*y = 0", "Homogeneous constant coefficients; distinct, repeated and complex characteristic roots."},
        {"differential_equations", "initial_value", "Initial-value problems", "2 3 1", "a y0 x", "Evaluate y'=a*y, y(0)=y0 using its analytic exponential solution."},
        {"differential_equations", "euler", "Euler's method", "1 1 0.1 10", "a y0 h steps", "y'=a*y; 1–10000 positive-size steps. Approximation error is shown, not hidden."},
        {"differential_equations", "rk4", "Runge–Kutta · RK4", "1 1 0.1 10", "a y0 h steps", "Classical four-stage RK4 for y'=a*y, with analytic-reference error and a trajectory."},
        {"logic", "number_systems", "Number systems", "FF hex decimal", "value source_base target_base", "Unsigned 64-bit binary, octal, decimal and hexadecimal integers; overflow rejected."},
        {"logic", "signed_arithmetic", "Two's-complement addition", "twos_add 4 0111 0001", "twos_add bits left_binary right_binary", "2–31 bit signed addition, retained bits and overflow."},
        {"logic", "truth_table", "Truth tables", "A & !B | C", "! NOT, & AND, | OR, ^ XOR, parentheses", "Up to six distinct Boolean variables; every assignment enumerated."},
        {"logic", "canonical_pos", "Canonical product of sums", "A & B", "A Boolean expression", "Canonical maxterm indices from a complete truth table."},
        {"logic", "kmap_minimization", "Karnaugh maps", "vars=3; minterms=1,3,5,7", "vars=2..4; minterms=...; dc=...", "Exact minimum sum of products for 2–4 variables; disjoint don't-cares; Gray-code map."},
        {"logic", "combinational_logic", "Combinational circuits", "full_adder 1 1 1", "full_adder A B Cin; mux4 S1 S0 D0 D1 D2 D3; comparator A B; decoder2 select", "Selected combinational components, not arbitrary gate-netlist synthesis."},
        {"logic", "sequential_logic", "Flip-flops", "jkff 1 1 0", "dff D Qprev; tff T Qprev; jkff J K Qprev", "One active clock edge for D, T and JK flip-flops; no arbitrary state-machine synthesis."},
        {"circuit", "voltage_divider", "Voltage dividers", "12 1000 2000", "Vin R1 R2", "Two positive resistors, unloaded output, DC ideal source. SI units."},
        {"circuit", "dc_nodal_analysis", "DC nodal analysis", "V V1 vin 0 12; R R1 vin vout 1000; R R2 vout 0 2000", "V name node+ node- volts; R name node1 node2 ohms", "Ground node 0 or gnd; ideal DC voltage sources and positive resistors in a bounded netlist."},
        {"circuit", "mesh_analysis", "Mesh analysis", "mesh 12 6 1000 1000 1000", "mesh Vleft Vright Rleft Rright Rshared", "Two coupled resistive meshes with the stated source polarity; not arbitrary mesh topology."},
        {"circuit", "source_transformation", "Source transformations", "thevenin 12 1000", "thevenin V R, or norton I R", "Ideal source conversion with a positive equivalent resistance."},
        {"circuit", "superposition", "Superposition", "superposition 10 1000 5 1000 1000", "superposition V1 R1 V2 R2 Rload", "Two sources feeding one loaded node through their series resistances."},
        {"circuit", "thevenin", "Thévenin equivalents", "thevenin 12 1000 2000", "thevenin Vth Rth Rload", "Load response from an already supplied equivalent; not automatic network reduction."},
        {"circuit", "norton", "Norton equivalents", "norton 0.012 1000 2000", "norton In Rn Rload", "Load response from an already supplied Norton equivalent."},
        {"circuit", "maximum_power", "Maximum power transfer", "maximum_power 12 1000", "maximum_power Vth Rth", "Resistive DC load match; Rload=Rth."},
        {"circuit", "rc_transient", "RC transients", "RC 10 1000 0.000001 0.001", "RC Vin R C t", "Zero-initial-voltage capacitor charging from a DC step. SI units."},
        {"circuit", "rl_transient", "RL transients", "RL 10 1000 0.001 0.001", "RL Vin R L t", "Zero-initial-current inductor driven by a DC step. SI units."},
        {"engineering", "unit_conversion", "Engineering units", "2.2 kOhm Ohm", "value from_unit to_unit", "Compatible SI dimensions only; case-sensitive m (milli) and M (mega)."},
        {"programming", "cpp_trace", "Trace assignments", "int x=4; x += 9;", "int x=N; x += M;", "Restricted teaching syntax only. No user code is compiled or executed."},
        {"programming", "branches", "Conditional branches", "int x=4; if (x > 2) x += 3; else x += 9;", "int x=N; if (x > M) x += A; else x += B;", "A single greater-than branch in the bounded C++ teaching interpreter."},
        {"programming", "loops", "For-loop tracing", "int sum=0; for (int i=1; i<=5; ++i) sum += i;", "int sum=0; for (int i=1; i<=N; ++i) sum += i;", "Inclusive integer summation, N <= 100000. Not general C++ execution."},
        {"programming", "arrays", "Array traversal", "sum [1,2,3,4]", "sum [comma-separated numbers]", "Bounded numeric array accumulation."},
        {"programming", "functions", "Function calls", "int f(int x){ return x*3; } f(4)", "int f(int x){ return x*N; } f(M)", "One restricted integer multiplier function; checked integer range."},
        {"programming", "recursion", "Factorial recursion", "fact(5)", "fact(N), or factorial(N)", "Integer factorial for 0 <= N <= 12, with a base case and unwind explanation."}
    };
    return topics;
}

std::string catalog_json() {
    std::ostringstream out;
    out << "{\"version\":\"0.3.0\",\"topics\":[";
    bool first = true;
    for (const auto& entry : topic_catalog()) {
        if (!first) out << ',';
        first = false;
        out << '{';
        const std::pair<std::string_view, std::string_view> fields[] = {
            {"domain", entry.domain}, {"topic", entry.topic}, {"title", entry.title},
            {"example", entry.example}, {"syntax", entry.syntax}, {"scope", entry.scope}
        };
        bool field_first = true;
        for (const auto& [key, value] : fields) {
            if (!field_first) out << ',';
            field_first = false;
            out << '"' << key << "\":\"" << json_escape(value) << '"';
        }
        out << '}';
    }
    out << "]}";
    return out.str();
}
} // namespace pocket_engineer
