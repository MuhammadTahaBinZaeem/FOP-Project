#pragma once
#include "pocket_engineer/engine.hpp"

namespace pocket_engineer {
struct InputResolution {
    ProblemSpec problem;
    std::string status{"resolved"};
    std::string confidence{"selected"};
    std::string reason;
    std::vector<std::string> variables;
    [[nodiscard]] std::string to_json(const ProblemSpec& original) const;
};
[[nodiscard]] InputResolution resolve_input(const ProblemSpec& original);
[[nodiscard]] std::string normalize_math_text(std::string text);
}
