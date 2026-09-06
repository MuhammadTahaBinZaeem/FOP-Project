#include "pocket_engineer/polynomial.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pocket_engineer {
namespace {
constexpr std::size_t degree_budget=32;
double finite(double value) {
    if(!std::isfinite(value))throw std::runtime_error("Polynomial calculation overflowed the finite real range");
    return value;
}
std::string format(double value) {
    finite(value);if(value==0)value=0;
    std::ostringstream output;output<<std::setprecision(12)<<value;return output.str();
}
std::string trim(std::string value) {
    const auto first=value.find_first_not_of(" \n\r\t");
    return first==std::string::npos?"":value.substr(first,value.find_last_not_of(" \n\r\t")-first+1);
}

struct Polynomial {
    // Coefficients are stored in ascending power order. Only exact zero trailing
    // coefficients are removed: tiny-but-valid coefficients must not disappear.
    std::vector<double> coefficients{0};
    explicit Polynomial(double constant=0):coefficients{constant}{}
    static Polynomial variable(){Polynomial p;p.coefficients={0,1};return p;}
    std::size_t degree()const{return coefficients.size()-1;}
    void normalize(){while(coefficients.size()>1&&coefficients.back()==0)coefficients.pop_back();}
    double evaluate(double x)const {
        double y=0;for(auto i=coefficients.rbegin();i!=coefficients.rend();++i)y=finite(y*x+*i);return y;
    }
    Polynomial scaled(double factor)const {
        Polynomial result=*this;for(auto& c:result.coefficients)c=finite(c*factor);result.normalize();return result;
    }
    Polynomial plus(const Polynomial& rhs,double sign=1)const {
        Polynomial result=*this;result.coefficients.resize(std::max(coefficients.size(),rhs.coefficients.size()),0);
        for(std::size_t i=0;i<rhs.coefficients.size();++i)result.coefficients[i]=finite(result.coefficients[i]+sign*rhs.coefficients[i]);
        result.normalize();return result;
    }
    Polynomial times(const Polynomial& rhs)const {
        if(degree()+rhs.degree()>degree_budget)throw std::runtime_error("Expanded polynomial degree exceeds 32");
        Polynomial result;result.coefficients.assign(degree()+rhs.degree()+1,0);
        for(std::size_t i=0;i<coefficients.size();++i)for(std::size_t j=0;j<rhs.coefficients.size();++j)
            result.coefficients[i+j]=finite(result.coefficients[i+j]+coefficients[i]*rhs.coefficients[j]);
        result.normalize();return result;
    }
    Polynomial power(unsigned exponent)const {
        if(degree()*exponent>degree_budget)throw std::runtime_error("Expanded polynomial degree exceeds 32");
        Polynomial result(1),base=*this;
        while(exponent){if(exponent&1u)result=result.times(base);exponent>>=1u;if(exponent)base=base.times(base);}
        return result;
    }
    Polynomial derivative()const {
        Polynomial result;
        if(degree()==0)return result;
        result.coefficients.resize(degree());
        for(std::size_t i=1;i<coefficients.size();++i)result.coefficients[i-1]=finite(static_cast<double>(i)*coefficients[i]);
        result.normalize();return result;
    }
    Polynomial integral()const {
        Polynomial result;result.coefficients.assign(coefficients.size()+1,0);
        for(std::size_t i=0;i<coefficients.size();++i)result.coefficients[i+1]=coefficients[i]/static_cast<double>(i+1);
        result.normalize();return result;
    }
    std::string text()const {
        std::string result;
        for(std::size_t n=coefficients.size();n>0;--n) {
            const auto i=n-1;const double value=coefficients[i];if(value==0)continue;
            if(result.empty()){if(value<0)result+='-';}else result+=value<0?" - ":" + ";
            const double magnitude=std::abs(value);
            if(i==0||magnitude!=1)result+=format(magnitude);
            if(i>0){result+='x';if(i>1)result+='^'+std::to_string(i);}
        }
        return result.empty()?"0":result;
    }
};

// Grammar: expression := term ((+|-) term)*; term := unary ((*|/) unary)*;
// unary := (+|-) unary | power; power := primary (^ nonnegative_integer)?;
// primary := number | x | '(' expression ')'. Juxtaposition before x or '(' is
// multiplication, e.g. 3x and (x+1)(x-1); two adjacent numbers are rejected.
class Parser {
public:
    explicit Parser(std::string text):text_(std::move(text)) {
        if(text_.empty()||text_.size()>512)throw std::runtime_error("Polynomial input must be 1–512 bytes");
    }
    Polynomial read(){auto result=expression();space();if(cursor_!=text_.size())throw std::runtime_error("Unsupported polynomial syntax near: "+text_.substr(cursor_,30));return result;}
private:
    struct Guard {
        unsigned& depth;
        explicit Guard(unsigned& value):depth(value){if(++depth>32)throw std::runtime_error("Polynomial nesting exceeds 32 levels");}
        ~Guard(){--depth;}
    };
    void space(){while(cursor_<text_.size()&&std::isspace(static_cast<unsigned char>(text_[cursor_])))++cursor_;}
    char peek(){space();return cursor_<text_.size()?text_[cursor_]:'\0';}
    bool take(char value){if(peek()==value){++cursor_;return true;}return false;}
    Polynomial expression(){auto p=term();while(true){if(take('+'))p=p.plus(term());else if(take('-'))p=p.plus(term(),-1);else return p;}}
    Polynomial term() {
        auto p=unary();
        while(true) {
            if(take('*'))p=p.times(unary());
            else if(take('/')){const auto divisor=unary();if(divisor.degree()!=0||divisor.coefficients[0]==0)throw std::runtime_error("Polynomial division requires a nonzero constant denominator");p=p.scaled(1/divisor.coefficients[0]);}
            else if(peek()=='x'||peek()=='(')p=p.times(unary());
            else return p;
        }
    }
    Polynomial unary(){Guard guard(depth_);if(take('+'))return unary();if(take('-'))return unary().scaled(-1);return power();}
    Polynomial power() {
        auto p=primary();
        if(take('^')) {
            space();unsigned exponent=0;bool digit=false;
            while(cursor_<text_.size()&&std::isdigit(static_cast<unsigned char>(text_[cursor_]))) {
                digit=true;exponent=10*exponent+static_cast<unsigned>(text_[cursor_++]-'0');
                if(exponent>32)throw std::runtime_error("Polynomial exponent must be an integer from 0 to 32");
            }
            if(!digit)throw std::runtime_error("Polynomial exponent must be a nonnegative integer");
            p=p.power(exponent);
        }
        return p;
    }
    Polynomial primary() {
        if(take('x'))return Polynomial::variable();
        if(take('(')){auto p=expression();if(!take(')'))throw std::runtime_error("Unclosed polynomial parenthesis");return p;}
        space();const char* start=text_.c_str()+cursor_;char* end=nullptr;
        if(!(std::isdigit(static_cast<unsigned char>(*start))||*start=='.'))throw std::runtime_error("Expected a number, x, or a polynomial in parentheses");
        const double value=std::strtod(start,&end);if(end==start)throw std::runtime_error("Malformed polynomial number");
        cursor_+=static_cast<std::size_t>(end-start);return Polynomial(finite(value));
    }
    std::string text_;std::size_t cursor_=0;unsigned depth_=0;
};

double scalar(const std::string& input) {
    const auto p=Parser(trim(input)).read();
    if(p.degree()!=0)throw std::runtime_error("Bounds and evaluation points must be real constants");
    return p.coefficients[0];
}

std::string chart(const Polynomial& polynomial,double lower=-2,double upper=2) {
    if(lower==upper){lower-=1;upper+=1;}
    if(lower>upper)std::swap(lower,upper);
    std::ostringstream points;points<<"{\"kind\":\"trajectory\",\"label\":\"Polynomial f(x)\",\"points\":[";
    for(int i=0;i<=128;++i) {
        const double x=lower+(upper-lower)*i/128;
        if(i)points<<',';
        points<<'['<<format(x)<<','<<format(polynomial.evaluate(x))<<']';
    }
    points<<"]}";return points.str();
}
void attach_chart(SolutionBundle& solution,const Polynomial& polynomial,double lower=-2,double upper=2) {
    try{solution.visual_json=chart(polynomial,lower,upper);}
    catch(const std::exception&){solution.warnings.push_back("The optional preview exceeded the finite plotting range; the algebraic answer is retained.");}
}

// Evaluation via an independent sum of powers, rather than Horner's method.
// Useful for detecting coefficient/index errors in the emitted transform.
long double power_sum(const Polynomial& p,long double x) {
    long double result=0;
    for(std::size_t i=0;i<p.coefficients.size();++i)result+=static_cast<long double>(p.coefficients[i])*std::pow(x,static_cast<int>(i));
    return result;
}
Verification verify_derivative(const Polynomial& original,const Polynomial& derivative) {
    long double largest=0;
    for(const long double x:{-1.25L,-0.5L,0.0L,0.5L,1.25L}) {
        // A five-point stencil is separate from symbolic coefficient shifting.
        const long double h=1e-4L;
        const long double numerical=(-power_sum(original,x+2*h)+8*power_sum(original,x+h)
            -8*power_sum(original,x-h)+power_sum(original,x-2*h))/(12*h);
        const long double actual=power_sum(derivative,x);
        largest=std::max(largest,std::abs(actual-numerical)/std::max(1.0L,std::abs(actual)));
    }
    const bool passed=std::isfinite(largest)&&largest<=1e-7L;
    return {passed?VerificationStatus::verified_numerical:VerificationStatus::not_verified,
        "five-point finite-difference cross-check",
        "Five sample points; maximum scaled error = "+format(static_cast<double>(largest))+"; tolerance = 1e-7. Sampling is not a symbolic proof."};
}
Verification verify_integral(const Polynomial& original,const Polynomial& antiderivative) {
    double residual=0;
    for(std::size_t i=0;i<original.coefficients.size();++i) {
        const double reconstructed=antiderivative.coefficients.size()>i+1?antiderivative.coefficients[i+1]*static_cast<double>(i+1):0;
        residual=std::max(residual,std::abs(reconstructed-original.coefficients[i])/std::max(1.0,std::abs(original.coefficients[i])));
    }
    return {residual<=1e-12?VerificationStatus::verified_numerical:VerificationStatus::verification_failed,
        "differentiate antiderivative coefficients",
        "Every original coefficient reconstructed; maximum scaled residual = "+format(residual)+"; tolerance = 1e-12"};
}
void derivative_steps(SolutionBundle& s,const Polynomial& p) {
    s.steps.push_back({"POLY_NORMALIZE","Expand products and powers, then collect coefficients of equal powers of x.","f(x) = "+p.text()});
    for(std::size_t n=p.coefficients.size();n>0;--n) {
        const auto i=n-1;const double coefficient=p.coefficients[i];if(coefficient==0)continue;
        if(i==0){s.steps.push_back({"CALC_CONSTANT","A constant does not change as x changes, so its derivative is zero.","d/dx("+format(coefficient)+") = 0"});continue;}
        Polynomial term;term.coefficients.assign(i,0);term.coefficients[i-1]=coefficient*static_cast<double>(i);
        s.steps.push_back({"CALC_POWER_RULE","Multiply the coefficient "+format(coefficient)+" by the exponent "+std::to_string(i)+", then reduce the exponent by one.","d/dx("+format(coefficient)+"x^"+std::to_string(i)+") = "+term.text()});
    }
    if(p.degree()==0&&p.coefficients[0]==0)s.steps.push_back({"CALC_ZERO","The zero function has zero slope everywhere.","d/dx(0) = 0"});
}
void integral_steps(SolutionBundle& s,const Polynomial& p) {
    s.steps.push_back({"POLY_NORMALIZE","Expand the polynomial and integrate each term separately using linearity.","f(x) = "+p.text()});
    for(std::size_t n=p.coefficients.size();n>0;--n) {
        const auto i=n-1;const double coefficient=p.coefficients[i];if(coefficient==0)continue;
        Polynomial term;term.coefficients.assign(i+2,0);term.coefficients[i+1]=coefficient/static_cast<double>(i+1);
        s.steps.push_back({"CALC_REVERSE_POWER_RULE","Increase exponent "+std::to_string(i)+" to "+std::to_string(i+1)+" and divide coefficient "+format(coefficient)+" by "+std::to_string(i+1)+".",term.text()});
    }
}
} // namespace

unsigned polynomial_equation_degree(std::string_view expression) {
    const std::string text(expression);const auto equal=text.find('=');
    if(equal==std::string::npos||text.find('=',equal+1)!=std::string::npos)throw std::runtime_error("Provide exactly one equals sign");
    const auto difference=Parser(text.substr(0,equal)).read().plus(Parser(text.substr(equal+1)).read(),-1);
    if(difference.degree()>2)throw std::runtime_error("Equation solving supports real linear and quadratic polynomials, not higher-degree equations");
    return static_cast<unsigned>(difference.degree());
}

SolutionBundle solve_polynomial_algebra(const ProblemSpec& problem) {
    SolutionBundle s;s.domain="algebra";s.topic=problem.topic;
    const auto equal=problem.input.find('=');
    if(equal==std::string::npos){
        const auto p=Parser(problem.input).read();s.answer=p.text();
        s.steps={{"POLY_EXPAND","Expand products and integer powers without changing the polynomial.",problem.input},
            {"POLY_COLLECT","Add coefficients belonging to the same power of x.",s.answer}};
        const auto replay=Parser(s.answer).read();double residual=0;
        for(std::size_t i=0;i<p.coefficients.size();++i)residual=std::max(residual,std::abs(p.coefficients[i]-(i<replay.coefficients.size()?replay.coefficients[i]:0))/std::max(1.0,std::abs(p.coefficients[i])));
        s.verification={residual<=1e-10?VerificationStatus::verified_numerical:VerificationStatus::verification_failed,"formatted-polynomial coefficient round-trip","Maximum scaled coefficient residual = "+format(residual)+"; tolerance = 1e-10. This checks the emitted polynomial, not arbitrary symbolic identities."};
        return s;
    }
    if(problem.input.find('=',equal+1)!=std::string::npos)throw std::runtime_error("Provide one equation at a time or choose linear systems");
    const auto lhs=Parser(problem.input.substr(0,equal)).read(),rhs=Parser(problem.input.substr(equal+1)).read(),p=lhs.plus(rhs,-1);
    if(p.degree()>2)throw std::runtime_error("Equation solving is limited to degree two");
    s.steps={{"ALG_EXPAND_BOTH_SIDES","Expand products and collect like terms separately on both sides.",lhs.text()+" = "+rhs.text()},
        {"ALG_MOVE_TO_ZERO","Subtract the right side from both sides; this preserves the solution set.",p.text()+" = 0"}};
    if(p.degree()==0){s.answer=p.coefficients[0]==0?"All real x satisfy the equation":"No solution";s.steps.push_back({"ALG_CONSTANT_EQUATION",p.coefficients[0]==0?"The two sides are identical for every real x.":"The remaining nonzero constant cannot equal zero.",s.answer});s.verification={VerificationStatus::verified_exact,"constant polynomial comparison",p.text()+" = 0"};return s;}
    std::vector<double> roots;
    if(p.degree()==1){s.topic="linear_equation";const double a=p.coefficients[1],b=p.coefficients[0];roots.push_back(finite(-b/a));s.answer="x = "+format(roots[0]);s.steps.push_back({"ALG_ISOLATE_AND_DIVIDE","Move the constant to the right, then divide by the nonzero coefficient of x.",format(a)+"x = "+format(-b)+"; "+s.answer});}
    else{
        s.topic="quadratic_equation";const double a=p.coefficients[2],b=p.coefficients[1],c=p.coefficients[0],disc=finite(b*b-4*a*c);
        s.steps.push_back({"ALG_DISCRIMINANT","Read a, b and c from the collected equation, then calculate b²−4ac.","a = "+format(a)+", b = "+format(b)+", c = "+format(c)+"; discriminant = "+format(disc)});
        if(disc<0)throw std::runtime_error("No real roots; complex-root output is not implemented for this algebra module");
        const double q=-0.5*(b+std::copysign(std::sqrt(disc),b));
        if(disc==0)roots={finite(-b/(2*a))};else if(q!=0)roots={finite(q/a),finite(c/q)};else roots={0};
        s.answer=roots.size()==1?"x = "+format(roots[0]):"x₁ = "+format(roots[0])+", x₂ = "+format(roots[1]);
        s.steps.push_back({"ALG_STABLE_QUADRATIC","Apply the quadratic formula using a cancellation-resistant evaluation for distinct roots.",s.answer});
    }
    double residual=0;
    for(double x:roots){const double left=lhs.evaluate(x),right=rhs.evaluate(x),error=std::abs(left-right)/std::max({1.0,std::abs(left),std::abs(right)});residual=std::max(residual,error);s.steps.push_back({"ALG_SUBSTITUTE_ROOT","Substitute this root into both original sides.","x = "+format(x)+": left = "+format(left)+", right = "+format(right)});}
    s.verification={residual<=1e-8?VerificationStatus::verified_numerical:VerificationStatus::verification_failed,"original-equation substitution","Maximum scaled residual = "+format(residual)+"; tolerance = 1e-8"};return s;
}

SolutionBundle solve_polynomial_calculus(const ProblemSpec& problem) {
    SolutionBundle solution;solution.domain="calculus";solution.topic=problem.topic;
    const auto input=trim(problem.input);
    if(problem.topic=="differentiation") {
        const auto polynomial=Parser(input).read(),derivative=polynomial.derivative();
        derivative_steps(solution,polynomial);solution.answer=derivative.text();
        solution.steps.push_back({"CALC_COLLECT_DERIVATIVE","Add the term derivatives; the derivative of a sum is the sum of the derivatives.","f'(x) = "+solution.answer});
        solution.verification=verify_derivative(polynomial,derivative);attach_chart(solution,polynomial);return solution;
    }
    if(problem.topic=="tangent_line"||problem.topic=="tangent") {
        const auto split=input.find(";at=");if(split==std::string::npos)throw std::runtime_error("Tangent input: polynomial;at=value");
        const auto polynomial=Parser(input.substr(0,split)).read(),derivative=polynomial.derivative();
        const double x0=scalar(input.substr(split+4)),y0=polynomial.evaluate(x0),slope=derivative.evaluate(x0),intercept=finite(y0-slope*x0);
        derivative_steps(solution,polynomial);
        solution.steps.push_back({"CALC_TANGENT_POINT","Evaluate the original function at the chosen x coordinate.","f("+format(x0)+") = "+format(y0)});
        solution.steps.push_back({"CALC_TANGENT_SLOPE","Evaluate the derivative at the same point to obtain the line's slope.","m = f'("+format(x0)+") = "+format(slope)});
        solution.answer="y = "+format(slope)+"x + "+format(intercept);
        solution.steps.push_back({"CALC_POINT_SLOPE","Use y - f(x0) = m(x - x0), then collect the intercept.",solution.answer});
        solution.verification=verify_derivative(polynomial,derivative);
        const double residual=std::abs((slope*x0+intercept)-y0);
        solution.verification.evidence+="; point residual = "+format(residual);
        attach_chart(solution,polynomial,x0-2,x0+2);return solution;
    }
    if(problem.topic=="integration"||problem.topic=="definite_integral") {
        if(!input.starts_with("integrate "))throw std::runtime_error("Integral input starts with 'integrate '");
        std::string expression;double lower=0,upper=0;
        if(problem.topic=="integration") {
            if(!input.ends_with(" dx"))throw std::runtime_error("Indefinite integral input ends with ' dx'");
            expression=input.substr(10,input.size()-13);
        }else {
            const auto from=input.rfind(" from "),to=input.rfind(" to ");
            if(from==std::string::npos||to==std::string::npos||to<=from)throw std::runtime_error("Definite input: integrate polynomial from lower to upper");
            expression=input.substr(10,from-10);lower=scalar(input.substr(from+6,to-from-6));upper=scalar(input.substr(to+4));
        }
        const auto polynomial=Parser(expression).read(),antiderivative=polynomial.integral();
        integral_steps(solution,polynomial);solution.verification=verify_integral(polynomial,antiderivative);
        if(problem.topic=="integration") {
            solution.answer=antiderivative.text()+" + C";
            solution.steps.push_back({"CALC_INTEGRATION_CONSTANT","Add one arbitrary constant: differentiating a constant gives zero.",solution.answer});
            attach_chart(solution,polynomial);
        }else {
            const double atUpper=antiderivative.evaluate(upper),atLower=antiderivative.evaluate(lower),value=finite(atUpper-atLower);
            solution.steps.push_back({"CALC_ANTIDERIVATIVE","Use this antiderivative; its additive constant cancels between endpoints.","F(x) = "+antiderivative.text()});
            solution.steps.push_back({"CALC_UPPER_BOUND","Substitute the upper integration limit.","F("+format(upper)+") = "+format(atUpper)});
            solution.steps.push_back({"CALC_LOWER_BOUND","Substitute the lower integration limit.","F("+format(lower)+") = "+format(atLower)});
            solution.answer="∫ = "+format(value);
            solution.steps.push_back({"CALC_FUNDAMENTAL_THEOREM","Subtract lower from upper. This is signed area, not necessarily geometric area.",format(atUpper)+" - ("+format(atLower)+") = "+format(value)});
            if(lower>upper)solution.assumptions.push_back("Reversed limits are retained; reversing the limits negates the integral.");
            attach_chart(solution,polynomial,lower,upper);
        }
        return solution;
    }
    throw std::runtime_error("Unsupported polynomial calculus topic");
}
} // namespace pocket_engineer
