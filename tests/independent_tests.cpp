#include "pocket_engineer/engine.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace pocket_engineer;
namespace {
std::uint64_t checks=0,failures=0;
Engine engine;
std::vector<double> times;
void check(bool condition,const std::string& name) {
    ++checks;
    if(!condition){++failures;if(failures<30)std::cerr<<"FAIL: "<<name<<'\n';}
}
SolutionBundle solve(std::string domain,std::string topic,std::string input) {
    const auto start=std::chrono::steady_clock::now();
    auto result=engine.solve({std::move(domain),std::move(topic),std::move(input)});
    times.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count());
    return result;
}
bool close(double actual,double expected,double tolerance=1e-9) {
    return std::isfinite(actual)&&std::abs(actual-expected)<=tolerance*std::max(1.0,std::abs(expected));
}
double rhs(const std::string& answer) {
    const auto at=answer.find('=');
    try{return std::stod(at==std::string::npos?answer:answer.substr(at+1));}
    catch(...){return std::numeric_limits<double>::quiet_NaN();}
}
// Intentionally independent of the core tokenizer and Boolean evaluator. K-map
// output contains only OR-separated AND terms with named literals or constants.
bool dnf(const std::string& expression,int assignment,int variables) {
    std::stringstream terms(expression);std::string term;
    while(std::getline(terms,term,'|')) {
        bool value=true,negate=false;
        for(char c:term) {
            if(c=='!')negate=true;
            else if(c>='A'&&c<'A'+variables) {
                const bool bit=(assignment&(1<<(variables-1-(c-'A'))))!=0;
                value=value&&(negate?!bit:bit);negate=false;
            }else if(c=='0')value=false;
        }
        if(value)return true;
    }
    return false;
}
std::string minterms(unsigned mask,int variables) {
    std::string text;for(int i=0;i<(1<<variables);++i)if(mask&(1u<<i)){if(!text.empty())text+=',';text+=std::to_string(i);}return text;
}
std::vector<double> matrix_numbers(const std::string& text) {
    std::vector<double> numbers;const char* at=text.c_str();
    while(*at){char* end=nullptr;const double value=std::strtod(at,&end);if(end!=at){numbers.push_back(value);at=end;}else ++at;}
    return numbers;
}
void regression_edges() {
    const std::pair<const char*,double> expressions[]={{"-2^2",-4},{"2^-3",0.125},{"2^3^2",512},{"(-2)^2",4},{"1.234e20",1.234e20},{"1.234e-20",1.234e-20},{"1/1e-16",1e16}};
    for(auto [input,expected]:expressions){const auto s=solve("algebra","numeric_evaluation",input);check(s.status=="success"&&close(rhs(s.answer),expected,1e-12),std::string("precedence/scientific: ")+input);if(expected!=0)check(close(rhs(s.answer)/expected,1),"small nonzero numbers preserved");}
    for(const auto* input:{"0/0","0^0","sqrt(-1)","log(0)","ln(-3)","10^10000","1+","2**3"})check(solve("algebra","numeric_evaluation",input).status=="error",std::string("invalid arithmetic: ")+input);
    check(solve("algebra","numeric_evaluation",std::string(200,'(')+"1"+std::string(200,')')).status=="error","bounded expression depth");
    check(solve("algebra","numeric_evaluation",std::string(5000,'1')).status=="error","bounded input bytes");
    check(solve("unknown","unknown","2+2").status=="error","unknown domain rejected");
    check(solve("linear_algebra","linear_system","1,1,2;2,2,5").answer=="No solution (inconsistent system)","inconsistent system");
    check(solve("linear_algebra","linear_system","1,1,2;2,2,4").answer.find("Infinitely many")!=std::string::npos,"free variables not false unique solution");
    check(solve("linear_algebra","linear_system","1,0,2;0,1,3;1,1,5").answer=="x1 = 2, x2 = 3","overdetermined consistent system");
    check(solve("linear_algebra","inverse","1,2;2,4").status=="error","singular inverse rejected");
    std::string oversized="1";for(int i=0;i<17;++i)oversized+=";1";
    check(solve("linear_algebra","rref",oversized).status=="error","matrix row budget");
    check(solve("logic","kmap","vars=2; minterms=0; dc=1,2").status=="success","don't-care equivalent-cover tie");
    check(solve("logic","kmap","vars=2; minterms=0; dc=0").status=="error","overlapping don't-cares rejected");
    check(solve("logic","kmap","vars=2; minterms=1junk").status=="error","malformed minterm rejected");
    check(solve("logic","number_systems","18446744073709551616 decimal hex").status=="error","unsigned base conversion overflow");
    check(solve("logic","number_systems","FFFFFFFFFFFFFFFF hex decimal").answer.find("18446744073709551615")!=std::string::npos,"uint64 maximum preserved");
    check(solve("engineering","unit_conversion","2 V Ohm").status=="error","dimension mismatch rejected");
    check(close(rhs(solve("engineering","unit_conversion","2 mOhm Ohm").answer),0.002),"milli is not mega");
    check(close(rhs(solve("engineering","unit_conversion","2 MOhm Ohm").answer),2000000),"mega prefix");
    check(solve("differential_equations","rk4","1 1 0.1 2147483647").status=="error","ODE iteration cap");
    check(solve("differential_equations","euler","1 1 1 10").verification.status==VerificationStatus::not_verified,"large discretization error not labelled verified");
    check(solve("programming","cpp_trace","int x=2147483647; x += 1;").status=="error","signed addition overflow");
    check(solve("programming","functions","int f(int x){ return x*2147483647; } f(2)").status=="error","signed multiplication overflow");
    const auto parsed=parse_request(R"({"domain":"algebra","topic":"numeric_evaluation","input":"2+\n3"})");
    check(parsed.input=="2+\n3","JSON newline unescaping");
    check(parse_request(R"({"input":"\uD83D\uDE80"})").input=="\xF0\x9F\x9A\x80","Unicode surrogate pair");
    for(const auto* invalid:{R"({"input":"x","input":"y"})",R"({"input":"\uD800"})",R"({"input":"\u0000"})",R"({"input":"\q"})",R"({"input":1})",R"({"input":"2",})",R"({"input":"2"}junk)",R"({"x":"2"})"}){
        const char* result=pe_solve_json(invalid);check(result&&std::string(result).find("\"status\":\"error\"")!=std::string::npos,"strict JSON rejects malformed request");pe_free_string(result);
    }
    check(json_escape(std::string(1,'\x01'))=="\\u0001","JSON control escaping");
    SolveOptions options;options.max_steps=1;
    auto bounded=engine.solve({"linear_algebra","rref","1,2;3,4"},options);
    check(bounded.steps.size()==1&&!bounded.warnings.empty(),"step display budget enforced");
}
void catalog_examples() {
    check(topic_catalog().size()==55,"55 catalog entries");
    for(const auto& topic:topic_catalog()) {
        const auto result=solve(std::string(topic.domain),std::string(topic.topic),std::string(topic.example));
        check(result.status=="success",std::string("runnable catalog example: ")+std::string(topic.topic)+" -> "+result.answer);
        check(!result.steps.empty(),std::string("steps exist: ")+std::string(topic.topic));
    }
}
void polynomial_cases() {
    const std::pair<const char*,const char*> derivatives[]={{"3x^4-2x+7","12x^3 - 2"},{"(x+1)(x-1)","2x"},{"(x+1)^3","3x^2 + 6x + 3"},{"7","0"},{"0","0"},{"-x^2","-2x"},{"x/2","0.5"},{"x^0","0"}};
    for(auto [input,answer]:derivatives)check(solve("calculus","differentiation",input).answer==answer,std::string("polynomial derivative: ")+input);
    check(solve("calculus","integration","integrate 3x^2-2x+7 dx").answer=="x^3 - x^2 + 7x + C","polynomial antiderivative");
    check(solve("calculus","definite_integral","integrate (x+1)^2 from 0 to 2").answer=="∫ = 8.66666666667","expanded definite integral");
    check(solve("calculus","definite_integral","integrate 3x^2 from 2 to 0").answer=="∫ = -8","reversed integral bounds");
    check(solve("calculus","tangent_line","7;at=0").answer=="y = 0x + 7","constant tangent");
    for(const char* input:{"x^33","x^-1","sin(x)","1/x","x/0","2 3","x^2^3","(x+1"})
        check(solve("calculus","differentiation",input).status=="error",std::string("unsupported polynomial rejected: ")+input);
    // Independent closed-form integrals with varying signed coefficients and
    // powers, not snapshots produced by the solver itself.
    for(int a=-8;a<=8;++a)for(int n=0;n<=8;++n)for(int upper=-3;upper<=3;++upper) {
        const auto result=solve("calculus","definite_integral","integrate "+std::to_string(a)+"x^"+std::to_string(n)+"+2 from 0 to "+std::to_string(upper));
        const long double expected=static_cast<long double>(a)*std::pow(static_cast<long double>(upper),n+1)/(n+1)+2*upper;
        check(result.status=="success"&&close(rhs(result.answer),static_cast<double>(expected)),"independent polynomial integral family");
    }
}
void independent_random(unsigned count) {
    std::mt19937 generator(0x504533);
    std::uniform_int_distribution<int> small(-30,30),positive(1,100);
    for(unsigned i=0;i<count;++i) {
        const int a=small(generator),b=small(generator),c=small(generator);
        const auto arithmetic=solve("algebra","numeric_evaluation",std::to_string(a)+"*("+std::to_string(b)+"+"+std::to_string(c)+")");
        check(arithmetic.status=="success"&&close(rhs(arithmetic.answer),a*(b+c)),"independent integer arithmetic");
        // Direct 3x3 Leibniz formula is independent of elimination in the core.
        std::array<int,9> m{};for(auto& value:m)value=small(generator);
        std::string input;for(std::size_t j=0;j<9;++j){if(j)input+=j%3==0?';':',';input+=std::to_string(m[j]);}
        const double determinant=m[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*m[7]-m[4]*m[6]);
        check(close(rhs(solve("linear_algebra","determinant",input).answer),determinant),"3x3 Leibniz determinant oracle");
        // Build a nonsingular system with a known solution, not a core-produced oracle.
        const int p=positive(generator),q=positive(generator),x=small(generator),y=small(generator);
        const auto system=solve("linear_algebra","linear_system",std::to_string(p)+",1,"+std::to_string(p*x+y)+";-1,"+std::to_string(q)+","+std::to_string(-x+q*y));
        const auto second=system.answer.find("x2 =");
        check(system.status=="success"&&second!=std::string::npos&&close(rhs(system.answer),x)&&close(rhs(system.answer.substr(second)),y),"constructed linear system oracle");
        const auto inverse=solve("linear_algebra","inverse",std::to_string(p)+",1;-1,"+std::to_string(q));
        const auto numbers=matrix_numbers(inverse.answer);const double det=p*q+1;
        check(numbers.size()==4&&close(numbers[0],q/det)&&close(numbers[1],-1/det)&&close(numbers[2],1/det)&&close(numbers[3],p/det),"adjugate inverse oracle");
        const double voltage=positive(generator),r1=positive(generator)*10,r2=positive(generator)*10;
        std::ostringstream divider;divider<<voltage<<' '<<r1<<' '<<r2;
        const auto circuit=solve("circuit","voltage_divider",divider.str());
        check(close(rhs(circuit.answer),voltage/(1+r1/r2)),"divider ratio oracle");
        std::ostringstream netlist;netlist<<"V V1 a 0 "<<voltage<<"; R R1 a b "<<r1<<"; R R2 b 0 "<<r2;
        const auto nodal=solve("circuit","dc_nodal_analysis",netlist.str());const auto position=nodal.answer.find("V(b)");
        check(position!=std::string::npos&&close(rhs(nodal.answer.substr(position)),voltage/(1+r1/r2)),"MNA compared to closed-form network");
        const int bits=2+static_cast<int>(i%10),mod=1<<bits,left=positive(generator)%mod,right=positive(generator)%mod;
        const auto binary=[bits](int n){std::string result;for(int j=bits-1;j>=0;--j)result+=n&(1<<j)?'1':'0';return result;};
        const int l=left>=mod/2?left-mod:left,r=right>=mod/2?right-mod:right,total=l+r;
        const auto signedResult=solve("logic","signed_arithmetic","twos_add "+std::to_string(bits)+" "+binary(left)+" "+binary(right));
        const bool overflow=total<-mod/2||total>=mod/2;
        check(signedResult.answer.ends_with(std::string("overflow = ")+(overflow?"1":"0")),"signed range overflow oracle");
        const auto magnitude=solve("linear_algebra","vectors","magnitude:"+std::to_string(a)+","+std::to_string(b)+","+std::to_string(c));
        check(close(rhs(magnitude.answer),std::hypot(a,b,c)),"scaled hypot norm oracle");
        const int n=positive(generator);const auto loop=solve("programming","loops","int sum=0; for (int i=1; i<="+std::to_string(n)+"; ++i) sum += i;");
        int sum=0;for(int j=1;j<=n;++j)sum+=j;
        check(close(rhs(loop.answer),sum),"explicit accumulation vs triangular formula");
    }
}
void exhaustive_logic(bool stress) {
    // All 256 three-variable functions, plus every disjoint ON/DC assignment
    // for three variables (3^8 = 6561). No core answer is used as expected data.
    for(unsigned code=0;code<6561;++code) {
        unsigned value=code,on=0,dc=0;for(unsigned bit=0;bit<8;++bit){const auto state=value%3;value/=3;if(state==1)on|=1u<<bit;else if(state==2)dc|=1u<<bit;}
        const auto result=solve("logic","kmap","vars=3; minterms="+minterms(on,3)+"; dc="+minterms(dc,3));
        check(result.status=="success","all 3-variable ON/DC maps solve");
        if(result.status!="success")continue;
        for(int assignment=0;assignment<8;++assignment)if(!(dc&(1u<<assignment)))
            check(dnf(result.answer.substr(4),assignment,3)==static_cast<bool>(on&(1u<<assignment)),"independent DNF truth-table equivalence");
    }
    for(unsigned mask=0;mask<65536;++mask) {
        // Deterministically stride through the 4-variable function space, plus
        // the full ON set; keep sanitizer CI bounded.
        if(!stress&&mask%97!=0&&mask!=65535)continue;
        const auto result=solve("logic","kmap","vars=4; minterms="+minterms(mask,4));
        check(result.status=="success","4-variable K-map sample");
        if(result.status=="success")for(int row=0;row<16;++row)check(dnf(result.answer.substr(4),row,4)==static_cast<bool>(mask&(1u<<row)),"4-variable emitted DNF oracle");
    }
    for(int a=0;a<2;++a)for(int b=0;b<2;++b)for(int c=0;c<2;++c){const auto result=solve("logic","combinational","full_adder "+std::to_string(a)+" "+std::to_string(b)+" "+std::to_string(c));check(result.answer=="Sum = "+std::to_string((a^b)^c)+"; Cout = "+std::to_string((a&b)|(a&c)|(b&c)),"full-adder independent Boolean formula");}
}
} // namespace

int main(int argc,char** argv) {
    const bool stress=argc>2&&std::string(argv[2])=="--stress";
    regression_edges();catalog_examples();polynomial_cases();independent_random(stress?50000:5000);exhaustive_logic(stress);
    std::sort(times.begin(),times.end());
    const double p50=times[times.size()/2],p95=times[times.size()*95/100],maximum=times.back();
    std::ostringstream report;report<<"{\"suite\":\"independent-v3\",\"seed\":5260595,\"checks\":"<<checks<<",\"passed\":"<<checks-failures<<",\"failed\":"<<failures<<",\"timings_us\":{\"p50\":"<<p50<<",\"p95\":"<<p95<<",\"max\":"<<maximum<<"},\"limitations\":\"Independent oracles cover selected families and edge cases, not every supported topic. Catalog checks only establish runnable examples. Timing is machine-specific; not an Android benchmark.\"}";
    std::cout<<report.str()<<'\n';
    if(argc>1){std::ofstream output(argv[1]);if(!output){std::cerr<<"Cannot write report\n";return 2;}output<<report.str()<<'\n';}
    return failures?1:0;
}
