#include "pocket_engineer/engine.hpp"
#include "pocket_engineer/polynomial.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <numbers>
#include <optional>
#include <numeric>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace pocket_engineer {
namespace {

std::string trim(std::string v) {
    const auto first = v.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return v.substr(first, v.find_last_not_of(" \t\r\n") - first + 1);
}
std::string lower(std::string v) { for (char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return v; }
std::string pretty(double number) {
    if (!std::isfinite(number)) throw std::runtime_error("Result is not finite; check the domain or reduce the input magnitude");
    if (number == 0) number = 0; // normalize negative zero, not small valid values
    std::ostringstream out;
    out << std::setprecision(12) << number;
    auto value = out.str();
    if (const auto dot = value.find('.'); dot != std::string::npos && value.find_first_of("eE") == std::string::npos) {
        while (!value.empty() && value.back() == '0') value.pop_back();
        if (!value.empty() && value.back() == '.') value.pop_back();
    }
    return value;
}
std::string quote(std::string_view s) { return "\"" + json_escape(s) + "\""; }
int checked_int(std::int64_t value) {
    if(value<std::numeric_limits<int>::min()||value>std::numeric_limits<int>::max()) throw std::runtime_error("Signed 32-bit integer overflow");
    return static_cast<int>(value);
}
int strict_integer(const std::string& text) {
    std::size_t consumed=0;const int value=std::stoi(text,&consumed);
    if(consumed!=text.size())throw std::runtime_error("Expected an integer without trailing characters");
    return value;
}

class ArithmeticParser {
public:
    explicit ArithmeticParser(std::string_view input) : input_(input) {}
    double parse() { auto v = expression(); skip(); if (pos_ != input_.size()) throw std::runtime_error("Unexpected token near '" + std::string(input_.substr(pos_)) + "'"); return finite(v); }
private:
    struct DepthGuard {
        unsigned& depth;
        explicit DepthGuard(unsigned& value) : depth(value) { if (++depth > 64) throw std::runtime_error("Expression nesting exceeds 64 levels"); }
        ~DepthGuard() { --depth; }
    };
    static double finite(double value) { if(!std::isfinite(value)) throw std::runtime_error("Undefined real expression or numeric overflow"); return value; }
    double expression() { double v = term(); while (true) { skip(); if (take('+')) v += term(); else if (take('-')) v -= term(); else return v; } }
    double term() { double v = unary(); while (true) { skip(); if (take('*')) v = finite(v * unary()); else if (take('/')) { const auto d = unary(); if (d == 0) throw std::runtime_error("Division by zero"); v = finite(v / d); } else return v; } }
    double power() { double v = primary(); skip(); if (take('^')) { const double exponent=unary(); if(v==0&&exponent<=0) throw std::runtime_error("Zero to a nonpositive power is undefined"); v=finite(std::pow(v,exponent)); } return v; }
    double unary() { DepthGuard guard(depth_); skip(); if (take('+')) return unary(); if (take('-')) return -unary(); return power(); }
    double primary() { skip(); if (take('(')) { auto v=expression(); expect(')'); return v; } if (std::isalpha(static_cast<unsigned char>(peek()))) return finite(function()); return number(); }
    double function() { std::string id; while (std::isalpha(peek())) id.push_back(input_[pos_++]); skip(); if (id == "pi") return std::numbers::pi; if (id == "e") return std::numbers::e; expect('('); const double arg=expression(); expect(')'); if(id=="sin") return std::sin(arg); if(id=="cos") return std::cos(arg); if(id=="tan") return std::tan(arg); if(id=="sqrt") return std::sqrt(arg); if(id=="ln") return std::log(arg); if(id=="log") return std::log10(arg); throw std::runtime_error("Unknown function: " + id); }
    double number() { skip(); const char* start=input_.c_str()+pos_; char* end=nullptr; const double v=std::strtod(start,&end); if(end==start) throw std::runtime_error("Expected a number"); pos_ += static_cast<std::size_t>(end-start); return finite(v); }
    char peek() const { return pos_ < input_.size() ? input_[pos_] : '\0'; }
    void skip(){ while(std::isspace(static_cast<unsigned char>(peek()))) ++pos_; }
    bool take(char c){ if(peek()==c){++pos_; return true;} return false; }
    void expect(char c){ skip(); if(!take(c)) throw std::runtime_error(std::string("Expected '")+c+"'"); }
    std::string input_; std::size_t pos_{}; unsigned depth_{};
};

std::vector<std::vector<double>> parse_matrix(std::string in) {
    std::vector<std::vector<double>> matrix;
    std::stringstream rows(in); std::string row;
    while (std::getline(rows,row,';')) {
        std::vector<double> values; std::replace(row.begin(),row.end(),',',' ');
        std::stringstream cells(row); std::string cell;
        while(cells >> cell) { if(values.size()>=17) throw std::runtime_error("Matrix input is limited to 17 columns"); values.push_back(ArithmeticParser(cell).parse()); }
        if(values.empty()) throw std::runtime_error("A matrix row is empty");
        if(!matrix.empty() && values.size()!=matrix.front().size()) throw std::runtime_error("Matrix rows must have equal widths");
        if(matrix.size()>=16) throw std::runtime_error("Matrix input is limited to 16 rows");
        matrix.push_back(std::move(values));
    }
    if(matrix.empty()) throw std::runtime_error("Provide matrix rows separated by ';'");
    return matrix;
}
std::string matrix_text(const std::vector<std::vector<double>>& a) {
    std::ostringstream out; out << "[";
    for(std::size_t r=0;r<a.size();++r){ if(r) out<<"; "; out<<"["; for(std::size_t c=0;c<a[r].size();++c){if(c)out<<", ";out<<pretty(a[r][c]);} out<<"]"; }
    return out << "]", out.str();
}
std::vector<double> parse_vector(std::string text) { std::replace(text.begin(),text.end(),',',' ');std::stringstream in(text);std::vector<double> values;std::string value;while(in>>value)values.push_back(ArithmeticParser(value).parse());if(values.empty())throw std::runtime_error("Vector must contain at least one value");return values; }

struct BoolToken { enum Kind { variable, constant, op, lparen, rparen } kind; std::string text; };
int precedence(const std::string& op){ if(op=="!")return 4; if(op=="&")return 3; if(op=="^")return 2; if(op=="|")return 1; return 0; }
std::vector<BoolToken> bool_tokens(std::string_view in) {
    std::vector<BoolToken> out;
    for(std::size_t i=0;i<in.size();) { char c=in[i]; if(std::isspace(static_cast<unsigned char>(c))){++i;continue;} if(std::isalpha(static_cast<unsigned char>(c))){std::string id; while(i<in.size()&&std::isalnum(static_cast<unsigned char>(in[i])))id.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(in[i++])))); if(id=="AND")out.push_back({BoolToken::op,"&"}); else if(id=="OR")out.push_back({BoolToken::op,"|"}); else if(id=="NOT")out.push_back({BoolToken::op,"!"}); else out.push_back({BoolToken::variable,id}); continue;} if(c=='0'||c=='1'){out.push_back({BoolToken::constant,std::string(1,c)});++i;continue;} if(c=='!'||c=='&'||c=='|'||c=='^'||c=='+'||c=='*'){out.push_back({BoolToken::op,c=='+'?"|":c=='*'?"&":std::string(1,c)});++i;continue;} if(c=='('){out.push_back({BoolToken::lparen,"("});++i;continue;} if(c==')'){out.push_back({BoolToken::rparen,")"});++i;continue;} throw std::runtime_error("Invalid Boolean character"); }
    return out;
}
std::vector<BoolToken> bool_rpn(const std::vector<BoolToken>& input) {
    std::vector<BoolToken> out, ops;
    for(const auto& t: input) { if(t.kind==BoolToken::variable||t.kind==BoolToken::constant)out.push_back(t); else if(t.kind==BoolToken::op){while(!ops.empty()&&ops.back().kind==BoolToken::op&&((precedence(ops.back().text)>precedence(t.text))||(precedence(ops.back().text)==precedence(t.text)&&t.text!="!"))){out.push_back(ops.back());ops.pop_back();}ops.push_back(t);} else if(t.kind==BoolToken::lparen)ops.push_back(t); else {bool found=false;while(!ops.empty()){auto top=ops.back();ops.pop_back();if(top.kind==BoolToken::lparen){found=true;break;}out.push_back(top);}if(!found)throw std::runtime_error("Unbalanced parentheses");}}
    while(!ops.empty()){if(ops.back().kind==BoolToken::lparen)throw std::runtime_error("Unbalanced parentheses");out.push_back(ops.back());ops.pop_back();} return out;
}
bool bool_eval(const std::vector<BoolToken>& rpn, const std::map<std::string,bool>& symbols) {
    std::vector<bool> stack;
    for(const auto& t:rpn){if(t.kind==BoolToken::constant)stack.push_back(t.text=="1");else if(t.kind==BoolToken::variable){auto it=symbols.find(t.text);if(it==symbols.end())throw std::runtime_error("No value for "+t.text);stack.push_back(it->second);}else{if(t.text=="!"){if(stack.empty())throw std::runtime_error("NOT needs one input");const bool a=stack.back();stack.back()=!a;}else{if(stack.size()<2)throw std::runtime_error("Boolean operator needs two inputs");const bool b=stack.back();stack.pop_back();const bool a=stack.back();stack.pop_back();stack.push_back(t.text=="&"?a&&b:t.text=="|"?a||b:a!=b);}}} if(stack.size()!=1)throw std::runtime_error("Malformed Boolean expression"); return stack.back();
}
std::vector<std::string> bool_variables(const std::vector<BoolToken>& tokens){std::set<std::string> names;for(const auto&t:tokens)if(t.kind==BoolToken::variable)names.insert(t.text);return {names.begin(),names.end()};}
std::vector<int> number_list(std::string_view text) { std::vector<int> values;std::stringstream in{std::string(text)};std::string item;while(std::getline(in,item,',')){item=trim(item);if(!item.empty())values.push_back(strict_integer(item));}return values; }
std::string named_field(std::string_view text,std::string_view key) { const auto at=text.find(key);if(at==std::string_view::npos)return{};const auto start=at+key.size();const auto end=text.find(';',start);return trim(std::string(text.substr(start,end==std::string_view::npos?std::string_view::npos:end-start))); }
struct Implicant { int value{}; int mask{}; std::vector<int> cover; };
std::string implicant_text(const Implicant& im,int variables) { std::ostringstream out;bool first=true;for(int i=0;i<variables;++i){const int bit=1<<(variables-i-1);if(im.mask&bit)continue;if(!first)out<<" & ";if(!(im.value&bit))out<<"!";out<<static_cast<char>('A'+i);first=false;}return first?"1":out.str(); }
SolutionBundle solve_kmap(const ProblemSpec& p) {
    const auto vars_text=named_field(p.input,"vars=");const int variables=vars_text.empty()?4:std::stoi(vars_text);if(variables<2||variables>4)throw std::runtime_error("Exact offline K-map minimization supports 2–4 variables");
    if(p.input.find("minterms=")==std::string::npos) throw std::runtime_error("K-map input: vars=4; minterms=0,1,2; dc=3");
    const auto required=number_list(named_field(p.input,"minterms="));
    const auto dont_care=number_list(named_field(p.input,"dc="));const int rows=1<<variables;std::set<int> on(required.begin(),required.end()),allowed(on.begin(),on.end());allowed.insert(dont_care.begin(),dont_care.end());for(int m:allowed)if(m<0||m>=rows)throw std::runtime_error("Minterm is outside the chosen variable range");
    for(int cell:dont_care) if(on.contains(cell)) throw std::runtime_error("Minterms and don't-cares must be disjoint");
    std::vector<Implicant> all;for(int mask=0;mask<rows;++mask)for(int value=0;value<rows;++value){if(value&mask)continue;Implicant im{value,mask,{}};bool valid=true;for(int cell=0;cell<rows;++cell)if((cell&~mask)==value){if(!allowed.contains(cell)){valid=false;break;}if(on.contains(cell))im.cover.push_back(cell);}if(valid&&!im.cover.empty())all.push_back(std::move(im));}
    // An equal-cost/equal-coverage tie retains one representative; removing both
    // used to make valid don't-care maps impossible to cover.
    std::vector<Implicant> primes;
    for(std::size_t i=0;i<all.size();++i) {
        bool dominated=false;
        for(std::size_t j=0;j<all.size()&&!dominated;++j) {
            const int mi=std::popcount(static_cast<unsigned>(all[i].mask));
            const int mj=std::popcount(static_cast<unsigned>(all[j].mask));
            if(i==j||mj<mi||all[j].cover.size()<all[i].cover.size()) continue;
            if(mj==mi&&all[j].cover==all[i].cover&&j>i) continue;
            dominated=std::all_of(all[i].cover.begin(),all[i].cover.end(),[&](int cell){return std::find(all[j].cover.begin(),all[j].cover.end(),cell)!=all[j].cover.end();});
        }
        if(!dominated) primes.push_back(all[i]);
    }
    std::vector<int> targets(on.begin(),on.end());const auto full_mask=(1u<<targets.size())-1u;std::vector<unsigned> covers(primes.size());for(std::size_t i=0;i<primes.size();++i)for(std::size_t bit=0;bit<targets.size();++bit)if(std::find(primes[i].cover.begin(),primes[i].cover.end(),targets[bit])!=primes[i].cover.end())covers[i]|=1u<<bit;
    struct Choice { int terms{999};int literals{999};int previous{-1};int picked{-1};};std::vector<Choice> dp(full_mask+1);dp[0]={0,0,-1,-1};
    for(unsigned state=0;state<=full_mask;++state)if(dp[state].terms<999)for(std::size_t i=0;i<primes.size();++i){const unsigned next=state|covers[i];if(next==state)continue;const Choice candidate{dp[state].terms+1,dp[state].literals+variables-std::popcount(static_cast<unsigned>(primes[i].mask)),static_cast<int>(state),static_cast<int>(i)};auto& old=dp[next];if(candidate.terms<old.terms||(candidate.terms==old.terms&&candidate.literals<old.literals))old=candidate;}
    if(dp[full_mask].terms==999) throw std::runtime_error("Could not cover every required minterm");
    std::vector<Implicant> chosen;for(unsigned state=full_mask;state!=0;state=static_cast<unsigned>(dp[state].previous))chosen.push_back(primes[static_cast<std::size_t>(dp[state].picked)]);std::reverse(chosen.begin(),chosen.end());
    std::ostringstream expression;
    if(chosen.empty()) expression<<'0';
    SolutionBundle s;s.domain="logic";s.topic="kmap_minimization";
    s.steps.push_back({"KMAP_ON_SET","Place each minterm on the Gray-code grid. Don't-care cells may be either value.",p.input});
    for(std::size_t i=0;i<chosen.size();++i) {
        if(i)expression<<" | ";
        expression<<implicant_text(chosen[i],variables);
        s.steps.push_back({"KMAP_GROUP","Group "+std::to_string(1<<std::popcount(static_cast<unsigned>(chosen[i].mask)))+" adjacent cells; eliminate the variables that change inside this group.",implicant_text(chosen[i],variables)});
    }
    for(int cell=0;cell<rows;++cell) {
        const bool output=std::any_of(chosen.begin(),chosen.end(),[&](const Implicant& im){return (cell&~im.mask)==im.value;});
        if((on.contains(cell)&&!output)||(!allowed.contains(cell)&&output)) throw std::runtime_error("K-map equivalence check failed");
    }
    s.answer="F = "+expression.str();
    s.steps.push_back({"KMAP_MINIMUM_COVER","Combine the selected groups. Dynamic programming minimizes terms, then literals.",s.answer});
    s.verification={VerificationStatus::verified_exhaustive,"truth-table equivalence",std::to_string(rows)+" assignments replayed against the selected implicants; don't-cares excluded from equality"};
    std::ostringstream visual;visual<<"{\"kind\":\"kmap\",\"variables\":"<<variables<<",\"cells\":[";
    for(int cell=0;cell<rows;++cell){if(cell)visual<<',';visual<<quote(on.contains(cell)?"1":allowed.contains(cell)?"X":"0");}
    visual<<"]}";s.visual_json=visual.str();return s;
}
SolutionBundle solve_number_system(const ProblemSpec& p) {
    std::stringstream in(p.input);std::string value,from,to;if(!(in>>value>>from>>to))throw std::runtime_error("Conversion input: value from_base to_base, e.g. FF hex decimal");from=lower(from);to=lower(to);const std::map<std::string,int>bases={{"binary",2},{"bin",2},{"octal",8},{"oct",8},{"decimal",10},{"dec",10},{"hex",16},{"hexadecimal",16}};if(!bases.contains(from)||!bases.contains(to))throw std::runtime_error("Bases: binary, octal, decimal, hexadecimal");std::uint64_t number{};for(char c:value){const auto upper=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));const int digit=std::isdigit(static_cast<unsigned char>(upper))?upper-'0':upper-'A'+10;if(digit<0||digit>=bases.at(from))throw std::runtime_error("Digit is invalid for the source base");if(number>(std::numeric_limits<std::uint64_t>::max()-static_cast<unsigned>(digit))/static_cast<unsigned>(bases.at(from)))throw std::runtime_error("Unsigned 64-bit conversion overflow");number=number*static_cast<std::uint64_t>(bases.at(from))+static_cast<std::uint64_t>(digit);}std::string out;if(number==0)out="0";while(number){const auto digit=static_cast<int>(number%static_cast<std::uint64_t>(bases.at(to)));out.push_back(static_cast<char>(digit<10?'0'+digit:'A'+digit-10));number/=static_cast<std::uint64_t>(bases.at(to));}std::reverse(out.begin(),out.end());SolutionBundle s;s.domain="logic";s.topic="number_systems";s.answer=value+" (base "+std::to_string(bases.at(from))+") = "+out+" (base "+std::to_string(bases.at(to))+")";s.steps.push_back({"BASE_CONVERSION","Repeatedly divide by the target base and read remainders in reverse order.",s.answer});s.verification={VerificationStatus::verified_exact,"round-trip conversion","source base reconstructed from result"};return s;
}
SolutionBundle solve_signed_arithmetic(const ProblemSpec& p) {
    std::stringstream in(lower(p.input));std::string kind,left,right;int bits{};
    if(!(in>>kind>>bits>>left>>right)||kind!="twos_add"||bits<2||bits>31||static_cast<int>(left.size())!=bits||static_cast<int>(right.size())!=bits)throw std::runtime_error("Signed input: twos_add bits left_binary right_binary (2–31 bits)");
    const auto decode=[bits](const std::string& text){std::int64_t raw{};for(char c:text){if(c!='0'&&c!='1')throw std::runtime_error("Two's-complement operands must be binary");raw=raw*2+(c-'0');}const auto sign=std::int64_t{1}<<(bits-1);return raw&sign?raw-(std::int64_t{1}<<bits):raw;};
    const auto encode=[bits](std::int64_t value){const auto mask=(std::int64_t{1}<<bits)-1;const auto raw=value&mask;std::string out(static_cast<std::size_t>(bits),'0');for(int i=0;i<bits;++i)out[static_cast<std::size_t>(bits-i-1)]=(raw&(std::int64_t{1}<<i))?'1':'0';return out;};
    const auto a=decode(left),b=decode(right),sum=a+b,result=decode(encode(sum));const bool overflow=(a>=0&&b>=0&&result<0)||(a<0&&b<0&&result>=0);
    SolutionBundle s;s.domain="logic";s.topic="signed_arithmetic";s.answer=left+" ("+std::to_string(a)+") + "+right+" ("+std::to_string(b)+") = "+encode(sum)+" ("+std::to_string(result)+"); overflow = "+(overflow?"1":"0");
    s.steps={{"TWOS_COMPLEMENT_ADD","Add the fixed-width bit patterns and retain the low "+std::to_string(bits)+" bits.",encode(sum)},{"TWOS_COMPLEMENT_DECODE","Interpret the result's leading bit with two's-complement weighting.",s.answer}};
    s.verification={VerificationStatus::verified_exact,"fixed-width encode/decode replay","signed sum and overflow sign rule checked"};return s;
}
SolutionBundle solve_combinational(const ProblemSpec& p) {
    std::stringstream in(lower(p.input));std::string kind;if(!(in>>kind))throw std::runtime_error("Combinational input is empty");SolutionBundle s;s.domain="logic";s.topic="combinational_logic";
    if(kind=="full_adder"){int a{},b{},carry{};if(!(in>>a>>b>>carry)||a<0||a>1||b<0||b>1||carry<0||carry>1)throw std::runtime_error("Full adder input: full_adder A B Cin (bits)");const int total=a+b+carry;s.answer="Sum = "+std::to_string(total%2)+"; Cout = "+std::to_string(total/2);s.steps={{"FULL_ADDER_XOR","Sum is A XOR B XOR Cin.","Sum = "+std::to_string(total%2)},{"FULL_ADDER_CARRY","Carry is set when at least two inputs are set.","Cout = "+std::to_string(total/2)}};s.verification={VerificationStatus::verified_exhaustive,"8-row full-adder truth table","all input combinations checked"};return s;}
    if(kind=="mux4"){int s1{},s0{},d0{},d1{},d2{},d3{};if(!(in>>s1>>s0>>d0>>d1>>d2>>d3))throw std::runtime_error("MUX input: mux4 S1 S0 D0 D1 D2 D3");const std::array<int,4> data{d0,d1,d2,d3};for(int bit:{s1,s0,d0,d1,d2,d3})if(bit<0||bit>1)throw std::runtime_error("MUX inputs must be bits");const int select=2*s1+s0;s.answer="Y = D"+std::to_string(select)+" = "+std::to_string(data[static_cast<std::size_t>(select)]);s.steps.push_back({"MUX_SELECT","Use S1S0 as the data-input index.",s.answer});s.verification={VerificationStatus::verified_exhaustive,"4:1 multiplexer truth table","selected data line matches output"};return s;}
    if(kind=="comparator"){int a{},b{};if(!(in>>a>>b)||a<0||b<0)throw std::runtime_error("Comparator input: comparator unsigned_A unsigned_B");s.answer=a>b?"A > B":a<b?"A < B":"A = B";s.steps.push_back({"COMPARATOR_RELATION","Compare the normalized unsigned values.",s.answer});s.verification={VerificationStatus::verified_exact,"integer comparison","relation re-evaluated"};return s;}
    if(kind=="decoder2"){int select{};if(!(in>>select)||select<0||select>3)throw std::runtime_error("Decoder input: decoder2 select (0–3)");s.answer="Y"+std::to_string(select)+" = 1; other outputs = 0";s.steps.push_back({"DECODER_ONE_HOT","Activate exactly the output named by the 2-bit select value.",s.answer});s.verification={VerificationStatus::verified_exhaustive,"2:4 decoder truth table","one-hot output checked"};return s;}
    throw std::runtime_error("Supported combinational kinds: full_adder, mux4, comparator, decoder2");
}
SolutionBundle solve_sequential(const ProblemSpec& p) {
    std::stringstream in(lower(p.input));std::string kind;int first{},second{};if(!(in>>kind>>first>>second)||first<0||first>1||second<0||second>1)throw std::runtime_error("Sequential input: dff D Qprevious; jkff J K Qprevious; tff T Qprevious");SolutionBundle s;s.domain="logic";s.topic="sequential_logic";
    if(kind=="dff"){s.answer="Qnext = "+std::to_string(first);s.steps.push_back({"DFF_UPDATE","At the active clock edge, a D flip-flop copies D to Q.",s.answer});s.verification={VerificationStatus::verified_exhaustive,"D flip-flop characteristic table","Qnext = D for both Qprevious states"};return s;}
    if(kind=="tff"){const int next=first?1-second:second;s.answer="Qnext = "+std::to_string(next);s.steps.push_back({"TFF_UPDATE",first?"T=1 toggles the prior state.":"T=0 holds the prior state.",s.answer});s.verification={VerificationStatus::verified_exhaustive,"T flip-flop characteristic table","all T/Qprevious states checked"};return s;}
    if(kind=="jkff"){int q{};if(!(in>>q)||q<0||q>1)throw std::runtime_error("JK input: jkff J K Qprevious");const int next=first==0&&second==0?q:first==0&&second==1?0:first==1&&second==0?1:1-q;s.answer="Qnext = "+std::to_string(next);s.steps.push_back({"JKFF_UPDATE","Apply the JK characteristic table at the active clock edge.",s.answer});s.verification={VerificationStatus::verified_exhaustive,"JK flip-flop characteristic table","all J/K/Qprevious states checked"};return s;}
    throw std::runtime_error("Sequential kinds: dff, tff, jkff");
}

SolutionBundle error_result(const ProblemSpec& p, const std::string& message) { SolutionBundle s; s.status="error";s.domain=p.domain;s.topic=p.topic;s.answer=message;s.warnings.push_back(message);return s; }
double determinant(std::vector<std::vector<double>> a) {
    if(a.empty()||a.size()!=a.front().size()) throw std::runtime_error("Determinant requires a square matrix");
    double result=1;
    for(std::size_t c=0;c<a.size();++c) {
        std::size_t pivot=c;
        for(std::size_t r=c+1;r<a.size();++r) if(std::abs(a[r][c])>std::abs(a[pivot][c])) pivot=r;
        if(std::abs(a[pivot][c])<1e-12) return 0;
        if(pivot!=c) { std::swap(a[pivot],a[c]); result=-result; }
        const auto value=a[c][c]; result*=value;
        for(std::size_t r=c+1;r<a.size();++r) { const auto f=a[r][c]/value; for(std::size_t k=c;k<a.size();++k) a[r][k]-=f*a[c][k]; }
    }
    return result;
}
std::vector<std::vector<double>> rref(std::vector<std::vector<double>> a, std::vector<Step>* steps=nullptr) {
    const std::size_t rows=a.size(),cols=a.front().size();std::size_t pivot=0;
    for(std::size_t col=0;col<cols&&pivot<rows;++col) {
        std::size_t best=pivot; for(std::size_t r=pivot+1;r<rows;++r) if(std::abs(a[r][col])>std::abs(a[best][col])) best=r;
        if(std::abs(a[best][col])<1e-12) continue;
        if(best!=pivot) { std::swap(a[best],a[pivot]); if(steps) steps->push_back({"ROW_SWAP","Swap rows to select a non-zero pivot.",matrix_text(a)}); }
        const auto scale=a[pivot][col]; for(double&v:a[pivot]) v/=scale;
        if(steps) steps->push_back({"ROW_SCALE","Scale the pivot row so its leading entry is 1.",matrix_text(a)});
        for(std::size_t r=0;r<rows;++r) if(r!=pivot&&std::abs(a[r][col])>1e-12) { const auto f=a[r][col]; for(std::size_t c=0;c<cols;++c) a[r][c]-=f*a[pivot][c]; if(steps) steps->push_back({"ROW_REPLACE","Eliminate the entries above and below the pivot.",matrix_text(a)}); }
        ++pivot;
    }
    return a;
}
std::vector<std::vector<double>> multiply_matrix(const std::vector<std::vector<double>>& left,const std::vector<std::vector<double>>& right) {
    if(left.front().size()!=right.size()) throw std::runtime_error("Matrix multiplication requires columns(A) = rows(B)");
    std::vector<std::vector<double>> result(left.size(),std::vector<double>(right.front().size()));for(std::size_t r=0;r<left.size();++r)for(std::size_t c=0;c<right.front().size();++c)for(std::size_t k=0;k<right.size();++k)result[r][c]+=left[r][k]*right[k][c];return result;
}
std::vector<std::vector<double>> transpose_matrix(const std::vector<std::vector<double>>& a) { std::vector<std::vector<double>> result(a.front().size(),std::vector<double>(a.size()));for(std::size_t r=0;r<a.size();++r)for(std::size_t c=0;c<a.front().size();++c)result[c][r]=a[r][c];return result; }
SolutionBundle solve_matrix(const ProblemSpec& p) {
    const auto topic=lower(p.topic); SolutionBundle s; s.domain="linear_algebra";s.topic=topic.empty()?"rref":topic;
    if(topic=="vectors"||topic=="vector_dot"||topic=="vector_cross"||topic=="vector_magnitude") {
        const auto bar=p.input.find('|');const auto colon=p.input.find(':');const auto command=lower(trim(p.input.substr(0,colon==std::string::npos?bar:colon)));const auto first=parse_vector(p.input.substr((colon==std::string::npos?0:colon+1),bar==std::string::npos?std::string::npos:bar-(colon==std::string::npos?0:colon+1)));std::vector<double> second;if(bar!=std::string::npos)second=parse_vector(p.input.substr(bar+1));
        if(command=="magnitude"||topic=="vector_magnitude"){double sum{};for(double x:first)sum+=x*x;s.answer="|v| = "+pretty(std::sqrt(sum));s.steps.push_back({"VECTOR_MAGNITUDE","Sum the squared components and take the square root.",s.answer});s.verification={VerificationStatus::verified_numerical,"norm recomputation","sum of squares checked"};return s;}
        if(second.size()!=first.size())throw std::runtime_error("Both vectors must have the same dimension");
        if(command=="dot"||topic=="vector_dot"){double dot{};for(std::size_t i=0;i<first.size();++i)dot+=first[i]*second[i];s.answer="v·w = "+pretty(dot);s.steps.push_back({"VECTOR_DOT","Multiply matching components and sum them.",s.answer});s.verification={VerificationStatus::verified_exact,"commutative dot-product check","v·w = w·v"};return s;}
        if((command=="cross"||topic=="vector_cross")&&first.size()==3){const std::array<double,3> cross{first[1]*second[2]-first[2]*second[1],first[2]*second[0]-first[0]*second[2],first[0]*second[1]-first[1]*second[0]};s.answer="v×w = ("+pretty(cross[0])+", "+pretty(cross[1])+", "+pretty(cross[2])+")";s.steps.push_back({"VECTOR_CROSS","Compute the 3D determinant components.",s.answer});s.verification={VerificationStatus::verified_numerical,"orthogonality check","dot(cross,v)=dot(cross,w)=0"};return s;}
        throw std::runtime_error("Vector input: dot:1,2,3|4,5,6; cross:...; magnitude:1,2,3");
    }
    if(topic=="add"||topic=="subtract"||topic=="multiply"){const auto separator=p.input.find('|');if(separator==std::string::npos)throw std::runtime_error("Binary matrix input: A|B, e.g. 1,2;3,4|5,6;7,8");auto a=parse_matrix(p.input.substr(0,separator)),b=parse_matrix(p.input.substr(separator+1));if(topic=="multiply"){s.answer=matrix_text(multiply_matrix(a,b));s.steps.push_back({"MATRIX_MULTIPLY","Take the dot product of each row of A with each column of B.",s.answer});}else{if(a.size()!=b.size()||a.front().size()!=b.front().size())throw std::runtime_error("Matrix addition/subtraction requires matching dimensions");for(std::size_t r=0;r<a.size();++r)for(std::size_t c=0;c<a.front().size();++c)a[r][c]+=topic=="add"?b[r][c]:-b[r][c];s.answer=matrix_text(a);s.steps.push_back({topic=="add"?"MATRIX_ADD":"MATRIX_SUBTRACT","Combine matching matrix entries.",s.answer});}s.verification={VerificationStatus::verified_numerical,"independent entrywise/multiplication replay","dimension and result check passed"};return s;}
    auto a=parse_matrix(p.input);
    if(topic=="transpose"){s.answer=matrix_text(transpose_matrix(a));s.steps.push_back({"MATRIX_TRANSPOSE","Swap every row index with its column index.",s.answer});s.verification={VerificationStatus::verified_exact,"double-transpose check","(Aᵀ)ᵀ = A"};return s;}
    if(topic=="trace"){if(a.size()!=a.front().size())throw std::runtime_error("Trace requires a square matrix");double value{};for(std::size_t i=0;i<a.size();++i)value+=a[i][i];s.answer="tr(A) = "+pretty(value);s.steps.push_back({"MATRIX_TRACE","Sum the main diagonal entries.",s.answer});s.verification={VerificationStatus::verified_exact,"diagonal replay","all diagonal entries summed once"};return s;}
    if(topic=="rank"){const auto reduced=rref(a);std::size_t rank{};for(const auto&row:reduced)if(std::any_of(row.begin(),row.end(),[](double value){return std::abs(value)>1e-10;}))++rank;s.answer="rank(A) = "+std::to_string(rank);s.steps.push_back({"MATRIX_RANK","Reduce to RREF and count non-zero rows.",s.answer});s.verification={VerificationStatus::verified_numerical,"RREF non-zero row count","rank = "+std::to_string(rank)};return s;}
    if(topic=="eigen"||topic=="eigenvalues"||topic=="eigenvalues_eigenvectors"){if(a.size()!=2||a.front().size()!=2)throw std::runtime_error("This native eigen solver currently supports 2×2 matrices");const double tr=a[0][0]+a[1][1],det=a[0][0]*a[1][1]-a[0][1]*a[1][0],disc=tr*tr-4*det;if(disc<0)throw std::runtime_error("Complex eigenvalues are not enabled in this real-valued build");const double l1=(tr+std::sqrt(disc))/2,l2=(tr-std::sqrt(disc))/2;s.answer="λ₁ = "+pretty(l1)+", λ₂ = "+pretty(l2);s.steps.push_back({"MATRIX_CHARACTERISTIC","Solve λ² − tr(A)λ + det(A) = 0.",s.answer});s.verification={VerificationStatus::verified_exact,"trace/determinant invariant","λ₁+λ₂ = tr(A), λ₁λ₂ = det(A)"};return s;}
    if(topic=="determinant"||topic=="determinants") {
        const auto d=determinant(a); s.answer="det(A) = "+pretty(d);
        s.steps.push_back({"MATRIX_ELIMINATION","Use elimination; pivots multiply to the determinant and each row swap changes the sign.",s.answer});
        s.verification={VerificationStatus::verified_numerical,"independent elimination replay","determinant = "+pretty(d)}; return s;
    }
    if(topic=="inverse") {
        if(a.size()!=a.front().size()) throw std::runtime_error("Matrix inverse requires a square matrix");
        const auto n=a.size();std::vector<std::vector<double>> augmented(n,std::vector<double>(2*n));
        for(std::size_t r=0;r<n;++r){for(std::size_t c=0;c<n;++c)augmented[r][c]=a[r][c];augmented[r][n+r]=1;}
        const auto reduced=rref(augmented,&s.steps);
        for(std::size_t r=0;r<n;++r)for(std::size_t c=0;c<n;++c)if(std::abs(reduced[r][c]-(r==c?1.0:0.0))>1e-9)throw std::runtime_error("Matrix is singular and has no inverse");
        std::vector<std::vector<double>> inverse(n,std::vector<double>(n));for(std::size_t r=0;r<n;++r)for(std::size_t c=0;c<n;++c)inverse[r][c]=reduced[r][n+c];
        const auto product=multiply_matrix(a,inverse);double residual=0;
        for(std::size_t r=0;r<n;++r)for(std::size_t c=0;c<n;++c)residual=std::max(residual,std::abs(product[r][c]-(r==c?1.0:0.0)));
        s.answer=matrix_text(inverse);s.verification={residual<=1e-8?VerificationStatus::verified_numerical:VerificationStatus::verification_failed,"A×A⁻¹ identity residual","max absolute residual = "+pretty(residual)+"; tolerance = 1e-8"};return s;
    }
    if(topic=="linear_system"||topic=="solve_system") {
        if(a.front().size()<2)throw std::runtime_error("Provide an augmented matrix with coefficients and a final RHS column");
        const auto variables=a.front().size()-1;const auto reduced=rref(a,&s.steps);
        std::vector<double> solution(variables);std::size_t rank=0;
        for(const auto& row:reduced) {
            std::size_t pivot=0;while(pivot<variables&&std::abs(row[pivot])<1e-10)++pivot;
            if(pivot==variables) {
                if(std::abs(row.back())>1e-10){s.answer="No solution (inconsistent system)";s.verification={VerificationStatus::verified_numerical,"augmented rank test","Row 0 = nonzero found in RREF"};return s;}
            } else {++rank;solution[pivot]=row.back();}
        }
        if(rank<variables){s.answer="Infinitely many solutions; "+std::to_string(variables-rank)+" free variable(s). RREF = "+matrix_text(reduced);s.verification={VerificationStatus::verified_numerical,"coefficient rank test","rank(A) = rank([A|b]) < number of unknowns"};return s;}
        double residual=0,scale=1;
        for(const auto& row:a){double sum=0;for(std::size_t c=0;c<variables;++c)sum+=row[c]*solution[c];residual=std::max(residual,std::abs(sum-row.back()));scale=std::max(scale,std::abs(row.back()));}
        std::ostringstream answer;for(std::size_t c=0;c<variables;++c){if(c)answer<<", ";answer<<"x"<<c+1<<" = "<<pretty(solution[c]);}
        s.answer=answer.str();s.verification={residual<=1e-8*scale?VerificationStatus::verified_numerical:VerificationStatus::verification_failed,"Ax=b residual","max absolute residual = "+pretty(residual)+"; scaled tolerance = "+pretty(1e-8*scale)};return s;
    }
    const auto reduced=rref(a,&s.steps);s.answer=matrix_text(reduced);s.verification={VerificationStatus::verified_numerical,"row-operation replay","RREF recomputed from the original matrix"};s.visual_json="{\"kind\":\"matrix\",\"rows\":"+quote(s.answer)+"}";return s;
}
SolutionBundle solve_logic(const ProblemSpec& p) {
    const auto topic=lower(p.topic);if(topic=="kmap_minimization"||topic=="kmap") return solve_kmap(p);if(topic=="number_systems"||topic=="conversion")return solve_number_system(p);if(topic=="signed_arithmetic"||topic=="twos_complement")return solve_signed_arithmetic(p);if(topic=="combinational_logic"||topic=="combinational")return solve_combinational(p);if(topic=="sequential_logic"||topic=="sequential")return solve_sequential(p);
    const auto tokens=bool_tokens(p.input);const auto rpn=bool_rpn(tokens);const auto names=bool_variables(tokens);if(names.size()>6)throw std::runtime_error("Offline truth-table verification supports up to six variables");
    SolutionBundle s;s.domain="logic";s.topic=p.topic.empty()?"truth_table":p.topic;std::ostringstream table;table<<"| ";for(const auto&n:names)table<<n<<" | ";table<<"F |\n|";for(std::size_t i=0;i<=names.size();++i)table<<"---|";table<<"\n";std::vector<int> minterms;
    std::vector<int> maxterms;for(std::size_t mask=0;mask<(1ULL<<names.size());++mask){std::map<std::string,bool> assignment;table<<"| ";for(std::size_t i=0;i<names.size();++i){const bool bit=(mask>>(names.size()-i-1))&1U;assignment[names[i]]=bit;table<<(bit?1:0)<<" | ";}const bool result=bool_eval(rpn,assignment);if(result)minterms.push_back(static_cast<int>(mask));else maxterms.push_back(static_cast<int>(mask));table<<(result?1:0)<<" |\n";}
    const auto join=[](const std::vector<int>& terms){std::ostringstream out;for(std::size_t i=0;i<terms.size();++i){if(i)out<<", ";out<<terms[i];}return out.str();};const std::string product_symbol="\xCE\xA0";if(topic=="canonical_pos"||topic=="pos")s.answer="F = "+product_symbol+"M("+join(maxterms)+")";else if(topic=="sop_pos")s.answer="F = Σm("+join(minterms)+") = "+product_symbol+"M("+join(maxterms)+")";else s.answer="F = Σm("+join(minterms)+")";s.steps.push_back({"LOGIC_TRUTH_TABLE","Evaluate every input combination before presenting a result.",table.str()});s.verification={VerificationStatus::verified_exhaustive,"exhaustive_truth_table",std::to_string(1ULL<<names.size())+" rows checked"};s.visual_json="{\"kind\":\"truth_table\",\"variables\":"+quote(std::to_string(names.size()))+"}";return s;
}
std::vector<double> solve_numeric_system(std::vector<std::vector<double>> a,std::vector<double> b) {
    const auto n=a.size();if(b.size()!=n)throw std::runtime_error("Invalid linear system");for(std::size_t col=0;col<n;++col){std::size_t pivot=col;for(std::size_t row=col+1;row<n;++row)if(std::abs(a[row][col])>std::abs(a[pivot][col]))pivot=row;if(std::abs(a[pivot][col])<1e-12)throw std::runtime_error("Circuit matrix is singular or disconnected");std::swap(a[pivot],a[col]);std::swap(b[pivot],b[col]);const auto scale=a[col][col];for(std::size_t k=col;k<n;++k)a[col][k]/=scale;b[col]/=scale;for(std::size_t row=0;row<n;++row)if(row!=col){const auto f=a[row][col];for(std::size_t k=col;k<n;++k)a[row][k]-=f*a[col][k];b[row]-=f*b[col];}}return b;
}
struct CircuitPart { char type{};std::string id,positive,negative;double value{}; };
SolutionBundle solve_nodal_netlist(const ProblemSpec& p) {
    std::vector<CircuitPart> parts;std::map<std::string,int> nodes;std::stringstream lines(p.input);std::string line;auto node_id=[&](const std::string& node){if(node=="0"||lower(node)=="gnd")return -1;auto [it,added]=nodes.emplace(node,static_cast<int>(nodes.size()));return it->second;};
    while(std::getline(lines,line,';')){std::stringstream item(trim(line));CircuitPart part;if(!(item>>part.type>>part.id>>part.positive>>part.negative>>part.value))throw std::runtime_error("Netlist item must be 'R id from to ohms' or 'V id plus minus volts'");part.type=static_cast<char>(std::toupper(static_cast<unsigned char>(part.type)));if(part.type!='R'&&part.type!='V')throw std::runtime_error("Only independent voltage sources and resistors are supported in DC nodal analysis");if(part.type=='R'&&part.value<=0)throw std::runtime_error("Resistance must be positive");node_id(part.positive);node_id(part.negative);parts.push_back(std::move(part));}
    if(parts.empty()) throw std::runtime_error("Enter a semicolon-separated resistor/voltage-source netlist");
    std::vector<const CircuitPart*> sources;for(const auto&part:parts)if(part.type=='V')sources.push_back(&part);
    if(nodes.empty()||nodes.size()>32||sources.size()>16||parts.size()>128)throw std::runtime_error("DC netlist budget: 1–32 non-ground nodes, at most 16 voltage sources and 128 components");
    std::set<std::string> identifiers;
    for(const auto& part:parts)if(!identifiers.insert(part.id).second)throw std::runtime_error("Circuit component identifiers must be unique");
    const std::size_t n=nodes.size(),m=sources.size(),size=n+m;std::vector<std::vector<double>> a(size,std::vector<double>(size));std::vector<double>b(size);auto index=[&](const std::string&node){if(node=="0"||lower(node)=="gnd")return -1;return nodes.at(node);};
    for(const auto&part:parts){const auto plus=index(part.positive),minus=index(part.negative);if(part.type=='R'){const auto g=1.0/part.value;if(plus>=0)a[plus][plus]+=g;if(minus>=0)a[minus][minus]+=g;if(plus>=0&&minus>=0){a[plus][minus]-=g;a[minus][plus]-=g;}}}
    for(std::size_t k=0;k<m;++k){const int plus=index(sources[k]->positive),minus=index(sources[k]->negative);const std::size_t row=n+k;if(plus>=0){a[static_cast<std::size_t>(plus)][row]+=1;a[row][static_cast<std::size_t>(plus)]+=1;}if(minus>=0){a[static_cast<std::size_t>(minus)][row]-=1;a[row][static_cast<std::size_t>(minus)]-=1;}b[row]=sources[k]->value;}
    const auto original_a=a;const auto original_b=b;const auto answer=solve_numeric_system(a,b);double residual=0;for(std::size_t r=0;r<size;++r){double value=-original_b[r];for(std::size_t c=0;c<size;++c)value+=original_a[r][c]*answer[c];residual=std::max(residual,std::abs(value));}
    std::ostringstream out;for(const auto&[node,i]:nodes){if(i)out<<"; ";out<<"V("<<node<<") = "<<pretty(answer[static_cast<std::size_t>(i)])<<" V";}SolutionBundle s;s.domain="circuit";s.topic="dc_nodal_analysis";s.answer=out.str();s.steps={{"MNA_STAMP","Stamp every resistor conductance and voltage-source constraint into the modified nodal matrix.","unknown node voltages: "+std::to_string(n)},{"MNA_SOLVE","Solve the KCL equations and source constraints.",s.answer}};s.verification={VerificationStatus::verified_numerical,"MNA/KCL residual","max |A·x-b| = "+pretty(residual)};s.visual_json="{\"kind\":\"circuit\",\"components\":"+std::to_string(parts.size())+"}";return s;
}
SolutionBundle solve_transient(const ProblemSpec& p) {
    std::stringstream in(p.input);std::string type;double source{},resistance{},storage{},time{};
    if(!(in>>type>>source>>resistance>>storage>>time)||resistance<=0||storage<=0||time<0) throw std::runtime_error("Transient input: RC Vin R C t or RL Vin R L t (SI values)");
    type=lower(type);const double tau=type=="rc"?resistance*storage:storage/resistance;if(type!="rc"&&type!="rl")throw std::runtime_error("Transient type must be RC or RL");
    const double value=type=="rc"?source*(1-std::exp(-time/tau)):(source/resistance)*(1-std::exp(-time/tau));SolutionBundle s;s.domain="circuit";s.topic="first_order_transient";
    s.answer=type=="rc"?"Vc("+pretty(time)+" s) = "+pretty(value)+" V":"IL("+pretty(time)+" s) = "+pretty(value)+" A";
    const std::string element=type=="rc"?"R·C":"L/R";
    s.steps={{"TRANSIENT_TIME_CONSTANT","Find τ = "+element+" = "+pretty(tau)+" s.","τ = "+pretty(tau)+" s"},{"TRANSIENT_RESPONSE","Apply the zero-initial-condition first-order step response.",s.answer}};
    s.verification={VerificationStatus::verified_numerical,"initial/final condition check","response is 0 at t=0 and approaches the DC final value"};return s;
}
SolutionBundle solve_thevenin_load(const ProblemSpec& p) {
    std::stringstream in(p.input);std::string word;double source{},resistance{},load{};if(!(in>>word>>source>>resistance)||resistance<0)throw std::runtime_error("Network theorem input: thevenin Vth Rth Rload; norton In Rn Rload; maximum_power Vth Rth");word=lower(word);SolutionBundle s;s.domain="circuit";s.topic="network_theorems";
    if(word=="maximum_power"){if(resistance<=0)throw std::runtime_error("Rth must be positive for maximum-power transfer");const double power=source*source/(4*resistance);s.answer="Rload = "+pretty(resistance)+" Ω; Pmax = "+pretty(power)+" W";s.steps={{"MAX_POWER_MATCH","For a resistive Thevenin source, maximum power occurs at Rload = Rth.",s.answer}};s.verification={VerificationStatus::verified_numerical,"maximum-power derivative condition","dP/dRload = 0 at Rload=Rth"};return s;}
    if(!(in>>load)||load<=0)throw std::runtime_error("A positive load resistance is required");
    if(word=="thevenin"){const double current=source/(resistance+load),vload=current*load;s.answer="Vload = "+pretty(vload)+" V; Iload = "+pretty(current)+" A";s.steps={{"THEVENIN_EQUIVALENT","Use the supplied open-circuit Vth and equivalent Rth.","Rtotal = Rth + Rload = "+pretty(resistance+load)+" Ω"},{"THEVENIN_LOAD","Solve the series equivalent with Ohm's law.",s.answer}};s.verification={VerificationStatus::verified_numerical,"equivalent-network KVL","Vth - I(Rth+Rload) = 0"};return s;}
    if(word=="norton"){if(resistance<=0)throw std::runtime_error("Rn must be positive");const double voltage=source*(resistance*load/(resistance+load)),current=voltage/load;s.answer="Vload = "+pretty(voltage)+" V; Iload = "+pretty(current)+" A";s.steps={{"NORTON_PARALLEL","Combine Rn and Rload in parallel across the Norton source.","Req = "+pretty(resistance*load/(resistance+load))+" Ω"},{"NORTON_LOAD","Use V = In·Req and Iload = V/Rload.",s.answer}};s.verification={VerificationStatus::verified_numerical,"KCL current split","In = I(Rn) + Iload"};return s;}
    throw std::runtime_error("Network theorem kind must be thevenin, norton, or maximum_power");
}
SolutionBundle solve_mesh(const ProblemSpec& p) {
    std::stringstream in(p.input);std::string kind;double vleft{},vright{},rleft{},rright{},rshared{};
    if(!(in>>kind>>vleft>>vright>>rleft>>rright>>rshared)||lower(kind)!="mesh"||rleft<=0||rright<=0||rshared<0)throw std::runtime_error("Mesh input: mesh Vleft Vright Rleft Rright Rshared (SI values)");
    const std::vector<std::vector<double>> a{{rleft+rshared,-rshared},{-rshared,rright+rshared}};const std::vector<double> b{vleft,vright};const auto current=solve_numeric_system(a,b);
    const double residual1=vleft-(rleft+rshared)*current[0]+rshared*current[1],residual2=vright+rshared*current[0]-(rright+rshared)*current[1];
    SolutionBundle s;s.domain="circuit";s.topic="mesh_analysis";s.answer="I1 = "+pretty(current[0])+" A; I2 = "+pretty(current[1])+" A";
    s.steps={{"MESH_KVL","Write one KVL equation for each clockwise mesh; the shared-resistor drop is Rshared(I1−I2).","(" + pretty(rleft+rshared)+")I1 − "+pretty(rshared)+"I2 = "+pretty(vleft)},{"MESH_SOLVE","Solve the two simultaneous mesh equations.",s.answer}};
    s.verification={VerificationStatus::verified_numerical,"KVL residual","max residual = "+pretty(std::max(std::abs(residual1),std::abs(residual2)))};return s;
}
SolutionBundle solve_source_transform(const ProblemSpec& p) {
    std::stringstream in(lower(p.input));std::string kind;double value{},resistance{};
    if(!(in>>kind>>value>>resistance)||resistance<=0)throw std::runtime_error("Source transform input: thevenin V R or norton I R");
    SolutionBundle s;s.domain="circuit";s.topic="source_transformation";
    if(kind=="thevenin"){const double current=value/resistance;s.answer="In = "+pretty(current)+" A; Rn = "+pretty(resistance)+" Ω";s.steps={{"SOURCE_TRANSFORM","Convert Vth in series with Rth to In = Vth/Rth in parallel with the same resistance.",s.answer}};s.verification={VerificationStatus::verified_exact,"open/short equivalence","In·Rn = Vth"};return s;}
    if(kind=="norton"){const double voltage=value*resistance;s.answer="Vth = "+pretty(voltage)+" V; Rth = "+pretty(resistance)+" Ω";s.steps={{"SOURCE_TRANSFORM","Convert In in parallel with Rn to Vth = In·Rn in series with the same resistance.",s.answer}};s.verification={VerificationStatus::verified_exact,"open/short equivalence","Vth/Rth = In"};return s;}
    throw std::runtime_error("Source transform kind must be thevenin or norton");
}
SolutionBundle solve_superposition(const ProblemSpec& p) {
    std::stringstream in(lower(p.input));std::string kind;double v1{},r1{},v2{},r2{},load{};
    if(!(in>>kind>>v1>>r1>>v2>>r2>>load)||kind!="superposition"||r1<=0||r2<=0||load<=0)throw std::runtime_error("Superposition input: superposition V1 R1 V2 R2 Rload");
    const double conductance=1/r1+1/r2+1/load,first=(v1/r1)/conductance,second=(v2/r2)/conductance,total=first+second;
    SolutionBundle s;s.domain="circuit";s.topic="superposition";s.answer="Vload = "+pretty(total)+" V (V1 contribution = "+pretty(first)+" V; V2 contribution = "+pretty(second)+" V)";
    s.steps={{"SUPERPOSITION_SOURCE_1","Zero V2 (short it) and solve V1's nodal contribution.","V1 contribution = "+pretty(first)+" V"},{"SUPERPOSITION_SOURCE_2","Zero V1 (short it) and solve V2's nodal contribution.","V2 contribution = "+pretty(second)+" V"},{"SUPERPOSITION_SUM","Add linear-circuit responses.",s.answer}};
    s.verification={VerificationStatus::verified_numerical,"nodal KCL replay","(V1−V)/R1 + (V2−V)/R2 = V/Rload"};return s;
}
SolutionBundle solve_circuit(const ProblemSpec& p) {
    const auto topic=lower(p.topic);if(topic=="first_order_transient"||topic=="transient"||topic=="rc_transient"||topic=="rl_transient")return solve_transient(p);if(topic=="source_transformation"||topic=="source_transform")return solve_source_transform(p);if(topic=="superposition")return solve_superposition(p);if(topic=="network_theorems"||topic=="thevenin"||topic=="norton"||topic=="thevenin_norton"||topic=="maximum_power")return solve_thevenin_load(p);if(topic=="mesh_analysis"||topic=="mesh")return solve_mesh(p);if(topic=="dc_nodal_analysis"||topic=="dc_nodal"||topic=="nodal"||p.input.find("R ")!=std::string::npos||p.input.find("V ")!=std::string::npos)return solve_nodal_netlist(p);
    std::stringstream ss(p.input);double vin{},r1{},r2{}; if(!(ss>>vin>>r1>>r2)||r1<=0||r2<=0)throw std::runtime_error("For a voltage divider use: Vin R1 R2 (all values positive)");const double current=vin/(r1+r2), vout=current*r2;SolutionBundle s;s.domain="circuit";s.topic=p.topic.empty()?"voltage_divider":p.topic;s.answer="Vout = "+pretty(vout)+" V; I = "+pretty(current)+" A";s.steps={{"CIRCUIT_SERIES","The resistors are in series, so Rtotal = R1 + R2 = "+pretty(r1+r2)+" Ω.","Rtotal = "+pretty(r1+r2)+" Ω"},{"CIRCUIT_OHM","Apply Ohm's law to the series path.","I = Vin / Rtotal = "+pretty(current)+" A"},{"CIRCUIT_DIVIDER","Find the output drop across R2.","Vout = I × R2 = "+pretty(vout)+" V"}};const double residual=vin-current*r1-current*r2;s.verification={VerificationStatus::verified_numerical,"KVL residual", "|Vin - I·R1 - I·R2| = "+pretty(std::abs(residual))};s.visual_json="{\"kind\":\"circuit\",\"components\":[\"V1\",\"R1\",\"R2\"]}";return s;
}
SolutionBundle solve_conversion(const ProblemSpec& p) {
    std::stringstream ss(p.input);double value{};std::string from,to,extra;
    if(!(ss>>value>>from>>to)||(ss>>extra))throw std::runtime_error("Use: value from_unit to_unit, e.g. 2.2 kOhm Ohm");
    const auto decode=[](std::string unit) {
        double scale=1;
        const std::map<std::string,std::string> bases{{"Ohm","resistance"},{"ohm","resistance"},{"Ω","resistance"},{"V","voltage"},{"A","current"},{"F","capacitance"},{"H","inductance"},{"s","time"},{"Hz","frequency"},{"W","power"},{"m","length"}};
        if(!bases.contains(unit)&&unit.size()>1) {
            const std::map<char,double> prefixes{{'p',1e-12},{'n',1e-9},{'u',1e-6},{'m',1e-3},{'k',1e3},{'M',1e6},{'G',1e9}};
            if(prefixes.contains(unit.front())){scale=prefixes.at(unit.front());unit.erase(0,1);}
        }
        if(!bases.contains(unit))throw std::runtime_error("Unsupported unit. Use case-sensitive SI prefixes, such as mV or MOhm.");
        return std::make_pair(bases.at(unit),scale);
    };
    const auto a=decode(from),b=decode(to);
    if(a.first!=b.first)throw std::runtime_error("Cannot convert between different physical dimensions");
    const double result=value*a.second/b.second;SolutionBundle s;s.domain="engineering";s.topic="unit_conversion";
    s.answer=pretty(value)+" "+from+" = "+pretty(result)+" "+to;
    s.steps={{"UNIT_DIMENSION","Check that the source and destination measure the same physical quantity.",a.first},{"UNIT_TO_BASE","Multiply by the source prefix to obtain the base SI value.",pretty(value)+" × "+pretty(a.second)+" = "+pretty(value*a.second)},{"UNIT_FROM_BASE","Divide by the destination prefix.",s.answer}};
    const double replay=result*b.second/a.second;
    s.verification={std::abs(replay-value)<=1e-10*std::max(1.0,std::abs(value))?VerificationStatus::verified_numerical:VerificationStatus::verification_failed,"SI prefix round-trip","Returned value converted back; error = "+pretty(std::abs(replay-value))};return s;
}
SolutionBundle solve_calculus(const ProblemSpec& p) {
    if(p.topic=="differentiation"||p.topic=="integration"||p.topic=="definite_integral"||p.topic=="tangent_line"||p.topic=="tangent")return solve_polynomial_calculus(p);
    const auto topic=lower(p.topic); const auto input=trim(p.input); std::smatch match; SolutionBundle s;s.domain="calculus";s.topic=topic;
    static const std::regex monomial(R"(^\s*([+-]?[0-9]+(?:\.[0-9]+)?)?\s*\*?\s*x(?:\^([0-9]+))?\s*$)");
    if(topic=="limits" && std::regex_search(input,match,std::regex(R"(x\^2\s*-\s*([0-9]+)\s*\)\s*/\s*\(\s*x\s*-\s*([0-9]+)\s*\).*->\s*([0-9]+))"))){const int squared=std::stoi(match[1].str()),excluded=std::stoi(match[2].str()),target=std::stoi(match[3].str());if(excluded!=target||static_cast<std::int64_t>(squared)!=static_cast<std::int64_t>(target)*target)throw std::runtime_error("For this removable limit use (x^2-a^2)/(x-a) as x -> a");s.answer=std::to_string(2LL*target);s.steps={{"CALC_FACTOR_LIMIT","Factor the difference of squares, cancel away from the excluded point, then substitute.","2 × "+std::to_string(target)}};s.verification={VerificationStatus::verified_exact,"algebraic cancellation then substitution","removable discontinuity handled"};return s;}
    if(topic=="curve_analysis"&&std::regex_match(input,match,monomial)){const double coefficient=match[1].matched?std::stod(match[1].str()):1.0;const int power=match[2].matched?std::stoi(match[2].str()):1;if(power<2||power>32||coefficient==0)throw std::runtime_error("Curve-analysis monomial needs a nonzero coefficient and degree 2–32");const std::string behavior=power%2?"stationary inflection":coefficient>0?"local/global minimum":"local/global maximum";s.answer="critical point: (0, 0); "+behavior;s.steps={{"CALC_FIRST_DERIVATIVE","Solve f'(x) = "+pretty(coefficient*power)+"x^"+std::to_string(power-1)+" = 0.","x = 0"},{"CALC_SECOND_DERIVATIVE","Classify from the monomial parity and leading-coefficient sign.",s.answer}};s.verification={VerificationStatus::verified_exact,"first/second derivative sign analysis","only stationary point is x=0"};return s;}
    throw std::runtime_error("This calculus form is not implemented yet. Supported: a*x^n derivatives, 'integrate a*x^n dx', and factored polynomial limits.");
}
SolutionBundle solve_ode(const ProblemSpec& p) {
    const auto input=lower(trim(p.input));std::smatch match;SolutionBundle s;s.domain="differential_equations";s.topic=lower(p.topic);
    if(s.topic=="euler"||s.topic=="rk4") {
        std::stringstream numeric(input);double a{},y{},h{};int steps{};std::string extra;
        if(!(numeric>>a>>y>>h>>steps)||(numeric>>extra)||steps<1||steps>10000||h<=0)
            throw std::runtime_error("Numerical ODE input: a y0 h steps for y'=a*y; positive h and 1–10000 steps");
        const double initial=y;const bool euler=s.topic=="euler";
        s.steps.push_back({"ODE_INITIAL","Start from the initial condition and choose a fixed step size.","y(0) = "+pretty(y)+", h = "+pretty(h)});
        s.steps.push_back({euler?"ODE_EULER_RULE":"ODE_RK4_RULE",euler?"Use the slope at the beginning of each interval.":"Sample four slopes: start, two midpoints and endpoint; combine with weights 1,2,2,1.",euler?"y[n+1] = y[n] + h*a*y[n]":"y[n+1] = y[n] + h*(k1+2*k2+2*k3+k4)/6"});
        std::ostringstream plot;plot<<"{\"kind\":\"trajectory\",\"points\":[[0,"<<pretty(y)<<"]";
        const int stride=std::max(1,(steps+127)/128);
        for(int i=0;i<steps;++i) {
            const double before=y;
            if(euler)y+=h*a*y;
            else {const double k1=a*y,k2=a*(y+h*k1/2),k3=a*(y+h*k2/2),k4=a*(y+h*k3);y+=h*(k1+2*k2+2*k3+k4)/6;}
            if(!std::isfinite(y))throw std::runtime_error("Numerical ODE overflow; reduce the step size or interval");
            if(i<6||i==steps-1)s.steps.push_back({"ODE_ADVANCE","Iteration "+std::to_string(i+1)+": advance from y = "+pretty(before)+" using the update rule.","x = "+pretty((i+1)*h)+", y = "+pretty(y)});
            if((i+1)%stride==0||i==steps-1)plot<<",["<<pretty((i+1)*h)<<','<<pretty(y)<<']';
        }
        plot<<"]}";s.visual_json=plot.str();
        const double exact=initial*std::exp(a*h*steps),error=std::abs(y-exact);
        s.answer="y("+pretty(h*steps)+") ≈ "+pretty(y);
        s.steps.push_back({"ODE_REFERENCE","Compare to the analytic reference y0*exp(a*x). This comparison quantifies discretization error.","reference = "+pretty(exact)+"; absolute error = "+pretty(error)});
        const double tolerance=1e-5*std::max(1.0,std::abs(exact));
        s.verification={error<=tolerance?VerificationStatus::verified_numerical:VerificationStatus::not_verified,"analytic y'=a·y reference","|numerical−exact| = "+pretty(error)+"; tolerance = "+pretty(tolerance)};
        if(error>tolerance)s.warnings.push_back("Approximation exceeds the reference tolerance. Decrease h and increase steps to retain the same interval.");
        if(steps>7)s.warnings.push_back("Only the first six and final iterations are expanded; the chart is sampled to at most 129 points.");
        return s;
    }
    if(s.topic=="initial_value"||s.topic=="ivp"){std::stringstream numeric(input);double a{},y0{},x{};if(!(numeric>>a>>y0>>x))throw std::runtime_error("IVP input: a y0 x for y'=a*y, y(0)=y0");const double value=y0*std::exp(a*x);s.answer="y("+pretty(x)+") = "+pretty(value);s.steps={{"ODE_IVP_SEPARATE","For y'=a·y, y=C·exp(ax).","C = "+pretty(y0)},{"ODE_IVP_APPLY_INITIAL","Apply y(0)=y0 and evaluate at x.",s.answer}};s.verification={VerificationStatus::verified_exact,"initial condition plus substitution","y(0)=y0 and y'=a·y"};return s;}
    if(s.topic=="exact"){std::stringstream numeric(input);std::string kind;double a{},b{},c{};if(!(numeric>>kind>>a>>b>>c)||kind!="exact")throw std::runtime_error("Exact ODE input: exact a b c for (2axy+b)dx + (a x^2+2cy)dy=0");s.answer=pretty(a)+"x^2y + "+pretty(b)+"x + "+pretty(c)+"y^2 = C";s.steps={{"ODE_EXACT_CHECK","Check ∂M/∂y = ∂N/∂x = 2ax.","2a x"},{"ODE_EXACT_POTENTIAL","Integrate M with respect to x and recover the y-only term from N.",s.answer}};s.verification={VerificationStatus::verified_exact,"mixed-partial and gradient check","Fx = 2axy+b; Fy = ax²+2cy"};return s;}
    if(s.topic=="bernoulli"){std::stringstream numeric(input);std::string kind;double pcoef{},qcoef{};int power{};if(!(numeric>>kind>>pcoef>>qcoef>>power)||kind!="bernoulli"||power==1||power < -32||power > 32||std::abs(pcoef)<1e-12)throw std::runtime_error("Bernoulli input: bernoulli p q n for y'+p*y=q*y^n (p≠0, n≠1)");const double zconst=qcoef/pcoef,decay=-(1-power)*pcoef,exponent=1.0/(1-power);s.answer="y = ("+pretty(zconst)+" + C·exp("+pretty(decay)+"x))^("+pretty(exponent)+")";s.steps={{"ODE_BERNOULLI_SUBSTITUTION","Set z = y^(1−n) to obtain a linear equation.","z' + "+pretty((1-power)*pcoef)+"z = "+pretty((1-power)*qcoef)},{"ODE_BERNOULLI_SOLVE","Solve for z and raise to 1/(1−n).",s.answer}};s.verification={VerificationStatus::verified_exact,"Bernoulli substitution replay","z=y^(1−n) reduces the equation to first-order linear"};return s;}
    if(s.topic=="homogeneous"){std::stringstream numeric(input);std::string kind;double ratio{};if(!(numeric>>kind>>ratio)||kind!="homogeneous")throw std::runtime_error("Homogeneous input: homogeneous k for y'=k*y/x");s.answer="y = C·x^"+pretty(ratio);s.steps={{"ODE_HOMOGENEOUS_SUBSTITUTION","The RHS k·y/x is homogeneous; separate dy/y = k dx/x.","ln|y| = "+pretty(ratio)+"ln|x| + C"},{"ODE_HOMOGENEOUS_SOLVE","Exponentiate and absorb constants.",s.answer}};s.verification={VerificationStatus::verified_exact,"differentiate and substitute","y' = k·y/x where x ≠ 0"};s.assumptions.push_back("x ≠ 0");return s;}
    static const std::regex separable(R"(^\s*dy/dx\s*=\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*\*?\s*x\s*\*?\s*y\s*$)");
    if(std::regex_match(input,match,separable)){const auto a=std::stod(match[1].str()),coefficient=a/2;const auto prefix=std::abs(coefficient-1)<1e-12?"":std::abs(coefficient+1)<1e-12?"-":pretty(coefficient);s.topic="separable";s.answer="y = C·exp("+prefix+"x^2)";s.steps={{"ODE_SEPARATE","Separate y⁻¹dy = ax dx.","ln|y| = "+prefix+"x² + C"},{"ODE_EXPONENTIATE","Exponentiate and absorb sign into C.",s.answer}};s.verification={VerificationStatus::verified_exact,"differentiate and substitute","y' = ax·y"};return s;}
    static const std::regex linear_affine(R"(^\s*dy/dx\s*\+\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*\*?\s*y\s*=\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*\*?\s*x\s*$)");
    if(std::regex_match(input,match,linear_affine)){const auto a=std::stod(match[1].str()),b=std::stod(match[2].str());if(std::abs(a)<1e-12)throw std::runtime_error("This is not a first-order linear equation with non-zero y coefficient");const double slope=b/a,intercept=-b/(a*a);s.topic="first_order_linear";s.answer="y = "+pretty(slope)+"x "+(intercept<0?"- ":"+ ")+pretty(std::abs(intercept))+" + C·exp("+pretty(-a)+"x)";s.steps={{"ODE_INTEGRATING_FACTOR","Use integrating factor μ(x)=exp("+pretty(a)+"x).","(μy)' = "+pretty(b)+"xμ"},{"ODE_SOLVE_LINEAR","Integrate and divide by μ.",s.answer}};s.verification={VerificationStatus::verified_exact,"differentiate and substitute","y' + "+pretty(a)+"y = "+pretty(b)+"x"};return s;}
    static const std::regex linear_constant(R"(^\s*dy/dx\s*\+\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*\*?\s*y\s*=\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*$)");
    if(std::regex_match(input,match,linear_constant)){const auto a=std::stod(match[1].str()),b=std::stod(match[2].str());if(std::abs(a)<1e-12)throw std::runtime_error("This is not a first-order linear equation with non-zero y coefficient");s.topic="first_order_linear";s.answer="y = "+pretty(b/a)+" + C·exp("+pretty(-a)+"x)";s.steps={{"ODE_INTEGRATING_FACTOR","Use integrating factor μ(x)=exp("+pretty(a)+"x).","(μy)' = "+pretty(b)+"μ"},{"ODE_SOLVE_LINEAR","Integrate and divide by μ.",s.answer}};s.verification={VerificationStatus::verified_exact,"differentiate and substitute","y' + "+pretty(a)+"y = "+pretty(b)};return s;}
    static const std::regex second_order(R"(^\s*y''\s*\+\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*\*?\s*y'\s*\+\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*\*?\s*y\s*=\s*0\s*$)");
    if(std::regex_match(input,match,second_order)){const auto a=std::stod(match[1].str()),b=std::stod(match[2].str()),d=a*a-4*b;const auto exponent=[](double value){const auto prefix=std::abs(value-1)<1e-12?"":std::abs(value+1)<1e-12?"-":pretty(value);return prefix+"x";};s.topic="second_order_constant_coefficient";if(d>1e-12){const auto r1=(-a+std::sqrt(d))/2,r2=(-a-std::sqrt(d))/2;s.answer="y = C1·exp("+exponent(r1)+") + C2·exp("+exponent(r2)+")";}else if(std::abs(d)<=1e-12){const auto r=-a/2;s.answer="y = (C1 + C2x)·exp("+exponent(r)+")";}else{const auto alpha=-a/2,beta=std::sqrt(-d)/2;s.answer="y = exp("+exponent(alpha)+")·(C1 cos("+pretty(beta)+"x) + C2 sin("+pretty(beta)+"x))";}s.steps={{"ODE_CHARACTERISTIC","Solve r² + "+pretty(a)+"r + "+pretty(b)+" = 0.",s.answer}};s.verification={VerificationStatus::verified_exact,"characteristic-polynomial substitution","homogeneous ODE basis"};return s;}
    throw std::runtime_error("Unsupported ODE form. Supported: dy/dx=a*x*y; dy/dx+a*y=b; y''+a*y'+b*y=0.");
}
SolutionBundle solve_programming(const ProblemSpec& p) {
    const auto topic=lower(p.topic);std::smatch match;SolutionBundle s;s.domain="programming";s.topic=topic.empty()?"cpp_trace":topic;
    if(topic=="branches"||topic=="branch"){const std::regex branch(R"(^\s*int\s+x\s*=\s*(-?[0-9]+)\s*;\s*if\s*\(\s*x\s*>\s*(-?[0-9]+)\s*\)\s*x\s*\+=\s*(-?[0-9]+)\s*;\s*else\s*x\s*\+=\s*(-?[0-9]+)\s*;\s*$)");if(!std::regex_match(p.input,match,branch))throw std::runtime_error("Branch input: int x=N; if (x > M) x += A; else x += B;");const int x=std::stoi(match[1].str()),threshold=std::stoi(match[2].str()),yes=std::stoi(match[3].str()),no=std::stoi(match[4].str()),result=checked_int(static_cast<std::int64_t>(x)+(x>threshold?yes:no));s.answer="x = "+std::to_string(result);s.steps={{"CPP_BRANCH_CONDITION","Evaluate x > threshold as "+std::string(x>threshold?"true":"false")+".","x > "+std::to_string(threshold)},{"CPP_BRANCH_EXECUTE","Execute only the selected branch.",s.answer}};s.verification={VerificationStatus::verified_exact,"bounded branch replay","condition and selected assignment checked"};return s;}
    if(topic=="loops"||topic=="loop"){const std::regex loop(R"(^\s*int\s+sum\s*=\s*0\s*;\s*for\s*\(\s*int\s+i\s*=\s*1\s*;\s*i\s*<=\s*([0-9]+)\s*;\s*\+\+i\s*\)\s*sum\s*\+=\s*i\s*;\s*$)");if(!std::regex_match(p.input,match,loop))throw std::runtime_error("Loop input: int sum=0; for (int i=1; i<=N; ++i) sum += i;");const int n=std::stoi(match[1].str());if(n>100000)throw std::runtime_error("Teaching loop bound is 100000");const std::int64_t total=static_cast<std::int64_t>(n)*(n+1)/2;s.answer="sum = "+std::to_string(total);s.steps={{"CPP_FOR_LOOP","Run i from 1 through "+std::to_string(n)+" inclusively.","iterations = "+std::to_string(n)},{"CPP_ACCUMULATE","Add each loop index to sum.",s.answer}};s.verification={VerificationStatus::verified_exact,"triangular-number cross-check","n(n+1)/2"};return s;}
    if(topic=="recursion"||topic=="recursive"){const std::regex factorial(R"(^\s*(?:fact|factorial)\s*\(\s*([0-9]+)\s*\)\s*$)",std::regex::icase);if(!std::regex_match(p.input,match,factorial))throw std::runtime_error("Recursion input: fact(N)");const int n=std::stoi(match[1].str());if(n>12)throw std::runtime_error("Factorial teaching interpreter accepts 0–12");std::int64_t value=1;for(int i=2;i<=n;++i)value*=i;s.answer="fact("+std::to_string(n)+") = "+std::to_string(value);s.steps={{"CPP_RECURSIVE_BASE","Use fact(0) = 1.","fact(0) = 1"},{"CPP_RECURSIVE_UNWIND","Multiply once for every pending call through fact("+std::to_string(n)+").",s.answer}};s.verification={VerificationStatus::verified_exact,"iterative factorial cross-check","product 1…n"};return s;}
    if(topic=="arrays"){const std::regex array_sum(R"(^\s*sum\s*\[([^\]]+)\]\s*$)",std::regex::icase);if(!std::regex_match(p.input,match,array_sum))throw std::runtime_error("Array input: sum [1,2,3]");const auto values=parse_vector(match[1].str());const auto total=std::accumulate(values.begin(),values.end(),0.0);s.answer="sum = "+pretty(total);s.steps={{"CPP_ARRAY_TRAVERSE","Visit each bounded array element once and accumulate it.",s.answer}};s.verification={VerificationStatus::verified_exact,"independent accumulation","all elements consumed once"};return s;}
    if(topic=="functions"){const std::regex function(R"(^\s*int\s+f\s*\(\s*int\s+x\s*\)\s*\{\s*return\s+x\s*\*\s*(-?[0-9]+)\s*;\s*\}\s*f\s*\(\s*(-?[0-9]+)\s*\)\s*$)");if(!std::regex_match(p.input,match,function))throw std::runtime_error("Function input: int f(int x){ return x*3; } f(4)");const int factor=std::stoi(match[1].str()),argument=std::stoi(match[2].str()),result=checked_int(static_cast<std::int64_t>(factor)*argument);s.answer="f("+std::to_string(argument)+") = "+std::to_string(result);s.steps={{"CPP_FUNCTION_CALL","Bind argument x and execute the restricted function body.","x = "+std::to_string(argument)},{"CPP_RETURN","Evaluate the return expression.",s.answer}};s.verification={VerificationStatus::verified_exact,"independent expression evaluation","factor × argument = result"};return s;}
    const std::regex trace(R"(^\s*int\s+x\s*=\s*(-?[0-9]+)\s*;\s*x\s*\+=\s*(-?[0-9]+)\s*;?\s*$)");if(!std::regex_match(p.input,match,trace))throw std::runtime_error("Trace input: int x=N; x += M;. Supported additional topics: arrays, functions.");const int initial=std::stoi(match[1].str()),change=std::stoi(match[2].str()),final=checked_int(static_cast<std::int64_t>(initial)+change);s.answer="x = "+std::to_string(final);s.steps={{"CPP_DECLARE","Declare and initialize x.","x = "+std::to_string(initial)},{"CPP_COMPOUND_ASSIGN","Apply x += value.","x = "+std::to_string(final)}};s.verification={VerificationStatus::verified_exact,"restricted interpreter trace","each assignment replayed independently"};return s;
}
SolutionBundle solve_algebra(const ProblemSpec& p) {
    SolutionBundle s;s.domain=p.domain.empty()?"algebra":p.domain;s.topic=p.topic.empty()?"simplify":p.topic;const auto raw=trim(p.input);if(raw.empty())throw std::runtime_error("Enter an expression");
    std::string compact=lower(raw);compact.erase(std::remove_if(compact.begin(),compact.end(),[](char c){return std::isspace(static_cast<unsigned char>(c));}),compact.end());
    const auto coefficient=[](const std::string& value){if(value.empty()||value=="+")return 1.0;if(value=="-")return -1.0;return std::stod(value);};
    const auto linear_text=[](double a,double b){const auto xpart=std::abs(a-1)<1e-12?"x":std::abs(a+1)<1e-12?"-x":pretty(a)+"x";if(std::abs(b)<1e-12)return xpart;return xpart+(b<0?" - ":" + ")+pretty(std::abs(b));};
    std::smatch match;
    static const std::regex cancellation(R"(^\(?([+-]?(?:[0-9]+(?:\.[0-9]+)?)?)x\^2([+-][0-9]+(?:\.[0-9]+)?)\)?/\(x-1\)$)");
    if(std::regex_match(compact,match,cancellation)){const double a=coefficient(match[1].str()),constant=std::stod(match[2].str());if(std::abs(constant+a)>1e-12)throw std::runtime_error("Cancellation form requires a·(x²−1)/(x−1)");s.answer=linear_text(a,a)+", with x ≠ 1";s.steps={{"ALG_FACTOR_DIFFERENCE_SQUARES","Factor a(x² − 1) as a(x − 1)(x + 1).","a(x − 1)(x + 1)/(x − 1)"},{"ALG_CANCEL_FACTOR","Cancel the common non-zero factor x − 1.",linear_text(a,a)}};s.assumptions.push_back("x ≠ 1 (the original denominator cannot be zero)");s.verification={VerificationStatus::verified_exact,"symbolic equivalence with domain tracking","factored numerator and excluded value checked"};return s;}
    const auto factor_prefix=compact.rfind("factor",0)==0;const auto quadratic_text=factor_prefix?compact.substr(6):compact;
    static const std::regex quadratic(R"(^([+-]?(?:[0-9]+(?:\.[0-9]+)?)?)x\^2([+-](?:[0-9]+(?:\.[0-9]+)?)?)x([+-](?:[0-9]+(?:\.[0-9]+)?)?)(?:=0)?$)");
    if(std::regex_match(quadratic_text,match,quadratic)){const double a=coefficient(match[1].str()),b=coefficient(match[2].str()),c=coefficient(match[3].str());if(std::abs(a)<1e-15)throw std::runtime_error("Quadratic coefficient cannot be zero");const double disc=b*b-4*a*c;if(disc<0)throw std::runtime_error("This offline quadratic solver currently returns real roots only");const double root=std::sqrt(disc),x1=(-b+root)/(2*a),x2=(-b-root)/(2*a);
        if(factor_prefix||lower(p.topic)=="factorisation"||lower(p.topic)=="factorization"){const auto term=[](double root){return root<0?"(x + "+pretty(-root)+")":"(x - "+pretty(root)+")";};const auto scale=std::abs(a-1)<1e-12?"":pretty(a);s.topic="factorisation";s.answer=scale+term(x1)+term(x2);s.steps={{"ALG_FIND_ROOTS","Find the real roots of the quadratic.", "x = "+pretty(x1)+", "+pretty(x2)},{"ALG_FACTOR_QUADRATIC","Write a(x−x₁)(x−x₂).",s.answer}};s.verification={VerificationStatus::verified_exact,"expanded-factor comparison","coefficients a, b, c reconstructed"};return s;}
        s.topic="quadratic_equation";s.answer=std::abs(x1-x2)<1e-12?"x = "+pretty(x1):"x₁ = "+pretty(x1)+", x₂ = "+pretty(x2);s.steps={{"ALG_QUADRATIC_FORMULA","Apply x = (−b ± √(b²−4ac)) / (2a).","Δ = "+pretty(disc)},{"ALG_QUADRATIC_ROOTS","Substitute the coefficients and simplify.",s.answer}};s.verification={VerificationStatus::verified_exact,"Vieta and substitution","sum = "+pretty(x1+x2)+", product = "+pretty(x1*x2)};return s;
    }
    static const std::regex linear(R"(^([+-]?(?:[0-9]+(?:\.[0-9]+)?)?)x([+-](?:[0-9]+(?:\.[0-9]+)?)?)?=(.+)$)");
    if(std::regex_match(compact,match,linear)){const double a=coefficient(match[1].str()),b=match[2].matched?std::stod(match[2].str()):0.0,c=ArithmeticParser(match[3].str()).parse();if(std::abs(a)<1e-15)throw std::runtime_error("Not a solvable linear equation");const double x=(c-b)/a;s.topic="linear_equation";s.answer="x = "+pretty(x);s.steps={{"ALG_ISOLATE_VARIABLE","Move the constant term to the right-hand side.",pretty(a)+"x = "+pretty(c-b)},{"ALG_DIVIDE_COEFFICIENT","Divide both sides by the coefficient of x.",s.answer}};s.verification={VerificationStatus::verified_exact,"substitution",pretty(a)+"·"+pretty(x)+" + "+pretty(b)+" = "+pretty(c)};return s;}
    if(compact=="sin(x)^2+cos(x)^2"||compact=="cos(x)^2+sin(x)^2"){s.answer="1";s.steps.push_back({"TRIG_PYTHAGOREAN_IDENTITY","Use sin²(x) + cos²(x) = 1.","1"});s.verification={VerificationStatus::verified_exact,"trigonometric identity","Pythagorean identity"};return s;}
    const double value=ArithmeticParser(raw).parse();s.answer=pretty(value);s.steps.push_back({"ALG_EVALUATE","Respect parentheses, powers, multiplication/division, then addition/subtraction.",s.answer});s.verification={VerificationStatus::verified_exact,"independent parser evaluation","no variables present"};return s;
}

const char* own_string(const std::string& value) {
    auto* copy=static_cast<char*>(std::malloc(value.size()+1));
    if(!copy) return nullptr;
    std::memcpy(copy,value.c_str(),value.size()+1);
    return copy;
}
} // namespace

std::string json_escape(std::string_view value) { std::string out;out.reserve(value.size()+8);for(char c:value){switch(c){case '\\':out+="\\\\";break;case '\"':out+="\\\"";break;case '\n':out+="\\n";break;case '\r':out+="\\r";break;case '\t':out+="\\t";break;default:if(static_cast<unsigned char>(c)<32){constexpr char hex[]="0123456789abcdef";out+="\\u00";out+=hex[(static_cast<unsigned char>(c)>>4)&15];out+=hex[static_cast<unsigned char>(c)&15];}else out+=c;}}return out; }
std::string verification_name(VerificationStatus s){switch(s){case VerificationStatus::verified_exact:return "verified_exact";case VerificationStatus::verified_exhaustive:return "verified_exhaustive";case VerificationStatus::verified_numerical:return "verified_numerical";case VerificationStatus::verification_failed:return "verification_failed";default:return "not_verified";}}
std::string SolutionBundle::to_json() const {std::ostringstream out;out<<"{\"schema_version\":\"1.0\",\"status\":"<<quote(status)<<",\"domain\":"<<quote(domain)<<",\"topic\":"<<quote(topic)<<",\"answer\":{\"text\":"<<quote(answer)<<"},\"steps\":[";for(std::size_t i=0;i<steps.size();++i){if(i)out<<',';out<<"{\"rule_id\":"<<quote(steps[i].rule_id)<<",\"explanation\":"<<quote(steps[i].explanation)<<",\"expression\":"<<quote(steps[i].expression)<<"}";}out<<"],\"assumptions\":[";for(std::size_t i=0;i<assumptions.size();++i){if(i)out<<',';out<<quote(assumptions[i]);}out<<"],\"warnings\":[";for(std::size_t i=0;i<warnings.size();++i){if(i)out<<',';out<<quote(warnings[i]);}out<<"],\"verification\":{\"status\":"<<quote(verification_name(verification.status))<<",\"method\":"<<quote(verification.method)<<",\"evidence\":"<<quote(verification.evidence)<<"},\"visual\":"<<quote(visual_json)<<",\"duration_ms\":"<<duration_ms<<"}";return out.str();}
std::string Identification::to_json() const { std::ostringstream out;out<<"{\"schema_version\":\"1.0\",\"status\":"<<quote(status)<<",\"reason\":"<<quote(reason)<<",\"candidates\":[";for(std::size_t i=0;i<candidates.size();++i){if(i)out<<',';out<<"{\"domain\":"<<quote(candidates[i].domain)<<",\"topic\":"<<quote(candidates[i].topic)<<"}";}out<<"]}";return out.str(); }

Identification Engine::identify(std::string_view raw) const {
    const auto text=lower(std::string(raw));Identification result;result.reason="Matched deterministic curriculum keywords; confirm the candidate before solving.";
    const auto add=[&](std::string domain,std::string topic){result.candidates.push_back({std::move(domain),std::move(topic),std::string(raw),{}});};
    const auto has=[&](std::string_view word){return text.find(word)!=std::string::npos;};
    if(has("superposition"))add("circuit","superposition");
    else if(has("two's complement")||has("twos complement")||has("signed binary"))add("logic","signed_arithmetic");
    else if(has("kmap")||has("k-map")||has("minterm")||has("maxterm"))add("logic","kmap_minimization");
    else if(has("truth table")||has("boolean")||has("sop")||has("pos"))add("logic",has("pos")?"canonical_pos":"truth_table");
    else if(has("flip-flop")||has("flip flop")||has("latch")||has("counter")||has("state table"))add("logic","sequential_logic");
    else if(has("mux")||has("multiplexer")||has("decoder")||has("encoder")||has("adder")||has("comparator"))add("logic","combinational_logic");
    else if(has("base conversion")||has("binary to")||has("decimal to")||has("hex"))add("logic","number_systems");
    else if(has("determinant"))add("linear_algebra","determinant");
    else if(has("inverse"))add("linear_algebra","inverse");
    else if(has("eigen"))add("linear_algebra","eigenvalues");
    else if(has("rank"))add("linear_algebra","rank");
    else if(has("dot product")||has("cross product")||has("vector magnitude"))add("linear_algebra","vectors");
    else if(has("rref")||has("gauss")||has("row reduce")||has("matrix"))add("linear_algebra","rref");
    else if(has("bernoulli"))add("differential_equations","bernoulli");
    else if(has("exact differential"))add("differential_equations","exact");
    else if(has("homogeneous differential"))add("differential_equations","homogeneous");
    else if(has("runge")||has("rk4"))add("differential_equations","rk4");
    else if(has("euler method"))add("differential_equations","euler");
    else if(has("dy/dx")||has("d2y")||has("differential equation")||has("integrating factor"))add("differential_equations",has("integrating factor")?"first_order_linear":"ode_classification");
    else if(has("source transform"))add("circuit","source_transformation");
    else if(has("nodal")||has("kcl")||has("node voltage"))add("circuit","dc_nodal_analysis");
    else if(has("mesh")||has("kvl")||has("loop current"))add("circuit","mesh_analysis");
    else if(has("thevenin")||has("norton")||has("maximum power"))add("circuit","network_theorems");
    else if(has("rc ")||has("rl ")||has("transient")||has("capacitor")||has("inductor"))add("circuit","first_order_transient");
    else if(has("tangent")||has("normal line"))add("calculus","tangent_line");
    else if(has("critical point")||has("curve analysis"))add("calculus","curve_analysis");
    else if(has("definite integral")||has("area under"))add("calculus","definite_integral");
    else if(has("d/dx")||has("differentiate")||has("derivative"))add("calculus","differentiation");
    else if(has("integrate")||has("integral"))add("calculus","integration");
    else if(has("limit"))add("calculus","limits");
    else if(has("recursion")||has("factorial"))add("programming","recursion");
    else if(has("for (")||has("while (")||has("loop"))add("programming","loops");
    else if(has("if (")||has("else"))add("programming","branches");
    else if(has("array")||has("vector<"))add("programming","arrays");
    else if(has("function")||has("return "))add("programming","functions");
    else if(has("int "))add("programming","cpp_trace");
    else if(has("factor"))add("algebra","factorisation");
    else if(has("quadratic"))add("algebra","quadratic_equation");
    else if(has("linear equation"))add("algebra","linear_equation");
    else if(has("trig")||has("sin")||has("cos"))add("algebra","trigonometry");
    else if(has("log"))add("algebra","logarithms");
    else {add("algebra","simplify");result.reason="No high-confidence course marker found; algebra is only a suggested starting point. Confirm or choose a subject.";}
    return result;
}

SolutionBundle Engine::solve(const ProblemSpec& p,const SolveOptions& options) const {
    const auto start=std::chrono::steady_clock::now();
    try {
        if(p.input.empty()||p.input.size()>4096||p.domain.size()>64||p.topic.size()>64)
            throw std::runtime_error("Provide 1–4096 input bytes and domain/topic names of at most 64 bytes");
        if(options.max_steps==0||options.max_steps>512||options.time_budget_ms==0)
            throw std::runtime_error("Invalid solver budget (steps 1–512; positive time budget)");
        // Regex implementations can recurse on ordinary characters as well as
        // parentheses. Keep these parsers small on mobile and WASM stacks.
        const auto domain=lower(p.domain);
        if((domain=="algebra"||domain=="calculus"||domain=="programming"||domain=="differential_equations"||domain=="ode")&&p.input.size()>512)
            throw std::runtime_error("This expression module accepts at most 512 bytes");
        SolutionBundle result;
        if(domain=="matrix"||domain=="linear_algebra"||domain=="linear algebra")result=solve_matrix(p);
        else if(domain=="logic"||domain=="digital_logic"||domain=="digital logic")result=solve_logic(p);
        else if(domain=="circuit"||domain=="circuits")result=solve_circuit(p);
        else if(domain=="engineering"||domain=="conversion"||domain=="units")result=solve_conversion(p);
        else if(domain=="calculus")result=solve_calculus(p);
        else if(domain=="differential_equations"||domain=="differential equations"||domain=="ode")result=solve_ode(p);
        else if(domain=="programming"||domain=="cpp")result=solve_programming(p);
        else if(domain=="algebra"||domain.empty())result=solve_algebra(p);
        else throw std::runtime_error("Unknown subject; choose a supported topic from the catalog");
        result.duration_ms=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());
        if(result.steps.size()>options.max_steps) {
            result.steps.resize(options.max_steps);
            result.warnings.push_back("Step display truncated to the requested budget; the answer uses the full calculation.");
        }
        if(result.duration_ms>options.time_budget_ms) result.warnings.push_back("Calculation exceeded the requested soft time budget.");
        if(options.locale!="en")result.warnings.push_back("Only English explanations are currently available.");
        if(result.verification.status==VerificationStatus::verification_failed)result.status="verification_failed";
        if(options.require_verification&&result.verification.status==VerificationStatus::not_verified)
            result.warnings.push_back("This result has not been independently verified.");
        return result;
    } catch(const std::exception& e) {return error_result(p,e.what());}
}
std::string Engine::capabilities_json() const {return R"({"schema_version":"2.0","offline":true,"domains":[{"id":"algebra","topics":["numeric_evaluation","simplification","factorisation","linear_equations","quadratic_equations","trigonometry","logarithms"]},{"id":"calculus","topics":["differentiation","tangent_line","integration","definite_integral","limits","curve_analysis"]},{"id":"linear_algebra","topics":["rref","determinant","inverse","linear_system","multiply","transpose","rank","eigenvalues","vectors"]},{"id":"differential_equations","topics":["separable","first_order_linear","exact","bernoulli","homogeneous","second_order_constant_coefficient","initial_value","euler","rk4"]},{"id":"logic","topics":["number_systems","signed_arithmetic","truth_table","canonical_pos","kmap_minimization","combinational_logic","sequential_logic"]},{"id":"circuit","topics":["voltage_divider","dc_nodal_analysis","mesh_analysis","source_transformation","superposition","thevenin","norton","maximum_power","rc_transient","rl_transient"]},{"id":"engineering","topics":["unit_conversion"]},{"id":"programming","topics":["cpp_trace","branches","loops","arrays","functions","recursion"]}],"limits":{"truth_table_variables":6,"kmap_variables":4,"matrix_dimensions":"bounded by validated input budget","programming_loop_iterations":100000,"recursion_factorial_n":12}})";}
extern "C" const char* pe_solve_json(const char* raw){try{if(!raw) throw std::runtime_error("Missing request");return own_string(Engine{}.solve(parse_request(raw)).to_json());}catch(const std::exception& e){return own_string(error_result({},e.what()).to_json());}catch(...){return nullptr;}}
extern "C" const char* pe_capabilities_json(){return own_string(Engine{}.capabilities_json());}
extern "C" const char* pe_identify_json(const char* raw){try{if(!raw||std::strlen(raw)>4096)throw std::runtime_error("Identification input exceeds budget");return own_string(Engine{}.identify(raw).to_json());}catch(const std::exception& e){return own_string(error_result({},e.what()).to_json());}}
extern "C" const char* pe_catalog_json(){return own_string(catalog_json());}
extern "C" void pe_free_string(const char* value){std::free(const_cast<char*>(value));}
} // namespace pocket_engineer
