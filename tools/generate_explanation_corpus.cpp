#include "pocket_engineer/engine.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct Topic {
    std::string domain;
    std::string id;
    std::string method;
    std::string check;
};

const std::vector<Topic> topics{
    {"algebra","numeric_evaluation","evaluate with precedence","independently recompute the expression"},
    {"algebra","simplification","factor before cancelling","compare domains before and after cancellation"},
    {"algebra","factorisation","find roots and rebuild factors","expand factors back to the original polynomial"},
    {"algebra","linear_equations","isolate the variable","substitute the proposed value"},
    {"algebra","quadratic_equations","use the discriminant and quadratic formula","check Vieta relations and substitute roots"},
    {"algebra","trigonometry","apply a named identity","evaluate identity equivalence"},
    {"algebra","logarithms","respect the logarithm base and domain","independently evaluate the numeric expression"},
    {"calculus","differentiation","apply the derivative rule","differentiate term by term"},
    {"calculus","tangent_line","differentiate, then use point-slope form","check the point and slope"},
    {"calculus","integration","reverse the derivative rule","differentiate the antiderivative"},
    {"calculus","definite_integral","use an antiderivative and bounds","evaluate upper minus lower bound"},
    {"calculus","limits","factor the removable form before substitution","check the excluded point and simplified limit"},
    {"calculus","curve_analysis","find and classify critical points","inspect derivative signs"},
    {"linear_algebra","rref","apply elementary row operations","replay row operations"},
    {"linear_algebra","determinant","use stable elimination","check pivots and row-swap signs"},
    {"linear_algebra","inverse","use Gauss-Jordan elimination","multiply A by the candidate inverse"},
    {"linear_algebra","linear_system","reduce the augmented matrix","evaluate the Ax=b residual"},
    {"linear_algebra","multiply","take row-column dot products","check dimensions and entries"},
    {"linear_algebra","transpose","swap row and column indices","transpose twice"},
    {"linear_algebra","rank","count non-zero RREF rows","recompute the reduced form"},
    {"linear_algebra","eigenvalues","solve the characteristic polynomial","compare trace and determinant invariants"},
    {"linear_algebra","vectors","use component-wise vector rules","check symmetry or orthogonality"},
    {"differential_equations","separable","separate variables and integrate","differentiate and substitute"},
    {"differential_equations","first_order_linear","use an integrating factor","substitute the candidate function"},
    {"differential_equations","exact","recover a potential function","compare mixed partial derivatives"},
    {"differential_equations","bernoulli","linearize with the Bernoulli substitution","replay the substitution"},
    {"differential_equations","homogeneous","substitute a homogeneous ratio","differentiate the family"},
    {"differential_equations","second_order_constant_coefficient","solve the characteristic equation","substitute the basis solutions"},
    {"differential_equations","initial_value","apply the initial condition after solving","check both ODE and initial value"},
    {"differential_equations","euler","advance one bounded Euler step at a time","compare with the reference solution"},
    {"differential_equations","rk4","calculate four RK slopes per step","compare with the reference solution"},
    {"logic","number_systems","convert through positional notation","round-trip to the source base"},
    {"logic","signed_arithmetic","add fixed-width two's-complement patterns","decode the result and test overflow"},
    {"logic","truth_table","enumerate all input rows","check every Boolean assignment"},
    {"logic","canonical_pos","enumerate zero-output rows","check the full truth table"},
    {"logic","kmap","group valid adjacent minterms","compare the result on every K-map row"},
    {"logic","combinational","apply the component truth table","enumerate component inputs"},
    {"logic","sequential","apply the characteristic table at the clock edge","check state transitions"},
    {"circuit","voltage_divider","combine series resistance and Ohm's law","evaluate the KVL residual"},
    {"circuit","dc_nodal","stamp and solve MNA equations","evaluate the KCL residual"},
    {"circuit","mesh","write one KVL equation per mesh","evaluate mesh KVL residuals"},
    {"circuit","source_transform","preserve terminal equivalence","compare open-circuit and short-circuit behavior"},
    {"circuit","superposition","activate one independent source at a time","sum and replay the nodal result"},
    {"circuit","thevenin","solve the equivalent series network","evaluate equivalent-network KVL"},
    {"circuit","norton","solve current division in the parallel network","evaluate KCL"},
    {"circuit","maximum_power","match the load to the Thevenin resistance","check the stationary-power condition"},
    {"circuit","rc_transient","compute the time constant and step response","check initial and final values"},
    {"circuit","rl_transient","compute the time constant and step response","check initial and final values"},
    {"programming","cpp_trace","execute the restricted statements in order","replay each assignment"},
    {"programming","branches","evaluate the condition before one branch","check the selected branch only"},
    {"programming","loops","state bounds and accumulate each iteration","compare with an independent closed form"},
    {"programming","arrays","visit every bounded element once","independently accumulate values"},
    {"programming","functions","bind the parameter before evaluating return","re-evaluate the return expression"},
    {"programming","recursion","state the base case and unwind calls","compare with an iterative calculation"},
    {"engineering","unit_conversion","convert through the SI base unit","replay the scale factor"}
};

constexpr std::array difficulties{"easy","medium","hard"};
constexpr std::array lead_ins{
    "Start by identifying the confirmed problem type.",
    "Before calculating, rewrite the givens in the solver's structured form.",
    "Keep the unknown, units, variable order, and restrictions visible.",
    "State the governing rule before substituting any values.",
    "Use one transformation per explanation step so the reasoning is auditable.",
    "Do not infer missing circuit, matrix, or initial-condition data.",
    "Name the mathematical object being transformed before changing it.",
    "Keep exact symbolic structure until a numerical evaluation is required."
};

constexpr std::array endings{
    "Mention any domain restriction or unit assumption explicitly.",
    "Do not hide a cancellation, sign change, row operation, or state update.",
    "Use the same variable order in the explanation and verification.",
    "Present intermediate values with enough precision to reproduce the check.",
    "Distinguish the solving step from the independent verification step.",
    "If the structured form is incomplete, ask for the missing data instead of guessing.",
    "Keep the final answer separate from the method used to verify it.",
    "Explain why the selected rule applies before reporting its result."
};

std::string escape(const std::string& value) {
    return pocket_engineer::json_escape(value);
}

std::string step_name(std::size_t phase) {
    constexpr std::array names{"interpret","prepare","transform","present","verify"};
    return names[phase];
}

std::string text_for(const Topic& topic,std::string_view difficulty,std::size_t index) {
    const auto phase=index%5;
    const auto lead=lead_ins[index%lead_ins.size()];
    const auto ending=endings[(index/5)%endings.size()];
    const std::string scope=topic.domain+"/"+topic.id;
    if(phase==0) return std::string(lead)+" This is a "+std::string(difficulty)+" "+scope+" problem. Confirm the topic and preserve the original input.";
    if(phase==1) return "List the known values, requested quantity, and constraints for "+scope+". "+ending;
    if(phase==2) return "For "+scope+", apply the method: "+topic.method+". Show the resulting expression or state after each legal transformation.";
    if(phase==3) return "Present the intermediate result for "+scope+" in a readable form, then connect it to the requested answer. "+ending;
    return "Verify the "+scope+" result by "+topic.check+". Report whether the independent check agrees with the proposed answer.";
}
}

int main(int argc,char** argv) {
    const std::filesystem::path output=argc>1?argv[1]:"explanation-data";
    const int per_topic=argc>2?std::stoi(argv[2]):10000;
    if(per_topic<1) {
        std::cerr<<"lines per topic must be positive\n";
        return 2;
    }
    std::filesystem::create_directories(output);
    std::ofstream manifest(output/"manifest.json");
    manifest<<"{\n  \"schema_version\": \"1.0\",\n  \"kind\": \"generic_step_explanation_training_text\",\n  \"topics\": "<<topics.size()<<",\n  \"lines_per_topic\": "<<per_topic<<",\n  \"difficulty_distribution\": \"round_robin_easy_medium_hard\",\n";
    std::uint64_t total{};
    for(const auto& topic:topics) {
        const auto path=output/(topic.domain+"__"+topic.id+".jsonl");
        std::ofstream file(path);
        for(int i=0;i<per_topic;++i) {
            const auto difficulty=difficulties[static_cast<std::size_t>(i)%difficulties.size()];
            const auto phase=static_cast<std::size_t>(i)%5;
            const auto text=text_for(topic,difficulty,static_cast<std::size_t>(i));
            file<<"{\"id\":\""<<topic.domain<<"."<<topic.id<<"."<<difficulty<<"."<<i<<"\",\"domain\":\""<<topic.domain<<"\",\"topic\":\""<<topic.id<<"\",\"difficulty\":\""<<difficulty<<"\",\"phase\":\""<<step_name(phase)<<"\",\"source\":\"deterministic_generic_template\",\"text\":\""<<escape(text)<<"\"}\n";
            ++total;
        }
    }
    manifest<<"  \"total_lines\": "<<total<<",\n  \"generator\": \"pe-generate-explanations\"\n}\n";
    std::cout<<"Generated "<<total<<" generic step-by-step explanation text rows in "<<output<<".\n";
}
