#pragma once

#include "pocket_engineer/engine.hpp"
#include <complex>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace pocket_engineer::workbench {

// Bounded structured protocol for editors and engineering laboratories. The
// presentation layer sends data; C++ owns validation, topology and
// calculations.
class Json {
public:
  using Array = std::vector<Json>;
  using Object = std::map<std::string, Json, std::less<>>;
  using Value =
      std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
  Json() : value_(nullptr) {}
  Json(std::nullptr_t) : value_(nullptr) {}
  Json(bool value) : value_(value) {}
  Json(double value);
  Json(int value) : value_(static_cast<double>(value)) {}
  Json(std::size_t value) : value_(static_cast<double>(value)) {}
  Json(const char *value) : value_(std::string(value)) {}
  Json(std::string value) : value_(std::move(value)) {}
  Json(std::string_view value) : value_(std::string(value)) {}
  Json(Array value) : value_(std::move(value)) {}
  Json(Object value) : value_(std::move(value)) {}
  [[nodiscard]] static Json parse(std::string_view input, std::size_t byte_limit = 32768);
  [[nodiscard]] std::string dump() const;
  [[nodiscard]] bool is_null() const;
  [[nodiscard]] bool is_number() const;
  [[nodiscard]] bool is_string() const;
  [[nodiscard]] const std::string &string() const;
  [[nodiscard]] double number() const;
  [[nodiscard]] int integer(int minimum, int maximum) const;
  [[nodiscard]] bool boolean() const;
  [[nodiscard]] const Array &array() const;
  [[nodiscard]] Array &array();
  [[nodiscard]] const Object &object() const;
  [[nodiscard]] Object &object();
  [[nodiscard]] const Json &at(std::string_view key) const;
  [[nodiscard]] const Json *find(std::string_view key) const;
  [[nodiscard]] std::string text(std::string_view key,
                                 std::string fallback = {}) const;
  [[nodiscard]] double numeric(std::string_view key, double fallback) const;
  void only(std::initializer_list<std::string_view> allowed) const;

private:
  Value value_;
};

[[nodiscard]] std::string trim(std::string_view text);
[[nodiscard]] std::vector<std::string> split(std::string_view text,
                                             char separator);
[[nodiscard]] std::string format(double value, int precision = 12);
[[nodiscard]] double finite_number(std::string_view text);
[[nodiscard]] std::vector<double> numbers(std::string_view text,
                                          std::size_t maximum);
[[nodiscard]] Json array_json(const std::vector<double> &values);
[[nodiscard]] Json complex_json(std::complex<double> value);
[[nodiscard]] std::string complex_text(std::complex<double> value);
[[nodiscard]] bool identifier(std::string_view value, std::size_t maximum = 24);

struct LinearResult {
  std::vector<std::complex<double>> values;
  double normalized_residual{};
  double pivot_ratio{};
};
using ComplexMatrix = std::vector<std::vector<std::complex<double>>>;
class FactorizedSystem {
public:
  explicit FactorizedSystem(const ComplexMatrix &matrix);
  [[nodiscard]] LinearResult
  solve(const std::vector<std::complex<double>> &rhs) const;

private:
  ComplexMatrix original_, lu_;
  std::vector<std::size_t> permutation_;
  double pivot_ratio_{};
};
[[nodiscard]] LinearResult
solve_linear(const ComplexMatrix &matrix,
             const std::vector<std::complex<double>> &rhs);

// Generic drawing commands are rendered to a raster canvas by the UI. They are
// data, never HTML/SVG/scripts, and can also be exported as PNG without a
// server.
class Drawing {
public:
  Drawing(double width, double height);
  void line(double x1, double y1, double x2, double y2,
            std::string color = "#234b40", double width = 2);
  void circle(double x, double y, double radius, std::string fill = "#ffffff",
              std::string stroke = "#234b40");
  void text(double x, double y, std::string value, int size = 13,
            std::string anchor = "center");
  void rectangle(double x, double y, double width, double height,
                 std::string fill = "#ffffff");
  void curve(double x1, double y1, double cx1, double cy1, double cx2,
             double cy2, double x2, double y2);
  void arrow(double x1, double y1, double x2, double y2);
  [[nodiscard]] Json json(std::string title) const;

private:
  double width_, height_;
  Json::Array commands_;
};

[[nodiscard]] Json schema(const ProblemSpec &request);
[[nodiscard]] Json guided_input(const ProblemSpec &request);
[[nodiscard]] Json kmap_editor(const ProblemSpec &request);
[[nodiscard]] SolutionBundle extended_kmap(const ProblemSpec &request);
[[nodiscard]] Json circuit_editor(const ProblemSpec &request);
[[nodiscard]] SolutionBundle circuit_analysis(const ProblemSpec &request);
[[nodiscard]] SolutionBundle circuit_study(const ProblemSpec &request);
[[nodiscard]] SolutionBundle state_machine(const ProblemSpec &request);
[[nodiscard]] SolutionBundle signals(const ProblemSpec &request);
[[nodiscard]] SolutionBundle digital_signals(const ProblemSpec &request);
[[nodiscard]] Json engineering_schema(const ProblemSpec &request);
[[nodiscard]] Json engineering_form_input(const ProblemSpec &request);
[[nodiscard]] std::string dispatch(const ProblemSpec &request);

} // namespace pocket_engineer::workbench
