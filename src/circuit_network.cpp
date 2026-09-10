#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <set>
#include <sstream>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
using C = std::complex<double>;
struct Part {
  char type{};
  std::string name, a, b, positive_control, negative_control, current_control;
  double value{}, phase{};
  int branch{-1};
};
struct Network {
  std::vector<Part> parts;
  std::vector<std::string> nodes;
  std::map<std::string, int, std::less<>> indices, branches;
  std::string analysis;
  double frequency{}, step{};
  std::size_t unknowns{};
};
struct Stamped {
  ComplexMatrix matrix;
  std::vector<C> rhs;
};
[[noreturn]] void fail(std::string text) {
  throw std::runtime_error(std::move(text));
}
std::string node_name(std::string name) {
  if (name == "gnd" || name == "GND")
    return "0";
  if (!identifier(name))
    fail("Node labels use 1–24 letters, digits or underscores");
  return name;
}
double engineering_value(std::string text) {
  static const std::map<std::string, double> suffixes{
      {"p", 1e-12}, {"n", 1e-9}, {"u", 1e-6},  {"m", 1e-3}, {"k", 1e3},
      {"K", 1e3},   {"M", 1e6},  {"meg", 1e6}, {"G", 1e9}};
  const auto end = text.find_last_of("0123456789.");
  if (end == std::string::npos)
    fail("Missing component value");
  double scale = 1;
  if (end + 1 < text.size()) {
    const auto suffix = text.substr(end + 1);
    const auto found = suffixes.find(suffix);
    if (found == suffixes.end())
      fail("Unknown SI suffix: " + suffix +
           ". Use p, n, u, m, k, M, meg or G.");
    scale = found->second;
    text.resize(end + 1);
  }
  const auto result = finite_number(text) * scale;
  if (!std::isfinite(result) || std::abs(result) > 1e12)
    fail("Component magnitude exceeds 1e12 SI units");
  return result;
}
Network read_network(const ProblemSpec &request, const Json &settings) {
  settings.only(
      {"analysis", "frequency", "step", "steps", "initial", "observe"});
  Network network;
  network.analysis = settings.text("analysis", "dc");
  if (network.analysis != "dc" && network.analysis != "ac" &&
      network.analysis != "transient")
    fail("Choose dc, ac or transient analysis");
  network.frequency = settings.numeric("frequency", 1000);
  network.step = settings.numeric("step", .0001);
  if (network.analysis == "ac" &&
      (!(network.frequency > 0) || network.frequency > 1e12))
    fail("AC frequency must be in (0, 1e12] Hz");
  if (network.analysis == "transient" &&
      (!(network.step > 0) || network.step > 1e6 || network.step < 1e-12))
    fail("Transient step must be between 1e-12 and 1e6 seconds");
  std::string input = request.input;
  std::replace(input.begin(), input.end(), '\n', ';');
  std::set<std::string> labels, nodes;
  for (const auto &row : split(input, ';')) {
    if (row.empty())
      continue;
    if (network.parts.size() >= 48)
      fail("Interactive networks are limited to 48 components");
    std::istringstream stream(row);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token)
      tokens.push_back(token);
    if (tokens.size() < 5 || tokens[0].size() != 1)
      fail("Component syntax: R R1 node1 node2 1k; V V1 plus minus 12");
    Part part;
    part.type = tokens[0][0];
    part.name = tokens[1];
    part.a = node_name(tokens[2]);
    part.b = node_name(tokens[3]);
    if (std::string("RCLVIGEFH").find(part.type) == std::string::npos)
      fail("Unsupported component: " + tokens[0] +
           ". Linear R, C, L, V, I, G, E, F and H models are supported; "
           "nonlinear devices need a separate model.");
    if (!identifier(part.name) || !labels.insert(part.name).second)
      fail("Component labels must be unique identifiers");
    nodes.insert(part.a);
    nodes.insert(part.b);
    std::string value;
    if (part.type == 'G' || part.type == 'E') {
      if (tokens.size() != 7)
        fail("Controlled-voltage syntax: E/G name out+ out- control+ control- "
             "gain");
      part.positive_control = node_name(tokens[4]);
      part.negative_control = node_name(tokens[5]);
      value = tokens[6];
      nodes.insert(part.positive_control);
      nodes.insert(part.negative_control);
    } else if (part.type == 'F' || part.type == 'H') {
      if (tokens.size() != 6)
        fail("Controlled-current syntax: F/H name out+ out- "
             "controlling_voltage_source gain");
      part.current_control = tokens[4];
      value = tokens[5];
    } else {
      if (tokens.size() != 5)
        fail("Unexpected component parameters for " + part.name);
      value = tokens[4];
    }
    const auto phase = value.find('@');
    if (phase != std::string::npos) {
      if (network.analysis != "ac" || (part.type != 'V' && part.type != 'I'))
        fail("Only independent AC sources accept magnitude@phase_degrees");
      part.phase = finite_number(value.substr(phase + 1));
      if (std::abs(part.phase) > 360000)
        fail("Source phase is outside the supported range");
      value.resize(phase);
    }
    part.value = engineering_value(value);
    if ((part.type == 'R' || part.type == 'C' || part.type == 'L') &&
        (part.value <= 0 || part.value < 1e-15))
      fail("R, C and L require positive values of at least 1e-15 SI units; "
           "draw a wire for a short");
    network.parts.push_back(part);
  }
  if (network.parts.empty())
    fail("Add at least one component");
  if (!nodes.contains("0"))
    fail("The circuit needs a reference ground (node 0)");
  nodes.erase("0");
  if (nodes.empty() || nodes.size() > 32)
    fail("The circuit needs 1–32 non-ground nodes");
  network.nodes.assign(nodes.begin(), nodes.end());
  for (std::size_t i = 0; i < network.nodes.size(); i++)
    network.indices[network.nodes[i]] = static_cast<int>(i);
  network.unknowns = network.nodes.size();
  for (auto &part : network.parts)
    if (part.type == 'V' || part.type == 'E' || part.type == 'H' ||
        part.type == 'L') {
      part.branch = static_cast<int>(network.unknowns++);
      network.branches[part.name] = part.branch;
    }
  for (const auto &part : network.parts)
    if (!part.current_control.empty() &&
        !network.branches.contains(part.current_control))
      fail("Current control " + part.current_control +
           " must name V, E, H or L. Insert a 0 V sensing source when needed.");
  return network;
}
int index(const Network &net, const std::string &node) {
  return node == "0" ? -1 : net.indices.at(node);
}
C voltage(const Network &net, const std::vector<C> &values,
          const std::string &node) {
  return node == "0" ? C{}
                     : values[static_cast<std::size_t>(net.indices.at(node))];
}
void add(ComplexMatrix &matrix, int row, int col, C value) {
  if (row >= 0 && col >= 0)
    matrix[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] +=
        value;
}
void inject(std::vector<C> &rhs, int a, int b, C value) {
  if (a >= 0)
    rhs[static_cast<std::size_t>(a)] -= value;
  if (b >= 0)
    rhs[static_cast<std::size_t>(b)] += value;
}
void admittance(ComplexMatrix &matrix, int a, int b, C value) {
  add(matrix, a, a, value);
  add(matrix, b, b, value);
  add(matrix, a, b, -value);
  add(matrix, b, a, -value);
}
C excitation(const Network &net, const Part &part) {
  return net.analysis == "ac"
             ? std::polar(part.value, part.phase * std::numbers::pi / 180)
             : C{part.value};
}
Stamped stamp(const Network &net, const std::map<std::string, double> &memory) {
  Stamped out{ComplexMatrix(net.unknowns, std::vector<C>(net.unknowns)),
              std::vector<C>(net.unknowns)};
  const C jw{0, 2 * std::numbers::pi * net.frequency};
  for (const auto &part : net.parts) {
    const auto a = index(net, part.a), b = index(net, part.b);
    if (part.type == 'R')
      admittance(out.matrix, a, b, 1 / part.value);
    else if (part.type == 'C') {
      if (net.analysis == "ac")
        admittance(out.matrix, a, b, jw * part.value);
      else if (net.analysis == "transient") {
        const double g = part.value / net.step;
        admittance(out.matrix, a, b, g);
        inject(out.rhs, a, b, -g * memory.at(part.name));
      }
    } else if (part.type == 'I')
      inject(out.rhs, a, b, excitation(net, part));
    else if (part.type == 'G') {
      const auto cp = index(net, part.positive_control),
                 cn = index(net, part.negative_control);
      add(out.matrix, a, cp, part.value);
      add(out.matrix, a, cn, -part.value);
      add(out.matrix, b, cp, -part.value);
      add(out.matrix, b, cn, part.value);
    } else if (part.type == 'F') {
      const auto control = net.branches.at(part.current_control);
      add(out.matrix, a, control, part.value);
      add(out.matrix, b, control, -part.value);
    } else {
      const auto branch = part.branch;
      add(out.matrix, a, branch, 1);
      add(out.matrix, b, branch, -1);
      add(out.matrix, branch, a, 1);
      add(out.matrix, branch, b, -1);
      if (part.type == 'V')
        out.rhs[static_cast<std::size_t>(branch)] = excitation(net, part);
      if (part.type == 'E') {
        add(out.matrix, branch, index(net, part.positive_control), -part.value);
        add(out.matrix, branch, index(net, part.negative_control), part.value);
      }
      if (part.type == 'H')
        add(out.matrix, branch, net.branches.at(part.current_control),
            -part.value);
      if (part.type == 'L') {
        if (net.analysis == "ac")
          add(out.matrix, branch, branch, -jw * part.value);
        if (net.analysis == "transient") {
          const auto r = part.value / net.step;
          add(out.matrix, branch, branch, -r);
          out.rhs[static_cast<std::size_t>(branch)] = -r * memory.at(part.name);
        }
      }
    }
  }
  return out;
}
C current(const Network &net, const Part &part, const std::vector<C> &x,
          const std::map<std::string, double> &previous) {
  const auto v = voltage(net, x, part.a) - voltage(net, x, part.b);
  if (part.branch >= 0)
    return x[static_cast<std::size_t>(part.branch)];
  switch (part.type) {
  case 'R':
    return v / part.value;
  case 'I':
    return excitation(net, part);
  case 'C':
    if (net.analysis == "ac")
      return C{0, 2 * std::numbers::pi * net.frequency} * part.value * v;
    else if (net.analysis == "transient")
      return part.value / net.step * (v - previous.at(part.name));
    else
      return 0;
  case 'G':
    return part.value * (voltage(net, x, part.positive_control) -
                         voltage(net, x, part.negative_control));
  case 'F':
    return part.value *
           x[static_cast<std::size_t>(net.branches.at(part.current_control))];
  default:
    fail("Missing component current model");
  }
}
struct Replay {
  double kcl{}, constraints{}, power{};
  Json::Array rows;
};
Replay replay(const Network &net, const std::vector<C> &x,
              const std::map<std::string, double> &memory,
              bool collect = true) {
  std::vector<C> balance(net.nodes.size());
  std::vector<double> magnitude(net.nodes.size());
  C power{};
  double power_scale = 0;
  Replay check;
  // Near-zero rows need a scale-aware absolute floor; dividing harmless
  // cancellation noise by a fixed 1e-12 can falsely reject a valid network.
  double voltage_scale = 0, current_scale = 0;
  for (std::size_t i = 0; i < net.nodes.size(); ++i)
    voltage_scale = std::max(voltage_scale, std::abs(x[i]));
  for (const auto &part : net.parts) {
    const auto v = voltage(net, x, part.a) - voltage(net, x, part.b),
               i = current(net, part, x, memory), s = v * std::conj(i);
    current_scale = std::max(current_scale, std::abs(i));
    power += s;
    power_scale += std::abs(s);
    const auto a = index(net, part.a), b = index(net, part.b);
    if (a >= 0) {
      balance[static_cast<std::size_t>(a)] += i;
      magnitude[static_cast<std::size_t>(a)] += std::abs(i);
    }
    if (b >= 0) {
      balance[static_cast<std::size_t>(b)] -= i;
      magnitude[static_cast<std::size_t>(b)] += std::abs(i);
    }
    C required = v;
    if (part.type == 'V')
      required = excitation(net, part);
    if (part.type == 'E')
      required = part.value * (voltage(net, x, part.positive_control) -
                               voltage(net, x, part.negative_control));
    if (part.type == 'H')
      required =
          part.value *
          x[static_cast<std::size_t>(net.branches.at(part.current_control))];
    if (part.type == 'L')
      required =
          net.analysis == "ac"
              ? C{0, 2 * std::numbers::pi * net.frequency} * part.value * i
          : net.analysis == "transient"
              ? part.value / net.step * (i - memory.at(part.name))
              : C{};
    check.constraints =
        std::max(check.constraints,
                 std::abs(v - required) /
                     std::max({1e-12, std::abs(v) + std::abs(required), voltage_scale * 1e-6}));
    if (collect)
      check.rows.emplace_back(Json::Object{{"component", part.name},
                                           {"type", std::string(1, part.type)},
                                           {"positive", part.a},
                                           {"negative", part.b},
                                           {"voltage", complex_json(v)},
                                           {"current", complex_json(i)},
                                           {"power", complex_json(s)}});
  }
  for (std::size_t i = 0; i < balance.size(); i++)
    check.kcl = std::max(check.kcl,
                         std::abs(balance[i]) / std::max({1e-12, magnitude[i], current_scale * 1e-6}));
  check.power = std::abs(power) / std::max(1e-12, power_scale);
  return check;
}
std::map<std::string, double> initial_memory(const Network &net,
                                             const Json &settings) {
  std::map<std::string, double> out;
  for (const auto &part : net.parts)
    if (part.type == 'C' || part.type == 'L')
      out[part.name] = 0;
  if (const auto initial = settings.find("initial"))
    for (const auto &[name, value] : initial->object()) {
      if (net.analysis != "transient")
        fail("Initial conditions apply only to transient analysis");
      if (!out.contains(name))
        fail("Initial condition must name a capacitor voltage or inductor "
             "current: " +
             name);
      out[name] = value.number();
      if (std::abs(out[name]) > 1e9)
        fail("Initial condition is too large");
    }
  return out;
}
void update_memory(const Network &net, const std::vector<C> &x,
                   std::map<std::string, double> &memory) {
  for (const auto &part : net.parts) {
    if (part.type == 'C')
      memory[part.name] =
          (voltage(net, x, part.a) - voltage(net, x, part.b)).real();
    if (part.type == 'L')
      memory[part.name] = x[static_cast<std::size_t>(part.branch)].real();
  }
}
struct Transient {
  std::vector<C> final;
  std::map<std::string, double> previous;
  Json::Array points;
  double max_residual{}, max_kcl{};
};
Transient integrate(const Network &net, const Json &settings, int count,
                    const std::string &observed, bool keep_points) {
  auto memory = initial_memory(net, settings);
  const auto first = stamp(net, memory);
  const FactorizedSystem factor(first.matrix);
  Transient result;
  for (int i = 1; i <= count; i++) {
    const auto stamped = stamp(net, memory);
    const auto solved = factor.solve(stamped.rhs);
    result.max_residual =
        std::max(result.max_residual, solved.normalized_residual);
    // Reconstruct branch laws and KCL independently of the stamped matrix.
    const auto verification = replay(net, solved.values, memory, false);
    result.max_kcl = std::max(result.max_kcl, verification.kcl);
    if (verification.kcl > 1e-7 || verification.constraints > 1e-7)
      fail("Transient branch-law replay failed; reduce scaling or review the "
           "circuit");
    if (keep_points &&
        (i == 1 || i == count || i % std::max(1, count / 200) == 0))
      result.points.emplace_back(Json::Array{
          net.step * i, voltage(net, solved.values, observed).real()});
    if (i == count) {
      result.final = solved.values;
      result.previous = memory;
    }
    update_memory(net, solved.values, memory);
  }
  return result;
}
} // namespace

SolutionBundle circuit_analysis(const ProblemSpec &request) {
  const auto settings = request.payload.empty() ? Json(Json::Object{})
                                                : Json::parse(request.payload);
  const auto network = read_network(request, settings);
  auto memory = initial_memory(network, settings);
  SolutionBundle result;
  result.domain = "circuit";
  result.topic = "network_analysis";
  result.assumptions = {
      "Ideal linear lumped components. Node 0 is ground. Each branch current "
      "is positive from its first terminal to its second.",
      "R in ohms, C in farads, L in henries, sources in volts/amperes. M means "
      "mega; m means milli.",
      "No diode, transistor, saturation, transmission-line, thermal or "
      "switching-device model is inferred from a drawing."};
  result.steps.push_back(
      {"MNA_UNKNOWNS",
       "Assign one voltage to every non-ground node and a branch-current "
       "unknown to each voltage-defined element.",
       std::to_string(network.nodes.size()) + " node voltages + " +
           std::to_string(network.unknowns - network.nodes.size()) +
           " branch currents"});
  for (const auto &part : network.parts) {
    std::string law;
    switch (part.type) {
    case 'R':
      law =
          "I = (V(" + part.a + ") - V(" + part.b + ")) / " + format(part.value);
      break;
    case 'C':
      law = network.analysis == "dc"   ? "I = 0 (DC open circuit)"
            : network.analysis == "ac" ? "I = j*2*pi*f*C*(Va-Vb)"
                                       : "I[k] = (C/h)*(v[k]-v[k-1])";
      break;
    case 'L':
      law = network.analysis == "dc"   ? "Va - Vb = 0 (DC short circuit)"
            : network.analysis == "ac" ? "Va - Vb = j*2*pi*f*L*I"
                                       : "Va[k]-Vb[k] = (L/h)*(I[k]-I[k-1])";
      break;
    case 'V':
      law = "Va - Vb = " + complex_text(excitation(network, part));
      break;
    case 'I':
      law = "I(a to b) = " + complex_text(excitation(network, part));
      break;
    case 'G':
      law = "I(a to b) = " + format(part.value) + " * (V(" +
            part.positive_control + ") - V(" + part.negative_control + "))";
      break;
    case 'E':
      law = "Va - Vb = " + format(part.value) + " * (V(" +
            part.positive_control + ") - V(" + part.negative_control + "))";
      break;
    case 'F':
      law = "I(a to b) = " + format(part.value) + " * I(" +
            part.current_control + ")";
      break;
    case 'H':
      law = "Va - Vb = " + format(part.value) + " * I(" + part.current_control +
            ")";
      break;
    }
    result.steps.push_back(
        {"BRANCH_LAW",
         "Stamp " + part.name +
             " into the node KCL and voltage-constraint equations.",
         law});
  }
  std::vector<C> x;
  double residual{};
  Json::Array points;
  double refinement = 0;
  if (network.analysis == "transient") {
    const auto count =
        settings.find("steps") ? settings.at("steps").integer(1, 2000) : 100;
    const auto observed = settings.text("observe", network.nodes.back());
    if (observed != "0" && !network.indices.contains(observed))
      fail("Unknown observation node: " + observed);
    if (static_cast<double>(count) *
            static_cast<double>(network.unknowns * network.unknowns) >
        3000000)
      fail("Transient work budget exceeded. Reduce steps or circuit size.");
    const auto trajectory = integrate(network, settings, count, observed, true);
    x = trajectory.final;
    memory = trajectory.previous;
    points = trajectory.points;
    residual = trajectory.max_residual;
    auto half = network;
    half.step /= 2;
    const auto refined = integrate(half, settings, count * 2, observed, false);
    for (std::size_t i = 0; i < network.nodes.size(); i++)
      refinement = std::max(refinement, std::abs(x[i] - refined.final[i]));
    result.steps.push_back(
        {"BACKWARD_EULER",
         "Reuse a scaled-pivot LU factorization for fixed-step backward Euler; "
         "update capacitor-voltage and inductor-current history at each step.",
         "h=" + format(network.step) + " s, steps=" + std::to_string(count) +
             ", endpoint=" + format(network.step * count) + " s"});
    result.steps.push_back(
        {"STEP_REFINEMENT",
         "Repeat with half the timestep. The largest endpoint node-voltage "
         "difference estimates discretization sensitivity; KCL verification "
         "alone does not prove time-domain accuracy.",
         "max |V_h - V_h/2| = " + format(refinement) + " V"});
    result.warnings.push_back(
        "Backward Euler is a first-order, numerically damped approximation. "
        "Reduce h until the answer converges for your required accuracy. "
        "Plotted node voltages begin at t=h; the initial conditions apply to "
        "capacitor voltages and inductor currents at t=0.");
  } else {
    const auto stamped = stamp(network, memory);
    const auto solved = solve_linear(stamped.matrix, stamped.rhs);
    x = solved.values;
    residual = solved.normalized_residual;
    if (solved.pivot_ratio < 1e-8)
      result.warnings.push_back(
          "Network scaling is ill-conditioned; verify with changed "
          "units/magnitudes and higher precision when necessary.");
    result.steps.push_back(
        {"SCALED_PIVOT_LU",
         "Solve the modified nodal equations using scaled partial pivoting; "
         "then substitute into the original equations.",
         "normalized matrix residual = " + format(residual)});
  }
  if (network.analysis == "ac")
    result.assumptions.push_back(
        "Sinusoidal steady state at " + format(network.frequency) +
        " Hz. Source magnitudes are RMS phasors; @phase is in degrees. Complex "
        "power S = V * conjugate(I), with no extra one-half factor.");
  const auto checked = replay(network, x, memory);
  const double worst =
      std::max({residual, checked.kcl, checked.constraints, checked.power});
  result.verification = {
      worst <= 1e-7 ? VerificationStatus::verified_numerical
                    : VerificationStatus::verification_failed,
      "original branch laws, KCL and total complex-power balance",
      "matrix=" + format(residual) + "; KCL=" + format(checked.kcl) +
          "; constraints=" + format(checked.constraints) +
          "; power balance=" + format(checked.power) + " (normalized)"};
  Json::Array node_rows;
  std::ostringstream answer;
  answer << "Node voltages relative to ground\n";
  for (const auto &node : network.nodes) {
    const auto v = voltage(network, x, node);
    node_rows.emplace_back(
        Json::Object{{"node", node}, {"voltage", complex_json(v)}});
    answer << "V(" << node << ") = "
           << (network.analysis == "ac" ? complex_text(v) : format(v.real()))
           << " V\n";
  }
  answer << "\nBranch currents (first terminal to second)\n";
  for (const auto &part : network.parts) {
    const auto i = current(network, part, x, memory);
    answer << "I(" << part.name << ") = "
           << (network.analysis == "ac" ? complex_text(i) : format(i.real()))
           << " A\n";
  }
  result.answer = answer.str();
  result.steps.push_back(
      {"CIRCUIT_REPLAY",
       "Reconstruct each branch current from its constitutive law, sum "
       "currents at every node and check total absorbed/delivered power.",
       result.verification.evidence});
  Json::Object visual{{"kind", "network"},
                      {"analysis", network.analysis},
                      {"nodes", node_rows},
                      {"branches", checked.rows},
                      {"points", points},
                      {"label", "Transient observation-node voltage"},
                      {"refinement_volts", refinement}};
  result.visual_json = Json(visual).dump();
  return result;
}
namespace {
struct OperatingPoint {
  std::vector<C> values;
  double residual{};
  Replay replay;
};
OperatingPoint operating_point(const Network &network, bool collect = false) {
  const auto system = stamp(network, {});
  const auto solved = solve_linear(system.matrix, system.rhs);
  const auto checked = replay(network, solved.values, {}, collect);
  const double residual = std::max({solved.normalized_residual, checked.kcl,
                                   checked.constraints, checked.power});
  if (residual > 1e-7)
    fail("An operating point failed its branch-law verification");
  return {solved.values, residual, checked};
}
void study_verification(SolutionBundle &result, double residual,
                        std::string method, std::string evidence,
                        double tolerance = 1e-7) {
  if (!std::isfinite(residual))
    fail("Nonfinite circuit-study verification residual");
  result.verification = {
      residual <= tolerance ? VerificationStatus::verified_numerical
                            : VerificationStatus::verification_failed,
      std::move(method),
      std::move(evidence) + "; maximum normalized residual=" + format(residual)};
  if (residual > tolerance) {
    result.status = "verification_failed";
    result.warnings.push_back("This circuit study did not meet its verification "
                              "tolerance. Review scaling and conditioning.");
  }
}
void require_node(const Network &network, const std::string &name) {
  if (name != "0" && !network.indices.contains(name))
    fail("Unknown node: " + name + ". Use the labels shown on the schematic.");
}
std::string unique_name(const Network &network, const std::string &prefix) {
  for (unsigned index = 0; index < 100; ++index) {
    const auto name = prefix + std::to_string(index);
    if (std::none_of(network.parts.begin(), network.parts.end(),
                     [&](const auto &part) { return part.name == name; }))
      return name;
  }
  fail("Could not allocate an internal test-source name");
}
Json node_rows(const Network &network, const std::vector<C> &values) {
  Json::Array rows;
  for (const auto &name : network.nodes)
    rows.emplace_back(Json::Object{
        {"node", name}, {"voltage", complex_json(voltage(network, values, name))}});
  return rows;
}
double compare_vectors(const std::vector<C> &first,
                       const std::vector<C> &second) {
  if (first.size() != second.size())
    fail("Internal circuit-vector size mismatch");
  double result = 0;
  for (std::size_t i = 0; i < first.size(); ++i)
    result = std::max(result, std::abs(first[i] - second[i]) /
                                  std::max({1., std::abs(first[i]),
                                            std::abs(second[i])}));
  return result;
}
std::string unknown_name(const Network &network, std::size_t column) {
  if (column < network.nodes.size())
    return "V(" + network.nodes[column] + ")";
  for (const auto &part : network.parts)
    if (part.branch == static_cast<int>(column))
      return "I(" + part.name + ")";
  fail("Unlabeled modified-nodal unknown");
}
void append_equations(SolutionBundle &result, const Network &network) {
  const auto system = stamp(network, {});
  for (std::size_t row = 0; row < system.matrix.size(); ++row) {
    std::string expression;
    for (std::size_t column = 0; column < system.matrix[row].size(); ++column) {
      const C coefficient = system.matrix[row][column];
      if (coefficient == C{})
        continue;
      if (!expression.empty())
        expression += " + ";
      expression += "(" + complex_text(coefficient) + ")*" +
                    unknown_name(network, column);
    }
    expression += " = " + complex_text(system.rhs[row]);
    result.steps.push_back(
        {"MNA_EQUATION",
         row < network.nodes.size()
             ? "Apply KCL at node " + network.nodes[row] +
                   "; currents leaving the node are positive."
             : "Apply the voltage constraint for " + unknown_name(network, row) +
                   ".",
         expression});
  }
}
SolutionBundle port_equivalent(const Network &network, const Json &settings) {
  settings.only({"operation", "analysis", "frequency", "positive", "negative",
                 "load_real", "load_imag"});
  const auto positive = node_name(settings.at("positive").string());
  const auto negative = node_name(settings.text("negative", "0"));
  require_node(network, positive);
  require_node(network, negative);
  if (positive == negative)
    fail("The equivalent-circuit port needs two distinct nodes");
  const auto open = operating_point(network);
  const C vth = voltage(network, open.values, positive) -
                voltage(network, open.values, negative);
  auto suppressed = network;
  for (auto &part : suppressed.parts)
    if (part.type == 'V' || part.type == 'I')
      part.value = 0;
  // A 1 A source injecting into the positive terminal measures driving-point
  // impedance while all dependent sources remain active. No current unknown
  // is added: the same modified-nodal matrix size is retained.
  Part test;
  test.type = 'I';
  test.name = unique_name(suppressed, "pe_test");
  test.a = negative;
  test.b = positive;
  test.value = 1;
  suppressed.parts.push_back(test);
  const auto driven = operating_point(suppressed);
  const C impedance = voltage(suppressed, driven.values, positive) -
                      voltage(suppressed, driven.values, negative);
  SolutionBundle result;
  result.domain = "circuit";
  result.topic = "port_equivalent";
  result.answer = "Port: " + positive + " (+) to " + negative + " (−)\n" +
                  "Thévenin voltage = " + complex_text(vth) + " V\n" +
                  "Equivalent impedance = " + complex_text(impedance) +
                  " Ω\n";
  Json::Object equivalent{{"positive", positive},
                          {"negative", negative},
                          {"vth", complex_json(vth)},
                          {"impedance", complex_json(impedance)}};
  if (std::abs(impedance) > 1e-12) {
    const C norton = vth / impedance;
    result.answer += "Norton short-circuit current = " + complex_text(norton) +
                     " A (delivered from + to −)\n";
    equivalent["norton_current"] = complex_json(norton);
  } else {
    equivalent["norton_current"] = Json();
    result.warnings.push_back(
        "The port impedance is numerically zero. A finite Norton current "
        "cannot be reported for an ideal nonzero voltage source.");
  }
  if (impedance.real() > 1e-12) {
    const C optimum = network.analysis == "ac" ? std::conj(impedance)
                                                 : C{impedance.real()};
    const double maximum_power = std::norm(vth) / (4 * impedance.real());
    equivalent["conjugate_match"] = complex_json(optimum);
    equivalent["maximum_available_power_w"] = maximum_power;
    result.answer += "Maximum available load power = " + format(maximum_power) +
                     " W at Zload = " + complex_text(optimum) + " Ω\n";
  } else {
    equivalent["maximum_available_power_w"] = Json();
    result.warnings.push_back(
        "No finite passive-load maximum-power claim is made when Re(Zth) is "
        "zero or negative; active-source stability is outside this model.");
  }
  result.steps = {
      {"PORT_OPEN_CIRCUIT",
       "Remove only the external load. Solve the supplied network with all "
       "sources active; its terminal voltage is the Thévenin voltage.",
       "Vth = V(" + positive + ") − V(" + negative + ") = " +
           complex_text(vth) + " V"},
      {"PORT_SOURCE_SUPPRESSION",
       "Set independent voltage sources to zero volts and independent current "
       "sources to zero amperes. Keep every dependent source active.",
       "Zero-voltage sources retain their branch-current unknowns and behave "
       "as ideal shorts; zero-current sources contribute no injection."},
      {"PORT_TEST_CURRENT",
       "Inject a 1 A test current into the positive port terminal and withdraw "
       "it at the negative terminal. Solve the original branch laws again.",
       "Zth = Vtest / 1 A = " + complex_text(impedance) + " Ω"},
      {"PORT_MATCHING",
       network.analysis == "ac"
           ? "For RMS phasors and Re(Zth)>0, conjugate matching cancels the "
             "reactive part and maximizes average power."
           : "For positive DC Thévenin resistance, match the load resistance "
             "to the source resistance for maximum load power.",
       "Pmax = |Vth|² / (4 Re(Zth))"}};
  double residual = std::max(open.residual, driven.residual);
  // Independently validate the port model by attaching a passive probe load to
  // the full MNA equations. This does not use the Thévenin answer to solve it.
  const double load_real = settings.numeric("load_real", 1000);
  const double load_imag = settings.numeric("load_imag", 0);
  if (load_real < 0 || load_real > 1e12 || std::abs(load_imag) > 1e12 ||
      (network.analysis == "dc" && load_imag != 0))
    fail("Probe load needs nonnegative resistance; its reactance must be zero "
         "in DC mode and magnitudes must not exceed 1e12 Ω");
  const C load{load_real, load_imag};
  if (std::abs(load) < 1e-12)
    fail("The probe load cannot be zero. Use a positive load to verify the "
         "equivalent; the Norton answer separately reports short-circuit current.");
  auto loaded = stamp(network, {});
  admittance(loaded.matrix, index(network, positive), index(network, negative),
             1. / load);
  const auto direct = solve_linear(loaded.matrix, loaded.rhs);
  const C measured_voltage = voltage(network, direct.values, positive) -
                             voltage(network, direct.values, negative);
  if (std::abs(impedance + load) < 1e-12)
    fail("The port and load impedances cancel; the ideal loaded system has no "
         "finite ordinary solution");
  const C predicted_current = vth / (impedance + load);
  const C predicted_voltage = predicted_current * load;
  residual = std::max({residual, direct.normalized_residual,
                       std::abs(measured_voltage - predicted_voltage) /
                           std::max({1., std::abs(measured_voltage),
                                     std::abs(predicted_voltage)})});
  // Include the external load current in a separate node-current replay.
  std::vector<C> balance(network.nodes.size());
  std::vector<double> scale(network.nodes.size());
  const auto accumulate = [&](const std::string &a, const std::string &b, C i) {
    const int p = index(network, a), n = index(network, b);
    if (p >= 0) {
      balance[static_cast<std::size_t>(p)] += i;
      scale[static_cast<std::size_t>(p)] += std::abs(i);
    }
    if (n >= 0) {
      balance[static_cast<std::size_t>(n)] -= i;
      scale[static_cast<std::size_t>(n)] += std::abs(i);
    }
  };
  for (const auto &part : network.parts)
    accumulate(part.a, part.b, current(network, part, direct.values, {}));
  accumulate(positive, negative, measured_voltage / load);
  for (std::size_t i = 0; i < balance.size(); ++i)
    residual = std::max(residual,
                        std::abs(balance[i]) / std::max(1e-12, scale[i]));
  equivalent["probe_load"] = complex_json(load);
  equivalent["probe_voltage_full_network"] = complex_json(measured_voltage);
  equivalent["probe_voltage_equivalent"] = complex_json(predicted_voltage);
  equivalent["probe_current"] = complex_json(predicted_current);
  result.answer += "\nProbe load " + complex_text(load) + " Ω: voltage = " +
                   complex_text(measured_voltage) + " V; current = " +
                   complex_text(predicted_current) + " A.";
  result.steps.push_back(
      {"PORT_LOADED_REPLAY",
       "Attach the specified probe admittance to the full nodal equations, "
       "solve independently, then compare with the equivalent-circuit divider "
       "and replay KCL including the external load.",
       "Vload(full network)=" + complex_text(measured_voltage) +
           "; Vload(equivalent)=" + complex_text(predicted_voltage)});
  result.assumptions = {
      "The supplied netlist excludes the external load. Existing components "
      "remain part of the source network, even if connected across the port.",
      "Dependent sources remain active during impedance measurement. An "
      "internally singular source-suppressed network is rejected.",
      network.analysis == "ac"
          ? "Single-frequency linear steady state; source magnitudes are RMS. "
            "The equivalent is valid at this frequency only."
          : "Ideal linear DC network; capacitors are open and inductors short."};
  study_verification(result, residual,
                      "open/test-source branch laws and independent loaded MNA",
                      "Three operating-point constructions checked");
  result.visual_json =
      Json(Json::Object{{"kind", "network"},
                        {"analysis", network.analysis},
                        {"nodes", node_rows(network, open.values)},
                        {"branches", replay(network, open.values, {}, true).rows},
                        {"points", Json::Array{}},
                        {"equivalent", equivalent}})
          .dump();
  return result;
}
SolutionBundle superposition_study(const Network &network,
                                   const Json &settings) {
  settings.only({"operation", "analysis", "frequency", "observe"});
  const auto observed = node_name(settings.text("observe", network.nodes.back()));
  require_node(network, observed);
  std::vector<std::size_t> sources;
  for (std::size_t i = 0; i < network.parts.size(); ++i)
    if (network.parts[i].type == 'V' || network.parts[i].type == 'I')
      sources.push_back(i);
  if (sources.empty() || sources.size() > 16)
    fail("Superposition needs 1–16 independent sources");
  const auto original = stamp(network, {});
  const FactorizedSystem factor(original.matrix);
  const auto full = factor.solve(original.rhs);
  std::vector<C> sum(network.unknowns);
  Json::Array contributions;
  SolutionBundle result;
  result.domain = "circuit";
  result.topic = "network_superposition";
  result.answer = "Contributions to V(" + observed + ")\n";
  double residual = full.normalized_residual;
  for (const auto source : sources) {
    auto partial = network;
    for (std::size_t i = 0; i < partial.parts.size(); ++i)
      if (i != source && (partial.parts[i].type == 'V' ||
                          partial.parts[i].type == 'I'))
        partial.parts[i].value = 0;
    const auto stamped = stamp(partial, {});
    const auto solved = factor.solve(stamped.rhs);
    const auto checked = replay(partial, solved.values, {}, false);
    residual = std::max({residual, solved.normalized_residual, checked.kcl,
                         checked.constraints, checked.power});
    for (std::size_t i = 0; i < sum.size(); ++i)
      sum[i] += solved.values[i];
    const C contribution = voltage(network, solved.values, observed);
    const auto &source_part = network.parts[source];
    contributions.emplace_back(
        Json::Object{{"source", source_part.name},
                      {"observed_voltage", complex_json(contribution)},
                      {"nodes", node_rows(network, solved.values)},
                      {"matrix_residual", solved.normalized_residual},
                      {"kcl_residual", checked.kcl}});
    result.answer += source_part.name + ": " + complex_text(contribution) +
                     " V\n";
    result.steps.push_back(
        {"SUPERPOSITION_SOURCE",
         "Keep " + source_part.name +
             " active. Suppress all other independent sources but leave "
             "dependent sources active. Reuse the unchanged matrix "
             "factorization with this source's right-hand side.",
         "V(" + observed + ") from " + source_part.name + " = " +
             complex_text(contribution) + " V; KCL residual=" +
             format(checked.kcl)});
  }
  const auto checked_full = replay(network, full.values, {}, true);
  residual = std::max({residual, compare_vectors(sum, full.values),
                       checked_full.kcl, checked_full.constraints,
                       checked_full.power});
  const auto total = voltage(network, sum, observed);
  result.answer += "Sum = " + complex_text(total) +
                   " V\nAll-sources solve = " +
                   complex_text(voltage(network, full.values, observed)) + " V";
  result.steps.push_back(
      {"SUPERPOSITION_SUM",
       "Add corresponding node voltages and voltage-source branch currents, "
       "including their signs or complex phases. Compare every unknown with "
       "the simultaneous all-sources solution.",
       std::to_string(sum.size()) + " unknowns compared"});
  result.assumptions = {
      "Linear ideal DC or single-frequency AC network. Dependent sources are "
      "never suppressed; the topology and coefficient matrix remain fixed.",
      "Superposition applies to voltages and currents, not power. Compute "
      "power from the total voltage and total current, not a sum of "
      "individual-source powers."};
  study_verification(result, residual,
                      "individual source KCL and all-unknown superposition",
                      std::to_string(sources.size()) + " source responses checked");
  result.visual_json =
      Json(Json::Object{{"kind", "network"},
                        {"analysis", network.analysis},
                        {"nodes", node_rows(network, full.values)},
                        {"branches", checked_full.rows},
                        {"points", Json::Array{}},
                        {"contributions", contributions}})
          .dump();
  return result;
}
SolutionBundle sweep_study(Network network, const Json &settings) {
  settings.only({"operation", "analysis", "frequency", "frequency_high",
                 "count", "observe"});
  if (network.analysis != "ac")
    fail("A frequency sweep requires AC analysis");
  const double low = network.frequency;
  const double high = settings.numeric("frequency_high", 100000);
  const int count = settings.find("count") ? settings.at("count").integer(2, 256)
                                            : 64;
  if (!(high > low) || high > 1e12)
    fail("Sweep high frequency must exceed the starting frequency and be at "
         "most 1e12 Hz");
  if (static_cast<double>(count) *
          static_cast<double>(network.unknowns * network.unknowns *
                               network.unknowns) >
      8000000)
    fail("Frequency-sweep work budget exceeded. Reduce sample count or "
         "circuit size.");
  const auto observed = node_name(settings.text("observe", network.nodes.back()));
  require_node(network, observed);
  Json::Array rows,points,phases;
  double residual = 0, peak = -1, peak_frequency = low, previous_phase = 0;
  for (int sample = 0; sample < count; ++sample) {
    network.frequency = std::exp(std::log(low) +
        (std::log(high) - std::log(low)) * sample / (count - 1));
    const auto point = operating_point(network);
    residual = std::max(residual, point.residual);
    const C value = voltage(network, point.values, observed);
    const double magnitude = std::abs(value);
    if (magnitude > peak) {
      peak = magnitude;
      peak_frequency = network.frequency;
    }
    double phase = std::arg(value) * 180 / std::numbers::pi;
    if (sample) {
      while (phase - previous_phase > 180)
        phase -= 360;
      while (phase - previous_phase < -180)
        phase += 360;
    }
    previous_phase = phase;
    rows.emplace_back(Json::Object{{"frequency_hz", network.frequency},
                                   {"voltage", complex_json(value)},
                                   {"phase_deg", phase},
                                   {"residual", point.residual}});
    points.emplace_back(Json::Array{std::log10(network.frequency), magnitude});
    phases.emplace_back(Json::Array{std::log10(network.frequency), phase});
  }
  SolutionBundle result;
  result.domain = "circuit";
  result.topic = "ac_frequency_sweep";
  result.answer = "V(" + observed + ") swept at " + std::to_string(count) +
                  " frequencies.\nLargest sampled RMS magnitude = " +
                  format(peak) + " V at " + format(peak_frequency) + " Hz.";
  result.steps = {
      {"AC_LOG_SWEEP", "Choose logarithmically spaced frequencies between the "
                        "specified endpoints.",
       "f[k] = exp(log(fmin) + k*(log(fmax)−log(fmin))/(N−1))"},
      {"AC_DYNAMIC_STAMPS", "At each frequency, update capacitor admittance "
                             "jωC and inductor impedance jωL; keep the same "
                             "topology and RMS source phasors.",
       "ω = 2πf; independent source magnitude and phase do not vary with f"},
      {"AC_SWEEP_REPLAY", "Solve each complex system and independently replay "
                           "branch laws, KCL and complex-power balance.",
       std::to_string(count) + " separately checked frequency points"}};
  result.assumptions = {
      "A sampled frequency sweep, not a pole/root calculation. A narrow "
      "resonance between samples can be missed. Refine the sweep near peaks.",
      "The plotted voltage is absolute RMS magnitude, not a transfer gain "
      "unless the chosen excitation is a 1 V source.",
      "The horizontal axis is log10(frequency in Hz). Phase is unwrapped "
      "between sampled points; phase of an exact zero is conventional."};
  study_verification(result, residual, "branch laws at every sweep frequency",
                      std::to_string(count) + " frequency points checked");
  result.visual_json =
      Json(Json::Object{{"kind", "signal"},
                        {"label", "RMS node voltage versus log10(frequency in Hz)"},
                        {"points", points},
                        {"secondary_label", "Phase in degrees versus log10(frequency in Hz)"},
                        {"secondary_points", phases},
                        {"sweep", rows}}).dump();
  return result;
}
std::vector<C> sensitivity_rhs(const Network &network, const Part &part,
                               const std::vector<C> &operating) {
  std::vector<C> rhs(network.unknowns);
  const int a = index(network, part.a), b = index(network, part.b);
  const C v = voltage(network, operating, part.a) -
              voltage(network, operating, part.b);
  const C jw{0, 2 * std::numbers::pi * network.frequency};
  if (part.type == 'R')
    inject(rhs, a, b, -v / (part.value * part.value));
  else if (part.type == 'C' && network.analysis == "ac")
    inject(rhs, a, b, jw * v);
  else if (part.type == 'L' && network.analysis == "ac")
    rhs[static_cast<std::size_t>(part.branch)] =
        jw * operating[static_cast<std::size_t>(part.branch)];
  else if (part.type == 'I')
    inject(rhs, a, b, network.analysis == "ac"
                          ? std::polar(1., part.phase * std::numbers::pi / 180)
                          : C{1});
  else if (part.type == 'V')
    rhs[static_cast<std::size_t>(part.branch)] =
        network.analysis == "ac"
            ? std::polar(1., part.phase * std::numbers::pi / 180)
            : C{1};
  else if (part.type == 'G' || part.type == 'E') {
    const C control = voltage(network, operating, part.positive_control) -
                      voltage(network, operating, part.negative_control);
    if (part.type == 'G')
      inject(rhs, a, b, control);
    else
      rhs[static_cast<std::size_t>(part.branch)] = control;
  } else if (part.type == 'F' || part.type == 'H') {
    const C control =
        operating[static_cast<std::size_t>(network.branches.at(part.current_control))];
    if (part.type == 'F')
      inject(rhs, a, b, control);
    else
      rhs[static_cast<std::size_t>(part.branch)] = control;
  }
  return rhs;
}
SolutionBundle sensitivity_study(const Network &network, const Json &settings) {
  settings.only({"operation", "analysis", "frequency", "observe", "tolerance"});
  const auto observed = node_name(settings.text("observe", network.nodes.back()));
  require_node(network, observed);
  const double tolerance = settings.numeric("tolerance", .01);
  if (!(tolerance > 0) || tolerance > .5)
    fail("Component tolerance must be a fraction in (0, 0.5]");
  if (4. * static_cast<double>(network.parts.size()) *
          static_cast<double>(network.unknowns * network.unknowns * network.unknowns) >
      12000000)
    fail("Sensitivity cross-check budget exceeded. Use a smaller network.");
  const auto system = stamp(network, {});
  const FactorizedSystem factor(system.matrix);
  const auto operating = factor.solve(system.rhs);
  const C v = voltage(network, operating.values, observed);
  Json::Array rows;
  double worst = operating.normalized_residual, first_order_bound = 0;
  SolutionBundle result;
  result.domain = "circuit";
  result.topic = "component_sensitivity";
  result.answer = "Local sensitivities of V(" + observed + ") = " +
                  complex_text(v) + " V\n";
  for (std::size_t index = 0; index < network.parts.size(); ++index) {
    const auto &part = network.parts[index];
    const auto rhs = sensitivity_rhs(network, part, operating.values);
    const auto derivative = factor.solve(rhs);
    const C analytic = voltage(network, derivative.values, observed);
    const double h = part.value == 0 ? 1e-4 : std::abs(part.value) * 1e-3;
    C finite_difference{};
    // Four separate perturbed solves implement a fourth-order central stencil.
    // They do not use the analytical derivative stamps or reuse their RHS.
    const int offsets[]{-2, -1, 1, 2};
    const double weights[]{1., -8., 8., -1.};
    for (int sample = 0; sample < 4; ++sample) {
      auto perturbed = network;
      perturbed.parts[index].value += h * offsets[sample];
      const auto point = operating_point(perturbed);
      finite_difference += weights[sample] *
                           voltage(perturbed, point.values, observed);
      worst = std::max(worst, point.residual);
    }
    finite_difference /= 12 * h;
    const double difference = std::abs(analytic - finite_difference) /
        std::max({1e-9, std::abs(analytic), std::abs(finite_difference)});
    worst = std::max({worst, difference, derivative.normalized_residual});
    const double bound = std::abs(analytic * part.value) * tolerance;
    first_order_bound += bound;
    const Json normalized = std::abs(v) > 1e-12
                                ? complex_json(analytic * part.value / v)
                                : Json();
    rows.emplace_back(Json::Object{{"component", part.name},
                                   {"parameter", part.value},
                                   {"absolute_derivative", complex_json(analytic)},
                                   {"finite_difference", complex_json(finite_difference)},
                                   {"normalized_sensitivity", normalized},
                                   {"first_order_bound_v", bound},
                                   {"cross_check", difference}});
    result.answer += "dV/d(" + part.name + ") = " + complex_text(analytic) +
                     " V per SI-unit parameter\n";
  }
  result.answer += "\nFirst-order worst-direction voltage-change estimate "
                   "for simultaneous ±" + format(tolerance * 100) +
                   "% parameter changes: " + format(first_order_bound) + " V.";
  result.steps = {
      {"IMPLICIT_SENSITIVITY",
       "Differentiate the modified-nodal equations with respect to one real "
       "component parameter while keeping all other parameters fixed.",
       "A·(dx/dp) = db/dp − (dA/dp)·x"},
      {"REUSED_SENSITIVITY_LU",
       "Solve each derivative right-hand side with the nominal LU "
       "factorization. AC source derivatives vary magnitude, not phase.",
       std::to_string(network.parts.size()) + " parameter derivatives"},
      {"SENSITIVITY_FOUR_POINT_CHECK",
       "Independently perturb each parameter at −2h, −h, +h and +2h; solve "
       "four full networks and compare the fourth-order central difference.",
       "f′(p) ≈ [f(p−2h) − 8f(p−h) + 8f(p+h) − f(p+2h)]/(12h)"},
      {"FIRST_ORDER_TOLERANCE",
       "Sum magnitudes of local derivative contributions for the stated "
       "fractional parameter tolerance. This is a local linearization, not a "
       "rigorous global tolerance bound or probability distribution.",
       "estimated |ΔV| ≤ sum |(dV/dp)·p|·tolerance = " +
           format(first_order_bound) + " V"}};
  result.assumptions = {
      "DC/AC ideal linear component parameters; source phase is held fixed. "
      "Capacitance and inductance sensitivity is zero in ideal DC steady state.",
      "The estimate includes source amplitudes and dependent-source gains as "
      "well as passive values. A zero nominal parameter receives zero "
      "fractional tolerance, though its absolute derivative is still shown.",
      "Near-zero outputs have undefined normalized sensitivity. Large "
      "tolerances or nearly singular networks can invalidate first-order "
      "predictions. Finite-difference cancellation can fail the numerical check."};
  study_verification(result, worst, "analytic MNA derivatives versus perturbed networks",
                      "Four independent perturbation solves per parameter", 2e-5);
  result.visual_json = Json(Json::Object{
      {"kind", "network"}, {"analysis", network.analysis},
      {"nodes", node_rows(network, operating.values)},
      {"branches", replay(network, operating.values, {}, true).rows},
      {"points", Json::Array{}}, {"sensitivities", rows},
      {"first_order_bound_v", first_order_bound}}).dump();
  return result;
}
} // namespace

SolutionBundle circuit_study(const ProblemSpec &request) {
  const auto settings = Json::parse(request.payload);
  const auto operation = settings.at("operation").string();
  const auto analysis = settings.text("analysis", "dc");
  if (analysis != "dc" && analysis != "ac")
    fail("Network studies use DC or AC steady state; select the transient "
         "solver for initial-value calculations");
  const Json basic(Json::Object{{"analysis", analysis},
                                {"frequency", settings.numeric("frequency", 1000)}});
  const auto network = read_network(request, basic);
  SolutionBundle result;
  if (operation == "port")
    result = port_equivalent(network, settings);
  else if (operation == "superposition")
    result = superposition_study(network, settings);
  else if (operation == "sweep")
    result = sweep_study(network, settings);
  else if (operation == "sensitivity")
    result = sensitivity_study(network, settings);
  else
    fail("Choose port, superposition, sweep or sensitivity study");
  append_equations(result, network);
  return result;
}
} // namespace pocket_engineer::workbench
