# Algebraic Expression Solver

An iterative C++ algebraic expression solver that works like a small compiler pipeline: it lexes input, normalizes tokens, parses to an AST, runs repeated symbolic simplification passes, then prints both symbolic and numeric results where possible.

This project was developed through a long sequence of refinement waves documented directly in `project.cpp`. The comments in the source show the evolution from a baseline lexer/parser/evaluator into a more capable symbolic algebra engine with support for trigonometric identities, logarithms, powers, roots, division rewrites, compact input canonicalization, and final-stage cleanup.

## What It Does

- Accepts a single algebraic expression from standard input.
- Parses the expression into tokens and an abstract syntax tree.
- Applies multiple optimization and simplification stages.
- Preserves symbolic output while also supporting numeric evaluation when variables are provided.
- Handles user-friendly shorthand such as compact multiplication and function-style inputs that are normalized before parsing.

## Pipeline Overview

The main solver flow is documented in the source as a compiler-style pipeline:

1. Preprocess the raw input expression.
2. Lex the input into typed tokens.
3. Normalize tokens for parser safety and canonical form.
4. Convert infix tokens to postfix notation using shunting-yard logic.
5. Build an AST from the postfix stream.
6. Run repeated symbolic simplification passes.
7. Apply cleanup and round-trip stabilization passes.
8. Print the final symbolic result and any available numeric evaluation.

The source comments describe the implementation as a rewrite/evaluation engine rather than a formal proof assistant.

## Supported Input and Rewrite Behavior

The documented evolution in `project.cpp` shows support for:

- Basic arithmetic and operator precedence.
- Power expressions and right-associative exponent parsing.
- Trigonometric functions such as `sin`, `cos`, `tan`, `cot`, `sec`, and `csc`.
- Logarithmic and exponential forms such as `log`, `ln`, `exp`, and `log10` handling in the runner.
- Square roots and general `root(n, expr)` normalization.
- Implicit multiplication and compact symbolic input such as `yx`, `ab`, `2pi`, `cosx`, and `sqrtyx` after preprocessing.
- Algebraic identities, including commutative and associative normalization, cancellation, quotient reductions, perfect-square and perfect-power recognition, and selected trig/log inverse rules.

The comment history in `project.cpp` also documents incremental support for:

- Difference-of-squares quotient handling.
- Fraction and denominator normalization.
- Conjugate-based rewriting.
- Power expansion in bounded cases.
- Root and square-root simplification.
- Sign cleanup in rendered output.
- Readability-focused canonical output formatting.

## Version History Summary

The source contains a reconstructed change ledger with 21 major tracked waves:

- v1: baseline lex/parse/optimize/eval pipeline.
- v2: broader function support and stronger algebra rules.
- v3: readability improvements and multi-letter input flexibility.
- v4: precedence fixes, exponent canonicalization, and fallback evaluation.
- v5: normalization improvements for compact symbolic forms.
- v6: deeper commutative/associative cancellation stabilization.
- v7: bounded power-variant exploration.
- v8: multinomial power expansion and identity corrections.
- v9: additive numerator division cancellation.
- v10: formula-division pipeline coupling enhancements.
- v11: generalized perfect-power detection.
- v12: root/sqrt and division-formula stabilization.
- v13: associative/commutative factor cancellation in division.
- v14: symmetric quotient coverage for difference of squares.
- v15: sqrt multiplication stability and render-safe tokens.
- v16: open-form fraction, trig, log, and quotient reinforcement.
- v17: remaining targeted canonicalization closures.
- v18: pipeline documentation and worked examples.
- v19: standalone input canonicalization parity.
- v20: basic trig coverage and additive conjugate exposure.
- v21: quotient preservation and numeric log semantics split.

## Main Files

- `project.cpp`: main solver implementation and primary documentation source.
- `project_test_runner.cpp`: runner used for solver validation and comparison.
- `project_gui.cpp`: GUI-oriented variant.
- `project.cpp.bak`: backup copy of the solver source.
- `output/`: generated reports and historical result artifacts.

## Build

The project appears to be a straightforward C++17 codebase. A typical build from this folder is:

```bash
g++ -std=c++17 -O2 project.cpp -o project
g++ -std=c++17 -O2 project_test_runner.cpp -o project_test_runner
```

If your environment requires additional libraries or platform flags, keep the same source files and adapt the compile command accordingly.

## Run

Run the solver binary and enter an expression at the prompt:

```bash
./project
```

Example interaction:

```text
================ Algebraic Expression Solver ================
Enter an expression (type 'quit' to exit):
```

The solver accepts `quit` or `exit` to close immediately.

## Validation

The repository includes a test runner and a final test harness under `project_test_runner.cpp` and `output/Final tests/`. Those files are intended for regression checking and result comparison against the saved corpus in `output/`.

## Notes

- The source comments are unusually detailed and intentionally describe the implementation as a staged pipeline.
- The README mirrors that documentation so the project can be understood without reading the full source first.
- Historical result summaries and walkthrough notes live in the `output/` tree.
