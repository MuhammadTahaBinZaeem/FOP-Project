#include "pocket_engineer/input.hpp"
#include "pocket_engineer/polynomial.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

namespace pocket_engineer {
namespace {
std::string trim(std::string s){const auto a=s.find_first_not_of(" \t\r\n");return a==std::string::npos?"":s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
std::string lower(std::string s){for(char& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
bool wordchar(char c){return std::isalnum(static_cast<unsigned char>(c))||c=='_';}
std::size_t phrase(const std::string& text,const std::string& part){
    std::size_t at=0;while((at=text.find(part,at))!=std::string::npos){const auto end=at+part.size();if((at==0||!wordchar(text[at-1]))&&(end==text.size()||!wordchar(text[end])))return at;++at;}return std::string::npos;
}
void replace(std::string& text,const std::string& from,const std::string& to){std::size_t at=0;while((at=text.find(from,at))!=std::string::npos){text.replace(at,from.size(),to);at+=to.size();}}
void words(std::string& text,const std::string& from,const std::string& to){std::size_t at;while((at=phrase(lower(text),from))!=std::string::npos)text.replace(at,from.size(),to);}
std::string number(double x){if(!std::isfinite(x))throw std::runtime_error("Input quantity is outside the finite range");std::ostringstream out;out<<std::setprecision(15)<<x;return out.str();}
std::string unicode(std::string s){
    for(const auto& [a,b]:std::vector<std::pair<std::string,std::string>>{{"−","-"},{"–","-"},{"×","*"},{"·","*"},{"÷","/"},{"π","pi"},{"θ","theta"},{"Ω","Ohm"},{"µ","u"},{"μ","u"},{"’","'"},{"′","'"},{"″","''"},{"¬","!"},{"∧","&"},{"∨","|"},{"⊕","^"},{"\xC2\xA0"," "},{"\\cdot","*"},{"\\times","*"},{"\\pi","pi"},{"\\left",""},{"\\right",""}})replace(s,a,b);
    const std::vector<std::pair<std::string,char>> supers{{"⁰",'0'},{"¹",'1'},{"²",'2'},{"³",'3'},{"⁴",'4'},{"⁵",'5'},{"⁶",'6'},{"⁷",'7'},{"⁸",'8'},{"⁹",'9'},{"⁻",'-'}};
    std::string out;bool power=false;
    for(std::size_t i=0;i<s.size();){bool found=false;for(const auto& [symbol,digit]:supers)if(s.compare(i,symbol.size(),symbol)==0){if(!power)out+='^';out+=digit;i+=symbol.size();power=true;found=true;break;}if(!found){out+=s[i++];power=false;}}
    replace(out,"**","^");return out;
}
std::string polite(std::string s){
    s=trim(s);while(!s.empty()&&s.back()=='?')s.pop_back();
    const std::vector<std::string> prefixes{"please ","can you ","could you ","would you ","help me ","what is ","what's ","what are ","find ","calculate ","compute ","evaluate ","give me ","show me ","solve for x:","solve for x ","solve ","use ","the following ","this ","the "};
    bool changed=true;while(changed){changed=false;const auto l=lower(s);for(const auto& p:prefixes)if(l.starts_with(p)){s=trim(s.substr(p.size()));changed=true;break;}}
    for(const auto& suffix:{" step by step"," with steps"," please"," thanks",", solve for x"," with respect to x"})if(lower(s).ends_with(suffix))s=trim(s.substr(0,s.size()-std::string(suffix).size()));
    static const std::regex equationPrefix(R"(^(?:(?:for\s+[a-z]\s*:?\s*)|(?:(?:equation|equations|value)\s*(?:of\s+)?\s*:?\s+)))",std::regex::icase);s=std::regex_replace(s,equationPrefix,"");
    return s;
}
std::string fillers(std::string s){
    s=trim(s);bool changed=true;while(changed){changed=false;for(const auto& p:{"of ","the ","following ","this ","matrix ","expression ","polynomial ","equation ","function ","to solve ","to simplify ","with ","for ",":","="})if(lower(s).starts_with(p)){s=trim(s.substr(std::string(p).size()));changed=true;break;}}
    return s;
}
// Only complete number words are converted; arbitrary prose remains in the
// expression and will fail validation rather than having its numbers extracted.
std::string number_words(std::string text){
    static const std::map<std::string,int> small{{"zero",0},{"one",1},{"two",2},{"three",3},{"four",4},{"five",5},{"six",6},{"seven",7},{"eight",8},{"nine",9},{"ten",10},{"eleven",11},{"twelve",12},{"thirteen",13},{"fourteen",14},{"fifteen",15},{"sixteen",16},{"seventeen",17},{"eighteen",18},{"nineteen",19},{"twenty",20},{"thirty",30},{"forty",40},{"fifty",50},{"sixty",60},{"seventy",70},{"eighty",80},{"ninety",90}};
    std::string out;std::size_t i=0;
    while(i<text.size()){
        if(!std::isalpha(static_cast<unsigned char>(text[i]))){out+=text[i++];continue;}
        const auto start=i;while(i<text.size()&&std::isalpha(static_cast<unsigned char>(text[i])))++i;
        const auto first=lower(text.substr(start,i-start));if(!small.contains(first)){out+=text.substr(start,i-start);continue;}
        long long group=small.at(first),total=0;std::size_t end=i;bool scale=false;
        while(end<text.size()){
            std::size_t next=end;while(next<text.size()&&(text[next]==' '||text[next]=='-'))++next;
            const auto begin=next;while(next<text.size()&&std::isalpha(static_cast<unsigned char>(text[next])))++next;
            const auto token=lower(text.substr(begin,next-begin));
            if(token=="hundred"){group=(group==0?1:group)*100;scale=true;}
            else if(token=="thousand"||token=="million"){total+=group*(token=="thousand"?1000:1000000);group=0;scale=true;}
            else if(small.contains(token)&&((group%100>=20&&small.at(token)<10)||scale)){group+=small.at(token);scale=false;}
            else if(token=="and"&&scale){
                auto look=next;while(look<text.size()&&text[look]==' ')++look;const auto beginWord=look;while(look<text.size()&&std::isalpha(static_cast<unsigned char>(text[look])))++look;
                if(!small.contains(lower(text.substr(beginWord,look-beginWord))))break;
                end=next;continue;
            }
            else break;
            if(total+group>999999999)throw std::runtime_error("Spelled-out number exceeds 999999999");
            end=next;
        }
        out+=std::to_string(total+group);i=end;
    }
    return out;
}
bool enclosed(const std::string& s,char left,char right){if(s.size()<2||s.front()!=left||s.back()!=right)return false;int depth=0;for(std::size_t i=0;i<s.size();++i){if(s[i]==left)++depth;if(s[i]==right)--depth;if(depth==0&&i+1<s.size())return false;if(depth<0) return false;}return depth==0;}
std::string matrix(std::string text){
    text=fillers(trim(text));
    static const std::regex assignment(R"(^[A-Za-z][A-Za-z0-9_]*\s*=\s*)");text=std::regex_replace(text,assignment,"");
    replace(text,"\\begin{bmatrix}","");replace(text,"\\end{bmatrix}","");replace(text,"\\begin{pmatrix}","");replace(text,"\\end{pmatrix}","");replace(text,"\\\\",";");replace(text,"&",",");
    int depth=0;for(char c:text){if(c=='['&&++depth>3)throw std::runtime_error("Matrix brackets are nested too deeply");if(c==']'&&--depth<0)throw std::runtime_error("Unbalanced matrix brackets");}if(depth)throw std::runtime_error("Unbalanced matrix brackets");
    while(enclosed(text,'[',']'))text=trim(text.substr(1,text.size()-2));
    if(enclosed(text,'(',')')&&text.find(';')!=std::string::npos)text=trim(text.substr(1,text.size()-2));
    static const std::regex between(R"(\]\s*[,;]?\s*\[)");text=std::regex_replace(text,between,";");
    replace(text,"[","");replace(text,"]","");replace(text,"\r","");replace(text,"\n",";");
    while(text.find(";;")!=std::string::npos)replace(text,";;",";");
    text=trim(text);if(!text.empty()&&text.back()==';')text.pop_back();return text;
}
struct Affine {
    double constant=0;std::map<std::string,double> coefficients;
    void check(){if(!std::isfinite(constant))throw std::runtime_error("Linear expression overflow");for(auto it=coefficients.begin();it!=coefficients.end();){if(!std::isfinite(it->second))throw std::runtime_error("Linear coefficient overflow");if(it->second==0)it=coefficients.erase(it);else ++it;}}
    Affine scale(double factor)const{auto out=*this;out.constant*=factor;for(auto& [name,c]:out.coefficients)c*=factor;out.check();return out;}
    Affine plus(const Affine& rhs,double sign=1)const{auto out=*this;out.constant+=sign*rhs.constant;for(const auto&[name,c]:rhs.coefficients)out.coefficients[name]+=sign*c;out.check();return out;}
    Affine times(const Affine& rhs)const{if(!coefficients.empty()&&!rhs.coefficients.empty())throw std::runtime_error("This system contains nonlinear products; linear-system solving would be incorrect");return coefficients.empty()?rhs.scale(constant):scale(rhs.constant);}
};
class LinearParser {
    std::string s;std::size_t at=0;unsigned depth=0;
    void space(){while(at<s.size()&&std::isspace(static_cast<unsigned char>(s[at])))++at;}
    char peek(){space();return at<s.size()?s[at]:'\0';}
    bool take(char c){if(peek()==c){++at;return true;}return false;}
    Affine expression(){auto p=term();while(true){if(take('+'))p=p.plus(term());else if(take('-'))p=p.plus(term(),-1);else return p;}}
    Affine term(){auto p=unary();while(true){if(take('*'))p=p.times(unary());else if(take('/')){const auto q=unary();if(!q.coefficients.empty()||q.constant==0)throw std::runtime_error("Linear division requires a nonzero constant");p=p.scale(1/q.constant);}else if(peek()=='('||std::isalpha(static_cast<unsigned char>(peek())))p=p.times(unary());else return p;}}
    Affine unary(){if(++depth>32)throw std::runtime_error("Linear expression nesting exceeds 32");Affine p;if(take('+'))p=unary();else if(take('-'))p=unary().scale(-1);else p=primary();--depth;return p;}
    Affine primary(){
        if(take('(')){auto p=expression();if(!take(')'))throw std::runtime_error("Unclosed linear parenthesis");return p;}
        if(std::isalpha(static_cast<unsigned char>(peek()))){std::string name;while(at<s.size()&&std::isalnum(static_cast<unsigned char>(s[at])))name+=s[at++];if(name.size()>8)throw std::runtime_error("Use short variable names in a linear system");Affine p;p.coefficients[name]=1;return p;}
        space();const char* begin=s.c_str()+at;char* end=nullptr;const double value=std::strtod(begin,&end);if(end==begin||!std::isfinite(value))throw std::runtime_error("Expected a linear coefficient, variable or parenthesis");at+=static_cast<std::size_t>(end-begin);Affine p;p.constant=value;return p;
    }
public:
    explicit LinearParser(std::string text):s(std::move(text)){if(s.size()>512)throw std::runtime_error("Each linear equation side accepts at most 512 bytes");}
    Affine read(){auto p=expression();if(peek())throw std::runtime_error("Unsupported or nonlinear term near "+s.substr(at,20));return p;}
};
std::string equations(std::string text,std::vector<std::string>& names){
    text=fillers(text);replace(text,"\n",";");words(text,"and",";");replace(text,",",";");std::stringstream rows(text);std::string row;std::vector<Affine> all;std::set<std::string> variables;
    while(std::getline(rows,row,';')){row=trim(row);if(row.empty())continue;const auto equal=row.find('=');if(equal==std::string::npos||row.find('=',equal+1)!=std::string::npos)throw std::runtime_error("Each equation needs exactly one equals sign");const auto lhs=LinearParser(row.substr(0,equal)).read(),rhs=LinearParser(row.substr(equal+1)).read();for(const auto& [n,c]:lhs.coefficients)variables.insert(n);for(const auto& [n,c]:rhs.coefficients)variables.insert(n);all.push_back(lhs.plus(rhs,-1));}
    if(all.empty()||all.size()>16||variables.empty()||variables.size()>16)throw std::runtime_error("Provide 1–16 linear equations and 1–16 named unknowns");
    names.assign(variables.begin(),variables.end());std::string out;
    for(const auto& r:all){if(!out.empty())out+=';';for(const auto& n:names)out+=number(r.coefficients.contains(n)?r.coefficients.at(n):0)+',';out+=number(-r.constant);}return out;
}
struct Command {const char* words;const char* domain;const char* topic;};
// Long/specific tasks precede generic words. These are grammatical course
// markers, not fuzzy substring guesses (e.g. "position" is not "POS").
const std::vector<Command>& commands(){static const std::vector<Command> list{
    {"karnaugh","logic","kmap_minimization"},{"k-map","logic","kmap_minimization"},{"kmap","logic","kmap_minimization"},{"minterms","logic","kmap_minimization"},{"maxterms","logic","kmap_minimization"},
    {"truth table","logic","truth_table"},{"canonical pos","logic","canonical_pos"},{"product of sums","logic","canonical_pos"},{"boolean","logic","truth_table"},
    {"two's complement","logic","signed_arithmetic"},{"twos_add","logic","signed_arithmetic"},{"full adder","logic","combinational_logic"},{"full_adder","logic","combinational_logic"},{"mux4","logic","combinational_logic"},{"decoder2","logic","combinational_logic"},{"comparator","logic","combinational_logic"},
    {"jk flip flop","logic","sequential_logic"},{"jk flip-flop","logic","sequential_logic"},{"d flip flop","logic","sequential_logic"},{"t flip flop","logic","sequential_logic"},{"jkff","logic","sequential_logic"},{"dff","logic","sequential_logic"},{"tff","logic","sequential_logic"},
    {"runge-kutta","differential_equations","rk4"},{"runge kutta","differential_equations","rk4"},{"rk4","differential_equations","rk4"},{"euler","differential_equations","euler"},{"bernoulli","differential_equations","bernoulli"},{"homogeneous","differential_equations","homogeneous"},{"exact differential","differential_equations","exact"},{"exact","differential_equations","exact"},{"initial value","differential_equations","initial_value"},
    {"voltage divider","circuit","voltage_divider"},{"nodal analysis","circuit","dc_nodal_analysis"},{"kcl","circuit","dc_nodal_analysis"},{"mesh analysis","circuit","mesh_analysis"},{"mesh","circuit","mesh_analysis"},{"source transformation","circuit","source_transformation"},{"superposition","circuit","superposition"},{"maximum power","circuit","maximum_power"},{"thevenin","circuit","thevenin"},{"thévenin","circuit","thevenin"},{"norton","circuit","norton"},{"rc transient","circuit","rc_transient"},{"rl transient","circuit","rl_transient"},
    {"dot product","linear_algebra","vectors"},{"cross product","linear_algebra","vectors"},{"vector magnitude","linear_algebra","vectors"},{"magnitude","linear_algebra","vectors"},{"determinant","linear_algebra","determinant"},{"det","linear_algebra","determinant"},{"inverse","linear_algebra","inverse"},{"inv","linear_algebra","inverse"},{"eigenvalues","linear_algebra","eigenvalues"},{"transpose","linear_algebra","transpose"},{"rank","linear_algebra","rank"},{"rref","linear_algebra","rref"},{"row reduce","linear_algebra","rref"},{"row reduction","linear_algebra","rref"},{"multiply matrices","linear_algebra","multiply"},{"matrix multiplication","linear_algebra","multiply"},{"linear system","linear_algebra","linear_system"},{"system of equations","linear_algebra","linear_system"},{"simultaneous equations","linear_algebra","linear_system"},
    {"tangent of","algebra","trigonometry"},{"tangent line","calculus","tangent_line"},{"tangent","calculus","tangent_line"},{"curve analysis","calculus","curve_analysis"},{"definite integral","calculus","definite_integral"},{"area under","calculus","definite_integral"},{"differentiate","calculus","differentiation"},{"derivative","calculus","differentiation"},{"d/dx","calculus","differentiation"},{"integrate","calculus","integration"},{"integral","calculus","integration"},{"limit","calculus","limits"},
    {"factorial","programming","recursion"},{"fact","programming","recursion"},{"for loop","programming","loops"},{"sum array","programming","arrays"},{"sum the array","programming","arrays"},{"trace","programming","cpp_trace"},
    {"factorise","algebra","factorisation"},{"factorize","algebra","factorisation"},{"factor","algebra","factorisation"},{"simplify","algebra","simplify"},{"expand","algebra","simplify"},{"quadratic equation","algebra","quadratic_equation"},{"linear equation","algebra","linear_equation"},{"scientific calculator","algebra","numeric_evaluation"}
};return list;}
bool known(const ProblemSpec& p){return std::any_of(topic_catalog().begin(),topic_catalog().end(),[&](const auto& t){return t.domain==p.domain&&t.topic==p.topic;});}
std::string canonical_unit(std::string unit){
    unit=trim(unit);const auto l=lower(unit);
    static const std::map<std::string,std::string> names{{"ohm","Ohm"},{"ohms","Ohm"},{"kiloohm","kOhm"},{"kiloohms","kOhm"},{"kilohm","kOhm"},{"kilohms","kOhm"},{"megohm","MOhm"},{"megohms","MOhm"},{"megaohms","MOhm"},{"volt","V"},{"volts","V"},{"amp","A"},{"amps","A"},{"ampere","A"},{"amperes","A"},{"farad","F"},{"farads","F"},{"henry","H"},{"henries","H"},{"second","s"},{"seconds","s"},{"sec","s"},{"seconds","s"},{"hertz","Hz"},{"watt","W"},{"watts","W"},{"meter","m"},{"meters","m"},{"metre","m"},{"metres","m"},{"millivolts","mV"},{"milliamps","mA"},{"milliseconds","ms"},{"microseconds","us"},{"microfarads","uF"},{"nanofarads","nF"},{"picofarads","pF"},{"millihenries","mH"},{"microhenries","uH"},{"kilometers","km"},{"kilometres","km"}};
    if(names.contains(l))return names.at(l);
    for(const auto& base:{"ohms","ohm","volts","volt","farads","farad","henries","henry","seconds","second"})if(l.ends_with(base)&&unit.size()>std::string(base).size()){
        const auto prefix=unit.substr(0,unit.size()-std::string(base).size());
        const auto b=canonical_unit(base);if(prefix.size()==1)return (prefix=="K"?"k":prefix)+b;
    }
    return unit;
}
double quantity(std::string value,const std::string& expected){
    value=trim(value);const char* start=value.c_str();char* end=nullptr;double result=std::strtod(start,&end);if(end==start||!std::isfinite(result))throw std::runtime_error("Expected a finite numeric quantity");
    auto unit=canonical_unit(trim(value.substr(static_cast<std::size_t>(end-start))));if(unit.empty())return result;
    if(unit==expected)return result;
    static const std::map<char,double> prefixes{{'p',1e-12},{'n',1e-9},{'u',1e-6},{'m',1e-3},{'k',1e3},{'M',1e6},{'G',1e9}};
    if(unit.size()>1&&prefixes.contains(unit.front())&&unit.substr(1)==expected)return result*prefixes.at(unit.front());
    throw std::runtime_error("Quantity "+value+" must have units compatible with "+expected);
}
std::map<std::string,std::string> fields(std::string text){
    text=number_words(text);std::map<std::string,std::string> result;
    // A separator is mandatory, so a word followed by an unrelated number is
    // never interpreted as a named circuit quantity.
    static const std::regex field(R"(\b([A-Za-z][A-Za-z0-9_]*)\s*(?:=|:)\s*)");
    std::size_t consumed=0;
    for(std::sregex_iterator i(text.begin(),text.end(),field),end;i!=end;++i){
        const auto start=static_cast<std::size_t>(i->position());
        if(start<consumed)continue;
        const auto valueStart=start+static_cast<std::size_t>(i->length());
        char* valueEnd=nullptr;const double value=std::strtod(text.c_str()+valueStart,&valueEnd);
        if(valueEnd==text.c_str()+valueStart||!std::isfinite(value))throw std::runtime_error("Each named parameter needs a finite number");
        auto stop=static_cast<std::size_t>(valueEnd-text.c_str()),unitStart=stop;
        while(unitStart<text.size()&&std::isspace(static_cast<unsigned char>(text[unitStart])))++unitStart;
        auto unitEnd=unitStart;while(unitEnd<text.size()&&std::isalpha(static_cast<unsigned char>(text[unitEnd])))++unitEnd;
        auto after=unitEnd;while(after<text.size()&&std::isspace(static_cast<unsigned char>(text[after])))++after;
        const auto unit=text.substr(unitStart,unitEnd-unitStart);
        // A following label is not a unit (a=1 y0=2), and conjunctions are
        // separators, not dimensions. Unknown units still reach validation.
        if(!unit.empty()&&unit!="and"&&(after==text.size()||(text[after]!='='&&text[after]!=':'&&!std::isdigit(static_cast<unsigned char>(text[after])))))stop=unitEnd;
        if(!result.emplace(lower((*i)[1].str()),trim(text.substr(valueStart,stop-valueStart))).second)throw std::runtime_error("A parameter is supplied more than once");
        auto gap=lower(text.substr(consumed,start-consumed));words(gap,"and","");words(gap,"with","");words(gap,"given","");
        if(gap.find_first_not_of(" ,;:\t\r\n")!=std::string::npos)throw std::runtime_error("Unrecognized text between named parameters: "+trim(gap));
        consumed=stop;
    }
    if(consumed){auto rest=lower(text.substr(consumed));words(rest,"and","");if(rest.find_first_not_of(" ,;\t\r\n")!=std::string::npos)throw std::runtime_error("Unrecognized text after named parameters: "+trim(rest));}
    return result;
}
std::string numeric_tuple(std::string body,std::size_t count){
    replace(body,","," ");std::stringstream stream(body);double value;std::string out;
    for(std::size_t i=0;i<count;++i){if(!(stream>>value)||!std::isfinite(value))throw std::runtime_error("Expected exactly "+std::to_string(count)+" finite numeric parameters");if(i)out+=' ';out+=number(value);}
    std::string extra;if(stream>>extra)throw std::runtime_error("Unexpected trailing parameter or condition: "+extra);return out;
}
std::string named_values(const std::map<std::string,std::string>& supplied,const std::vector<std::pair<std::string,std::string>>& required){
    std::set<std::string> allowed;std::string out;
    for(const auto& [name,unit]:required){allowed.insert(name);if(!supplied.contains(name))throw std::runtime_error("Missing parameter "+name+". Supply named values such as Vin=12V, R1=1kOhm, R2=2kOhm.");if(!out.empty())out+=' ';out+=number(quantity(supplied.at(name),unit));}
    for(const auto&[name,value]:supplied)if(!allowed.contains(name))throw std::runtime_error("Unexpected parameter "+name+"; it must not be silently ignored");
    return out;
}
std::string circuit_input(ProblemSpec& p,const std::string& raw,std::string body){
    if(p.topic=="dc_nodal_analysis"){
        const auto start=body.find_first_of("VRvr");if(start==std::string::npos)return body;
        // The netlist grammar itself validates every token and component.
        replace(body,"\n",";");return body;
    }
    auto supplied=fields(body);std::string prefix;
    if(p.topic=="mesh_analysis")prefix="mesh";else if(p.topic=="source_transformation")prefix=phrase(lower(raw),"norton")!=std::string::npos?"norton":"thevenin";
    else if(p.topic=="maximum_power")prefix="maximum_power";else if(p.topic=="rc_transient")prefix="RC";else if(p.topic=="rl_transient")prefix="RL";
    else if(p.topic=="superposition"||p.topic=="thevenin"||p.topic=="norton")prefix=p.topic;
    if(!supplied.empty()){
        std::vector<std::pair<std::string,std::string>> parameters;
        if(p.topic=="voltage_divider")parameters={{"vin","V"},{"r1","Ohm"},{"r2","Ohm"}};
        else if(p.topic=="mesh_analysis")parameters={{"vleft","V"},{"vright","V"},{"rleft","Ohm"},{"rright","Ohm"},{"rshared","Ohm"}};
        else if(p.topic=="superposition")parameters={{"v1","V"},{"r1","Ohm"},{"v2","V"},{"r2","Ohm"},{"rload","Ohm"}};
        else if(p.topic=="rc_transient"||p.topic=="rl_transient")parameters={{"vin","V"},{"r","Ohm"},{p.topic=="rc_transient"?"c":"l",p.topic=="rc_transient"?"F":"H"},{"t","s"}};
        else if(prefix=="thevenin"||prefix=="maximum_power")parameters={{"vth","V"},{"rth","Ohm"}};
        else if(prefix=="norton")parameters={{"in","A"},{"rn","Ohm"}};
        if(p.topic=="thevenin"||p.topic=="norton")parameters.emplace_back("rload","Ohm");
        body=named_values(supplied,parameters);
    }else {
        if(!prefix.empty()&&lower(body).starts_with(lower(prefix)+" "))body=trim(body.substr(prefix.size()));
        const std::size_t count=p.topic=="voltage_divider"||p.topic=="thevenin"||p.topic=="norton"?3:p.topic=="mesh_analysis"||p.topic=="superposition"?5:p.topic=="rc_transient"||p.topic=="rl_transient"?4:2;
        body=numeric_tuple(body,count);
    }
    if(!prefix.empty())body=prefix+' '+body;
    return body;
}
std::vector<int> index_list(std::string text){
    std::vector<int> out;for(char& c:text)if(c==','||c==';')c=' ';std::stringstream tokens(text);std::string token;
    while(tokens>>token){const auto dash=token.find('-',1);const auto integer=[](const std::string& value){if(value.empty()||!std::all_of(value.begin(),value.end(),[](char c){return c>='0'&&c<='9';}))throw std::runtime_error("Minterm indices must be nonnegative integers");if(value.size()>2)throw std::runtime_error("K-map index exceeds the 4-variable range");return std::stoi(value);};
        const int first=integer(token.substr(0,dash)),last=dash==std::string::npos?first:integer(token.substr(dash+1));if(first>last||last>15)throw std::runtime_error("K-map ranges must be increasing and between 0 and 15");for(int v=first;v<=last;++v)out.push_back(v);}
    return out;
}
std::string join_indices(const std::vector<int>& values){std::string out;for(int v:values){if(!out.empty())out+=',';out+=std::to_string(v);}return out;}
std::string kmap_input(const std::string& original,std::vector<std::string>& variables){
    auto text=original;replace(text,"Σm","minterms(");replace(text,"ΠM","maxterms(");replace(text,"minterms((","minterms(");replace(text,"maxterms((","maxterms(");
    std::smatch match;unsigned count=0;
    static const std::regex vars(R"(\bvars\s*=\s*([0-9]+)|\b([0-9]+)[ -]+variables?\b)",std::regex::icase);
    if(std::regex_search(text,match,vars))count=static_cast<unsigned>(std::stoul(match[1].matched?match[1].str():match[2].str()));
    static const std::regex args(R"(\bF\s*\(([A-Za-z0-9_, ]+)\))",std::regex::icase);
    if(std::regex_search(text,match,args)){std::stringstream list(match[1].str());std::string name;while(std::getline(list,name,',')){name=trim(name);if(name.empty()||name.size()!=1||!std::isalpha(static_cast<unsigned char>(name[0])))throw std::runtime_error("Use distinct single-letter K-map variables");variables.push_back(name);}if(std::set<std::string>(variables.begin(),variables.end()).size()!=variables.size())throw std::runtime_error("K-map variable names must be distinct");if(count&&count!=variables.size())throw std::runtime_error("Conflicting K-map variable counts");count=static_cast<unsigned>(variables.size());}
    if(count<2||count>4)throw std::runtime_error("Specify 2–4 K-map variables, for example F(A,B,C) or vars=3; do not infer missing high-order variables from the largest minterm");
    static const std::regex terms(R"(\b(minterms?|maxterms?)\s*(?:=|:|are)?\s*[\(\[\{]?\s*([0-9, \t\r\n-]*))",std::regex::icase);
    if(!std::regex_search(text,match,terms))throw std::runtime_error("Supply minterms or maxterms explicitly");
    const bool zeros=lower(match[1].str()).starts_with("max");auto on=index_list(match[2].str());
    if(std::distance(std::sregex_iterator(text.begin(),text.end(),terms),std::sregex_iterator{})!=1)throw std::runtime_error("Supply one minterm or maxterm list, not conflicting lists");
    static const std::regex dc(R"(\b(?:dc|d|don't[ -]?cares?|dont[ -]?cares?)\s*(?:=|:|are)?\s*[\(\[\{]?\s*([0-9, \t\r\n-]*))",std::regex::icase);
    std::vector<int> dontCare;if(std::regex_search(text,match,dc))dontCare=index_list(match[1].str());
    for(int v:on)if(v>=(1<<count))throw std::runtime_error("Index exceeds the declared K-map variable count");
    for(int v:dontCare)if(v>=(1<<count)||std::find(on.begin(),on.end(),v)!=on.end())throw std::runtime_error("Don't-cares must be in range and disjoint from the specified terms");
    if(zeros){std::vector<int> ones;for(int v=0;v<(1<<count);++v)if(std::find(on.begin(),on.end(),v)==on.end()&&std::find(dontCare.begin(),dontCare.end(),v)==dontCare.end())ones.push_back(v);on=std::move(ones);}
    auto rest=std::regex_replace(text,args,"");rest=std::regex_replace(rest,vars,"");rest=std::regex_replace(rest,terms,"");rest=std::regex_replace(rest,dc,"");rest=lower(polite(rest));
    for(const auto& filler:{"karnaugh map","k-map","kmap","karnaugh","simplify","minimize","minimise","with","using","of","and","a","f","to"})words(rest,filler,"");
    if(rest.find_first_not_of(" =:;,()[]{}\t\r\n")!=std::string::npos)throw std::runtime_error("Unrecognized K-map condition: "+trim(rest)+". Supply an exact term list; additional conditions cannot be ignored.");
    return "vars="+std::to_string(count)+"; minterms="+join_indices(on)+(dontCare.empty()?"":"; dc="+join_indices(dontCare));
}
std::string vector_input(const std::string& raw,std::string body){
    const auto text=lower(raw);std::string kind=phrase(text,"cross product")!=std::string::npos?"cross":phrase(text,"magnitude")!=std::string::npos?"magnitude":"dot";
    if(body.starts_with("dot:")||body.starts_with("cross:")||body.starts_with("magnitude:"))return body;
    words(body,"and","|");replace(body,"(","");replace(body,")","");replace(body,"[","");replace(body,"]","");return kind+':'+body;
}
std::string boolean_input(std::string body){
    static const std::regex assignment(R"(^\s*F(?:\([^)]*\))?\s*=\s*)",std::regex::icase);body=std::regex_replace(body,assignment,"");
    words(body,"not","!");words(body,"and","&");words(body,"xor","^");words(body,"or","|");
    static const std::regex prime(R"(\b([A-Za-z])')");const bool shorthand=body.find('\'')!=std::string::npos;body=std::regex_replace(body,prime,"!$1");
    if(shorthand){static const std::regex product(R"(([A-Za-z\)])\s*([A-Za-z!\(]))");for(unsigned i=0;i<4;++i)body=std::regex_replace(body,product,"$1&$2");}
    return trim(body);
}
std::string ode_input(ProblemSpec& p,std::string body){
    body=normalize_math_text(body);
    if(p.topic=="rk4"||p.topic=="euler"||p.topic=="initial_value"){
        std::map<std::string,std::string> supplied;std::smatch m;
        static const std::regex equation(R"((?:dy/dx|y')\s*=\s*([+-]?(?:[0-9]+(?:\.[0-9]+)?)?)\s*\*?\s*y\b)");
        if(std::regex_search(body,m,equation)){const auto coefficient=m[1].str();const auto a=coefficient.empty()?"1":coefficient=="-"?"-1":coefficient=="+"?"1":coefficient;body.replace(static_cast<std::size_t>(m.position()),static_cast<std::size_t>(m.length()),"a="+a+" ");}
        static const std::regex initial(R"(y\s*\(\s*0\s*\)\s*=\s*([+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:e[+-]?[0-9]+)?))");body=std::regex_replace(body,initial,"y0=$1 ");
        supplied=fields(body);
        if(!supplied.empty()){
            // dy/dx is grammar, not an independent numeric parameter.
            supplied.erase("dx");
            if(p.topic=="initial_value")return named_values(supplied,{{"a",""},{"y0",""},{"x",""}});
            return named_values(supplied,{{"a",""},{"y0",""},{"h",""},{"steps",""}});
        }
        return numeric_tuple(body,p.topic=="initial_value"?3:4);
    }
    if(p.topic=="exact"||p.topic=="bernoulli"||p.topic=="homogeneous"){
        if(body.starts_with(p.topic+" "))body=trim(body.substr(p.topic.size()));
        return p.topic+' '+numeric_tuple(body,p.topic=="homogeneous"?1:3);
    }
    if(p.topic=="separable"||p.topic=="first_order_linear"){
        replace(body,"y'","dy/dx");
        static const std::regex xy(R"(([0-9])\s*\*?\s*x\s*\*?\s*y\b)");body=std::regex_replace(body,xy,"$1*x*y");
    }
    return body;
}
} // namespace

std::string normalize_math_text(std::string text){
    text=lower(unicode(text));text=number_words(text);
    for(const auto&[a,b]:std::vector<std::pair<std::string,std::string>>{{"multiplied by","*"},{"divided by","/"},{"raised to the power of","^"},{"raised to the power","^"},{"to the power of","^"},{"to the power","^"},{"equals","="},{"equal to","="},{"plus","+"},{"minus","-"},{"negative","-"},{"times","*"},{"squared","^2"},{"cubed","^3"},{"square root of","sqrt"},{"square root","sqrt"},{"natural log of","ln"},{"natural logarithm of","ln"},{"sine of","sin"},{"cosine of","cos"},{"tangent of","tan"}})words(text,a,b);
    replace(text,"√","sqrt ");
    static const std::regex atom(R"(\b(sqrt|sin|cos|tan|ln|log|abs|exp)\s+([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?|pi)\b)");text=std::regex_replace(text,atom,"$1($2)");
    static const std::regex degrees(R"(\b(sin|cos|tan)\s*\(\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+))\s*\)\s*(?:degrees|degree|deg|°))");text=std::regex_replace(text,degrees,"$1(($2)*pi/180)");
    static const std::regex percentOf(R"(([0-9]+(?:\.[0-9]+)?)\s*(?:%|percent)\s*of\b)");text=std::regex_replace(text,percentOf,"($1/100)*");
    static const std::regex percent(R"(([0-9]+(?:\.[0-9]+)?)\s*(?:%|percent\b))");text=std::regex_replace(text,percent,"($1/100)");
    return trim(text);
}

// The course-specific normalizers below never solve a stripped collection of
// numbers: they require explicit parameter labels, a recognized grammar, or the
// original positional contract. Any remaining prose is a validation error.
InputResolution resolve_input(const ProblemSpec& original){
    InputResolution result;result.problem=original;result.problem.mode="manual";
    try{
        if(original.input.empty()||original.input.size()>4096)throw std::runtime_error("Provide 1–4096 bytes of question text");
        std::string raw=unicode(original.input),body=polite(raw),l=lower(body),marker;bool explicitTask=false;
        for(const auto& cmd:commands()){const auto at=phrase(l,cmd.words);if(at!=std::string::npos){
            result.problem.domain=cmd.domain;result.problem.topic=cmd.topic;marker=cmd.words;
            if(at&&result.problem.topic!="kmap_minimization")throw std::runtime_error("I recognize "+marker+", but cannot safely ignore the preceding text. Start with the operation, then give its expression or named parameters.");
            body=fillers(body.substr(at+marker.size()));explicitTask=true;break;
        }}
        result.confidence=explicitTask?"explicit":"selected";
        auto math=normalize_math_text(body);
        if(marker=="tangent of")math=normalize_math_text("tangent of "+body);
        auto& p=result.problem;
        // Conversion has its own grammar so unit names are never lowercased
        // before SI prefix validation (milli and mega are different).
        std::smatch conversion;
        static const std::regex convert(R"(^convert\s+(.+?)\s+(?:from\s+)?([A-Za-z]+)\s+to\s+([A-Za-z]+)$)",std::regex::icase);
        if(!explicitTask&&std::regex_match(body,conversion,convert)){
            const auto from=lower(conversion[2].str()),to=lower(conversion[3].str());
            const std::set<std::string> bases{"binary","bin","octal","oct","decimal","dec","hexadecimal","hex"};
            p.domain=bases.contains(from)||bases.contains(to)?"logic":"engineering";p.topic=p.domain=="logic"?"number_systems":"unit_conversion";
            body=number_words(conversion[1].str())+' '+(p.domain=="logic"?from:canonical_unit(conversion[2].str()))+' '+(p.domain=="logic"?to:canonical_unit(conversion[3].str()));
            explicitTask=true;result.confidence="explicit";
        }
        if(!explicitTask){
            const auto full=lower(raw);
            const bool parameterForm=known(p)&&((p.domain=="circuit"&&(l.starts_with("vin")||l.starts_with("vth")||l.starts_with("vleft")||l.starts_with("v1=")||l.starts_with("in=")))||(p.domain=="differential_equations"&&(l.starts_with("a=")||l.starts_with("y0=")))||(p.domain=="calculus"&&l.find(";at=")!=std::string::npos)||(p.domain=="linear_algebra"&&(p.topic=="linear_system"||l.find('[')!=std::string::npos)));
            if(l.starts_with("int ")){p.domain="programming";p.topic=l.find("for (")!=std::string::npos?"loops":l.find("if (")!=std::string::npos?"branches":l.find("return ")!=std::string::npos?"functions":"cpp_trace";result.confidence="structural";}
            else if(l.starts_with("sum [")){p.domain="programming";p.topic="arrays";result.confidence="structural";}
            else if(l.starts_with("rc ")||l.starts_with("rl ")||l.starts_with("maximum_power ")){p.domain="circuit";p.topic=l.starts_with("rc ")?"rc_transient":l.starts_with("rl ")?"rl_transient":"maximum_power";result.confidence="structural";}
            else if(l.starts_with("v ")||l.starts_with("r ")){p.domain="circuit";p.topic="dc_nodal_analysis";result.confidence="structural";}
            else if(full.find("dy/dx")!=std::string::npos||full.find("y'")!=std::string::npos){p.domain="differential_equations";const auto eq=full.find('=');p.topic=full.find("y''")!=std::string::npos?"second_order_constant_coefficient":full.substr(0,eq).find('+')!=std::string::npos?"first_order_linear":"separable";result.confidence="structural";}
            else if(body.starts_with("vars=")||raw.find("Σm")!=std::string::npos||raw.find("ΠM")!=std::string::npos){result.problem.domain="logic";result.problem.topic="kmap_minimization";result.confidence="structural";}
            else if(math.find('=')!=std::string::npos&&!parameterForm){
                if(math.find(';')!=std::string::npos||math.find('\n')!=std::string::npos||phrase(math,"and")!=std::string::npos){result.problem.domain="linear_algebra";result.problem.topic="linear_system";}
                else if(math.find('x')==std::string::npos&&std::any_of(math.begin(),math.end(),[](char c){return c>='a'&&c<='z'&&c!='e';})){p.domain="linear_algebra";p.topic="linear_system";}
                else {result.problem.domain="algebra";result.problem.topic="linear_equation";}
                result.confidence="structural";
            }else if(!known(result.problem)){
                if(body.find(';')!=std::string::npos||body.find('[')!=std::string::npos)throw std::runtime_error("Matrix data alone does not specify an operation. Ask for its determinant, inverse, rank, transpose or row reduction.");
                result.problem.domain="algebra";result.problem.topic=math.find('x')!=std::string::npos?"simplify":"numeric_evaluation";result.confidence="structural";
            }
        }
        if(original.topic=="source_transformation"&&(marker=="thevenin"||marker=="norton"))p.topic="source_transformation";
        if(p.domain=="algebra"){
            p.input=math;
            if((p.topic=="linear_equation"||p.topic=="quadratic_equation")&&p.input.find('=')!=std::string::npos){const auto degree=polynomial_equation_degree(p.input);p.topic=degree==2?"quadratic_equation":"linear_equation";}
        }else if(p.domain=="linear_algebra"){
            if(p.topic=="linear_system"&&math.find('=')!=std::string::npos)p.input=equations(math,result.variables);
            else if(p.topic=="vectors")p.input=vector_input(raw,body);
            else if(p.topic=="multiply"){
                words(body,"and","|");const auto split=body.find('|');if(split==std::string::npos)throw std::runtime_error("Supply two matrices separated by 'and' or |.");p.input=matrix(body.substr(0,split))+'|'+matrix(body.substr(split+1));
            }else p.input=matrix(body);
        }else if(p.domain=="calculus"){
            if(math.starts_with("to "))math=trim(math.substr(3));
            static const std::regex function(R"(^f\s*\(\s*x\s*\)\s*=\s*)");math=std::regex_replace(math,function,"");
            p.input=math;
            if(p.topic=="integration"||p.topic=="definite_integral"){
                if(p.input.starts_with("integrate "))p.input=trim(p.input.substr(10));
                if(p.input.find(" from ")!=std::string::npos){p.topic="definite_integral";replace(p.input," dx from "," from ");}
                else if(p.topic=="integration"&&!p.input.ends_with(" dx"))p.input+=" dx";
                p.input="integrate "+p.input;
            }else if(p.topic=="tangent_line"){
                static const std::regex at(R"(\s+at\s+(?:x\s*=\s*)?(.+)$)");p.input=std::regex_replace(p.input,at,";at=$1");
            }else if(p.topic=="limits"&&!p.input.starts_with("limit "))p.input="limit "+p.input;
        }else if(p.domain=="logic"){
            if(p.topic=="kmap_minimization")p.input=kmap_input(raw,result.variables);
            else if(p.topic=="truth_table"||p.topic=="canonical_pos")p.input=boolean_input(body);
            else if(p.topic=="signed_arithmetic")p.input=explicitTask?"twos_add "+body:body;
            else if(p.topic=="combinational_logic"||p.topic=="sequential_logic"){
                std::string prefix=marker;for(const auto& [a,b]:std::vector<std::pair<std::string,std::string>>{{"full adder","full_adder"},{"jk flip flop","jkff"},{"jk flip-flop","jkff"},{"d flip flop","dff"},{"t flip flop","tff"}})if(prefix==a)prefix=b;
                p.input=explicitTask?prefix+' '+body:body;
            }else p.input=body;
        }else if(p.domain=="programming"){
            p.input=body;
            if(p.topic=="recursion"){body=normalize_math_text(body);if(!body.starts_with("fact(")&&!body.starts_with("factorial("))p.input="fact"+(body.starts_with('(')?body:'('+body+')');}
            else if(p.topic=="arrays"&&!body.starts_with("sum "))p.input="sum "+body;
        }else if(p.domain=="differential_equations")p.input=ode_input(p,body);
        else if(p.domain=="circuit")p.input=circuit_input(p,raw,body);
        else p.input=body;
        result.reason=(p.domain!=original.domain||p.topic!=original.topic)?"The question's operation or structure identifies a different problem type. The selected type was corrected.":"The question was normalized while retaining the selected operation.";
    }catch(const std::exception& e){result.status="needs_clarification";result.reason=e.what();}
    return result;
}
std::string InputResolution::to_json(const ProblemSpec& original)const{
    const auto q=[](const std::string& s){return '"'+json_escape(s)+'"';};
    std::string out="{\"status\":"+q(status)+",\"confidence\":"+q(confidence)+",\"original_input\":"+q(original.input)+",\"normalized_input\":"+q(problem.input)+",\"selected_domain\":"+q(original.domain)+",\"selected_topic\":"+q(original.topic)+",\"domain\":"+q(problem.domain)+",\"topic\":"+q(problem.topic)+",\"type_changed\":"+((original.domain!=problem.domain||original.topic!=problem.topic)?"true":"false")+",\"reason\":"+q(reason)+",\"variables\":[";
    for(std::size_t i=0;i<variables.size();++i){if(i)out+=',';out+=q(variables[i]);}return out+"]}";
}
} // namespace pocket_engineer
