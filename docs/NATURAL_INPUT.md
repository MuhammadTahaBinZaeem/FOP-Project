# Natural input and automatic type correction

The default website/Android mode interprets a question in **C++ on the device**.
No network language model, API key, or external question upload is involved.
The same mode is available with `pocket-engineer auto 'your question'`, or JSON
`{"mode":"auto","domain":"logic","topic":"truth_table","input":"Find the determinant of [[1,2],[3,4]]"}`.

An explicit operation overrides a mistaken dropdown selection. Structural forms
such as a polynomial equation, a linear system, an ODE, or C++ teaching snippet
can identify their module. Bare matrix data keeps a selected matrix operation;
without one it requires clarification. This is a bounded grammar, not arbitrary
word-problem understanding. Unsupported constraints must not be silently discarded.

The result includes the original input, normalized input, selected/resolved type,
whether it changed, a reason, and any variable-column mapping. The interface shows
that interpretation, updates the dropdowns, and keeps the original question in
the editor/history/export. Uncheck automatic interpretation to use exact manual
syntax. Direct C++ `ProblemSpec` and the CLI `solve` command default to manual;
JSON requests default to automatic, with `mode:"manual"` available explicitly.

## Supported wording examples

| Question | Interpretation |
| --- | --- |
| Could you calculate twenty five plus seven? | `25 + 7` |
| What is 2 × (3 + 4)²? | `2 * (3 + 4)^2` |
| What is sine of 30 degrees? | `sin((30)*pi/180)`; ordinary trig input remains radians |
| What is twenty percent of 150? | `(20/100)*150` |
| Expand (x+1)(x-1) | polynomial expansion, not root finding |
| Solve 2(x+3)=x+11 | collect both sides, solve, substitute back |
| Find the determinant of [[1,2],[3,4]] | determinant of a bracketed matrix |
| Solve 2(a+b)=6; a-b=-1 | augmented system, columns `a`, `b` |
| Multiply matrices [[1,2],[3,4]] and [[2,0],[1,2]] | two matrix operands |
| Differentiate f(x)=3x²−2x+7 | polynomial derivative |
| Integrate 3x² from 0 to 2 | definite integral |
| Find the tangent line to 3x² at x=2 | polynomial and evaluation point |
| K-map F(A,B,C)=Σm(1,3,5,7) | explicit three-variable on-set |
| K-map with 3 variables, maxterms=(0,2,4,6) | complement the zero-set, then minimize SOP |
| Truth table for NOT A AND B | `!A & B` |
| Voltage divider Vin=12V, R1=1kOhm, R2=2kOhm | dimension-checked SI values |
| RK4 dy/dx=y, y(0)=1, h=0.1, steps=10 | supported exponential ODE family and initial data |
| Convert 1011 from binary to decimal | integer base conversion |
| Convert 2.2 kiloohms to Ohm | compatible SI-unit conversion |
| Find the factorial of five | restricted teaching factorial, not executed C++ |

Named circuit parameters use `=` or `:`. Positional example inputs remain valid.
Use the catalog for the exact topology, equation family and numerical limits.
An ENA/LCA/ODE course name alone does not specify a solvable mathematical model.
Natural text does not add arbitrary nonlinear systems, arbitrary circuit
topologies, general symbolic integration, or unrestricted C++ execution.

Raw questions are bounded to 4,096 UTF-8 bytes; expression modules remain bounded
to 512 normalized bytes. Matrix, ODE, polynomial and Boolean limits are unchanged.
Tests distinguish original expected-answer checks from auto/manual equivalence
checks. See `tests/input_tests.cpp` and `tests/browser/natural-input.spec.cjs`.
