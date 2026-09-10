#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
constexpr int grid_width = 24, grid_height = 18;
constexpr double spacing = 32, margin = 24;
struct Point {
  int x{}, y{};
  auto operator<=>(const Point &) const = default;
};
struct Part {
  std::string id, type, value;
  Point a, b;
};
struct Wire {
  Point a, b;
};
struct Schematic {
  std::vector<Part> parts;
  std::vector<Wire> wires;
  std::vector<Point> grounds;
};
[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(message);
}
Point point(const Json &value) {
  const auto &xy = value.array();
  if (xy.size() != 2)
    fail("A terminal needs an [x,y] grid coordinate");
  return {xy[0].integer(0, grid_width), xy[1].integer(0, grid_height)};
}
Json position(Point value) { return Json::Array{value.x, value.y}; }
bool on(Point p, const Wire &w) {
  return w.a.x == w.b.x ? p.x == w.a.x && p.y >= std::min(w.a.y, w.b.y) &&
                              p.y <= std::max(w.a.y, w.b.y)
                        : p.y == w.a.y && p.x >= std::min(w.a.x, w.b.x) &&
                              p.x <= std::max(w.a.x, w.b.x);
}
Part part(const Json &value) {
  value.only({"id", "type", "value", "x", "y", "rotation"});
  Part result;
  result.id = value.at("id").string();
  result.type = value.at("type").string();
  result.value = value.at("value").string();
  if (!identifier(result.id) || result.type.size() != 1 ||
      std::string("RCLVI").find(result.type) == std::string::npos)
    fail("Draw R, C, L, V or I components with a unique short identifier. "
         "Controlled sources can be entered in the advanced netlist.");
  if (result.value.empty() || result.value.size() > 40 ||
      result.value.find_first_not_of("0123456789.eE+-@pnumkKMGg") !=
          std::string::npos)
    fail("Use a numeric component value with an optional SI prefix and AC "
         "phase, such as 1k, 10u or 2@90");
  result.a = {value.at("x").integer(0, grid_width),
              value.at("y").integer(0, grid_height)};
  const int rotation =
      value.find("rotation") ? value.at("rotation").integer(0, 1) : 0;
  result.b = {result.a.x + (rotation ? 0 : 4), result.a.y + (rotation ? 4 : 0)};
  if (result.b.x > grid_width || result.b.y > grid_height)
    fail("The component would extend beyond the drawing grid");
  return result;
}
void validate(const Schematic &model) {
  if (model.parts.size() > 48 || model.wires.size() > 96 ||
      model.grounds.size() > 16)
    fail("Drawing budget: 48 components, 96 wire segments and 16 ground "
         "symbols");
  std::set<std::string> names;
  for (std::size_t i = 0; i < model.parts.size(); i++) {
    const auto &a = model.parts[i];
    if (!names.insert(a.id).second)
      fail("Duplicate component name: " + a.id);
    // Reserve the central symbol plus its value label; terminals may meet.
    const double ax = (a.a.x + a.b.x) * .5, ay = (a.a.y + a.b.y) * .5;
    for (std::size_t j = 0; j < i; j++) {
      const auto &b = model.parts[j];
      const double bx = (b.a.x + b.b.x) * .5, by = (b.a.y + b.b.y) * .5;
      const double aw = a.a.y == a.b.y ? 1.6 : 1.2,
                   ah = a.a.y == a.b.y ? 1.2 : 1.6;
      const double bw = b.a.y == b.b.y ? 1.6 : 1.2,
                   bh = b.a.y == b.b.y ? 1.2 : 1.6;
      if (std::abs(ax - bx) < aw + bw && std::abs(ay - by) < ah + bh)
        fail("Component symbols or labels would overlap. Leave more grid space "
             "between them.");
    }
  }
  for (const auto &w : model.wires)
    if (w.a == w.b || (w.a.x != w.b.x && w.a.y != w.b.y))
      fail("Wires must be nonzero horizontal or vertical segments");
}
Schematic read(const Json &value) {
  value.only({"components", "wires", "grounds"});
  Schematic out;
  for (const auto &p : value.at("components").array())
    out.parts.push_back(part(p));
  for (const auto &w : value.at("wires").array()) {
    w.only({"a", "b"});
    out.wires.push_back({point(w.at("a")), point(w.at("b"))});
  }
  for (const auto &g : value.at("grounds").array())
    out.grounds.push_back(point(g));
  validate(out);
  return out;
}
Json state(const Schematic &model) {
  Json::Array parts, wires, grounds;
  for (const auto &p : model.parts)
    parts.emplace_back(Json::Object{{"id", p.id},
                                    {"type", p.type},
                                    {"value", p.value},
                                    {"x", p.a.x},
                                    {"y", p.a.y},
                                    {"rotation", p.a.x == p.b.x ? 1 : 0}});
  for (const auto &w : model.wires)
    wires.emplace_back(
        Json::Object{{"a", position(w.a)}, {"b", position(w.b)}});
  for (const auto g : model.grounds)
    grounds.push_back(position(g));
  return Json::Object{
      {"components", parts}, {"wires", wires}, {"grounds", grounds}};
}
Schematic preset(const std::string &name) {
  if (name != "divider" && name != "rc" && name != "rlc" && name != "empty")
    fail("Unknown circuit preset");
  if (name == "empty")
    return {};
  Schematic model;
  model.parts = {{"V1", "V", "12", {4, 4}, {4, 8}},
                 {"R1", "R", "1k", {10, 4}, {14, 4}},
                 {"R2", "R", "2k", {20, 4}, {20, 8}}};
  if (name == "rc")
    model.parts[2] = {"C1", "C", "1u", {20, 4}, {20, 8}};
  if (name == "rlc") {
    model.parts[0].value = "1";
    model.parts[1].value = "100";
    model.parts[2] = {"C1", "C", "1u", {20, 4}, {20, 8}};
    model.parts.push_back({"L1", "L", "10m", {10, 12}, {14, 12}});
  }
  model.wires = {{{4, 4}, {10, 4}},
                 {{14, 4}, {20, 4}},
                 {{4, 8}, {4, 12}},
                 {{20, 8}, {20, 12}}};
  if (name == "rlc") {
    model.wires.push_back({{4, 12}, {10, 12}});
    model.wires.push_back({{14, 12}, {20, 12}});
  } else
    model.wires.push_back({{4, 12}, {20, 12}});
  model.grounds = {{4, 12}};
  validate(model);
  return model;
}
struct Topology {
  std::string netlist;
  std::map<Point, std::string> names;
  Json::Array nodes, warnings;
};
Topology topology(const Schematic &model) {
  std::map<Point, std::size_t> index;
  auto add = [&](Point p) {
    if (!index.contains(p))
      index[p] = index.size();
  };
  for (const auto &p : model.parts) {
    add(p.a);
    add(p.b);
  }
  for (const auto &w : model.wires) {
    add(w.a);
    add(w.b);
  }
  for (const auto g : model.grounds)
    add(g);
  std::vector<std::size_t> parent(index.size());
  std::iota(parent.begin(), parent.end(), 0);
  const auto root = [&](std::size_t i) {
    while (parent[i] != i) {
      parent[i] = parent[parent[i]];
      i = parent[i];
    }
    return i;
  };
  const auto join = [&](Point a, Point b) {
    parent[root(index.at(a))] = root(index.at(b));
  };
  // Only terminals, wire endpoints and explicit grounds create junctions.
  // Crossing wire interiors are NOT silently joined.
  for (const auto &wire : model.wires)
    for (const auto &[p, id] : index) {
      (void)id;
      if (on(p, wire))
        join(wire.a, p);
    }
  for (const auto g : model.grounds)
    join(model.grounds.front(), g);
  std::map<std::size_t, std::string> names;
  if (!model.grounds.empty())
    names[root(index.at(model.grounds.front()))] = "0";
  Topology result;
  int number = 0;
  for (const auto &[p, id] : index) {
    const auto group = root(id);
    if (!names.contains(group))
      names[group] = "n" + std::to_string(++number);
    result.names[p] = names.at(group);
    result.nodes.emplace_back(
        Json::Object{{"position", position(p)}, {"node", names.at(group)}});
  }
  if (model.grounds.empty())
    result.warnings.emplace_back("Add a ground symbol before solving. All "
                                 "ground symbols represent the same node 0.");
  std::map<std::string, int> terminals;
  for (const auto &p : model.parts) {
    terminals[result.names.at(p.a)]++;
    terminals[result.names.at(p.b)]++;
    if (!result.netlist.empty())
      result.netlist += ';';
    result.netlist += p.type + " " + p.id + " " + result.names.at(p.a) + " " +
                      result.names.at(p.b) + " " + p.value;
    if (result.names.at(p.a) == result.names.at(p.b))
      result.warnings.emplace_back(
          p.id + " has both terminals on the same node (shorted component).");
  }
  for (const auto &[name, count] : terminals)
    if (count == 1 && name != "0")
      result.warnings.emplace_back(
          "Node " + name +
          " has only one component terminal; check for a missing wire.");
  return result;
}
Json draw(const Schematic &model, const Topology &graph) {
  Drawing drawing(grid_width * spacing + 2 * margin,
                  grid_height * spacing + 2 * margin);
  const auto px = [](int value) { return margin + value * spacing; };
  for (int y = 0; y <= grid_height; y++)
    for (int x = 0; x <= grid_width; x++)
      drawing.circle(px(x), px(y), 1, "#dce4df", "#dce4df");
  for (const auto &w : model.wires)
    drawing.line(px(w.a.x), px(w.a.y), px(w.b.x), px(w.b.y));
  for (const auto &p : model.parts) {
    const bool vertical = p.a.x == p.b.x;
    const auto xy = [&](double along, double across) {
      return vertical ? std::pair{px(p.a.x) + across, px(p.a.y) + along}
                      : std::pair{px(p.a.x) + along, px(p.a.y) + across};
    };
    const auto line = [&](double a, double b, double c, double d) {
      const auto start = xy(a, b), end = xy(c, d);
      drawing.line(start.first, start.second, end.first, end.second);
    };
    const auto middle = xy(64, 0);
    line(0, 0, 40, 0);
    line(88, 0, 128, 0);
    if (p.type == "R") {
      double previous = 40, offset = 0;
      for (int i = 0; i < 8; i++) {
        const double next = 43 + i * 6., height = i % 2 ? -9. : 9.;
        line(previous, offset, next, height);
        previous = next;
        offset = height;
      }
      line(previous, offset, 88, 0);
    } else if (p.type == "C") {
      line(40, 0, 58, 0);
      line(70, 0, 88, 0);
      line(58, -18, 58, 18);
      line(70, -18, 70, 18);
    } else if (p.type == "L") {
      for (int i = 0; i < 4; i++) {
        const auto a = xy(40 + i * 12., 0), b = xy(40 + i * 12., -18),
                   c = xy(52 + i * 12., -18), d = xy(52 + i * 12., 0);
        drawing.curve(a.first, a.second, b.first, b.second, c.first, c.second,
                      d.first, d.second);
      }
    } else {
      drawing.circle(middle.first, middle.second, 24);
      if (p.type == "V") {
        const auto a = xy(53, 0), b = xy(77, 0);
        drawing.text(a.first, a.second, "+", 15);
        drawing.text(b.first, b.second, "−", 15);
      } else {
        const auto a = xy(50, 0), b = xy(78, 0);
        drawing.arrow(a.first, a.second, b.first, b.second);
      }
    }
    const auto label = xy(64, vertical ? -42 : -32);
    drawing.text(label.first, label.second, p.id, 12);
    const auto value = xy(64, vertical ? 42 : 32);
    drawing.text(value.first, value.second, p.value, 12);
    drawing.circle(px(p.a.x), px(p.a.y), 3, "#234b40");
    drawing.circle(px(p.b.x), px(p.b.y), 3, "#234b40");
  }
  for (const auto &[p, name] : graph.names) {
    drawing.circle(px(p.x), px(p.y), 3, "#234b40");
    drawing.text(px(p.x) + 7, px(p.y) - 9, name, 10, "left");
  }
  for (const auto p : model.grounds) {
    const auto x = px(p.x), y = px(p.y);
    drawing.line(x, y, x, y + 9);
    drawing.line(x - 12, y + 9, x + 12, y + 9);
    drawing.line(x - 8, y + 14, x + 8, y + 14);
    drawing.line(x - 3, y + 19, x + 3, y + 19);
  }
  return drawing.json(
      "Circuit schematic. Dots are connected junctions; wire crossings without "
      "a dot are not connected. Source polarity and current arrows follow "
      "terminal one to terminal two.");
}
} // namespace
Json circuit_editor(const ProblemSpec &request) {
  auto model = request.input.empty() ? preset("divider")
                                     : read(Json::parse(request.input));
  if (!request.payload.empty()) {
    const auto action = Json::parse(request.payload);
    action.only(
        {"action", "preset", "component", "id", "a", "b", "position", "wire"});
    const auto name = action.at("action").string();
    if (name == "preset")
      model = preset(action.at("preset").string());
    else if (name == "place")
      model.parts.push_back(part(action.at("component")));
    else if (name == "replace") {
      const auto replacement = part(action.at("component"));
      const auto id = action.at("id").string();
      auto found = std::find_if(model.parts.begin(), model.parts.end(),
                                [&](const auto &p) { return p.id == id; });
      if (found == model.parts.end())
        fail("Select an existing component to edit");
      *found = replacement;
    } else if (name == "delete") {
      const auto id = action.at("id").string();
      const auto count =
          std::erase_if(model.parts, [&](const auto &p) { return p.id == id; });
      if (count == 0)
        fail("No component with that name");
    } else if (name == "wire") {
      const auto a = point(action.at("a")), b = point(action.at("b"));
      if (a == b)
        fail("Choose two different wire endpoints");
      const Point bend{b.x, a.y};
      if (a != bend)
        model.wires.push_back({a, bend});
      if (bend != b)
        model.wires.push_back({bend, b});
    } else if (name == "delete_wire") {
      const auto i = action.at("wire").integer(
          0, static_cast<int>(model.wires.size()) - 1);
      model.wires.erase(model.wires.begin() + i);
    } else if (name == "ground") {
      const auto p = point(action.at("position"));
      const auto found =
          std::find(model.grounds.begin(), model.grounds.end(), p);
      if (found == model.grounds.end())
        model.grounds.push_back(p);
      else
        model.grounds.erase(found);
    } else
      fail("Unknown circuit editing action");
  }
  validate(model);
  const auto graph = topology(model);
  return Json::Object{{"status", "success"},
                      {"state", state(model)},
                      {"drawing", draw(model, graph)},
                      {"input", graph.netlist},
                      {"nodes", graph.nodes},
                      {"warnings", graph.warnings},
                      {"grid", Json::Object{{"width", grid_width},
                                            {"height", grid_height},
                                            {"spacing", spacing},
                                            {"margin", margin}}}};
}
} // namespace pocket_engineer::workbench
