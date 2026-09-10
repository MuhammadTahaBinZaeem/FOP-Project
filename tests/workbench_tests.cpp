#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <iostream>
#include <numbers>
#include <random>
#include <set>
#include <stdexcept>
using namespace pocket_engineer;
using namespace pocket_engineer::workbench;
namespace {
int checks = 0, failed = 0;
void check(bool condition, const std::string &name) {
  ++checks;
  if (!condition) {
    ++failed;
    std::cerr << "FAIL: " << name << '\n';
  }
}
void close(double actual, double expected, const std::string &name,
           double tolerance = 1e-8) {
  check(std::abs(actual - expected) <=
            tolerance * std::max(1., std::abs(expected)),
        name + " actual=" + format(actual) + " expected=" + format(expected));
}
void rejects(const std::function<void()> &action, const std::string &name) {
  try {
    action();
    check(false, name);
  } catch (const std::exception &) {
    check(true, name);
  }
}
SolutionBundle network(std::string input, std::string payload = "{}") {
  return circuit_analysis(
      {"circuit", "network_analysis", std::move(input), std::move(payload)});
}
std::complex<double> node_voltage(const SolutionBundle &solution,
                                  const std::string &name) {
  check(solution.status == "success" &&
            solution.verification.status ==
                VerificationStatus::verified_numerical,
        "verified network result");
  const auto visual = Json::parse(solution.visual_json);
  for (const auto &row : visual.at("nodes").array())
    if (row.at("node").string() == name)
      return {row.at("voltage").at("real").number(),
              row.at("voltage").at("imag").number()};
  throw std::runtime_error("Node missing in test result");
}
void json_tests() {
  for (const auto text :
       {"null", "true", "false", "123", "-0.012e+2", "[]", "{}",
        "[1,\"two\",false]", "{\"text\":\"\\uD83D\\uDE00\",\"x\":[1,2]}"}) {
    const auto value = Json::parse(text);
    check(Json::parse(value.dump()).dump() == value.dump(), "JSON round trip");
  }
  for (const auto text :
       {"[1,]", "{\"x\":1,\"x\":2}", "01", "+1", "1.", "1e", "NaN", "[",
        "{\"x\":\"\\uD800\"}", "\"\\u0000\"", "true rubbish"})
    rejects([&] { (void)Json::parse(text); }, "strict JSON rejection");
  rejects(
      [] { (void)Json::parse(std::string(40, '[') + std::string(40, ']')); },
      "JSON depth bound");
  const ComplexMatrix a{{2., 1.}, {1., 3.}};
  const FactorizedSystem factor(a);
  for (int i = 0; i < 100; i++) {
    const auto solved = factor.solve({2. * i + 2., i + 6.});
    close(solved.values[0].real(), i, "reused LU x");
    close(solved.values[1].real(), 2, "reused LU y");
  }
  rejects([] { (void)solve_linear({{1., 1.}, {2., 2.}}, {1., 3.}); },
          "singular LU");
}
void circuit_tests() {
  close(node_voltage(network("V V1 in 0 12; R R1 in out 1k; R R2 out 0 2k"),
                     "out")
            .real(),
        8, "DC divider");
  close(node_voltage(network("V V1 a b 10; R R1 a 0 2k; R R2 b 0 1k"), "a")
            .real(),
        20. / 3, "floating ideal source");
  close(node_voltage(network("I I1 0 a 2m; R R1 a 0 1k"), "a").real(), 2,
        "independent current source");
  close(node_voltage(network("V V1 c 0 2; E E1 out 0 c 0 3; R R1 out 0 100"),
                     "out")
            .real(),
        6, "VCVS");
  close(node_voltage(network("V V1 c 0 2; G G1 out 0 c 0 3m; R R1 out 0 1k"),
                     "out")
            .real(),
        -6, "VCCS");
  close(node_voltage(
            network("V V1 a 0 5; R R1 a 0 1k; F F1 out 0 V1 2; R R2 out 0 1k"),
            "out")
            .real(),
        10, "CCCS");
  close(
      node_voltage(
          network("V V1 a 0 5; R R1 a 0 1k; H H1 out 0 V1 1000; R R2 out 0 1k"),
          "out")
          .real(),
      -5, "CCVS");
  close(node_voltage(network("V V1 in 0 12; L L1 in out 10m; R R1 out 0 2"),
                     "out")
            .real(),
        12, "DC inductor short");
  close(
      node_voltage(
          network("V V1 in 0 12; R R1 in out 1k; R R2 out 0 2k; C C1 out 0 1u"),
          "out")
          .real(),
      8, "DC capacitor open");
  const auto ac_payload =
      Json(Json::Object{{"analysis", "ac"},
                        {"frequency", 1 / (2 * std::numbers::pi * .001)}})
          .dump();
  const auto ac = node_voltage(
      network("V V1 in 0 1; R R1 in out 1k; C C1 out 0 1u", ac_payload), "out");
  close(ac.real(), .5, "RC phasor real");
  close(ac.imag(), -.5, "RC phasor imaginary");
  const auto phase =
      node_voltage(network("V V1 a 0 2@90; R R1 a 0 1k", ac_payload), "a");
  close(phase.real(), 0, "RMS source phase real");
  close(phase.imag(), 2, "RMS source phase imag");
  const auto transient = network(
      "V V1 in 0 1; R R1 in out 1k; C C1 out 0 1u",
      R"({"analysis":"transient","step":0.00001,"steps":100,"observe":"out"})");
  close(node_voltage(transient, "out").real(), 1 - std::pow(1.01, -100),
        "backward Euler recurrence", 1e-7);
  const auto charged = network(
      "V V1 in 0 1; R R1 in out 1k; C C1 out 0 1u",
      R"({"analysis":"transient","step":0.00001,"steps":100,"observe":"out","initial":{"C1":0.5}})");
  close(node_voltage(charged, "out").real(), 1 - .5 * std::pow(1.01, -100),
        "capacitor initial condition", 1e-7);
  for (const auto input :
       {"R R1 a b 1k", "V V1 a 0 12; V V2 a 0 5", "R R1 a 0 -1",
        "R R1 a 0 1; R R1 b 0 1", "D D1 a 0 0.7", "V V1 a 0 1 trailing",
        "F F1 a 0 no_source 2; R R1 a 0 1"})
    rejects([&] { (void)network(input); }, "invalid circuit");
  rejects(
      [] {
        (void)network("V V1 a 0 1; R R1 a 0 100",
                      R"({"analysis":"transient","steps":2001})");
      },
      "transient step bound");
  std::mt19937 rng(505);
  for (int trial = 0; trial < 500; trial++) {
    const double a = 1 + static_cast<double>(rng() % 1000),
                 b = 1 + static_cast<double>(rng() % 1000),
                 v = static_cast<double>(rng() % 1000) / 10;
    const auto input = "V V1 in 0 " + format(v) + "; R R1 in out " + format(a) +
                       "; R R2 out 0 " + format(b);
    close(node_voltage(network(input), "out").real(), v * b / (a + b),
          "independent randomized divider");
  }
}
void kmap_tests() {
  for (int variables = 2; variables <= 6; variables++) {
    const auto state =
        Json(Json::Object{
                 {"variables", variables},
                 {"cells", std::string(std::size_t{1} << variables, '0')}})
            .dump();
    const auto editor = kmap_editor({"logic", "kmap_editor", state, {}});
    std::set<int> indices;
    for (const auto &panel : editor.at("panels").array())
      for (const auto &row : panel.at("rows").array())
        for (const auto &cell : row.at("cells").array())
          indices.insert(cell.at("index").integer(0, 63));
    check(indices.size() == std::size_t{1} << variables,
          "every K-map minterm appears exactly once");
    auto current = editor.at("state").dump();
    for (const char expected : std::string("1X0")) {
      const auto edited = kmap_editor(
          {"logic", "kmap_editor", current, R"({"action":"cycle","cell":0})"});
      check(edited.at("state").at("cells").string()[0] == expected,
            "C++ cell cycle");
      current = edited.at("state").dump();
    }
    const auto all = extended_kmap(
        {"logic",
         "kmap",
         Json(Json::Object{
                  {"variables", variables},
                  {"cells", std::string(std::size_t{1} << variables, '1')}})
             .dump(),
         {}});
    check(all.answer == "1", "constant one K-map");
  }
  // Independent dynamic programming over covered truth-table sets finds the
  // exact term/literal optimum for EVERY 3-variable Boolean function.
  for (unsigned function = 0; function < 256; function++) {
    std::string cells(8, '0');
    for (unsigned i = 0; i < 8; i++)
      if (function & (1u << i))
        cells[i] = '1';
    std::vector<std::pair<unsigned, int>> candidates;
    for (unsigned mask = 0; mask < 8; mask++)
      for (unsigned value = 0; value < 8; value++)
        if (!(value & mask)) {
          unsigned cover = 0;
          for (unsigned i = 0; i < 8; i++)
            if ((i & ~mask) == value)
              cover |= 1u << i;
          if (!(cover & ~function))
            candidates.emplace_back(cover, 3 - std::popcount(mask));
        }
    std::vector<std::pair<int, int>> cost(256, {99, 99});
    cost[0] = {0, 0};
    for (unsigned covered = 0; covered < 256; covered++)
      if (!(covered & ~function) && cost[covered].first < 99)
        for (const auto &[cube, literals] : candidates) {
          const auto combined = covered | cube;
          cost[combined] = std::min(cost[combined],
                                    std::pair{cost[covered].first + 1,
                                              cost[covered].second + literals});
        }
    const auto result = extended_kmap(
        {"logic",
         "kmap",
         Json(Json::Object{{"variables", 3}, {"cells", cells}}).dump(),
         {}});
    const auto visual = Json::parse(result.visual_json);
    int literals = 0;
    unsigned actual = 0;
    for (const auto &group : visual.at("groups").array()) {
      literals += group.at("literals").integer(0, 3);
      for (const auto &cell : group.at("cells").array())
        actual |= 1u << cell.integer(0, 7);
    }
    check(actual == function, "K-map independent cube membership");
    check(std::pair{static_cast<int>(visual.at("groups").array().size()),
                    literals} == cost[function],
          "K-map exact 3-variable minimum");
  }
  std::mt19937 random(551);
  for (int trial = 0; trial < 200; trial++) {
    std::string cells(64, '0');
    for (auto &cell : cells)
      cell = "01X"[random() % 3];
    const auto result = extended_kmap(
        {"logic",
         "kmap",
         Json(Json::Object{{"variables", 6}, {"cells", cells}}).dump(),
         {}});
    const auto visual = Json::parse(result.visual_json);
    std::set<int> cover;
    for (const auto &group : visual.at("groups").array())
      for (const auto &cell : group.at("cells").array())
        cover.insert(cell.integer(0, 63));
    for (int i = 0; i < 64; i++)
      if (cells[static_cast<std::size_t>(i)] != 'X')
        check(cover.contains(i) == (cells[static_cast<std::size_t>(i)] == '1'),
              "random six-variable truth coverage");
  }
  rejects(
      [] {
        (void)extended_kmap(
            {"logic",
             "kmap",
             R"({"variables":2,"cells":"1111","groups":[[0,3]]})",
             {}});
      },
      "diagonal manual group");
  rejects(
      [] {
        (void)extended_kmap(
            {"logic",
             "kmap",
             R"({"variables":2,"cells":"1111","groups":[[0,1]]})",
             {}});
      },
      "incomplete manual cover");
  rejects(
      [] { (void)extended_kmap({"logic", "kmap", "vars=6; minterms=64", {}}); },
      "K-map bounds");
}
void drawing_tests() {
  for (const auto preset : {"divider", "rc", "rlc"}) {
    const auto editor = circuit_editor(
        {"circuit",
         "circuit_editor",
         {},
         Json(Json::Object{{"action", "preset"}, {"preset", preset}}).dump()});
    const auto solution = network(editor.at("input").string(),
                                  R"({"analysis":"ac","frequency":1000})");
    check(solution.verification.status ==
              VerificationStatus::verified_numerical,
          "drawn preset solves");
    check(editor.at("drawing").at("commands").array().size() > 400,
          "drawing includes bounded raster commands");
    check(circuit_editor(
              {"circuit", "circuit_editor", editor.at("state").dump(), {}})
                  .at("input")
                  .string() == editor.at("input").string(),
          "drawing topology round trip");
  }
  const auto initial = circuit_editor({"circuit", "circuit_editor", {}, {}});
  rejects(
      [&] {
        (void)circuit_editor(
            {"circuit", "circuit_editor", initial.at("state").dump(),
             R"({"action":"place","component":{"id":"R3","type":"R","value":"1k","x":10,"y":4,"rotation":0}})"});
      },
      "overlap rejected");
  const auto crossing = Json::parse(
      R"({"components":[],"grounds":[],"wires":[{"a":[0,4],"b":[8,4]},{"a":[4,0],"b":[4,8]}]})");
  const auto graph =
      circuit_editor({"circuit", "circuit_editor", crossing.dump(), {}});
  std::set<std::string> nodes;
  for (const auto &row : graph.at("nodes").array())
    nodes.insert(row.at("node").string());
  check(nodes.size() == 2, "wire interiors crossing are separate nets");
  const auto connected =
      circuit_editor({"circuit", "circuit_editor", crossing.dump(),
                      R"({"action":"ground","position":[4,4]})"});
  nodes.clear();
  for (const auto &row : connected.at("nodes").array())
    nodes.insert(row.at("node").string());
  check(nodes == std::set<std::string>{"0"},
        "explicit crossing junction joins nets");
}
SolutionBundle signal(Json::Object data) {
  return signals({"signals", "signals", Json(data).dump(), {}});
}
void signals_tests() {
  const auto impulse = signal({{"operation", "fft"}, {"x", "1,0,0,0,0,0,0,0"}});
  const auto spectrum = Json::parse(impulse.visual_json);
  for (const auto &bin : spectrum.at("bins").array()) {
    close(bin.at("value").at("real").number(), 1, "impulse Fourier real");
    close(bin.at("value").at("imag").number(), 0, "impulse Fourier imaginary");
  }
  std::mt19937 random(880);
  for (int trial = 0; trial < 120; trial++) {
    const auto size = std::size_t{1} << (random() % 6);
    std::vector<double> x(size);
    for (auto &value : x)
      value = (static_cast<double>(random() % 2001) - 1000) / 100;
    const auto result = signal({{"operation", "fft"}, {"x", array_json(x)}});
    const auto visual = Json::parse(result.visual_json);
    for (std::size_t k = 0; k < size; k++) {
      std::complex<long double> expected{};
      for (std::size_t n = 0; n < size; n++) {
        const long double angle =
            -2 * std::numbers::pi_v<long double> * k * n / size;
        expected += static_cast<long double>(x[n]) *
                    std::complex<long double>(std::cos(angle), std::sin(angle));
      }
      const auto &value = visual.at("bins").array()[k].at("value");
      close(value.at("real").number(), static_cast<double>(expected.real()),
            "FFT against direct long-double DFT");
      close(value.at("imag").number(), static_cast<double>(expected.imag()),
            "FFT imaginary direct DFT");
    }
  }
  const auto conv =
      Json::parse(
          signal({{"operation", "convolution"}, {"x", "1,2,3"}, {"h", "4,5"}})
              .visual_json)
          .at("samples")
          .array();
  const std::vector<double> expected{4, 13, 22, 15};
  for (std::size_t i = 0; i < expected.size(); i++)
    close(conv[i].number(), expected[i], "finite convolution oracle");
  const auto corr =
      Json::parse(
          signal({{"operation", "correlation"}, {"x", "1,2,3"}, {"h", "4,5"}})
              .visual_json)
          .at("samples")
          .array();
  const std::vector<double> correlation{5, 14, 23, 12};
  for (std::size_t i = 0; i < correlation.size(); i++)
    close(corr[i].number(), correlation[i], "cross correlation lag convention");
  for (const auto kind : {"power", "sine", "cosine"})
    for (const double rate : {-3., 0., 2.}) {
      const auto result =
          signal({{"operation", "laplace"},
                  {"terms", Json::Array{Json::Object{{"kind", kind},
                                                     {"rate", rate},
                                                     {"omega", 7},
                                                     {"power", 3},
                                                     {"amplitude", 2},
                                                     {"delay", .5}}}}});
      check(result.verification.status ==
                VerificationStatus::verified_numerical,
            "Laplace defining-integral check");
    }
  for (const auto denominator : {"1,1", "1,2,1", "1,3,2", "1,0,4"}) {
    const auto result = signal({{"operation", "inverse_laplace"},
                                {"numerator", "1"},
                                {"denominator", denominator},
                                {"duration", 1},
                                {"count", 3}});
    check(result.verification.status == VerificationStatus::verified_numerical,
          "inverse Laplace reconstruction");
    const auto y = Json::parse(result.visual_json).at("samples").array();
    const std::string d = denominator;
    const auto expected_value = d == "1,1"     ? std::exp(-1.)
                                : d == "1,2,1" ? std::exp(-1.)
                                : d == "1,3,2" ? std::exp(-1.) - std::exp(-2.)
                                               : std::sin(2.) / 2;
    close(y.back().number(), expected_value, "inverse Laplace time oracle");
  }
  const auto frequency = signal({{"operation", "frequency_response"},
                                 {"numerator", "1"},
                                 {"denominator", "0.001,1"},
                                 {"f_min", 1 / (2 * std::numbers::pi * .001)},
                                 {"f_max", 1000},
                                 {"count", 2}});
  const auto h = Json::parse(frequency.visual_json)
                     .at("response")
                     .array()
                     .front()
                     .at("value");
  close(h.at("real").number(), .5, "low-pass transfer real");
  close(h.at("imag").number(), -.5, "low-pass transfer imaginary");
  rejects([] { (void)signal({{"operation", "fft"}, {"x", "1,2,3"}}); },
          "nonpower FFT explicit padding required");
  rejects(
      [] {
        (void)signal({{"operation", "ifft"}, {"x", "1,2,3"}, {"pad", true}});
      },
      "inverse transform cannot silently pad");
  rejects(
      [] {
        (void)signal(
            {{"operation", "convolution"}, {"x", "1,NaN"}, {"h", "1"}});
      },
      "nonfinite signal");
  rejects(
      [] {
        (void)signal({{"operation", "inverse_laplace"},
                      {"numerator", "1,0"},
                      {"denominator", "1,1"}});
      },
      "improper rational transform");
}
void fsm_tests() {
  auto data = Json::parse(
      R"({"kind":"mealy","input_bits":1,"output_bits":1,"initial":"A","states":[{"name":"A"},{"name":"B"},{"name":"unused"}],"transitions":[{"from":"A","input":0,"to":"A","output":0},{"from":"A","input":1,"to":"B","output":1},{"from":"B","input":0,"to":"A","output":0},{"from":"B","input":1,"to":"B","output":1},{"from":"unused","input":0,"to":"unused","output":1},{"from":"unused","input":1,"to":"unused","output":0}],"sequence":"0,1,1,0"})");
  const auto result =
      state_machine({"logic", "state_machine", data.dump(), {}});
  const auto visual = Json::parse(result.visual_json);
  check(visual.at("groups").array().size() == 1,
        "equivalent reachable Mealy states merged");
  check(visual.at("unreachable").array().size() == 1,
        "unreachable state reported");
  check(result.answer.find("0 1 1 0") != std::string::npos,
        "Mealy trace matches input echo");
  check(visual.at("equations").array().size() == 2,
        "D input and output equations produced");
  data.object()["transitions"].array().pop_back();
  rejects(
      [&] { (void)state_machine({"logic", "state_machine", data.dump(), {}}); },
      "incomplete transition table rejected");
  const auto moore = state_machine(
      {"logic",
       "state_machine",
       R"({"kind":"moore","input_bits":1,"output_bits":1,"initial":"A","states":[{"name":"A","output":0},{"name":"B","output":1}],"transitions":[{"from":"A","input":0,"to":"A"},{"from":"A","input":1,"to":"B"},{"from":"B","input":0,"to":"A"},{"from":"B","input":1,"to":"B"}],"sequence":"1,0,1"})",
       {}});
  check(moore.answer.find("0 1 0 1") != std::string::npos,
        "Moore initial output then post-transition outputs");
  check(Json::parse(moore.visual_json).at("groups").array().size() == 2,
        "distinct Moore outputs retained");
}
void guided_tests() {
  for (const auto &info : topic_catalog()) {
    const auto target =
        Json::Object{{"domain", info.domain}, {"topic", info.topic}};
    const auto definition =
        schema({"workbench", "schema", Json(target).dump(), {}});
    const auto kind = definition.at("kind").string();
    if (kind == "circuit" || kind == "kmap")
      continue;
    Json::Object data = target;
    if (kind == "matrix")
      data["matrices"] = definition.at("matrices");
    else {
      Json::Object values;
      for (const auto &field : definition.at("fields").array())
        values[field.at("id").string()] = field.at("value");
      data["values"] = values;
    }
    const auto converted =
        guided_input({"workbench", "guided_input", Json(data).dump(), {}});
    const auto solved = Engine{}.solve({std::string(info.domain),
                                        std::string(info.topic),
                                        converted.at("input").string(),
                                        {},
                                        "manual"});
    check(solved.status == "success",
          "guided default solves: " + std::string(info.topic) + " " +
              solved.answer);
  }
  const char *result = pe_workbench_json(
      R"({"domain":"logic","topic":"kmap_solve","input":"vars=5; minterms=1,3,5,7","mode":"manual"})");
  check(result && Json::parse(result).at("status").string() == "success",
        "shared C ABI extended K-map");
  pe_free_string(result);
}
} // namespace
int main() {
  try {
    json_tests();
    circuit_tests();
    kmap_tests();
    drawing_tests();
    signals_tests();
    fsm_tests();
    guided_tests();
  } catch (const std::exception &error) {
    ++failed;
    std::cerr << "Unexpected exception: " << error.what() << '\n';
  }
  std::cout << checks << " workbench checks, " << failed << " failures\n";
  return failed ? 1 : 0;
}
