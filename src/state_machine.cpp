#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <numbers>
#include <queue>
#include <set>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
struct Transition {
  std::size_t next{};
  int output{};
  bool supplied{};
};
struct Machine {
  bool moore{};
  int input_bits{}, output_bits{};
  std::size_t initial{};
  std::vector<std::string> names;
  std::vector<int> outputs;
  std::vector<std::vector<Transition>> rows;
  std::vector<int> sequence;
};
[[noreturn]] void fail(const std::string &text) {
  throw std::runtime_error(text);
}
std::string binary(std::size_t value, int width) {
  std::string result;
  for (int i = width - 1; i >= 0; i--)
    result += (value & (std::size_t{1} << i)) ? '1' : '0';
  return result;
}
Machine read_machine(const Json &data) {
  data.only({"kind", "input_bits", "output_bits", "initial", "states",
             "transitions", "sequence"});
  Machine model;
  const auto kind = data.at("kind").string();
  if (kind != "moore" && kind != "mealy")
    fail("State-machine kind must be moore or mealy");
  model.moore = kind == "moore";
  model.input_bits = data.at("input_bits").integer(1, 2);
  model.output_bits = data.at("output_bits").integer(1, 4);
  const auto &states = data.at("states").array();
  if (states.empty() || states.size() > 16)
    fail("Use 1–16 states");
  std::map<std::string, std::size_t> ids;
  for (const auto &state : states) {
    state.only({"name", "output"});
    const auto name = state.at("name").string();
    if (!identifier(name, 12) || !ids.emplace(name, ids.size()).second)
      fail("State names must be unique identifiers of at most 12 characters");
    model.names.push_back(name);
    if (!model.moore && state.find("output"))
      fail("Mealy outputs belong to transitions, not states");
    model.outputs.push_back(model.moore ? state.at("output").integer(
                                              0, (1 << model.output_bits) - 1)
                                        : 0);
  }
  const auto initial = data.at("initial").string();
  if (!ids.contains(initial))
    fail("Initial state is not in the state list");
  model.initial = ids.at(initial);
  model.rows.assign(states.size(), std::vector<Transition>(
                                       std::size_t{1} << model.input_bits));
  const auto &transitions = data.at("transitions").array();
  if (transitions.size() !=
      states.size() * (std::size_t{1} << model.input_bits))
    fail("Supply exactly one transition for every state and input combination");
  for (const auto &row : transitions) {
    row.only({"from", "input", "to", "output"});
    const auto from = row.at("from").string(), to = row.at("to").string();
    if (!ids.contains(from) || !ids.contains(to))
      fail("Transition names an unknown state");
    const auto input = static_cast<std::size_t>(
        row.at("input").integer(0, (1 << model.input_bits) - 1));
    auto &target = model.rows[ids.at(from)][input];
    if (target.supplied)
      fail("Duplicate state/input transition");
    if (model.moore && row.find("output"))
      fail("Moore outputs belong to states, not transitions");
    target = {ids.at(to),
              model.moore
                  ? model.outputs[ids.at(from)]
                  : row.at("output").integer(0, (1 << model.output_bits) - 1),
              true};
  }
  if (const auto sequence = data.find("sequence")) {
    if (sequence->is_string()) {
      if (!trim(sequence->string()).empty())
        for (double value : numbers(sequence->string(), 256))
          model.sequence.push_back(
              Json(value).integer(0, (1 << model.input_bits) - 1));
    } else {
      if (sequence->array().size() > 256)
        fail("Simulation is limited to 256 input symbols");
      for (const auto &input : sequence->array())
        model.sequence.push_back(input.integer(0, (1 << model.input_bits) - 1));
    }
  }
  return model;
}
struct Reduced {
  Machine machine;
  std::vector<int> groups;
  std::vector<bool> reachable;
  int rounds{};
};
Reduced reduce(const Machine &model) {
  Reduced out;
  out.reachable.resize(model.names.size());
  std::queue<std::size_t> pending;
  pending.push(model.initial);
  out.reachable[model.initial] = true;
  while (!pending.empty()) {
    const auto state = pending.front();
    pending.pop();
    for (const auto &row : model.rows[state])
      if (!out.reachable[row.next]) {
        out.reachable[row.next] = true;
        pending.push(row.next);
      }
  }
  out.groups.assign(model.names.size(), -1);
  const auto partition = [&](bool successor) {
    std::map<std::vector<int>, int> known;
    std::vector<int> next(model.names.size(), -1);
    for (std::size_t state = 0; state < model.names.size(); state++)
      if (out.reachable[state]) {
        std::vector<int> key;
        if (model.moore)
          key.push_back(model.outputs[state]);
        for (const auto &edge : model.rows[state]) {
          if (!model.moore)
            key.push_back(edge.output);
          if (successor)
            key.push_back(out.groups[edge.next]);
        }
        const auto found = known.find(key);
        if (found == known.end()) {
          const auto group = static_cast<int>(known.size());
          known.emplace(key, group);
          next[state] = group;
        } else
          next[state] = found->second;
      }
    return next;
  };
  out.groups = partition(false);
  for (std::size_t pass = 0; pass <= model.names.size(); pass++) {
    auto next = partition(true);
    out.rounds++;
    if (next == out.groups)
      break;
    out.groups = std::move(next);
  }
  const auto count = static_cast<std::size_t>(
      *std::max_element(out.groups.begin(), out.groups.end()) + 1);
  out.machine.moore = model.moore;
  out.machine.input_bits = model.input_bits;
  out.machine.output_bits = model.output_bits;
  out.machine.initial = static_cast<std::size_t>(out.groups[model.initial]);
  out.machine.names.resize(count);
  out.machine.outputs.resize(count);
  out.machine.rows.resize(count);
  for (std::size_t state = 0; state < model.names.size(); state++)
    if (out.reachable[state]) {
      const auto group = static_cast<std::size_t>(out.groups[state]);
      if (out.machine.names[group].empty()) {
        out.machine.names[group] = "G" + std::to_string(group);
        out.machine.outputs[group] = model.outputs[state];
        for (const auto &edge : model.rows[state])
          out.machine.rows[group].push_back(
              {static_cast<std::size_t>(out.groups[edge.next]), edge.output,
               true});
      }
    }
  return out;
}
std::size_t verify_quotient(const Machine &original, const Reduced &reduced) {
  std::size_t checked = 0;
  for (std::size_t state = 0; state < original.names.size(); state++)
    if (reduced.reachable[state]) {
      const auto group = static_cast<std::size_t>(reduced.groups[state]);
      if (original.moore &&
          original.outputs[state] != reduced.machine.outputs[group])
        fail("Moore output equivalence check failed");
      for (std::size_t input = 0; input < original.rows[state].size();
           input++) {
        const auto &before = original.rows[state][input];
        const auto &after = reduced.machine.rows[group][input];
        if (static_cast<std::size_t>(reduced.groups[before.next]) !=
                after.next ||
            before.output != after.output)
          fail("State reduction transition/output check failed");
        checked++;
      }
    }
  return checked;
}
Json transition_table(const Machine &model) {
  Json::Array rows;
  for (std::size_t state = 0; state < model.names.size(); state++)
    for (std::size_t input = 0; input < model.rows[state].size(); input++) {
      const auto &edge = model.rows[state][input];
      rows.emplace_back(Json::Object{
          {"state", model.names[state]},
          {"input", binary(input, model.input_bits)},
          {"next", model.names[edge.next]},
          {"output",
           binary(static_cast<std::size_t>(model.moore ? model.outputs[state]
                                                       : edge.output),
                  model.output_bits)}});
    }
  return rows;
}
Json diagram(const Machine &model) {
  const auto count = model.names.size();
  const double dimension = std::max(600., static_cast<double>(count) * 100.),
               center = dimension / 2, radius = dimension / 2 - 125;
  Drawing drawing(dimension, dimension);
  std::vector<std::pair<double, double>> positions;
  for (std::size_t i = 0; i < count; i++) {
    const auto angle = 2 * std::numbers::pi * static_cast<double>(i) /
                           static_cast<double>(count) -
                       std::numbers::pi / 2;
    positions.emplace_back(center + radius * std::cos(angle),
                           center + radius * std::sin(angle));
  }
  for (std::size_t from = 0; from < count; from++) {
    std::map<std::size_t, std::string> labels;
    for (std::size_t input = 0; input < model.rows[from].size(); input++) {
      const auto &edge = model.rows[from][input];
      auto &text = labels[edge.next];
      if (!text.empty())
        text += ", ";
      text += binary(input, model.input_bits);
      if (!model.moore)
        text += "/" + binary(static_cast<std::size_t>(edge.output),
                             model.output_bits);
    }
    for (const auto &[to, label] : labels) {
      const auto [x1, y1] = positions[from];
      const auto [x2, y2] = positions[to];
      if (to == from) {
        const double angle = std::atan2(y1 - center, x1 - center),
                     dx = std::cos(angle), dy = std::sin(angle), px = -dy,
                     py = dx;
        const double ax = x1 + dx * 25 + px * 27, ay = y1 + dy * 25 + py * 27,
                     bx = x1 + dx * 25 - px * 27, by = y1 + dy * 25 - py * 27;
        drawing.curve(ax, ay, x1 + dx * 110 + px * 75, y1 + dy * 110 + py * 75,
                      x1 + dx * 110 - px * 75, y1 + dy * 110 - py * 75, bx, by);
        drawing.arrow(bx + dx * 12 - px * 5, by + dy * 12 - py * 5, bx, by);
        drawing.text(x1 + dx * 105, y1 + dy * 105, label, 12);
      } else {
        const double dx = x2 - x1, dy = y2 - y1, length = std::hypot(dx, dy),
                     ux = dx / length, uy = dy / length, px = -uy, py = ux;
        const double bend = 45. + static_cast<double>((from + to) % 3) * 20.;
        const double ax = x1 + ux * 38, ay = y1 + uy * 38, bx = x2 - ux * 38,
                     by = y2 - uy * 38;
        const double c1x = ax + dx / 3 + px * bend,
                     c1y = ay + dy / 3 + py * bend,
                     c2x = bx - dx / 3 + px * bend,
                     c2y = by - dy / 3 + py * bend;
        drawing.curve(ax, ay, c1x, c1y, c2x, c2y, bx, by);
        const double tangent = std::hypot(bx - c2x, by - c2y);
        drawing.arrow(bx - (bx - c2x) / tangent * 12,
                      by - (by - c2y) / tangent * 12, bx, by);
        const double lx = (ax + bx) / 2 + px * bend * .75,
                     ly = (ay + by) / 2 + py * bend * .75;
        drawing.rectangle(lx - static_cast<double>(label.size()) * 3.7 - 4,
                          ly - 10, static_cast<double>(label.size()) * 7.4 + 8,
                          20);
        drawing.text(lx, ly, label, 12);
      }
    }
  }
  for (std::size_t i = 0; i < count; i++) {
    const auto [x, y] = positions[i];
    drawing.circle(x, y, 38, i == model.initial ? "#e2edc3" : "#ffffff");
    drawing.text(x, y - 4, model.names[i], 13);
    if (model.moore)
      drawing.text(x, y + 16,
                   "out " + binary(static_cast<std::size_t>(model.outputs[i]),
                                   model.output_bits),
                   11);
  }
  const auto [x, y] = positions[model.initial];
  drawing.arrow(x - 90, y, x - 40, y);
  drawing.text(x - 90, y - 15, "start", 11);
  return drawing.json(
      model.moore ? "Moore state diagram: outputs inside states; inputs label "
                    "arrows. The complete table is the accessible reference."
                  : "Mealy state diagram: input/output labels on arrows. The "
                    "complete table is the accessible reference.");
}
Json equations(const Machine &model) {
  const int state_bits = std::max(
                1, static_cast<int>(std::bit_width(model.names.size() - 1))),
            variables = state_bits + model.input_bits;
  Json::Array result;
  for (int output = 0; output < state_bits + model.output_bits; output++) {
    std::string cells(std::size_t{1} << variables, 'X');
    for (std::size_t state = 0; state < model.names.size(); state++)
      for (std::size_t input = 0; input < model.rows[state].size(); input++) {
        const auto &edge = model.rows[state][input];
        const auto value =
            output < state_bits
                ? (edge.next >> (state_bits - 1 - output)) & 1u
                : static_cast<std::size_t>(
                      (model.moore ? model.outputs[state] : edge.output) >>
                      (model.output_bits - 1 - (output - state_bits))) &
                      1u;
        cells[(state << model.input_bits) | input] = value ? '1' : '0';
      }
    const auto solved = extended_kmap(
        {"logic",
         "kmap_minimization",
         Json(Json::Object{{"variables", variables}, {"cells", cells}}).dump(),
         {}});
    std::string mapping;
    for (int i = 0; i < variables; i++) {
      if (i)
        mapping += ", ";
      mapping += static_cast<char>('A' + i);
      mapping += "=";
      mapping += i < state_bits ? "Q" + std::to_string(state_bits - 1 - i)
                                : "X" + std::to_string(variables - 1 - i);
    }
    result.emplace_back(Json::Object{
        {"name", output < state_bits
                     ? "D" + std::to_string(state_bits - 1 - output)
                     : "Y" + std::to_string(model.output_bits - 1 -
                                            (output - state_bits))},
        {"expression", solved.answer},
        {"variables", mapping},
        {"verification", solved.verification.evidence},
        {"minimum_certified", solved.warnings.empty()}});
  }
  return result;
}
} // namespace
SolutionBundle state_machine(const ProblemSpec &request) {
  const auto original = read_machine(Json::parse(request.input));
  const auto reduced = reduce(original);
  const auto checked = verify_quotient(original, reduced);
  SolutionBundle out;
  out.domain = "logic";
  out.topic = "state_machine";
  Json::Array groups, unreachable, trace;
  std::string mapping;
  for (std::size_t group = 0; group < reduced.machine.names.size(); group++) {
    Json::Array members;
    std::string names;
    for (std::size_t state = 0; state < original.names.size(); state++)
      if (reduced.groups[state] == static_cast<int>(group)) {
        members.emplace_back(original.names[state]);
        if (!names.empty())
          names += ", ";
        names += original.names[state];
      }
    const auto name = "G" + std::to_string(group);
    groups.emplace_back(Json::Object{
        {"group", name},
        {"members", members},
        {"encoding",
         binary(group, std::max(1, static_cast<int>(std::bit_width(
                                       reduced.machine.names.size() - 1))))}});
    mapping += name + " = {" + names + "}\n";
  }
  for (std::size_t i = 0; i < original.names.size(); i++)
    if (!reduced.reachable[i])
      unreachable.emplace_back(original.names[i]);
  auto current = original.initial;
  std::string outputs;
  if (original.moore)
    outputs = binary(static_cast<std::size_t>(original.outputs[current]),
                     original.output_bits);
  for (std::size_t i = 0; i < original.sequence.size(); i++) {
    const auto input = static_cast<std::size_t>(original.sequence[i]);
    const auto edge = original.rows[current][input];
    const int output =
        original.moore ? original.outputs[edge.next] : edge.output;
    if (!outputs.empty())
      outputs += ' ';
    outputs += binary(static_cast<std::size_t>(output), original.output_bits);
    trace.emplace_back(
        Json::Object{{"step", i + 1},
                     {"from", original.names[current]},
                     {"input", binary(input, original.input_bits)},
                     {"to", original.names[edge.next]},
                     {"output", binary(static_cast<std::size_t>(output),
                                       original.output_bits)}});
    current = edge.next;
  }
  out.answer = std::to_string(original.names.size()) + " supplied states → " +
               std::to_string(reduced.machine.names.size()) +
               " reachable, behaviorally distinct states.\n" + mapping;
  if (!original.sequence.empty())
    out.answer += "Simulation outputs: " + outputs +
                  "\nFinal state: " + original.names[current];
  out.steps = {
      {"FSM_COMPLETENESS",
       "Require one deterministic transition for every state/input pair, valid "
       "state names and bounded output words.",
       std::to_string(original.names.size() *
                      (std::size_t{1} << original.input_bits)) +
           " supplied transitions"},
      {"FSM_REACHABILITY",
       "Explore all transitions from the specified initial state. Unreachable "
       "states are reported and omitted only from the reduced machine.",
       std::to_string(unreachable.size()) + " unreachable states"},
      {"FSM_PARTITION",
       "Initially partition by observable outputs, then refine by successor "
       "equivalence classes until the partition stabilizes.",
       std::to_string(reduced.rounds) + " refinement rounds\n" + mapping},
      {"FSM_EQUIVALENCE",
       "For every reachable original state and every input, check output "
       "equality and that its successor maps to the reduced successor. This "
       "finite relation proves equivalent future output behavior from reset.",
       std::to_string(checked) + " transition/output checks"},
      {"FSM_D_INPUTS",
       "Assign binary codes to reduced states. D-flip-flop inputs equal "
       "next-state bits; derive each bit's SOP and check all assigned "
       "truth-table rows.",
       "Unused state codes are don't-cares; the resulting circuit is not "
       "guaranteed self-recovering from an invalid state."}};
  out.assumptions = {
      original.moore ? "Moore trace includes the initial state's output, "
                       "followed by the new state's output after each input."
                     : "Mealy trace emits one transition output for each "
                       "input; no extra initial output.",
      "Synchronous deterministic machines, one active edge per input. No "
      "metastability, propagation delay or asynchronous hazard model.",
      "Binary inputs in the table; simulation input is comma-separated integer "
      "symbols (0–1 or 0–3). Dense diagrams may have crossing edges; the "
      "complete transition table remains authoritative."};
  out.verification = {
      VerificationStatus::verified_exhaustive,
      "reachable-state equivalence relation and Boolean truth-table checks",
      std::to_string(checked) + " original transition/output pairs replayed"};
  out.visual_json =
      Json(Json::Object{{"kind", "state_machine"},
                        {"drawing", diagram(reduced.machine)},
                        {"original_table", transition_table(original)},
                        {"table", transition_table(reduced.machine)},
                        {"groups", groups},
                        {"unreachable", unreachable},
                        {"trace", trace},
                        {"equations", equations(reduced.machine)}})
          .dump();
  return out;
}
} // namespace pocket_engineer::workbench
