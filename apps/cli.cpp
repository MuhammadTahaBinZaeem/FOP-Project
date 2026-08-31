#include "pocket_engineer/engine.hpp"

#include <iostream>
#include <sstream>

using pocket_engineer::Engine;
using pocket_engineer::ProblemSpec;

namespace {
void usage() {
    std::cout << "Pocket Engineer — verified offline solver\n\n"
              << "Usage:\n"
              << "  pocket-engineer identify <question>\n"
              << "  pocket-engineer solve <domain> <topic> <input>\n"
              << "  pocket-engineer capabilities\n\n"
              << "Examples:\n"
              << "  pocket-engineer solve algebra simplify '(x^2 - 1)/(x - 1)'\n"
              << "  pocket-engineer solve linear_algebra rref '1,2,5;3,4,11'\n"
              << "  pocket-engineer solve logic truth_table 'A & !B | C'\n"
              << "  pocket-engineer solve circuit voltage_divider '12 1000 2000'\n";
}
}

int main(int argc, char** argv) {
    Engine engine;
    if (argc == 2 && std::string_view(argv[1]) == "capabilities") { std::cout << engine.capabilities_json() << '\n'; return 0; }
    if (argc >= 3 && std::string_view(argv[1]) == "identify") { std::ostringstream raw; for (int i=2;i<argc;++i) { if(i>2) raw << ' '; raw << argv[i]; } std::cout << engine.identify(raw.str()).to_json() << '\n'; return 0; }
    if (argc < 5 || std::string_view(argv[1]) != "solve") { usage(); return argc == 1 ? 0 : 2; }
    std::ostringstream input;
    for (int i=4; i<argc; ++i) { if (i > 4) input << ' '; input << argv[i]; }
    const auto solution=engine.solve({argv[2], argv[3], input.str()});
    std::cout << solution.to_json() << '\n';
    return solution.status == "success" ? 0 : 1;
}
