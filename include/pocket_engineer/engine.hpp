#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pocket_engineer {

enum class VerificationStatus { verified_exact, verified_exhaustive, verified_numerical, not_verified, verification_failed };

struct SolveOptions {
    std::string locale{"en"};
    std::string explanation_level{"standard"};
    bool require_verification{true};
    std::uint32_t max_steps{512};
    std::uint32_t time_budget_ms{1000};
};

struct Step { std::string rule_id; std::string explanation; std::string expression; };
struct Verification { VerificationStatus status{VerificationStatus::not_verified}; std::string method; std::string evidence; };
struct SolutionBundle {
    std::string status{"success"};
    std::string domain;
    std::string topic;
    std::string answer;
    std::vector<Step> steps;
    std::vector<std::string> assumptions;
    std::vector<std::string> warnings;
    Verification verification;
    std::string visual_json;
    std::uint64_t duration_ms{};
    [[nodiscard]] std::string to_json() const;
};

struct ProblemSpec {
    std::string domain;
    std::string topic;
    std::string input;
    std::string payload;
};
struct Identification {
    std::string status{"needs_confirmation"};
    std::vector<ProblemSpec> candidates;
    std::string reason;
    [[nodiscard]] std::string to_json() const;
};

class Engine {
public:
    // Classification is advisory only: a UI must show the candidate and obtain confirmation before solve().
    [[nodiscard]] Identification identify(std::string_view raw_input) const;
    [[nodiscard]] SolutionBundle solve(const ProblemSpec& problem, const SolveOptions& options = {}) const;
    [[nodiscard]] std::string capabilities_json() const;
};

// C ABI designed for JNI / browser-WASM bindings. Call pe_free_string for every returned pointer.
extern "C" const char* pe_solve_json(const char* request_json);
extern "C" const char* pe_capabilities_json();
extern "C" const char* pe_identify_json(const char* input);
extern "C" const char* pe_catalog_json();
extern "C" void pe_free_string(const char* value);

struct TopicInfo {
    std::string_view domain, topic, title, example, syntax, scope;
};
[[nodiscard]] const std::vector<TopicInfo>& topic_catalog();
[[nodiscard]] std::string catalog_json();
[[nodiscard]] ProblemSpec parse_request(std::string_view json);

[[nodiscard]] std::string json_escape(std::string_view value);
[[nodiscard]] std::string verification_name(VerificationStatus status);

} // namespace pocket_engineer
