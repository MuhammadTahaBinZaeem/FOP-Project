#pragma once
#include "pocket_engineer/engine.hpp"

namespace pocket_engineer {
// Bounded, real univariate polynomial calculus. Throws on non-polynomial input;
// it never guesses a symbolic transformation for an unsupported function.
[[nodiscard]] SolutionBundle solve_polynomial_calculus(const ProblemSpec& problem);
}
