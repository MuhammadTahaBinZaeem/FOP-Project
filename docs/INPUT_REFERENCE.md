# Native solver input reference

Every solve receives a confirmed domain/topic pair. The forms below are the entire native contract used by the golden corpus; values are SI unless stated otherwise. Unsupported free-form variants return an error rather than a guessed answer.

## Algebra

| Topic | Input form | Example |
| --- | --- | --- |
| numeric_evaluation | numeric expression | 2*(3+4)^2 |
| simplification | a(x^2-1)/(x-1) | (3x^2-3)/(x-1) |
| factorisation | factor ax^2+bx+c, real roots | factor x^2-5x+6 |
| linear_equations | ax+b=c | 3x + 4 = 19 |
| quadratic_equations | ax^2+bx+c=0, real roots | x^2-5x+6=0 |
| trigonometry | Pythagorean identity | sin(x)^2 + cos(x)^2 |
| logarithms | numeric expression using log, ln, sqrt | log(100) |

## Calculus

Version 0.3.0 extends differentiation, tangent lines, indefinite integrals and definite integrals to real polynomials of expanded degree at most 32. Supported operators are +, -, *, division by a nonzero constant, parentheses, nonnegative integer powers, and implicit multiplication before x or a parenthesis. Examples: `3x^4-2x+7`, `(x+1)(x-1)`, `integrate (x+1)^2 from 0 to 2`. Transcendental functions and variable denominators are not polynomial inputs. The monomial examples below remain valid.

| Topic | Input form | Example |
| --- | --- | --- |
| differentiation | a*x^n monomial | 3x^4 |
| tangent_line | a*x^n;at=value | 3x^2;at=2 |
| integration | integrate a*x^n dx | integrate 4x^3 dx |
| definite_integral | integrate a*x^n from lower to upper | integrate 3x^2 from 0 to 2 |
| limits | limit (x^2-a^2)/(x-a) as x -> a | limit (x^2-25)/(x-5) as x -> 5 |
| curve_analysis | a*x^n, n at least 2 | -2x^4 |

## Linear algebra

Matrices use commas between entries, semicolons between rows, and A|B for a binary operation.

| Topic | Input form | Example |
| --- | --- | --- |
| rref | matrix | 1,2,5;3,4,11 |
| determinant | square matrix | 1,2;3,4 |
| inverse | non-singular square matrix | 1,2;3,4 |
| linear_system | augmented n by n+1 matrix | 1,1,3;2,-1,0 |
| multiply | A|B | 1,2;3,4|2,0;1,2 |
| transpose | matrix | 1,2,3;4,5,6 |
| rank | matrix | 1,2;2,4 |
| eigenvalues | real 2 by 2 matrix | 2,0;0,3 |
| vectors | dot:v|w, cross:v|w, or magnitude:v | dot:1,2,3|4,5,6 |

## Differential equations

| Topic | Input form | Example |
| --- | --- | --- |
| separable | dy/dx = a*x*y | dy/dx = 2*x*y |
| first_order_linear | dy/dx + a*y = b or b*x | dy/dx + 2*y = 4 |
| exact | exact a b c for (2axy+b)dx+(ax^2+2cy)dy=0 | exact 1 3 2 |
| bernoulli | bernoulli p q n for y'+py=qy^n | bernoulli 2 4 2 |
| homogeneous | homogeneous k for y'=k*y/x | homogeneous 3 |
| second_order_constant_coefficient | y''+a*y'+b*y=0 | y'' + 3*y' + 2*y = 0 |
| initial_value | a y0 x for y'=a*y, y(0)=y0 | 2 3 1 |
| euler | a y0 h steps for y'=a*y | 1 1 0.1 10 |
| rk4 | a y0 h steps for y'=a*y | 1 1 0.1 10 |

## DLD

| Topic | Input form | Example |
| --- | --- | --- |
| number_systems | value source_base target_base | FF hex decimal |
| signed_arithmetic | twos_add bits left_binary right_binary | twos_add 4 0111 0001 |
| truth_table | Boolean expression: !, &, ^, and \| | A & !B \| C |
| canonical_pos | Boolean expression | A & B |
| kmap | vars=2..4; minterms=list; optional dc=list | vars=3; minterms=1,3,5,7 |
| combinational | full_adder A B Cin; mux4 S1 S0 D0 D1 D2 D3; comparator A B; decoder2 select | full_adder 1 1 1 |
| sequential | dff D Qprev; tff T Qprev; jkff J K Qprev | jkff 1 1 0 |

## LCA / ENA

| Topic | Input form | Example |
| --- | --- | --- |
| voltage_divider | Vin R1 R2 | 12 1000 2000 |
| dc_nodal | semicolon-separated R/V netlist with ground 0 or gnd | V V1 vin 0 12; R R1 vin vout 1000; R R2 vout 0 2000 |
| mesh | mesh Vleft Vright Rleft Rright Rshared | mesh 12 6 1000 1000 1000 |
| source_transform | thevenin V R, or norton I R | thevenin 12 1000 |
| superposition | superposition V1 R1 V2 R2 Rload | superposition 10 1000 5 1000 1000 |
| thevenin | thevenin Vth Rth Rload | thevenin 12 1000 2000 |
| norton | norton In Rn Rload | norton 0.012 1000 2000 |
| maximum_power | maximum_power Vth Rth | maximum_power 12 1000 |
| rc_transient | RC Vin R C t | RC 10 1000 0.000001 0.001 |
| rl_transient | RL Vin R L t | RL 10 1000 0.001 0.001 |

## Programming and engineering units

| Topic | Input form | Example |
| --- | --- | --- |
| cpp_trace | int x=N; x += M; | int x=4; x += 9; |
| branches | int x=N; if (x > M) x += A; else x += B; | int x=4; if (x > 2) x += 3; else x += 9; |
| loops | summation for loop from 1 through N | int sum=0; for (int i=1; i<=5; ++i) sum += i; |
| arrays | sum [numbers] | sum [1,2,3,4] |
| functions | one restricted integer multiplier function | int f(int x){ return x*3; } f(4) |
| recursion | factorial form | fact(5) |
| unit_conversion | value from_unit to_unit | 2.2 kOhm Ohm |

The C++ core validates matrix dimensions, circuit positivity/ground, Boolean-variable count, K-map bounds, and teaching-language bounds before solving.
