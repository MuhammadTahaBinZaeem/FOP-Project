#include "pocket_engineer/engine.hpp"
#include "pocket_engineer/input.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace pocket_engineer;
namespace {
unsigned checks=0,failures=0;
void check(bool ok,const std::string& message){++checks;if(!ok){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
SolutionBundle automatic(const std::string& text,const std::string& domain="auto",const std::string& topic="auto"){
    return Engine{}.solve({domain,topic,text,{},"auto"});
}
void answer(const std::string& text,const std::string& expected,const std::string& domain="auto",const std::string& topic="auto"){
    const auto r=automatic(text,domain,topic);check(r.status=="success"&&r.answer==expected,text+" => "+r.answer+" (expected "+expected+")");
}
}
int main(){
    // Semantic preservation checks are deliberately counted separately from
    // independently specified numerical answers below.
    for(const auto& t:topic_catalog()){
        const auto legacy=Engine{}.solve({std::string(t.domain),std::string(t.topic),std::string(t.example)});
        const auto r=automatic(std::string(t.example),std::string(t.domain),std::string(t.topic));
        check(r.status=="success"&&r.answer==legacy.answer&&r.topic==t.topic,"Catalog auto/manual preservation: "+std::string(t.topic)+" => "+r.answer);
    }
    answer("Could you please calculate twenty five plus seven?","32");
    answer("What is one hundred and twenty three minus twenty three?","100");
    answer("Please find 2 × (3 + 4)²","98");
    answer("What is 2(3+4)?","14");
    answer("What is the square root of 81?","9");
    answer("Calculate √81 + 2","11");
    answer("What is twenty percent of 150?","30");
    answer("What is 10% + 20?","20.1");
    answer("What is sine of 30 degrees?","0.5");
    answer("Please expand (x+1)(x-1)","x^2 - 1");
    answer("Simplify x+x+2x","4x");
    answer("Simplify x^2-5x+6","x^2 - 5x + 6");
    answer("Can you solve this equation: 2x+3=11?","x = 4");
    answer("Solve for y: 2y+3=11","x1 = 4");
    answer("Differentiate x^3 with respect to x","3x^2");
    answer("Solve 2(x+3)=x+11","x = 5");
    answer("Please solve 3x+7=2x+10","x = 3");
    answer("2(x+3)=x+11","x = 5","linear_algebra","inverse");
    answer("2x+3=11","x = 4","calculus","differentiation");
    answer("Find the determinant of [[1,2],[3,4]]","det(A) = -2","logic","truth_table");
    answer("Please transpose [[1,2,3],[4,5,6]]","[[1, 4]; [2, 5]; [3, 6]]");
    answer("Find the rank of A = [[1,2],[2,4]]","rank(A) = 1");
    answer("Multiply matrices [[1,2],[3,4]] and [[2,0],[1,2]]","[[4, 4]; [10, 8]]");
    answer("Solve x+y=3 and 2x-y=0","x1 = 1, x2 = 2");
    answer("Solve 2(a+b)=6; a-b=-1","x1 = 1, x2 = 2");
    answer("Find the dot product of (1,2,3) and (4,5,6)","v·w = 32");
    answer("Differentiate f(x) = 3x² - 2x + 7","6x - 2","circuit","voltage_divider");
    answer("Integrate 3x² from 0 to 2","∫ = 8");
    answer("Find the tangent line to 3x² at x=2","y = 12x + -12");
    answer("Find the factorial of five","fact(5) = 120");
    answer("Use a K-map to simplify F(A,B,C)=Σm(1,3,5,7)","F = C");
    answer("K-map with 3 variables, maxterms=(0,2,4,6)","F = C");
    answer("Full adder 1 1 1","Sum = 1; Cout = 1");
    answer("JK flip flop 1 1 0","Qnext = 1");
    answer("Canonical POS for A'B+AB'","F = ΠM(0, 3)");
    answer("Voltage divider Vin=12V, R1=1kOhm, R2=2kOhm","Vout = 8 V; I = 0.004 A");
    answer("Voltage divider Vin=12 volts and R1=1000 ohms and R2=2 kiloohms","Vout = 8 V; I = 0.004 A");
    answer("Voltage divider Vin=12 R1=1000 R2=2000","Vout = 8 V; I = 0.004 A");
    answer("Solve dy/dx = 2xy","y = C·exp(x^2)");
    answer("Solve y′ + 2*y = 4","y = 2 + C·exp(-2x)");
    answer("RK4 dy/dx=1*y, y(0)=1, h=0.1, steps=10","y(1) ≈ 2.71827974414");
    const auto converted=automatic("Convert 1011 from binary to decimal");check(converted.status=="success"&&converted.answer.find("11")!=std::string::npos,"Natural base conversion: "+converted.answer);
    const auto units=automatic("Convert 2.2 kiloohms to Ohm");check(units.status=="success"&&units.answer.find("2200")!=std::string::npos,"Natural SI conversion: "+units.answer);
    for(const auto& text:{"[[1,2],[3,4]]","Find determinant and inverse of [[1,2],[3,4]]","Solve x*y=2 and x+y=3","Voltage divider Vin=12V, R1=1kOhm","Voltage divider Vin=12V, R1=1kOhm, R2=2kOhm, temperature=50","Voltage divider Vin=12V, R1=1kOhm, R2=2kOhm except R1 is zero","Voltage divider 12 1000 2000 500","Voltage divider Vin=12A, R1=1kOhm, R2=2kOhm","K-map minterms=1,3,5,7","K-map vars=3; minterms=1,3; dc=3","RK4 dy/dx=y+3, y(0)=1, h=0.1, steps=10"}){
        const auto r=automatic(text);check(r.status=="error","Ambiguous/unsupported text must not produce a guessed answer: "+std::string(text)+" => "+r.answer);
    }
    SolveOptions limited;limited.max_steps=1;const auto budget=Engine{}.solve({"logic","truth_table","Find the determinant of [[1,2],[3,4]]",{},"auto"},limited);check(budget.steps.size()<=1,"Interpretation respects the step budget");
    const auto wrong=automatic("Find the determinant of [[1,2],[3,4]]","logic","truth_table");check(wrong.interpretation_json.find("\"type_changed\":true")!=std::string::npos,"Wrong selection corrected transparently");
    check(automatic(std::string(4097,'x')).status=="error","Raw question budget");
    // Original, independently computed arithmetic and equation families; these
    // are not snapshots generated from the solver under test.
    for(int a=1;a<=100;++a)for(int b=1;b<=20;++b){
        answer("Please calculate "+std::to_string(a)+" plus "+std::to_string(b),std::to_string(a+b));
        const auto r=automatic("Solve "+std::to_string(a)+"(x+2)="+std::to_string(a*(b+2)));
        check(r.status=="success"&&r.answer=="x = "+std::to_string(b),"Generated parenthesized linear equation "+std::to_string(a)+", "+std::to_string(b));
    }
    std::cout<<"Natural input: "<<checks<<" checks, "<<failures<<" failures (55 catalog equivalence checks; remaining checks have explicit expectations).\n";
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
