#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <random>
#include <stdexcept>

using namespace pocket_engineer;
using namespace pocket_engineer::workbench;
namespace {
int checks{}, failures{};
void check(bool value, const std::string &message) {
  ++checks;
  if (!value) { ++failures; std::cerr << "FAIL " << message << '\n'; }
}
void close(double actual, double expected, const std::string &label, double tolerance=1e-7) {
  check(std::abs(actual-expected)<=tolerance*std::max(1.,std::abs(expected)),
        label+": actual="+format(actual)+" expected="+format(expected));
}
void complex_close(const Json &actual, std::complex<double> expected, const std::string &label) {
  close(actual.at("real").number(),expected.real(),label+" real");
  close(actual.at("imag").number(),expected.imag(),label+" imaginary");
}
template<class F> void rejects(F action,const std::string &label) {
  try { action();check(false,label); } catch(const std::exception&) {check(true,label);}
}
Json visual(const SolutionBundle &result) {
  check(result.status=="success","successful result: "+result.topic+" "+result.answer.substr(0,80));
  check(result.verification.status==VerificationStatus::verified_numerical || result.topic=="generator", "checked result: "+result.verification.evidence);
  return Json::parse(result.visual_json,1048576);
}
SolutionBundle study(const std::string &netlist,Json::Object options) {
  return circuit_study({"circuit","study",netlist,Json(options).dump()});
}
SolutionBundle signal(Json::Object options) {
  return signals({"signals","signals",Json(options).dump(),{}});
}
void circuit_studies() {
  const std::string divider="V V1 in 0 12; R R1 in out 1000; R R2 out 0 2000";
  const auto port=visual(study(divider,{{"operation","port"},{"positive","out"}})).at("equivalent");
  complex_close(port.at("vth"),8.,"Thévenin divider voltage");
  complex_close(port.at("impedance"),2000./3.,"Thévenin parallel resistance");
  complex_close(port.at("norton_current"),.012,"Norton current");
  close(port.at("maximum_available_power_w").number(),.024,"maximum available power");
  complex_close(port.at("probe_voltage_full_network"),4.8,"full-network external load");
  const auto controlled=visual(study("V V1 a 0 2; E E1 b 0 a 0 3; R R1 b out 500; R R2 out 0 1000",{{"operation","port"},{"positive","out"}})).at("equivalent");
  complex_close(controlled.at("vth"),4.,"dependent source retained in Vth");complex_close(controlled.at("impedance"),1000./3.,"dependent source retained during test-source method");
  const auto ac=visual(study("V V1 in 0 1; R R1 in out 1000; C C1 out 0 1u",{{"operation","port"},{"analysis","ac"},{"frequency",1/(2*std::numbers::pi*.001)},{"positive","out"},{"load_real",500},{"load_imag",250}})).at("equivalent");
  complex_close(ac.at("vth"),{.5,-.5},"AC equivalent voltage");complex_close(ac.at("impedance"),{500,-500},"AC equivalent impedance");complex_close(ac.at("conjugate_match"),{500,500},"AC conjugate matching");
  const auto sources=visual(study("V V1 a 0 10; V V2 b 0 5; R R1 a out 1000; R R2 b out 1000; R R3 out 0 1000",{{"operation","superposition"},{"observe","out"}}));
  check(sources.at("contributions").array().size()==2,"two source contributions");complex_close(sources.at("contributions").array()[0].at("observed_voltage"),10./3.,"first source contribution");complex_close(sources.at("contributions").array()[1].at("observed_voltage"),5./3.,"second source contribution");
  const auto sensitivity=visual(study(divider,{{"operation","sensitivity"},{"observe","out"}}));
  for(const auto &row:sensitivity.at("sensitivities").array()){
    const auto name=row.at("component").string();const double expected=name=="V1"?2./3.:name=="R1"?-12.*2000/9000000:12.*1000/9000000;
    complex_close(row.at("absolute_derivative"),expected,"analytic divider sensitivity "+name);
  }
  const auto sweep=visual(study("V V1 in 0 1; R R1 in out 1000; C C1 out 0 1u",{{"operation","sweep"},{"analysis","ac"},{"frequency",10},{"frequency_high",10000},{"count",64},{"observe","out"}}));
  for(const auto &row:sweep.at("sweep").array()){
    const double w=2*std::numbers::pi*row.at("frequency_hz").number()*.001;
    complex_close(row.at("voltage"),{1/(1+w*w),-w/(1+w*w)},"RC sweep closed-form oracle");
  }
  std::mt19937 random(5052026);
  for(int i=0;i<5000;i++){
    const double resistance=1.+random()%100000,capacitance=(1.+random()%1000)*1e-9,frequency=1.+random()%100000;
    const double source=(1.+random()%1000)/100;
    const auto solved=circuit_analysis({"circuit","network","V V1 in 0 "+format(source)+"; R R1 in out "+format(resistance)+"; C C1 out 0 "+format(capacitance),Json(Json::Object{{"analysis","ac"},{"frequency",frequency}}).dump()});
    const auto data=visual(solved);const double w=2*std::numbers::pi*frequency*resistance*capacitance;
    for(const auto &row:data.at("nodes").array())if(row.at("node").string()=="out")complex_close(row.at("voltage"),{source/(1+w*w),-source*w/(1+w*w)},"random AC low-pass oracle");
  }
  rejects([&]{(void)study(divider,{{"operation","port"},{"positive","absent"}});},"unknown port node rejected");
  rejects([&]{(void)study(divider,{{"operation","port"},{"positive","out"},{"load_real",0}});},"zero probe load rejected");
  rejects([&]{(void)study(divider,{{"operation","sweep"},{"analysis","dc"}});},"DC frequency sweep rejected");
}
void digital_filters() {
  const auto lowpass=visual(signal({{"operation","filter"},{"x","1,1,1,1,1"},{"numerator","0.5"},{"denominator","1,-0.5"}}));
  for(std::size_t i=0;i<5;i++)close(lowpass.at("samples").array()[i].number(),1-std::pow(.5,static_cast<double>(i+1)),"first-order step response");
  check(lowpass.at("stability").at("status").string()=="strictly_stable","stable first-order pole");
  const auto initial=visual(signal({{"operation","filter"},{"x","0,0,0"},{"numerator","1"},{"denominator","1,-0.5"},{"past_output","8"}}));
  close(initial.at("samples").array()[0].number(),4,"filter initial output history");close(initial.at("samples").array()[2].number(),1,"filter free response");
  const auto fir=visual(signal({{"operation","filter"},{"x","1,2,3"},{"numerator","2,3"},{"denominator","1"},{"past_input","4"}}));
  close(fir.at("samples").array()[0].number(),14,"FIR prior input");close(fir.at("samples").array()[2].number(),12,"FIR running sum");
  std::mt19937 random(66061);
  for(int trial=0;trial<1000;trial++){
    const double pole=(static_cast<double>(random()%1801)-900)/1000;
    const double scale=(1.+random()%100)/10;
    const auto data=visual(signal({{"operation","filter"},{"x","1,0,0,0,0,0,0,0"},{"numerator",Json::Array{scale}},{"denominator",Json::Array{1,-pole}}}));
    for(std::size_t n=0;n<8;n++)close(data.at("samples").array()[n].number(),scale*std::pow(pole,static_cast<int>(n)),"random one-pole impulse");
    check(data.at("stability").at("status").string()=="strictly_stable","random Schur inside-unit-circle");
  }
  for(double pole:{-1.2,-1.,1.,1.2}){
    const auto data=visual(signal({{"operation","filter"},{"x","1,0,0"},{"numerator","1"},{"denominator",Json::Array{1,-pole}}}));
    check(data.at("stability").at("status").string()!="strictly_stable","boundary or unstable Schur rejection");
  }
  for(const auto kind:{"lowpass","highpass","bandpass","bandstop"})for(const auto window:{"rectangular","hann","hamming","blackman"}){
    const auto data=visual(signal({{"operation","fir_design"},{"kind",kind},{"window",window},{"fs",1000},{"cutoff",100},{"cutoff_high",250},{"taps",31}}));
    const auto &coefficients=data.at("coefficients").array();check(coefficients.size()==31,"FIR tap count");
    for(std::size_t i=0;i<coefficients.size();i++)close(coefficients[i].number(),coefficients[coefficients.size()-1-i].number(),"FIR symmetry");
    const double frequency=std::string(kind)=="highpass"?500:std::string(kind)=="bandpass"?175:0;
    std::complex<long double> gain{};for(std::size_t k=0;k<coefficients.size();k++){const long double angle=-2*std::numbers::pi_v<long double>*frequency*k/1000;gain+=static_cast<long double>(coefficients[k].number())*std::complex<long double>{std::cos(angle),std::sin(angle)};}
    close(static_cast<double>(std::abs(gain)),1,"FIR reference gain direct oracle");
  }
  const auto response=visual(signal({{"operation","digital_response"},{"numerator","0.5,0.5"},{"denominator","1"},{"fs",1000},{"count",3}}));
  complex_close(response.at("response").array()[0].at("value"),1.,"moving average DC");complex_close(response.at("response").array()[1].at("value"),{.5,-.5},"moving average quarter sampling rate");complex_close(response.at("response").array()[2].at("value"),0.,"moving average Nyquist zero");
  rejects([]{(void)signal({{"operation","fir_design"},{"taps",32}});},"even type-I FIR tap count rejected");
  rejects([]{(void)signal({{"operation","filter"},{"x","1"},{"numerator","1"},{"denominator","0,1"}});},"zero leading denominator rejected");
}
void z_transforms() {
  const auto finite=visual(signal({{"operation","z_transform"},{"x","1,2,3"},{"origin",-1},{"z_real",2},{"z_imag",0}}));
  complex_close(finite.at("evaluation"),5.5,"finite bilateral Z at z=2");
  const auto zero=visual(signal({{"operation","z_transform"},{"x","1,2,3"},{"origin",-2},{"z_real",0},{"z_imag",0}}));complex_close(zero.at("evaluation"),3.,"left-sided polynomial evaluated at zero");
  rejects([]{(void)signal({{"operation","z_transform"},{"x","1,2"},{"z_real",0},{"z_imag",0}});},"finite Z pole at zero");
  for(const auto denominator:{"1,-0.5","1,-1,0.25","1,0,0.25","1,-0.75,0.125"}){
    const auto data=visual(signal({{"operation","inverse_z"},{"numerator","1"},{"denominator",denominator},{"count",32}}));
    const auto &samples=data.at("samples").array();for(int n=0;n<32;n++){
      const std::string d=denominator;const double expected=d=="1,-0.5"?std::pow(.5,n):d=="1,-1,0.25"?(n+1)*std::pow(.5,n):d=="1,0,0.25"?(n%2?0:std::pow(-.25,n/2)):2*std::pow(.5,n)-std::pow(.25,n);
      close(samples[static_cast<std::size_t>(n)].number(),expected,"closed-form inverse Z oracle "+d);
    }
  }
  const auto fir=visual(signal({{"operation","inverse_z"},{"numerator","1,2,3,4"},{"denominator","1"},{"count",8}}));for(int i=0;i<8;i++)close(fir.at("samples").array()[static_cast<std::size_t>(i)].number(),i<4?i+1:0,"finite FIR inverse Z");
}
void form_tests() {
  const auto catalog=engineering_schema({"workbench","engineering_schema",{}, {}});
  check(catalog.at("operations").array().size()==13,"13 runnable engineering forms");
  for(const auto &entry:catalog.at("operations").array()){
    Json::Object fields;for(const auto &field:entry.at("fields").array())fields[field.at("id").string()]=field.at("value");
    const auto converted=engineering_form_input({"workbench","engineering_input",Json(Json::Object{{"operation",entry.at("operation")},{"values",fields}}).dump(),{}});
    const auto solved=signals({"signals","signals",converted.at("input").dump(),{}});check(solved.status=="success","engineering form default solves: "+entry.at("operation").string()+" "+solved.answer.substr(0,80));
  }
  rejects([]{(void)engineering_form_input({"workbench","engineering_input",R"({"operation":"fft","values":{"x":"1,0","fs":"","pad":"false","window":"rectangular"}})",{}});},"empty numeric form field cannot silently become zero");
}
}
int main(){try{circuit_studies();digital_filters();z_transforms();form_tests();}catch(const std::exception &error){++failures;std::cerr<<"Unexpected exception: "<<error.what()<<'\n';}std::cout<<checks<<" extended engineering checks, "<<failures<<" failures\n";return failures?1:0;}
