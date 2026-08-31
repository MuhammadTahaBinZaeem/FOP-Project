#include "pocket_engineer/engine.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Topic { std::string domain; std::string id; };
constexpr std::array difficulties{"easy", "medium", "hard"};
const std::vector<Topic> topics{
    {"algebra","numeric_evaluation"}, {"algebra","simplification"}, {"algebra","factorisation"}, {"algebra","linear_equations"}, {"algebra","quadratic_equations"}, {"algebra","trigonometry"}, {"algebra","logarithms"},
    {"calculus","differentiation"}, {"calculus","tangent_line"}, {"calculus","integration"}, {"calculus","definite_integral"}, {"calculus","limits"}, {"calculus","curve_analysis"},
    {"linear_algebra","rref"}, {"linear_algebra","determinant"}, {"linear_algebra","inverse"}, {"linear_algebra","linear_system"}, {"linear_algebra","multiply"}, {"linear_algebra","transpose"}, {"linear_algebra","rank"}, {"linear_algebra","eigenvalues"}, {"linear_algebra","vectors"},
    {"differential_equations","separable"}, {"differential_equations","first_order_linear"}, {"differential_equations","exact"}, {"differential_equations","bernoulli"}, {"differential_equations","homogeneous"}, {"differential_equations","second_order_constant_coefficient"}, {"differential_equations","initial_value"}, {"differential_equations","euler"}, {"differential_equations","rk4"},
    {"logic","number_systems"}, {"logic","signed_arithmetic"}, {"logic","truth_table"}, {"logic","canonical_pos"}, {"logic","kmap"}, {"logic","combinational"}, {"logic","sequential"},
    {"circuit","voltage_divider"}, {"circuit","dc_nodal"}, {"circuit","mesh"}, {"circuit","source_transform"}, {"circuit","superposition"}, {"circuit","thevenin"}, {"circuit","norton"}, {"circuit","maximum_power"}, {"circuit","rc_transient"}, {"circuit","rl_transient"},
    {"programming","cpp_trace"}, {"programming","branches"}, {"programming","loops"}, {"programming","arrays"}, {"programming","functions"}, {"programming","recursion"}, {"engineering","unit_conversion"}
};

std::string esc(const std::string& value) {
    std::string out;
    for(char c:value) {
        if(c=='"'||c=='\\') out+='\\';
        if(c=='\n') out+="\\n";
        else if(c=='\r') out+="\\r";
        else if(c=='\t') out+="\\t";
        else out+=c;
    }
    return out;
}

std::string problem_for(const Topic& t, std::mt19937_64& rng, int difficulty) {
    const int span=10*(difficulty+1);
    std::uniform_int_distribution<int> n(1,span);
    const int a=n(rng),b=n(rng),c=n(rng);
    if(t.id=="numeric_evaluation") return std::to_string(a)+"*("+std::to_string(b)+"+"+std::to_string(c)+")";
    if(t.id=="simplification") return "("+std::to_string(a)+"x^2-"+std::to_string(a)+")/(x-1)";
    if(t.id=="factorisation") return "factor x^2-"+std::to_string(a+b)+"x+"+std::to_string(a*b);
    if(t.id=="linear_equations") return std::to_string(a)+"x + "+std::to_string(b)+" = "+std::to_string(c);
    if(t.id=="quadratic_equations") return "x^2 - "+std::to_string(a+b)+"x + "+std::to_string(a*b)+" = 0";
    if(t.id=="trigonometry") return "sin(x)^2 + cos(x)^2";
    if(t.id=="logarithms") return "log(100)";
    if(t.id=="differentiation") return std::to_string(a)+"x^"+std::to_string(difficulty+2);
    if(t.id=="tangent_line") return std::to_string(a)+"x^2;at="+std::to_string(b);
    if(t.id=="integration") return "integrate "+std::to_string(a)+"x^"+std::to_string(difficulty+1)+" dx";
    if(t.id=="definite_integral") return "integrate "+std::to_string(a)+"x^2 from 0 to "+std::to_string(difficulty+2);
    if(t.id=="limits") { const int point=a+difficulty;return "limit (x^2-"+std::to_string(point*point)+")/(x-"+std::to_string(point)+") as x -> "+std::to_string(point); }
    if(t.id=="curve_analysis") return std::to_string(a)+"x^"+std::to_string(difficulty+2);
    if(t.id=="rref") return std::to_string(a)+","+std::to_string(b)+","+std::to_string(a+b)+";"+std::to_string(b)+","+std::to_string(c)+","+std::to_string(b+c);
    if(t.id=="determinant") return std::to_string(a)+","+std::to_string(b)+";"+std::to_string(c)+","+std::to_string(a);
    if(t.id=="inverse") return "1,2;3,5";
    if(t.id=="linear_system") return "1,1,"+std::to_string(a+b)+";1,-1,"+std::to_string(a-b);
    if(t.id=="multiply") return "1,2;3,4|2,0;1,2";
    if(t.id=="transpose") return std::to_string(a)+","+std::to_string(b)+";"+std::to_string(c)+","+std::to_string(a);
    if(t.id=="rank") return std::to_string(a)+","+std::to_string(b)+";"+std::to_string(2*a)+","+std::to_string(2*b);
    if(t.id=="eigenvalues") return std::to_string(a)+",0;0,"+std::to_string(b);
    if(t.id=="vectors") return "dot:"+std::to_string(a)+","+std::to_string(b)+","+std::to_string(c)+"|"+std::to_string(c)+","+std::to_string(a)+","+std::to_string(b);
    if(t.id=="separable") return "dy/dx = "+std::to_string(a)+"*x*y";
    if(t.id=="first_order_linear") return "dy/dx + "+std::to_string(a)+"*y = "+std::to_string(b)+"*x";
    if(t.id=="exact") return "exact "+std::to_string(a)+" "+std::to_string(b)+" "+std::to_string(c);
    if(t.id=="bernoulli") return "bernoulli "+std::to_string(a)+" "+std::to_string(b)+" 2";
    if(t.id=="homogeneous") return "homogeneous "+std::to_string(a);
    if(t.id=="second_order_constant_coefficient") return "y'' + "+std::to_string(a+b)+"*y' + "+std::to_string(a*b)+"*y = 0";
    if(t.id=="initial_value") return std::to_string(a)+" "+std::to_string(b)+" "+std::to_string(difficulty+1);
    if(t.id=="euler"||t.id=="rk4") return std::to_string(a)+" "+std::to_string(b)+" 0.1 "+std::to_string(difficulty+2);
    if(t.id=="number_systems") return std::to_string(a*16+b)+" decimal binary";
    if(t.id=="signed_arithmetic") return "twos_add 8 "+std::string(a%2?"01111111":"11111111")+" "+std::string(b%2?"00000001":"00000001");
    if(t.id=="truth_table"||t.id=="canonical_pos") return "A & !B | C";
    if(t.id=="kmap") return "vars=3; minterms="+std::to_string(a%8)+","+std::to_string(b%8)+","+std::to_string(c%8);
    if(t.id=="combinational") return "full_adder "+std::to_string(a%2)+" "+std::to_string(b%2)+" "+std::to_string(c%2);
    if(t.id=="sequential") return "jkff "+std::to_string(a%2)+" "+std::to_string(b%2)+" "+std::to_string(c%2);
    if(t.id=="voltage_divider") return std::to_string(a+5)+" "+std::to_string(b*100)+" "+std::to_string(c*100);
    if(t.id=="dc_nodal") return "V V1 vin 0 "+std::to_string(a+5)+"; R R1 vin vout "+std::to_string(b*100)+"; R R2 vout 0 "+std::to_string(c*100);
    if(t.id=="mesh") return "mesh "+std::to_string(a+5)+" "+std::to_string(b+3)+" "+std::to_string(b*100)+" "+std::to_string(c*100)+" "+std::to_string((a%5+1)*100);
    if(t.id=="source_transform") return "thevenin "+std::to_string(a+5)+" "+std::to_string(b*100);
    if(t.id=="superposition") return "superposition "+std::to_string(a+5)+" "+std::to_string(b*100)+" "+std::to_string(c+3)+" "+std::to_string(a*100)+" "+std::to_string((b+c)*100);
    if(t.id=="thevenin") return "thevenin "+std::to_string(a+5)+" "+std::to_string(b*100)+" "+std::to_string(c*100);
    if(t.id=="norton") return "norton 0."+std::to_string(a%9+1)+" "+std::to_string(b*100)+" "+std::to_string(c*100);
    if(t.id=="maximum_power") return "maximum_power "+std::to_string(a+5)+" "+std::to_string(b*100);
    if(t.id=="rc_transient") return "RC "+std::to_string(a+5)+" "+std::to_string(b*100)+" 0.000001 0.001";
    if(t.id=="rl_transient") return "RL "+std::to_string(a+5)+" "+std::to_string(b*100)+" 0.001 0.001";
    if(t.id=="cpp_trace") return "int x="+std::to_string(a)+"; x += "+std::to_string(b)+";";
    if(t.id=="branches") return "int x="+std::to_string(a)+"; if (x > "+std::to_string(b)+") x += "+std::to_string(c)+"; else x += "+std::to_string(-c)+";";
    if(t.id=="loops") return "int sum=0; for (int i=1; i<="+std::to_string(a+difficulty)+"; ++i) sum += i;";
    if(t.id=="arrays") return "sum ["+std::to_string(a)+","+std::to_string(b)+","+std::to_string(c)+"]";
    if(t.id=="functions") return "int f(int x){ return x*"+std::to_string(a)+"; } f("+std::to_string(b)+")";
    if(t.id=="recursion") return "fact("+std::to_string(a%10)+")";
    return std::to_string(a)+" kOhm Ohm";
}
}

int main(int argc, char** argv) {
    const std::filesystem::path output=argc>1?argv[1]:"test-data";
    const int per_difficulty=argc>2?std::stoi(argv[2]):5000;
    if(per_difficulty<1){std::cerr<<"count must be positive\n";return 2;}
    std::filesystem::create_directories(output);
    std::ofstream manifest(output/"manifest.json");
    manifest<<"{\n  \"schema_version\": \"2.0\",\n  \"per_topic_per_difficulty\": "<<per_difficulty<<",\n  \"difficulty_levels\": [\"easy\", \"medium\", \"hard\"],\n  \"oracle\": \"native Pocket Engineer solver plus verifier\",\n  \"topics\": [\n";
    pocket_engineer::Engine engine;
    std::uint64_t total{};
    for(std::size_t t=0;t<topics.size();++t) {
        const auto& topic=topics[t];
        const auto directory=output/topic.domain/topic.id;
        std::filesystem::create_directories(directory);
        for(std::size_t d=0;d<difficulties.size();++d) {
            std::ofstream file(directory/(std::string(difficulties[d])+".jsonl"));
            std::mt19937_64 rng(0x50450000ULL+(t*1000)+(d*100));
            for(int i=0;i<per_difficulty;++i) {
                const auto input=problem_for(topic,rng,static_cast<int>(d));
                const auto result=engine.solve({topic.domain,topic.id,input,{}});
                if(result.status!="success" || result.verification.status==pocket_engineer::VerificationStatus::not_verified || result.verification.status==pocket_engineer::VerificationStatus::verification_failed) {
                    throw std::runtime_error("Fixture has no verified native result: "+topic.domain+"/"+topic.id+" input="+input+" error="+result.answer);
                }
                file<<"{\"id\":\""<<topic.domain<<'.'<<topic.id<<'.'<<difficulties[d]<<'.'<<i
                    <<"\",\"domain\":\""<<topic.domain<<"\",\"topic\":\""<<topic.id<<"\",\"difficulty\":\""<<difficulties[d]
                    <<"\",\"source\":\"deterministic_native_oracle\",\"input\":\""<<esc(input)
                    <<"\",\"expected_answer\":\""<<esc(result.answer)
                    <<"\",\"expected_verification\":\""<<pocket_engineer::verification_name(result.verification.status)<<"\"}\n";
            }
            total+=static_cast<std::uint64_t>(per_difficulty);
        }
        manifest<<"    {\"domain\": \""<<topic.domain<<"\", \"topic\": \""<<topic.id<<"\", \"solver_backed\": true}"<<(t+1==topics.size()?"\n":",\n");
    }
    manifest<<"  ],\n  \"total_cases\": "<<total<<",\n  \"generator\": \"pe-generate-tests\"\n}\n";
    std::cout<<"Generated "<<total<<" solver-backed deterministic cases in "<<output<<" ("<<per_difficulty<<" per topic × difficulty).\n";
}
