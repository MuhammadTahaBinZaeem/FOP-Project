#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
struct Field {
  std::string id, label, value, type{"number"};
  std::vector<std::string> options;
};
struct Form {
  std::string kind{"fields"}, prefix, separator{" "}, suffix;
  std::vector<Field> fields;
};
const TopicInfo &topic(const Json &target) {
  const auto domain = target.at("domain").string(),
             name = target.at("topic").string();
  const auto &catalog = topic_catalog();
  const auto found =
      std::find_if(catalog.begin(), catalog.end(), [&](const auto &item) {
        return item.domain == domain && item.topic == name;
      });
  if (found == catalog.end())
    throw std::runtime_error("Unknown guided-input topic");
  return *found;
}
Form form(const TopicInfo &info) {
  Form out;
  const std::string t(info.topic), domain(info.domain);
  if (domain == "linear_algebra" && t != "vectors") {
    out.kind = "matrix";
    return out;
  }
  if (t == "kmap_minimization") {
    out.kind = "kmap";
    return out;
  }
  if (t == "dc_nodal_analysis") {
    out.kind = "circuit";
    return out;
  }
  const auto field = [&](std::string id, std::string label, std::string value,
                         std::string type = "number",
                         std::vector<std::string> options = {}) {
    out.fields.push_back({std::move(id), std::move(label), std::move(value),
                          std::move(type), std::move(options)});
  };
  const auto numerical = [&](std::string prefix,
                             std::vector<std::string> labels,
                             std::vector<std::string> values) {
    out.prefix = std::move(prefix);
    for (std::size_t i = 0; i < labels.size(); i++)
      field("v" + std::to_string(i), labels[i], values[i]);
  };
  if (t == "voltage_divider")
    numerical(
        "",
        {"Source voltage (V)", "Upper resistance (Ω)", "Lower resistance (Ω)"},
        {"12", "1000", "2000"});
  else if (t == "mesh_analysis")
    numerical("mesh ",
              {"Left source (V)", "Right source (V)", "Left resistance (Ω)",
               "Right resistance (Ω)", "Shared resistance (Ω)"},
              {"12", "6", "1000", "1000", "1000"});
  else if (t == "superposition")
    numerical("superposition ",
              {"Source 1 (V)", "Series R1 (Ω)", "Source 2 (V)", "Series R2 (Ω)",
               "Load resistance (Ω)"},
              {"10", "1000", "5", "1000", "1000"});
  else if (t == "rc_transient" || t == "rl_transient")
    numerical(
        t == "rc_transient" ? "RC " : "RL ",
        {"Step voltage (V)", "Resistance (Ω)",
         t == "rc_transient" ? "Capacitance (F)" : "Inductance (H)",
         "Time (s)"},
        {"10", "1000", t == "rc_transient" ? "0.000001" : "0.001", "0.001"});
  else if (t == "thevenin" || t == "norton")
    numerical(
        t + " ",
        {t == "thevenin" ? "Equivalent voltage (V)" : "Equivalent current (A)",
         "Equivalent resistance (Ω)", "Load resistance (Ω)"},
        {t == "thevenin" ? "12" : "0.012", "1000", "2000"});
  else if (t == "maximum_power" || t == "source_transformation")
    numerical(t == "maximum_power" ? "maximum_power " : "thevenin ",
              {"Thévenin voltage (V)", "Equivalent resistance (Ω)"},
              {"12", "1000"});
  else if (t == "exact")
    numerical(
        "exact ",
        {"Coefficient a in 2axy", "Coefficient b", "Coefficient c in 2cy"},
        {"1", "3", "2"});
  else if (t == "bernoulli")
    numerical("bernoulli ", {"p in y′ + py = qyⁿ", "q", "Exponent n"},
              {"2", "4", "2"});
  else if (t == "homogeneous")
    numerical("homogeneous ", {"k in y′ = ky/x"}, {"3"});
  else if (t == "initial_value")
    numerical("", {"a in y′ = ay", "Initial y(0)", "Evaluate at x"},
              {"2", "3", "1"});
  else if (t == "euler" || t == "rk4")
    numerical(
        "", {"a in y′ = ay", "Initial y(0)", "Step size h", "Number of steps"},
        {"1", "1", "0.1", "10"});
  else if (t == "separable")
    numerical("", {"a in y′ = axy"}, {"2"});
  else if (t == "first_order_linear")
    numerical("", {"a in y′ + ay = b", "Constant forcing b"}, {"2", "4"});
  else if (t == "second_order_constant_coefficient")
    numerical("", {"a in y″ + ay′ + by = 0", "Coefficient b"}, {"3", "2"});
  else if (t == "tangent_line") {
    field("polynomial", "Polynomial", "3x^2", "text");
    field("at", "Evaluate tangent at x", "2");
  } else if (t == "definite_integral") {
    field("polynomial", "Polynomial", "(x+1)^2", "text");
    field("lower", "Lower bound", "0");
    field("upper", "Upper bound", "2");
  } else if (t == "limits") {
    field("a", "Removable point a in (x²−a²)/(x−a)", "5");
  } else if (t == "curve_analysis") {
    field("coefficient", "Coefficient a in axⁿ", "-2");
    field("power", "Power n (integer ≥ 2)", "4");
  } else if (t == "number_systems") {
    field("value", "Integer digits", "FF", "text");
    field("from", "Source base", "hex", "select",
          {"binary", "octal", "decimal", "hex"});
    field("to", "Target base", "decimal", "select",
          {"binary", "octal", "decimal", "hex"});
  } else if (t == "signed_arithmetic") {
    out.prefix = "twos_add ";
    field("bits", "Signed bit width", "4");
    field("left", "Left binary word", "0111", "text");
    field("right", "Right binary word", "0001", "text");
  } else if (t == "unit_conversion") {
    field("value", "Value", "2.2");
    field("from", "From unit (case-sensitive)", "kOhm", "text");
    field("to", "To unit (same dimension)", "Ohm", "text");
  } else if (t == "vectors") {
    field("operation", "Operation", "dot", "select",
          {"dot", "cross", "magnitude"});
    field("left", "Vector v (comma-separated)", "1,2,3", "text");
    field("right", "Vector w (ignored only for magnitude)", "4,5,6", "text");
  } else if (t == "cpp_trace")
    numerical("", {"Initial x", "Add to x"}, {"4", "9"});
  else if (t == "branches")
    numerical("",
              {"Initial x", "Comparison threshold", "Add when true",
               "Add when false"},
              {"4", "2", "3", "9"});
  else if (t == "loops")
    numerical("", {"Sum integers from 1 through N"}, {"5"});
  else if (t == "functions")
    numerical("", {"Multiplier N in f(x) = xN", "Argument M"}, {"3", "4"});
  else if (t == "recursion")
    numerical("", {"Factorial argument (0–12)"}, {"5"});
  else if (t == "arrays") {
    field("values", "Array elements", "1,2,3,4", "text");
  } else if (t == "sequential_logic") {
    out.prefix = "jkff ";
    field("j", "J input", "1", "select", {"0", "1"});
    field("k", "K input", "1", "select", {"0", "1"});
    field("q", "Previous Q", "0", "select", {"0", "1"});
  } else if (t == "combinational_logic") {
    out.prefix = "full_adder ";
    field("a", "Input A", "1", "select", {"0", "1"});
    field("b", "Input B", "1", "select", {"0", "1"});
    field("carry", "Carry in", "1", "select", {"0", "1"});
  } else {
    out.kind = "expression";
    field("expression", std::string(info.title), std::string(info.example),
          "text");
  }
  return out;
}
Json matrix(const std::string &text) {
  Json::Array rows;
  for (const auto &row : split(text, ';')) {
    Json::Array columns;
    for (const auto &entry : split(row, ','))
      columns.emplace_back(entry);
    rows.emplace_back(columns);
  }
  return rows;
}
std::string matrix_input(const Json &data, bool square) {
  const auto &rows = data.array();
  if (rows.empty() || rows.size() > 16)
    throw std::runtime_error("Use 1–16 matrix rows");
  const auto columns = rows.front().array().size();
  if (columns == 0 || columns > 17)
    throw std::runtime_error("Use 1–17 matrix columns");
  if (square && columns != rows.size())
    throw std::runtime_error("This operation requires a square matrix");
  std::string out;
  for (const auto &row : rows) {
    if (row.array().size() != columns)
      throw std::runtime_error("All matrix rows must have equal length");
    if (!out.empty())
      out += ';';
    bool first = true;
    for (const auto &entry : row.array()) {
      const double value =
          entry.is_string() ? finite_number(entry.string()) : entry.number();
      if (!first)
        out += ',';
      first = false;
      out += format(value, 17);
    }
  }
  return out;
}
} // namespace
Json schema(const ProblemSpec &request) {
  const auto target = Json::parse(request.input);
  target.only({"domain", "topic"});
  const auto &info = topic(target);
  const auto specification = form(info);
  Json::Array fields;
  for (const auto &field : specification.fields) {
    Json::Array options;
    for (const auto &option : field.options)
      options.emplace_back(option);
    fields.emplace_back(Json::Object{{"id", field.id},
                                     {"label", field.label},
                                     {"value", field.value},
                                     {"type", field.type},
                                     {"options", options}});
  }
  Json::Object out{{"status", "success"},   {"kind", specification.kind},
                   {"domain", info.domain}, {"topic", info.topic},
                   {"title", info.title},   {"scope", info.scope},
                   {"fields", fields}};
  if (specification.kind == "matrix") {
    const auto parts = split(info.example, '|');
    Json::Array matrices;
    for (const auto &value : parts)
      matrices.push_back(matrix(value));
    out["matrices"] = matrices;
    out["square"] = info.topic == "determinant" || info.topic == "inverse" ||
                    info.topic == "eigenvalues";
    out["augmented"] = info.topic == "linear_system";
    out["fixed_size"] = info.topic == "eigenvalues" ? 2 : 0;
  }
  if (info.topic == "source_transformation")
    out["note"] = "This form starts with a Thévenin source. Use advanced text "
                  "for a Norton source.";
  if (info.topic == "first_order_linear")
    out["note"] = "This form uses constant forcing. The advanced text input "
                  "also supports the documented linear forcing family.";
  if (info.topic == "sequential_logic")
    out["note"] =
        "This form is a JK flip-flop. Use advanced text for D/T flip-flops, or "
        "the state-machine lab for complete transition tables.";
  if (info.topic == "combinational_logic")
    out["note"] = "This form is a full adder. The advanced input also supports "
                  "the documented multiplexer, comparator and decoder cases.";
  return out;
}
Json guided_input(const ProblemSpec &request) {
  const auto data = Json::parse(request.input);
  data.only({"domain", "topic", "values", "matrices"});
  const auto &info = topic(data);
  const auto specification = form(info);
  std::string input;
  const std::string t(info.topic);
  if (specification.kind == "matrix") {
    const auto &matrices = data.at("matrices").array();
    if (matrices.size() != (t == "multiply" ? 2u : 1u))
      throw std::runtime_error("Wrong number of matrices for this operation");
    for (const auto &item : matrices) {
      if (!input.empty())
        input += '|';
      input += matrix_input(item, t == "inverse" || t == "determinant" ||
                                      t == "eigenvalues");
    }
    if (t == "eigenvalues" && matrices[0].array().size() != 2)
      throw std::runtime_error("Eigenvalue input must be 2 by 2");
    if (t == "linear_system" && matrices[0].array()[0].array().size() < 2)
      throw std::runtime_error("An augmented matrix needs at least one "
                               "coefficient and one right-hand-side column");
    if (t == "multiply" &&
        matrices[0].array()[0].array().size() != matrices[1].array().size())
      throw std::runtime_error("Columns of A must equal rows of B");
  } else {
    if (specification.kind == "kmap" || specification.kind == "circuit")
      throw std::runtime_error("Use the corresponding visual editor");
    const auto &values = data.at("values");
    std::map<std::string, std::string> fields;
    if (values.object().size() != specification.fields.size())
      throw std::runtime_error("Supply exactly the fields shown in this form");
    for (const auto &field : specification.fields) {
      const auto value = trim(values.at(field.id).string());
      if (value.empty() || value.size() > 4096)
        throw std::runtime_error("Missing or oversized field: " + field.label);
      if (field.type == "number")
        fields[field.id] = format(finite_number(value), 17);
      else if (field.type == "select") {
        if (std::find(field.options.begin(), field.options.end(), value) ==
            field.options.end())
          throw std::runtime_error("Invalid choice for " + field.label);
        fields[field.id] = value;
      } else
        fields[field.id] = value;
    }
    const auto get = [&](const std::string &id) -> const std::string & {
      return fields.at(id);
    };
    if (t == "separable")
      input = "dy/dx = " + get("v0") + "*x*y";
    else if (t == "first_order_linear")
      input = "dy/dx + " + get("v0") + "*y = " + get("v1");
    else if (t == "second_order_constant_coefficient")
      input = "y'' + " + get("v0") + "*y' + " + get("v1") + "*y = 0";
    else if (t == "tangent_line")
      input = get("polynomial") + ";at=" + get("at");
    else if (t == "definite_integral")
      input = "integrate " + get("polynomial") + " from " + get("lower") +
              " to " + get("upper");
    else if (t == "limits") {
      const double a = finite_number(get("a"));
      input = "limit (x^2-" + format(a * a) + ")/(x-" + get("a") +
              ") as x -> " + get("a");
    } else if (t == "curve_analysis")
      input = get("coefficient") + "x^" + get("power");
    else if (t == "vectors")
      input = get("operation") + ":" + get("left") +
              (get("operation") == "magnitude" ? "" : "|" + get("right"));
    else if (t == "cpp_trace")
      input = "int x=" + get("v0") + "; x += " + get("v1") + ";";
    else if (t == "branches")
      input = "int x=" + get("v0") + "; if (x > " + get("v1") +
              ") x += " + get("v2") + "; else x += " + get("v3") + ";";
    else if (t == "loops")
      input = "int sum=0; for (int i=1; i<=" + get("v0") + "; ++i) sum += i;";
    else if (t == "functions")
      input =
          "int f(int x){ return x*" + get("v0") + "; } f(" + get("v1") + ")";
    else if (t == "recursion")
      input = "fact(" + get("v0") + ")";
    else if (t == "arrays")
      input = "sum [" + get("values") + "]";
    else {
      input = specification.prefix;
      for (std::size_t i = 0; i < specification.fields.size(); i++) {
        if (i)
          input += specification.separator;
        input += get(specification.fields[i].id);
      }
      input += specification.suffix;
    }
  }
  if (input.size() > 4096)
    throw std::runtime_error(
        "Serialized input exceeds the solver's 4096-byte limit");
  return Json::Object{{"status", "success"},
                      {"domain", info.domain},
                      {"topic", info.topic},
                      {"input", input},
                      {"mode", "manual"}};
}
namespace {
Json compare_demo(const Json &data) {
  const auto start = std::chrono::steady_clock::now();
  data.only({"domain", "topic", "input", "expected_answer", "expected_verification"});
  const auto &info = topic(data);
  const auto expected = data.at("expected_answer").string();
  const auto expected_verification = data.at("expected_verification").string();
  if (data.at("input").string().size() > 4096 || expected.size() > 12000 ||
      expected_verification.size() > 64)
    throw std::runtime_error("Demo record exceeds the comparison budget");
  const auto solved = Engine{}.solve({std::string(info.domain), std::string(info.topic),
      data.at("input").string(), {}, "manual"});
  const auto actual_verification = verification_name(solved.verification.status);
  const double elapsed = std::chrono::duration<double, std::milli>(
      std::chrono::steady_clock::now() - start).count();
  return Json::Object{{"status", "success"},
      {"matches", solved.status == "success" && solved.answer == expected && actual_verification == expected_verification},
      {"actual_status", solved.status}, {"actual_answer", solved.answer},
      {"expected_answer", expected}, {"actual_verification", actual_verification},
      {"expected_verification", expected_verification}, {"engine_duration_ms", elapsed}};
}
Json compare_demo_batch(const Json &data) {
  data.only({"domain", "topic", "cases"});
  (void)topic(data); // Reject unknown domains/topics before any work.
  const auto &cases = data.at("cases").array();
  if (cases.empty() || cases.size() > 25)
    throw std::runtime_error("A demo batch contains 1–25 cases");
  // Validate every row before solving. Unique corpus indices ensure exported
  // expected/actual comparisons cannot silently be attached to a different row.
  std::vector<int> indices;
  for (const auto &row : cases) {
    row.only({"index", "input", "expected_answer", "expected_verification"});
    const int index = row.at("index").integer(0, 4999);
    if (std::find(indices.begin(), indices.end(), index) != indices.end())
      throw std::runtime_error("Duplicate demo index in batch");
    indices.push_back(index);
    if (row.at("input").string().size() > 4096 ||
        row.at("expected_answer").string().size() > 12000 ||
        row.at("expected_verification").string().size() > 64)
      throw std::runtime_error("Demo batch record exceeds its byte budget");
  }
  const auto start = std::chrono::steady_clock::now();
  Json::Array results;
  std::size_t matched{};
  for (std::size_t i = 0; i < cases.size(); ++i) {
    auto item = cases[i].object();
    item.erase("index");
    item["domain"] = data.at("domain");
    item["topic"] = data.at("topic");
    auto result = compare_demo(item);
    result.object()["index"] = indices[i];
    result.object()["input"] = cases[i].at("input");
    if (result.at("matches").boolean()) ++matched;
    results.push_back(std::move(result));
  }
  return Json::Object{{"status", "success"}, {"results", std::move(results)},
      {"tested", cases.size()}, {"matched", matched},
      {"batch_duration_ms", std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start).count()}};
}
} // namespace
std::string dispatch(const ProblemSpec &request) {
  const auto start = std::chrono::steady_clock::now();
  try {
    if (request.topic == "demo_compare")
      return compare_demo(Json::parse(request.input)).dump();
    if (request.topic == "demo_batch")
      return compare_demo_batch(Json::parse(request.input)).dump();
    if (request.topic == "schema")
      return schema(request).dump();
    if (request.topic == "engineering_schema")
      return engineering_schema(request).dump();
    if (request.topic == "engineering_input")
      return engineering_form_input(request).dump();
    if (request.topic == "guided_input")
      return guided_input(request).dump();
    if (request.topic == "kmap_editor")
      return kmap_editor(request).dump();
    if (request.topic == "circuit_editor")
      return circuit_editor(request).dump();
    SolutionBundle result;
    if (request.topic == "kmap_solve")
      result = extended_kmap(request);
    else if (request.topic == "network")
      result = circuit_analysis(request);
    else if (request.topic == "network_study")
      result = circuit_study(request);
    else if (request.topic == "state_machine")
      result = state_machine(request);
    else if (request.topic == "signals")
      result = signals(request);
    else
      throw std::runtime_error("Unknown engineering workbench operation");
    result.duration_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
            .count());
    if (result.verification.status == VerificationStatus::verification_failed)
      result.status = "verification_failed";
    return result.to_json();
  } catch (const std::exception &error) {
    SolutionBundle result;
    result.status = "error";
    result.domain = request.domain;
    result.topic = request.topic;
    result.answer = error.what();
    return result.to_json();
  }
}
} // namespace pocket_engineer::workbench
