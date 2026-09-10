#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <functional>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace pocket_engineer::workbench {
namespace {
using Mask = std::uint64_t;
struct Cube {
  unsigned value{}, mask{};
  Mask on{};
};
struct Map {
  int variables{};
  std::string cells, order;
  Json::Array groups;
};
[[noreturn]] void fail(const std::string &text) {
  throw std::runtime_error(text);
}
Map read_map(const Json &data) {
  data.only({"variables", "cells", "order", "groups"});
  Map map;
  map.variables = data.at("variables").integer(2, 6);
  map.cells = data.at("cells").string();
  map.order = data.text(
      "order",
      std::string("ABCDEF").substr(0, static_cast<std::size_t>(map.variables)));
  if (map.cells.size() != static_cast<std::size_t>(1 << map.variables))
    fail("The K-map needs exactly 2^variables cells");
  for (auto &cell : map.cells) {
    if (cell == 'x')
      cell = 'X';
    if (cell != '0' && cell != '1' && cell != 'X')
      fail("K-map cells must be 0, 1 or X");
  }
  auto order = map.order;
  std::sort(order.begin(), order.end());
  if (order !=
      std::string("ABCDEF").substr(0, static_cast<std::size_t>(map.variables)))
    fail("Variable order must be a permutation of the selected variables");
  if (const auto groups = data.find("groups")) {
    map.groups = groups->array();
    if (map.groups.size() > 64)
      fail("At most 64 groups are allowed");
  }
  return map;
}
std::string canonical(const Map &map) {
  std::string on, dc;
  for (std::size_t i = 0; i < map.cells.size(); i++) {
    auto *list = map.cells[i] == '1'   ? &on
                 : map.cells[i] == 'X' ? &dc
                                       : nullptr;
    if (list) {
      if (!list->empty())
        *list += ',';
      *list += std::to_string(i);
    }
  }
  return "vars=" + std::to_string(map.variables) + "; minterms=" + on +
         "; dc=" + dc;
}
std::string bits(unsigned value, int count) {
  std::string out;
  for (int bit = count - 1; bit >= 0; bit--)
    out += (value & (1u << bit)) ? '1' : '0';
  return out;
}
std::string term(const Cube &cube, int variables) {
  std::string out;
  for (int bit = variables - 1; bit >= 0; bit--)
    if (!(cube.mask & (1u << bit))) {
      if (!out.empty())
        out += " & ";
      if (!(cube.value & (1u << bit)))
        out += '!';
      out += static_cast<char>('A' + variables - 1 - bit);
    }
  return out.empty() ? "1" : out;
}
Cube validate_group(const Json &input, const Map &map) {
  const auto &list = input.array();
  if (list.empty() || !std::has_single_bit(list.size()))
    fail("A group must contain 1, 2, 4, 8, 16, 32 or 64 distinct cells");
  std::set<unsigned> cells;
  unsigned common = static_cast<unsigned>((1 << map.variables) - 1), vary = 0;
  Mask on = 0;
  for (const auto &cell : list) {
    const auto value =
        static_cast<unsigned>(cell.integer(0, (1 << map.variables) - 1));
    if (!cells.insert(value).second)
      fail("A manual group repeats a cell");
    if (map.cells[value] == '0')
      fail("A group cannot include a zero cell");
    common &= value;
    vary |= value;
    if (map.cells[value] == '1')
      on |= Mask{1} << value;
  }
  vary ^= common;
  if ((std::size_t{1} << std::popcount(vary)) != cells.size())
    fail("Those cells do not form a Boolean cube. Wrap-around and matching "
         "cells across panels are allowed; diagonal-only groups are not.");
  if (on == 0)
    fail("A useful group must cover at least one required 1");
  return {common, vary, on};
}
Json group_json(const Cube &cube, const Map &map, std::size_t id) {
  Json::Array members;
  for (unsigned cell = 0; cell < map.cells.size(); cell++)
    if ((cell & ~cube.mask) == cube.value)
      members.emplace_back(static_cast<int>(cell));
  return Json::Object{{"id", id + 1},
                      {"cells", members},
                      {"term", term(cube, map.variables)},
                      {"literals", map.variables - std::popcount(cube.mask)}};
}
struct Cover {
  std::vector<Cube> chosen;
  bool optimal{true};
  std::size_t visits{};
};
Cover minimize(const Map &map) {
  Mask on = 0, allowed = 0;
  for (unsigned i = 0; i < map.cells.size(); i++) {
    if (map.cells[i] == '1')
      on |= Mask{1} << i;
    if (map.cells[i] != '0')
      allowed |= Mask{1} << i;
  }
  if (on == 0)
    return {};
  std::vector<Cube> cubes;
  const auto size = static_cast<unsigned>(map.cells.size());
  for (unsigned mask = 0; mask < size; mask++)
    for (unsigned value = 0; value < size; value++)
      if (!(value & mask)) {
        Mask cover = 0;
        bool valid = true;
        for (unsigned cell = 0; cell < size; cell++)
          if ((cell & ~mask) == value) {
            if (!(allowed & (Mask{1} << cell))) {
              valid = false;
              break;
            }
            cover |= (on & (Mask{1} << cell));
          }
        if (valid && cover)
          cubes.push_back({value, mask, cover});
      }
  std::vector<Cube> primes;
  for (std::size_t i = 0; i < cubes.size(); i++) {
    bool dominated = false;
    for (std::size_t j = 0; j < cubes.size(); j++)
      if (i != j &&
          std::popcount(cubes[j].mask) >= std::popcount(cubes[i].mask) &&
          (cubes[i].on & ~cubes[j].on) == 0) {
        if (std::popcount(cubes[j].mask) > std::popcount(cubes[i].mask) ||
            cubes[j].on != cubes[i].on || j < i) {
          dominated = true;
          break;
        }
      }
    if (!dominated)
      primes.push_back(cubes[i]);
  }
  Cover best;
  Mask remaining = on;
  while (remaining) {
    auto pick = primes.begin();
    for (auto it = primes.begin(); it != primes.end(); ++it) {
      const auto score = std::popcount(it->on & remaining),
                 old = std::popcount(pick->on & remaining);
      if (score > old ||
          (score == old && std::popcount(it->mask) > std::popcount(pick->mask)))
        pick = it;
    }
    if (pick == primes.end() || !(pick->on & remaining))
      fail("Could not cover the required minterms");
    best.chosen.push_back(*pick);
    remaining &= ~pick->on;
  }
  auto cost = [&](const std::vector<Cube> &chosen) {
    int literals = 0;
    for (const auto &cube : chosen)
      literals += map.variables - std::popcount(cube.mask);
    return std::pair{chosen.size(), literals};
  };
  auto best_cost = cost(best.chosen);
  std::vector<Cube> path;
  std::unordered_map<Mask, std::pair<std::size_t, int>> seen;
  std::function<void(Mask, int)> visit = [&](Mask left, int literals) {
    if (++best.visits > 200000) {
      best.optimal = false;
      return;
    }
    if (!left) {
      const auto candidate = std::pair{path.size(), literals};
      if (candidate < best_cost) {
        best_cost = candidate;
        best.chosen = path;
      }
      return;
    }
    if (path.size() >= best_cost.first)
      return;
    const auto prior = seen.find(left);
    const auto current_cost = std::pair{path.size(), literals};
    if (prior != seen.end() && prior->second <= current_cost)
      return;
    if (seen.size() < 100000)
      seen[left] = current_cost;
    unsigned target = 0;
    std::size_t fewest = std::numeric_limits<std::size_t>::max();
    int maximum = 1;
    for (unsigned bit = 0; bit < size; bit++)
      if (left & (Mask{1} << bit)) {
        std::size_t choices = 0;
        for (const auto &cube : primes) {
          if (cube.on & (Mask{1} << bit))
            choices++;
          maximum = std::max(maximum, std::popcount(cube.on & left));
        }
        if (choices < fewest) {
          fewest = choices;
          target = bit;
        }
      }
    if (path.size() + static_cast<std::size_t>(
                          (std::popcount(left) + maximum - 1) / maximum) >
        best_cost.first)
      return;
    for (const auto &cube : primes)
      if (cube.on & (Mask{1} << target)) {
        path.push_back(cube);
        visit(left & ~cube.on,
              literals + map.variables - std::popcount(cube.mask));
        path.pop_back();
        if (!best.optimal)
          return;
      }
  };
  visit(on, 0);
  return best;
}
Map from_text(const std::string &input) {
  std::map<std::string, std::string> fields;
  for (const auto &field : split(input, ';')) {
    if (field.empty())
      continue;
    const auto eq = field.find('=');
    if (eq == std::string::npos ||
        !fields.emplace(trim(field.substr(0, eq)), trim(field.substr(eq + 1)))
             .second)
      fail("Invalid or duplicate K-map field");
  }
  for (const auto &[key, value] : fields) {
    (void)value;
    if (key != "vars" && key != "minterms" && key != "dc")
      fail("Unknown K-map field: " + key);
  }
  if (!fields.contains("vars") || !fields.contains("minterms"))
    fail("Use vars=2..6; minterms=...; dc=...");
  const auto count = Json(finite_number(fields.at("vars"))).integer(2, 6);
  Map map{count,
          std::string(std::size_t{1} << count, '0'),
          std::string("ABCDEF").substr(0, static_cast<std::size_t>(count)),
          {}};
  for (const auto key : {"minterms", "dc"})
    if (fields.contains(key) && !fields.at(key).empty())
      for (const auto &text : split(fields.at(key), ',')) {
        const auto cell =
            Json(finite_number(text)).integer(0, (1 << count) - 1);
        if (map.cells[static_cast<std::size_t>(cell)] != '0')
          fail("Minterms and don't-cares must be disjoint and must not repeat");
        map.cells[static_cast<std::size_t>(cell)] =
            std::string_view(key) == "dc" ? 'X' : '1';
      }
  return map;
}
} // namespace
Json kmap_editor(const ProblemSpec &request) {
  Map map = request.input.empty() ? Map{4, std::string(16, '0'), "ABCD", {}}
            : request.input.front() == '{'
                ? read_map(Json::parse(request.input))
                : from_text(request.input);
  if (!request.payload.empty()) {
    const auto action = Json::parse(request.payload);
    action.only({"action", "cell", "group"});
    const auto name = action.at("action").string();
    if (name == "cycle") {
      const auto cell = action.at("cell").integer(0, (1 << map.variables) - 1);
      auto &value = map.cells[static_cast<std::size_t>(cell)];
      value = value == '0' ? '1' : value == '1' ? 'X' : '0';
      map.groups.clear();
    } else if (name == "clear") {
      map.cells.assign(map.cells.size(), '0');
      map.groups.clear();
    } else if (name == "add_group") {
      (void)validate_group(action.at("group"), map);
      map.groups.push_back(action.at("group"));
    } else if (name == "clear_groups")
      map.groups.clear();
    else
      fail("Unknown K-map edit action");
  }
  const int plane_bits = std::max(0, map.variables - 4),
            remaining = map.variables - plane_bits, row_bits = remaining / 2,
            col_bits = remaining - row_bits;
  Json::Array panels;
  for (int plane = 0; plane < (1 << plane_bits); plane++) {
    const unsigned gray_plane = static_cast<unsigned>(plane ^ (plane >> 1));
    Json::Array rows, columns;
    for (int c = 0; c < (1 << col_bits); c++)
      columns.emplace_back(bits(static_cast<unsigned>(c ^ (c >> 1)), col_bits));
    for (int r = 0; r < (1 << row_bits); r++) {
      const auto gray_row = static_cast<unsigned>(r ^ (r >> 1));
      Json::Array cells;
      for (int c = 0; c < (1 << col_bits); c++) {
        const auto code = (gray_plane << remaining) | (gray_row << col_bits) |
                          static_cast<unsigned>(c ^ (c >> 1));
        unsigned cell = 0;
        for (int bit = 0; bit < map.variables; bit++)
          if (code & (1u << (map.variables - 1 - bit)))
            cell |= 1u << (map.variables - 1 -
                           (map.order[static_cast<std::size_t>(bit)] - 'A'));
        cells.emplace_back(
            Json::Object{{"index", static_cast<int>(cell)},
                         {"value", std::string(1, map.cells[cell])}});
      }
      rows.emplace_back(
          Json::Object{{"label", bits(gray_row, row_bits)}, {"cells", cells}});
    }
    panels.emplace_back(Json::Object{
        {"title", plane_bits ? map.order.substr(
                                   0, static_cast<std::size_t>(plane_bits)) +
                                   "=" + bits(gray_plane, plane_bits)
                             : "Karnaugh map"},
        {"row_variables", map.order.substr(static_cast<std::size_t>(plane_bits),
                                           static_cast<std::size_t>(row_bits))},
        {"column_variables",
         map.order.substr(static_cast<std::size_t>(plane_bits + row_bits))},
        {"columns", columns},
        {"rows", rows}});
  }
  Json::Array groups;
  for (std::size_t i = 0; i < map.groups.size(); i++)
    groups.push_back(group_json(validate_group(map.groups[i], map), map, i));
  return Json::Object{
      {"status", "success"},
      {"state", Json::Object{{"variables", map.variables},
                             {"cells", map.cells},
                             {"order", map.order},
                             {"groups", map.groups}}},
      {"panels", panels},
      {"groups", groups},
      {"input", canonical(map)},
      {"help", "Click a cell to cycle 0 → 1 → X. Row/column labels use Gray "
               "code. Groups may wrap across edges and combine matching cells "
               "across panels."}};
}
SolutionBundle extended_kmap(const ProblemSpec &request) {
  const auto map = request.input.starts_with('{')
                       ? read_map(Json::parse(request.input))
                       : from_text(request.input);
  Cover cover;
  if (!map.groups.empty()) {
    for (const auto &group : map.groups)
      cover.chosen.push_back(validate_group(group, map));
    cover.optimal = false;
  } else
    cover = minimize(map);
  std::string expression;
  Json::Array groups;
  for (std::size_t i = 0; i < cover.chosen.size(); i++) {
    if (!expression.empty())
      expression += " | ";
    expression += term(cover.chosen[i], map.variables);
    groups.push_back(group_json(cover.chosen[i], map, i));
  }
  if (expression.empty())
    expression = "0";
  for (unsigned cell = 0; cell < map.cells.size(); cell++)
    if (map.cells[cell] != 'X') {
      const bool actual = std::any_of(
          cover.chosen.begin(), cover.chosen.end(),
          [&](const auto &cube) { return (cell & ~cube.mask) == cube.value; });
      if (actual != (map.cells[cell] == '1'))
        fail("The groups do not cover every required 1, or cover a required 0");
    }
  SolutionBundle result;
  result.domain = "logic";
  result.topic = "kmap_minimization";
  result.answer = expression;
  result.steps.push_back(
      {"KMAP_ON_DC",
       "Treat 1 cells as required outputs, 0 cells as forbidden, and X cells "
       "as optional enlargement opportunities.",
       canonical(map)});
  for (std::size_t i = 0; i < cover.chosen.size(); i++)
    result.steps.push_back({"KMAP_GROUP",
                            "Group " + std::to_string(i + 1) +
                                ": eliminate every variable that changes "
                                "within this Boolean cube.",
                            groups[i].dump()});
  result.steps.push_back(
      {"KMAP_COVER",
       cover.optimal
           ? "Minimize product-term count, then literal count, using a bounded "
             "exhaustive cover search."
           : "Use the shown valid cover. A minimum-size proof is not claimed "
             "for manual groups or a search that reached its work limit.",
       expression});
  result.verification = {
      VerificationStatus::verified_exhaustive,
      "all specified truth-table rows replayed",
      std::to_string(map.cells.size()) +
          " input assignments checked; don't-care outputs unrestricted"};
  if (!cover.optimal)
    result.warnings.push_back(
        map.groups.empty()
            ? "Exact-cover work limit reached. The function is verified, but "
              "this cover is not certified minimum."
            : "Manual grouping is functionally checked, not certified "
              "minimum.");
  Json::Array cells;
  for (char cell : map.cells)
    cells.emplace_back(std::string(1, cell));
  result.visual_json = Json(Json::Object{{"kind", "kmap"},
                                         {"variables", map.variables},
                                         {"cells", cells},
                                         {"groups", groups},
                                         {"optimal", cover.optimal},
                                         {"search_states", cover.visits}})
                           .dump();
  return result;
}
} // namespace pocket_engineer::workbench
