#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <numbers>
#include <sstream>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
[[noreturn]] void invalid(std::string message) {
  throw std::runtime_error(std::move(message));
}
class Reader {
public:
  explicit Reader(std::string_view source, std::size_t byte_limit) : source_(source), node_limit_(byte_limit <= 32768 ? 12000u : 100000u) {
    if (byte_limit > 1048576 || source.size() > byte_limit)
      invalid("Structured JSON exceeds its bounded byte limit");
  }
  Json read() {
    auto result = value(0);
    space();
    if (pos_ != source_.size())
      invalid("Unexpected data after JSON");
    return result;
  }

private:
  void space() {
    while (pos_ < source_.size() &&
           (source_[pos_] == ' ' || source_[pos_] == '\t' ||
            source_[pos_] == '\r' || source_[pos_] == '\n'))
      ++pos_;
  }
  bool take(char c) {
    space();
    if (pos_ < source_.size() && source_[pos_] == c) {
      ++pos_;
      return true;
    }
    return false;
  }
  void expect(char c) {
    if (!take(c))
      invalid(std::string("Expected '") + c + "' in structured input");
  }
  bool literal(std::string_view word) {
    if (source_.substr(pos_, word.size()) == word) {
      pos_ += word.size();
      return true;
    }
    return false;
  }
  unsigned hex() {
    unsigned code = 0;
    for (int i = 0; i < 4; i++) {
      if (pos_ == source_.size())
        invalid("Incomplete Unicode escape");
      const char c = source_[pos_++];
      code <<= 4;
      if (c >= '0' && c <= '9')
        code += static_cast<unsigned>(c - '0');
      else if (c >= 'a' && c <= 'f')
        code += static_cast<unsigned>(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F')
        code += static_cast<unsigned>(c - 'A' + 10);
      else
        invalid("Invalid Unicode escape");
    }
    return code;
  }
  static void append(std::string &out, unsigned code) {
    if (code == 0)
      invalid("NUL is not allowed in text");
    if (code < 0x80)
      out += static_cast<char>(code);
    else if (code < 0x800) {
      out += static_cast<char>(0xc0 | (code >> 6));
      out += static_cast<char>(0x80 | (code & 63));
    } else if (code < 0x10000) {
      out += static_cast<char>(0xe0 | (code >> 12));
      out += static_cast<char>(0x80 | ((code >> 6) & 63));
      out += static_cast<char>(0x80 | (code & 63));
    } else {
      out += static_cast<char>(0xf0 | (code >> 18));
      out += static_cast<char>(0x80 | ((code >> 12) & 63));
      out += static_cast<char>(0x80 | ((code >> 6) & 63));
      out += static_cast<char>(0x80 | (code & 63));
    }
  }
  std::string string() {
    expect('"');
    std::string out;
    while (pos_ < source_.size()) {
      const unsigned char c = static_cast<unsigned char>(source_[pos_++]);
      if (c == '"')
        return out;
      if (c < 32)
        invalid("Unescaped control character in text");
      if (c != '\\') {
        out += static_cast<char>(c);
        continue;
      }
      if (pos_ == source_.size())
        invalid("Incomplete text escape");
      switch (source_[pos_++]) {
      case '"':
        out += '"';
        break;
      case '\\':
        out += '\\';
        break;
      case '/':
        out += '/';
        break;
      case 'b':
        out += '\b';
        break;
      case 'f':
        out += '\f';
        break;
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      case 'u': {
        unsigned code = hex();
        if (code >= 0xd800 && code <= 0xdbff) {
          if (pos_ + 2 > source_.size() || source_.substr(pos_, 2) != "\\u")
            invalid("Missing low surrogate");
          pos_ += 2;
          const unsigned low = hex();
          if (low < 0xdc00 || low > 0xdfff)
            invalid("Invalid low surrogate");
          code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
        } else if (code >= 0xdc00 && code <= 0xdfff)
          invalid("Unpaired surrogate");
        append(out, code);
        break;
      }
      default:
        invalid("Invalid text escape");
      }
    }
    invalid("Unterminated text");
  }
  Json value(unsigned depth) {
    if (depth > 16 || ++nodes_ > node_limit_)
      invalid(
          "Structured input is too deeply nested or contains too many values");
    space();
    if (pos_ == source_.size())
      invalid("Missing JSON value");
    if (source_[pos_] == '"')
      return string();
    if (take('{')) {
      Json::Object fields;
      if (take('}'))
        return fields;
      do {
        auto key = string();
        expect(':');
        if (!fields.emplace(key, value(depth + 1)).second)
          invalid("Duplicate field: " + key);
        if (take('}'))
          return fields;
        expect(',');
      } while (true);
    }
    if (take('[')) {
      Json::Array values;
      if (take(']'))
        return values;
      do {
        values.push_back(value(depth + 1));
        if (take(']'))
          return values;
        expect(',');
      } while (true);
    }
    if (literal("true"))
      return true;
    if (literal("false"))
      return false;
    if (literal("null"))
      return nullptr;
    const auto start = pos_;
    if (source_[pos_] == '-')
      ++pos_;
    if (pos_ == source_.size())
      invalid("Missing digits");
    if (source_[pos_] == '0')
      ++pos_;
    else {
      const auto digits = pos_;
      while (pos_ < source_.size() &&
             std::isdigit(static_cast<unsigned char>(source_[pos_])))
        ++pos_;
      if (digits == pos_)
        invalid("Invalid numeric value");
    }
    if (pos_ < source_.size() && source_[pos_] == '.') {
      ++pos_;
      const auto digits = pos_;
      while (pos_ < source_.size() &&
             std::isdigit(static_cast<unsigned char>(source_[pos_])))
        ++pos_;
      if (digits == pos_)
        invalid("Missing fractional digits");
    }
    if (pos_ < source_.size() &&
        (source_[pos_] == 'e' || source_[pos_] == 'E')) {
      ++pos_;
      if (pos_ < source_.size() &&
          (source_[pos_] == '+' || source_[pos_] == '-'))
        ++pos_;
      const auto digits = pos_;
      while (pos_ < source_.size() &&
             std::isdigit(static_cast<unsigned char>(source_[pos_])))
        ++pos_;
      if (digits == pos_)
        invalid("Missing exponent digits");
    }
    return finite_number(source_.substr(start, pos_ - start));
  }
  std::string_view source_;
  std::size_t pos_{};
  unsigned nodes_{};
  unsigned node_limit_{};
};
} // namespace
Json::Json(double value) : value_(value) {
  if (!std::isfinite(value))
    invalid("Non-finite number in structured result");
}
Json Json::parse(std::string_view input, std::size_t byte_limit) { return Reader(input, byte_limit).read(); }
bool Json::is_null() const {
  return std::holds_alternative<std::nullptr_t>(value_);
}
bool Json::is_number() const { return std::holds_alternative<double>(value_); }
bool Json::is_string() const {
  return std::holds_alternative<std::string>(value_);
}
const std::string &Json::string() const {
  if (!is_string())
    invalid("Expected text");
  return std::get<std::string>(value_);
}
double Json::number() const {
  if (!is_number())
    invalid("Expected a finite number");
  return std::get<double>(value_);
}
int Json::integer(int minimum, int maximum) const {
  const auto n = number();
  if (n < minimum || n > maximum || std::floor(n) != n)
    invalid("Integer must be between " + std::to_string(minimum) + " and " +
            std::to_string(maximum));
  return static_cast<int>(n);
}
bool Json::boolean() const {
  if (!std::holds_alternative<bool>(value_))
    invalid("Expected true or false");
  return std::get<bool>(value_);
}
const Json::Array &Json::array() const {
  if (!std::holds_alternative<Array>(value_))
    invalid("Expected a list");
  return std::get<Array>(value_);
}
Json::Array &Json::array() {
  if (!std::holds_alternative<Array>(value_))
    invalid("Expected a list");
  return std::get<Array>(value_);
}
const Json::Object &Json::object() const {
  if (!std::holds_alternative<Object>(value_))
    invalid("Expected an object");
  return std::get<Object>(value_);
}
Json::Object &Json::object() {
  if (!std::holds_alternative<Object>(value_))
    invalid("Expected an object");
  return std::get<Object>(value_);
}
const Json &Json::at(std::string_view key) const {
  const auto &fields = object();
  const auto found = fields.find(key);
  if (found == fields.end())
    invalid("Missing field: " + std::string(key));
  return found->second;
}
const Json *Json::find(std::string_view key) const {
  const auto &fields = object();
  const auto found = fields.find(key);
  return found == fields.end() ? nullptr : &found->second;
}
std::string Json::text(std::string_view key, std::string fallback) const {
  const auto item = find(key);
  return item ? item->string() : fallback;
}
double Json::numeric(std::string_view key, double fallback) const {
  const auto item = find(key);
  return item ? item->number() : fallback;
}
void Json::only(std::initializer_list<std::string_view> allowed) const {
  for (const auto &[key, value] : object()) {
    (void)value;
    if (std::find(allowed.begin(), allowed.end(), key) == allowed.end())
      invalid("Unknown field: " + key);
  }
}
std::string Json::dump() const {
  if (is_null())
    return "null";
  if (std::holds_alternative<bool>(value_))
    return boolean() ? "true" : "false";
  if (is_number())
    return format(number(), 17);
  if (is_string())
    return '"' + json_escape(string()) + '"';
  std::string out;
  if (std::holds_alternative<Array>(value_)) {
    out = '[';
    for (const auto &item : array()) {
      if (out.size() > 1)
        out += ',';
      out += item.dump();
    }
    out += ']';
  } else {
    out = '{';
    for (const auto &[key, item] : object()) {
      if (out.size() > 1)
        out += ',';
      out += '"' + json_escape(key) + "\":" + item.dump();
    }
    out += '}';
  }
  return out;
}
std::string trim(std::string_view text) {
  const auto start = text.find_first_not_of(" \t\n\r");
  return start == std::string_view::npos
             ? std::string{}
             : std::string(text.substr(start, text.find_last_not_of(" \t\n\r") -
                                                  start + 1));
}
std::vector<std::string> split(std::string_view text, char separator) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto end = text.find(separator, start);
    out.push_back(trim(text.substr(start, end == std::string_view::npos
                                              ? text.size() - start
                                              : end - start)));
    if (end == std::string_view::npos)
      break;
    start = end + 1;
  }
  return out;
}
std::string format(double value, int precision) {
  if (!std::isfinite(value))
    invalid("Non-finite numerical result");
  if (value == 0)
    value = 0;
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(precision) << value;
  return out.str();
}
double finite_number(std::string_view text) {
  const auto cleaned = trim(text);
  if (cleaned.empty())
    invalid("A numeric value is required");
  std::istringstream in(cleaned);
  in.imbue(std::locale::classic());
  double value{};
  in >> value;
  if (!in || !in.eof() || !std::isfinite(value))
    invalid("Invalid finite number: " + cleaned.substr(0, 60));
  return value;
}
std::vector<double> numbers(std::string_view text, std::size_t maximum) {
  std::string input(text);
  for (auto &c : input)
    if (c == ',' || c == ';' || c == '[' || c == ']')
      c = ' ';
  std::istringstream in(input);
  in.imbue(std::locale::classic());
  std::vector<double> out;
  std::string token;
  while (in >> token) {
    if (out.size() == maximum)
      invalid("Too many samples; maximum " + std::to_string(maximum));
    out.push_back(finite_number(token));
  }
  if (out.empty())
    invalid("At least one numeric sample is required");
  return out;
}
Json array_json(const std::vector<double> &values) {
  Json::Array out;
  for (auto value : values)
    out.emplace_back(value);
  return out;
}
Json complex_json(std::complex<double> value) {
  return Json::Object{{"real", value.real()},
                      {"imag", value.imag()},
                      {"magnitude", std::abs(value)},
                      {"phase_deg", std::arg(value) * 180 / std::numbers::pi}};
}
std::string complex_text(std::complex<double> value) {
  return format(value.real()) + (value.imag() < 0 ? " - j" : " + j") +
         format(std::abs(value.imag()));
}
bool identifier(std::string_view value, std::size_t maximum) {
  if (value.empty() || value.size() > maximum)
    return false;
  for (const auto c : value)
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
      return false;
  return true;
}

FactorizedSystem::FactorizedSystem(const ComplexMatrix &input)
    : original_(input), lu_(input) {
  const auto n = input.size();
  if (n == 0 || n > 96)
    invalid("Linear network requires 1–96 unknowns");
  auto &a = lu_;
  std::vector<double> scales(n);
  double largest_pivot = 0, smallest_pivot = std::numeric_limits<double>::max();
  for (std::size_t i = 0; i < n; i++)
    permutation_.push_back(i);
  for (std::size_t row = 0; row < n; row++) {
    if (a[row].size() != n)
      invalid("Invalid network matrix shape");
    for (auto value : a[row]) {
      if (!std::isfinite(std::abs(value)))
        invalid("Non-finite network coefficient");
      scales[row] = std::max(scales[row], std::abs(value));
    }
    if (scales[row] == 0)
      invalid("Disconnected or underdetermined network; check ground and "
              "connections");
  }
  for (std::size_t col = 0; col < n; col++) {
    std::size_t pivot = col;
    double score = 0;
    for (std::size_t row = col; row < n; row++) {
      const double ratio = std::abs(a[row][col]) / scales[row];
      if (ratio > score) {
        score = ratio;
        pivot = row;
      }
    }
    if (score < 1e-13)
      invalid("Singular or numerically ill-conditioned network; check floating "
              "nodes, ideal-source loops and incompatible constraints");
    std::swap(a[col], a[pivot]);
    std::swap(permutation_[col], permutation_[pivot]);
    std::swap(scales[col], scales[pivot]);
    largest_pivot = std::max(largest_pivot, score);
    smallest_pivot = std::min(smallest_pivot, score);
    for (std::size_t row = col + 1; row < n; row++) {
      const auto factor = a[row][col] / a[col][col];
      a[row][col] = factor;
      for (std::size_t j = col + 1; j < n; j++)
        a[row][j] -= factor * a[col][j];
    }
  }
  pivot_ratio_ = smallest_pivot / std::max(largest_pivot, 1e-30);
}
LinearResult
FactorizedSystem::solve(const std::vector<std::complex<double>> &rhs) const {
  const auto n = lu_.size();
  if (rhs.size() != n)
    invalid("Network right-hand side has the wrong size");
  const auto &a = lu_;
  const auto &input = original_;
  std::vector<std::complex<double>> b(n);
  for (std::size_t row = 0; row < n; row++) {
    b[row] = rhs[permutation_[row]];
    if (!std::isfinite(std::abs(b[row])))
      invalid("Non-finite network forcing");
    for (std::size_t col = 0; col < row; col++)
      b[row] -= a[row][col] * b[col];
  }
  std::vector<std::complex<double>> x(n);
  for (std::size_t row = n; row-- > 0;) {
    auto value = b[row];
    for (std::size_t col = row + 1; col < n; col++)
      value -= a[row][col] * x[col];
    x[row] = value / a[row][row];
    if (!std::isfinite(std::abs(x[row])))
      invalid("Network solution overflow");
  }
  double residual = 0;
  for (std::size_t row = 0; row < n; row++) {
    std::complex<double> value = -rhs[row];
    double denominator = std::abs(rhs[row]);
    for (std::size_t col = 0; col < n; col++) {
      value += input[row][col] * x[col];
      denominator += std::abs(input[row][col]) * std::abs(x[col]);
    }
    residual =
        std::max(residual, std::abs(value) / std::max(denominator, 1e-30));
  }
  if (residual > 1e-8)
    invalid("Network residual exceeds tolerance; no verified solution can be "
            "reported");
  return {std::move(x), residual, pivot_ratio_};
}
LinearResult solve_linear(const ComplexMatrix &input,
                          const std::vector<std::complex<double>> &rhs) {
  return FactorizedSystem(input).solve(rhs);
}
Drawing::Drawing(double width, double height)
    : width_(width), height_(height) {}
void Drawing::line(double x1, double y1, double x2, double y2,
                   std::string color, double width) {
  commands_.emplace_back(Json::Object{{"op", "line"},
                                      {"x1", x1},
                                      {"y1", y1},
                                      {"x2", x2},
                                      {"y2", y2},
                                      {"color", color},
                                      {"width", width}});
}
void Drawing::circle(double x, double y, double radius, std::string fill,
                     std::string stroke) {
  commands_.emplace_back(Json::Object{{"op", "circle"},
                                      {"x", x},
                                      {"y", y},
                                      {"r", radius},
                                      {"fill", fill},
                                      {"color", stroke}});
}
void Drawing::text(double x, double y, std::string value, int size,
                   std::string anchor) {
  commands_.emplace_back(Json::Object{{"op", "text"},
                                      {"x", x},
                                      {"y", y},
                                      {"text", value},
                                      {"size", size},
                                      {"anchor", anchor}});
}
void Drawing::rectangle(double x, double y, double width, double height,
                        std::string fill) {
  commands_.emplace_back(Json::Object{{"op", "rect"},
                                      {"x", x},
                                      {"y", y},
                                      {"w", width},
                                      {"h", height},
                                      {"fill", fill}});
}
void Drawing::curve(double x1, double y1, double cx1, double cy1, double cx2,
                    double cy2, double x2, double y2) {
  commands_.emplace_back(Json::Object{{"op", "curve"},
                                      {"x1", x1},
                                      {"y1", y1},
                                      {"cx1", cx1},
                                      {"cy1", cy1},
                                      {"cx2", cx2},
                                      {"cy2", cy2},
                                      {"x2", x2},
                                      {"y2", y2}});
}
void Drawing::arrow(double x1, double y1, double x2, double y2) {
  line(x1, y1, x2, y2);
  const auto a = std::atan2(y2 - y1, x2 - x1);
  line(x2, y2, x2 - 9 * std::cos(a - .45), y2 - 9 * std::sin(a - .45));
  line(x2, y2, x2 - 9 * std::cos(a + .45), y2 - 9 * std::sin(a + .45));
}
Json Drawing::json(std::string title) const {
  return Json::Object{{"kind", "drawing"},
                      {"title", std::move(title)},
                      {"width", width_},
                      {"height", height_},
                      {"commands", commands_}};
}
} // namespace pocket_engineer::workbench
