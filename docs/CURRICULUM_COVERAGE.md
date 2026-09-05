# Curriculum coverage and completion contract

Pocket Engineer exposes **55 bounded native problem contracts** in [the input reference](INPUT_REFERENCE.md). Every catalog entry has a runnable example and C++ calculation path. The test corpus has 5,000 rows at each of three difficulty labels per topic; these are regression snapshots, not independent correctness proofs. Independent tests cover selected families and adversarial cases, not every possible input or every runtime verification claim.

It does not claim to parse or correctly solve arbitrary handwritten or free-form engineering questions. Natural language is used only to propose a category; the student confirms the category and supplies the structured form before any calculation.

## Covered course families

| Family | Bounded native problem types | Verification |
| --- | --- | --- |
| Algebra | numeric evaluation, difference-of-squares cancellation, real quadratic factorisation, linear equations, real quadratics, trig identity, numeric logs | substitution, Vieta/domain checks, or independent evaluation |
| Calculus | polynomial derivatives, tangents and indefinite/definite integrals to degree 32; removable difference-of-squares limits and monomial curve classification | finite-difference sampling, coefficient reconstruction, endpoint evaluation, selected symbolic rules |
| Linear algebra | RREF, determinant, inverse, augmented systems, multiplication, transpose, rank, 2×2 real eigenvalues, dot/cross/magnitude | row-operation replay, residuals, invariants |
| Differential equations | separable, linear first-order (constant/linear forcing), exact template, Bernoulli template, homogeneous y'=ky/x, second-order constant coefficient, exponential IVP, Euler, RK4 | substitution, characteristic-polynomial, initial-condition, or analytic-reference check |
| DLD | base conversion, two’s-complement addition, truth table/SOP/POS, 2–4 variable K-map minimisation, full adder, 4:1 MUX, comparator, 2:4 decoder, D/T/JK flip-flops | exhaustive truth table or fixed-width replay |
| LCA / ENA | voltage divider, resistor/voltage-source DC MNA, two-mesh KVL, source transform, two-source superposition, Thevenin, Norton, maximum power, zero-initial-condition RC/RL step response | KCL/KVL/MNA residual, equivalent-network, or initial/final condition |
| Programming fundamentals | restricted C++ assignment trace, branch, summation loop, array sum, single-argument function, factorial recursion | bounded independent interpreter/replay |
| Engineering units | SI-prefix conversion for supported electrical units | SI scale-table replay |

## Identifier contract

The identify command is a deterministic, advisory classifier. It recognizes explicit markers such as K-map, two's complement, determinant, Bernoulli, RK4, nodal, mesh, superposition, tangent, factor, and C++ branch/loop/recursion syntax. It always returns needs_confirmation.

The confirmation step is required because a topic name does not provide enough mathematical information. After confirmation, the UI should present the exact structured input form from [INPUT_REFERENCE.md](INPUT_REFERENCE.md), reject incomplete values, and call the native solver only then.

## Completion evidence

For every declared subcategory:

1. A deterministic category route and a documented structured input form exist.
2. The solver runs in the C++20 core and emits semantic calculation steps.
3. The result includes check metadata. Numerical approximations outside tolerance may legitimately return not_verified and a warning; a method label is not a universal correctness certificate.
4. The native corpus generator stores 15,000 golden rows: 5,000 each at easy, medium, and hard difficulty.
5. Unit tests cover representative positive routes; parser/schema violations return an error rather than an invented answer.

Generate and replay the corpus:

~~~
./build/pe-generate-tests test-data 5000
./build/pe-verify-corpus test-data
~~~

## Explicit limits

- K-map minimisation is exact for 2–4 variables. Truth tables themselves support up to six variables.
- Eigenvalue output is for real 2×2 matrices; complex eigenvalues and eigenvectors are outside this native contract.
- Symbolic calculus and ODE forms are bounded to the listed templates; arbitrary CAS algebra is not claimed.
- DC MNA accepts resistors and independent voltage sources with a ground node. AC, dependent sources, and arbitrary transient initial conditions are not part of the current netlist contract.
- The teaching-language C++ interpreter is intentionally bounded; it is not a compiler or sandbox for arbitrary C++.

## Sources consulted

- [CEME Computer Engineering course content](https://ceme.nust.edu.pk/wp-content/uploads/2020/06/Course-Content-DCE.pdf)
- [NUST Digital Logic Design outline](https://nust.edu.pk/wp-content/uploads/course_content_files/530206396_CS-116%2CDigital%20Logic%20Design-compressed.pdf)
- [CEME undergraduate course content notice board](https://ceme.nust.edu.pk/downloads/student-notice-board-ug/)

These sources inform the course taxonomy. Implementation tests, independent reference comparisons and platform runs establish their particular claims. A same-engine replayable corpus establishes consistency only.
