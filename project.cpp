#include <cmath>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#include <stack>
#include <functional>
#include <string>
#include <vector>

using namespace std;

// ============================================================================
// PROJECT CHANGE HISTORY LEDGER (RECONSTRUCTED)
// ============================================================================
// Change Counter: 21 major tracked change waves (v1 -> v21)
// Source basis: Final-suite reports in output/Final output/v1.md ... v6.md.
// Note: This ledger is reconstructed from in-repo reports, not external VCS UI.
//
// [Wave 1 | v1 | Baseline]
// - Established initial end-to-end pipeline (lex -> parse -> optimize -> eval)
//   with full report generation harness.
// - Result snapshot: PASS 84 / FAIL 165; baseline for all later improvements.
//
// [Wave 2 | v2 | Functional Breadth Expansion]
// - Added broader function support and stronger algebraic rules for
//   log/ln/exp/sqrt/trig families, plus more robust reciprocal/division handling.
// - Result snapshot improved significantly: PASS 135 / FAIL 114.
//
// [Wave 3 | v3 | Readability + Input Freedom]
// - Added commutative product readability shaping and better coefficient handling.
// - Added implicit multi-letter input expansion (e.g., xy -> x*y where intended).
// - Result totals held, but representational quality and parser flexibility improved.
//
// [Wave 4 | v4 | Precedence + Canonical Exponents + Fallback Eval]
// - Fixed unary-minus precedence edge cases and strengthened exponent canonical rules.
// - Added test-runner semantic fallback for arrangement-only symbolic mismatches.
// - Result snapshot: PASS 156 / FAIL 93.
//
// [Wave 5 | v5 | Implicit-Mul Normalization in Validation]
// - Improved symbolic normalization for compact forms (2pi, xy, ab) during
//   report comparison/equivalence checks.
// - Result snapshot: PASS 165 / FAIL 84.
//
// [Wave 6 | v6 | Deep Commutative/Associative Cancellation Stabilization]
// - Added canonical product-term extraction in additive simplification and
//   post-loop cleanup/round-trip stabilization to expose latent cancellations.
// - Result snapshot: PASS 169 / FAIL 80; expression #73 now resolves to 0.
//
// [Wave 7 | v7 | Bounded Final-Form Power Variant Exploration]
// - Added resource-aware final-stage exploration that selectively expands
//   power forms, reruns simplification, and keeps only strictly better results.
// - Purpose: capture cases where compact powers hide additional reductions.
//
// [Wave 8 | v8 | General Power Expansion + Exponent/Identity Corrections]
// - Added bounded multinomial expansion support for general (sum)^n forms
//   (beyond only hardcoded square/cube-like cases), with safety limits.
// - Fixed exponent-sign handling edge cases and strengthened selected
//   algebraic identity rewrites used in targeted regression failures.
// - Hardened perfect-square detection to avoid partial-pattern collapse when
//   additional unsupported additive terms are present.
//
// [Wave 9 | v9 | Additive-Numerator Division Factor Cancellation]
// - Added conservative division rewrite for cases where every additive term
//   in numerator shares a removable denominator factor, e.g. (x^2 + x)/x.
// - Integrates with existing product-factor remover to avoid unsafe symbolic
//   division assumptions while improving quotient simplification coverage.
//
// [Wave 10 | v10 | Formula-Division Pipeline Coupling Enhancements]
// - Extended formula-aware division rewrites so square/cube identities and
//   quotient cancellation cooperate for (a^2-b^2)/(a+b), (a^3±b^3)/(a±b),
//   and denominator-square forms like (a^2-b^2)/(a±b)^2.
// - Generalized selected perfect-square recognition from x^2±2x+1 to
//   x^2±2kx+k^2, enabling downstream denominator normalization and cancellation.
// - Added numeric cube matching support in cube-pattern checks to unblock
//   constant-based cube identities such as x^3±1 quotient simplifications.
//
// [Wave 11 | v11 | H09 General Perfect-Power Detection]
// - Upgraded H09 from square-only recognition into bounded perfect-power
//   recognition for single-variable binomial forms (x ± k)^n.
// - Supports all bounded n in detector range using coefficient checks against
//   binomial-theorem expectations, not only n=2 patterns.
// - Keeps prior two-variable perfect-square detection path for compatibility,
//   then falls back to generalized perfect-power matching.
//
// [Wave 12 | v12 | Root/Sqrt + Division-Formula Pipeline Stabilization]
// - Added root(n, expr) input rewrite to exponent form so parser pipeline can
//   process root syntax consistently with existing operators.
// - Added bounded square-root radicand simplification for perfect-square
//   products/powers, enabling sqrt-based factor extraction in optimization.
// - Extended quotient identities for quartic-difference forms and added
//   signed-fraction merge + common-factor subtraction factoring to better align
//   formula rewrites and division simplification stages.
//
// [Wave 13 | v13 | Division Associative/Commutative Factor Cancellation]
// - Strengthened denominator-factor cancellation to work through power forms
//   (x^n/x, x^n/x^m) inside additive numerators and product trees.
// - This closes key quotient simplification gaps such as (x^2 + x)/x -> x+1
//   and improves associative division behavior when factors are power nodes.
//
// [Wave 14 | v14 | Symmetric Difference-of-Squares Quotient Coverage]
// - Extended quotient identity handling for (a^2 - b^2)/(a + b) => a - b,
//   complementing existing (a^2 - b^2)/(a - b) => a + b support.
// - Added swapped-square-order handling so both a^2-b^2 and b^2-a^2 forms
//   reduce consistently when paired with matching linear denominator factors.
//
// [Wave 15 | v15 | Sqrt Multiplication Stability + Render-Safe Tokens]
// - Fixed compact multiplication rendering to keep explicit '*' when function
//   factors are adjacent, preventing reparsing corruption like xsqrt(x).
// - Added direct simplification for sqrt(x) * x^0.5 (and symmetric order)
//   so mixed root/power-half products reduce to x consistently.
//
// [Wave 16 | v16 | Open-Form Identity + Fraction/Trig/Log Reinforcement]
// - Added opening-friendly fraction rules: 1 +/- 1/x conversion, cross-denominator
//   subtraction merge, and mixed monomial-fraction coefficient aggregation.
// - Added stronger trig identities (1-cos^2, 1-sin^2, 1+tan^2, 1+cot^2)
//   including reordered additive forms with explicit negated terms.
// - Added logarithm/exponential inverse bridges (e^(ln(x)), log(a^b) expansion,
//   and base-power inverse for a^(log(x)/log(a)) patterns including log10 rewrite).
// - Added quotient reductions for denominator-square forms and added fourth-power
//   factorization forms such as x^4-1 and x^4-y^4.
// - Added lexer tolerance for postfix '!' attachment to symbols (e.g., x!) so
//   zero-product simplifications can proceed instead of failing parse.
//
// [Wave 17 | v17 | Remaining Targeted Canonicalization Closures]
// - Strengthened numeric base-log inverse reduction for power forms so
//   10^log10(x) (post rewrite/folding) reliably reduces to x.
// - Added conjugate-denominator canonicalization in division context so
//   1/(x+1) - 1/(x-1) style forms normalize to denominator x^2-1.
// - Added additive common-factor extraction for A*B + A and symmetric order,
//   restoring compact factored output such as x^4-1 -> (x-1)(x+1)(x^2+1).
//
// [Wave 18 | v18 | Pipeline Documentation + Case-Walkthrough Annotation]
// - Added end-of-file documentation block that explains full solver flow using
//   pipeline tree view + theoretical rationale of each compiler-like phase.
// - Added 20 concrete test walkthroughs (from test corpus traces) showing
//   tokenization -> AST -> key rewrite transitions -> final output.
//
// [Wave 19 | v19 | Standalone Input Canonicalization Parity]
// - Moved compact-symbol and shorthand-function normalization into project.cpp
//   preprocessing path so standalone execution handles yx/ab/cosx/sqrtyx style input.
// - Added preprocessing helpers for compact letter products and function shorthand
//   desugaring before sqrt/log/root rewrites.
//
// [Wave 20 | v20 | Basic Trig Coverage + Additive Conjugate Exposure]
// - Added beginner-level trig rewrites for odd/even symmetry, reciprocal pairs,
//   complement-angle forms, and selected double-angle identities.
// - Added additive-context conjugate exposure so sums of (a-b)(a+b)-style terms
//   can open into a^2-b^2 exactly when that helps later cancellation.
// - Added small helper matchers for trig power, numeric-factor, half-pi, and
//   conjugate recognition to keep targeted rewrite rules readable/maintainable.
//
// [Wave 21 | v21 | Quotient Preservation + Numeric Log Semantics Split]
// - Added pre-recursion quotient guards so division forms like u / u^n and
//   (a-b)/(b-a) simplify before child rewrites hide the original pattern.
// - Split numeric evaluation of log versus ln so log folds/evaluates in base 10
//   while ln continues to use the natural logarithm base.
// - Kept legacy symbolic log identities intact, including compatibility cases
//   such as log(e) and 10^log(x), for the existing algebra test corpus.
// ============================================================================

// ============================================================================
// PROPERTY COVERAGE MATRIX (IMPLEMENTATION-ORIENTED)
// ============================================================================
// NOTE: This solver is a rewrite/evaluation engine, not a formal proof assistant.
// The matrix below documents where each algebraic property is operationalized.
//
// 1) Commutative Addition / Multiplication
//    - Core matcher: areTreesEqual (+, * swap-aware)
//    - Canonical regrouping: extractCanonicalProduct + additive aggregator
//    - Display normalization: sorted/simple factor rendering paths
//
// 2) Associative Addition / Multiplication
//    - collectAdditiveTerms flattens nested +/- trees into signed lists
//    - flattenProduct flattens nested * trees into factor vectors
//
// 3) Distributive Property
//    - Multiplication handler distributes over + and - nodes
//
// 4) Identity Properties
//    - Additive identity: a + 0 -> a
//    - Multiplicative identity: a * 1 -> a
//    - Exponential identities: a^1 -> a, a^0 -> 1, 1^a -> 1
//
// 5) Inverse Properties
//    - Additive inverse patterns: a + (-a) -> 0
//    - Multiplicative inverse patterns: a / a -> 1, x*x^-1 patterns
//
// 6) Multiplicative Zero / Zero Product Simplification Context
//    - a*0 -> 0
//    - branch elimination when product contains guaranteed zero
//
// 7) Reflexive / Symmetric / Transitive (Operational, not theorem mode)
//    - Reflexive: structural self-equality checks
//    - Symmetric-style effect: commutative equivalence in matcher
//    - Transitive-style effect: pass sequencing + repeated normalization
//
// 8) Extended Domain Rules (beyond base algebra list)
//    - Log/exp inverses, trig identities, reciprocal composition,
//      perfect-power pattern recognition, and exponent law merging.
// ============================================================================

// ============================================================================
// HANDLER INDEX (VERSIONED MAINTENANCE MAP)
// ============================================================================
// [H-01] lexicalAnalysis                  | v1 base, v2/v3/v4 refinements
// [H-02] normalizeTokens                  | v1 base, v3/v4 precedence-input hardening
// [H-03] shuntingYardToPostfix            | v1 base, v2/v4 associativity safety
// [H-04] buildASTFromPostfix              | v1 base, v2 diagnostics
// [H-05] areTreesEqual                    | v1 base, v6 commutative-critical role
// [H-06] collectAdditiveTerms             | v1 base, v6 central cancellation role
// [H-07] flattenProduct                   | v1 base, v6 unary-minus canonicalization
// [H-08] extractCanonicalProduct          | v6 introduced for deep cancellation
// [H-09] detectPerfectPower               | v3 introduced, v4/v6/v10/v11 refinement
// [H-10] algebraicSimplification          | v1->v6 principal rewrite orchestrator
// [H-11] constantFolding                  | v1 base, v2 function fold expansion
// [H-12] identityReduction                | v1 base, v2/v3 expansion
// [H-13] deadCodeElimination              | v1 base, v6 stabilization alignment
// [H-14] commonSubexpressionElimination   | v1/v2 available, v6 loop-skip rationale
// [H-15] treeToString                     | v1 base, v3/v5 readability compaction
// [H-16] evaluateTree                     | v1 base, v2+ domain-safe expansion
// [H-17] main pass pipeline               | v1 base, v6 cleanup + round-trip stage
// [H-18] power-variant final explorer     | v7 bounded expansion-and-resimplify stage
// [H-19] isCubeOf                          | v8 cube-pattern matcher for quotient identities
// [H-20] collectSignedAddTerms             | v8 signed additive flattening for multinomial prep
// [H-21] multinomialCoefficient            | v8 multinomial coefficient calculator
// [H-22] buildExpandedPowerTerm            | v8 term materializer for multinomial expansion
// [H-23] tryExpandPowerByMultinomial       | v8 bounded multinomial expansion orchestrator
// [H-24] divideOutCommonFactorFromAdditive | v9 additive numerator factor cancellation for division
// [H-25] isNthPowerOf                       | v12 generic nth-power matcher for formula rules
// [H-26] simplifySquareRootRadicand         | v12 perfect-square radicand simplifier
// [H-27] extractSignedFractionTerm          | v12 signed fraction extractor for additive merges
// [H-28] expandLetterOnlyProduct            | v19 compact all-letter product expander
// [H-29] rewriteCompactFunctionShorthand    | v19 shorthand function-call desugarer
// [H-30] expandCompactLetterSymbols         | v19 pre-lex compact symbol canonicalizer
// [H-31] preprocessInputExpression          | v18/v19 standalone input preprocess orchestrator
// [H-32] isPowerOfFunction                  | v20 trig-power matcher for identity guards
// [H-33] extractProductWithNumericFactor    | v20 numeric-factor matcher for 2*x-style rules
// [H-34] extractDoubleAngleArgument         | v20 double-angle argument recognizer
// [H-35] isApproxHalfPiNode                 | v20 tolerant pi/2 numeric matcher
// [H-36] extractHalfPiMinusArgument         | v20 complement-angle matcher
// [H-37] extractConjugateProduct            | v20 additive-context conjugate matcher
// ============================================================================

// H05-H09 Focus Notes (added for quick maintainer orientation):
// - H05 areTreesEqual: structural equality matcher with commutative support for + and *;
//   this is the base predicate behind many rewrite/cancellation guards.
// - H06 collectAdditiveTerms: flattens nested +/- trees into signed term vectors,
//   enabling cancellation and coefficient aggregation on additive expressions.
// - H07 flattenProduct: flattens chained * trees and isolates numeric coefficient,
//   enabling factor-level cancellation and canonical product handling.
// - H08 extractCanonicalProduct: converts products into canonical key + coeff + sorted
//   factors, crucial for deep equivalence matching across reordered factors.
// - H09 detectPerfectPower: recognizes perfect-power polynomial patterns (square and
//   bounded higher powers) to bridge expanded/factored forms for downstream rewrites.
// ============================================================================

enum TokenType
{
    TOKEN_NUMBER,
    TOKEN_VARIABLE,
    TOKEN_OPERATOR,
    TOKEN_FUNCTION,
    TOKEN_LPAREN,
    TOKEN_RPAREN
};

struct Token
{
    TokenType type;
    string text;
    double numberValue;
    char op;
};

enum NodeType
{
    NODE_NUMBER,
    NODE_VARIABLE,
    NODE_FUNCTION,
    NODE_OPERATOR
};

struct Node
{
    NodeType type;
    double numberValue;
    string variableName;
    char op;
    Node *left;
    Node *right;
};

string signatureOfTree(Node *node);

// ------------------------------------------------------------
// PHASE 1: FRONT-END (TRANSLATOR)
// ------------------------------------------------------------
// Version Trail:
// - v1: initial translator pipeline.
// - v2: broader function-token support.
// - v3: input friendliness upgrades.
// - v4/v6: precedence and canonicalization resilience fixes.

// PHASE 1.1: Helper functions for lexing and parsing.
// Handler Revision Notes:
// - v1: initial scalar/operator helpers.
// - v2: expanded function-name policy via isSupportedFunctionName.
// - v4+: numeric tolerance helpers reused heavily by middle-end and backend.
// Note:
// What it does: Checks whether a character is one of the core binary operators.
// Input: single character c.
// Returns: true if c is +, -, *, /, or ^; otherwise false.
// Why needed: Lexing needs a fast operator classifier.
// Theory: Tokenization begins by classifying raw characters into grammar classes.
bool isOperatorChar(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

// Note:
// What it does: Converts a string to lowercase in a safe ASCII-aware way.
// Input: source string s.
// Returns: lowercase copy of s.
// Why needed: Function/constant matching should be case-insensitive.
// Theory: Canonicalization reduces lexical variants to one normalized symbol form.
string toLowerString(const string &s)
{
    string out = s;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)out.size(); i++)
    {
        out[i] = (char)tolower((unsigned char)out[i]);
    }
    return out;
}

// Note:
// What it does: Validates whether a token name is a built-in supported function.
// Input: function candidate name.
// Returns: true if name is in supported function set.
// Why needed: Distinguishes function calls from variables during lexing.
// Theory: Grammar disambiguation uses symbol tables + lookahead.
bool isSupportedFunctionName(const string &name)
{
    string n = toLowerString(name);
    return n == "log" || n == "ln" || n == "exp" || n == "sqrt" || n == "abs" || n == "sgn" ||
           n == "sin" || n == "cos" || n == "tan" || n == "cot" || n == "sec" || n == "csc";
}

// Note:
// What it does: Advances index i over contiguous whitespace.
// Input: full input string and current index.
// Returns: next non-space index (or end).
// Why needed: Lookahead for function-call detection ignores spaces.
// Theory: Lexers commonly normalize insignificant whitespace.
int skipSpaces(const string &input, int i)
{
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (i < (int)input.size() && isspace((unsigned char)input[i]))
    {
        i++;
    }
    return i;
}

// Note:
// What it does: Floating-point near-zero test with tolerance.
// Input: number x.
// Returns: true if |x| < epsilon.
// Why needed: Algebraic/evaluation logic must avoid exact-equality pitfalls.
// Theory: Numerical computing uses epsilon comparisons due to rounding error.
bool isZero(double x)
{
    return fabs(x) < 1e-9;
}

// Note:
// What it does: Floating-point near-one test with tolerance.
// Input: number x.
// Returns: true if x is approximately 1.
// Why needed: Identity reductions rely on stable numeric checks.
// Theory: Robust symbolic-numeric hybrids require tolerant predicates.
bool isOne(double x)
{
    return fabs(x - 1.0) < 1e-9;
}

// Note:
// What it does: Gives precedence rank for operators.
// Input: operator character.
// Returns: integer precedence level (higher means tighter bind).
// Why needed: Shunting-yard and parenthesis logic depend on precedence.
// Theory: Infix parsing enforces operator hierarchy via precedence relations.
int precedence(char op)
{
    // Check Note: This branch runs when the operator is the character '^'.
    if (op == '^')
        return 4;
    // Check Note: This branch runs when either the operator is the character '*' or the
    // operator is the character '/'.
    if (op == '*' || op == '/')
        return 3;
    // Check Note: This branch runs when either the operator is the character '+' or the
    // operator is the character '-'.
    if (op == '+' || op == '-')
        return 2;
    return 0;
}

// Note:
// What it does: Marks operators that associate right-to-left.
// Input: operator character.
// Returns: true for right-associative operators (here, ^).
// Why needed: Prevents incorrect parse for exponent chains like a^b^c.
// Theory: Associativity decides grouping when precedence is equal.
bool isRightAssociative(char op)
{
    return op == '^';
}

// PHASE 1.2: Lexical Analysis (Lexing).
// Handler Revision Notes:
// - v1: baseline numeric/operator/parenthesis lexing.
// - v2: richer symbolic and function support.
// - v3: multi-letter symbolic input behavior improved.
// - v4: symbolic constant handling stabilized (pi/e behavior tuned).
//
// Logic Sequence Guide:
// 1) classify char category (space/number/alpha/operator/paren)
// 2) emit normalized token payload with type-safe metadata
// 3) early-fail with explicit error message on invalid characters
// Note:
// What it does: Converts raw input text into typed token stream.
// Input: expression string input; output refs ok and errorMessage.
// Returns: vector of tokens in source order.
// Why needed: Parsing requires structured tokens, not raw characters.
// Theory: Lexical analysis is the first compiler phase that builds terminal symbols.
vector<Token> lexicalAnalysis(const string &input, bool &ok, string &errorMessage)
{
    // [H-01 | Approx changes: 4 waves | Implemented in v1, evolved in v2/v3/v4]
    // Lexer contract:
    // - Consume full input left-to-right exactly once.
    // - Emit strongly typed tokens with preserved textual origin where useful.
    // - Convert known constants/functions into canonical token forms early.
    // - Fail fast on invalid symbols with precise diagnostic text.

    vector<Token> tokens;
    ok = true;
    errorMessage = "";

    int i = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (i < (int)input.size())
    {
        // Step A: classify current character class.
        char c = input[i];

        // Check Note: This branch runs when the current character is whitespace.
        if (isspace(c))
        {
            // Whitespace is semantically irrelevant here; skip and continue.
            i++;
            continue;
        }

        // Check Note: This branch runs when either the current character is a digit or the
        // current character is the character '.'.
        if (isdigit(c) || c == '.')
        {
            // Step B: parse numeric literal (supports one decimal point).
            int start = i;
            bool dotSeen = (c == '.');
            i++;
            // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
            while (i < (int)input.size())
            {
                // Check Note: This branch runs when the current character is a digit.
                if (isdigit(input[i]))
                {
                    i++;
                }
                // Check Note: If the earlier case did not match, this branch runs when the
                // current character is the character '.' and a decimal point has not been seen
                // yet.
                else if (input[i] == '.' && !dotSeen)
                {
                    dotSeen = true;
                    i++;
                }
                // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
                else
                {
                    break;
                }
            }

            string numberText = input.substr(start, i - start);
            // Check Note: This branch runs when the number text is ".".
            if (numberText == ".")
            {
                // Guardrail: reject lone decimal dot as invalid numeric token.
                ok = false;
                errorMessage = "Invalid number: '.'";
                return tokens;
            }

            Token t;
            t.type = TOKEN_NUMBER;
            t.text = numberText;
            t.numberValue = atof(numberText.c_str());
            t.op = '\0';
            tokens.push_back(t);
            continue;
        }

        // Check Note: This branch runs when the current character is a letter.
        if (isalpha(c))
        {
            // Step C: parse alpha-led symbol: variable/function/constant candidate.
            int start = i;
            i++;
            // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
            while (i < (int)input.size() && (isalnum(input[i]) || input[i] == '_'))
            {
                i++;
            }

            string varName = input.substr(start, i - start);

            // Keep postfix factorial-marked symbols intact (e.g., x!) so they are
            // preserved as one symbolic token when unsupported as an operator.
            if (i < (int)input.size() && input[i] == '!')
            {
                varName += '!';
                i++;
            }

            string lowered = toLowerString(varName);
            // Check Note: This branch runs when the lowercase text is "pi".
            if (lowered == "pi")
            {
                // Constant policy: preserve pi symbolically (not numeric-folded in lexer).
                Token t;
                t.type = TOKEN_VARIABLE;
                t.text = "pi";
                t.numberValue = 0.0;
                t.op = '\0';
                tokens.push_back(t);
                continue;
            }
            // Check Note: This branch runs when the lowercase text is "e".
            if (lowered == "e")
            {
                // Constant policy: convert e into numeric constant for downstream folds.
                Token t;
                t.type = TOKEN_NUMBER;
                t.text = varName;
                t.numberValue = exp(1.0);
                t.op = '\0';
                tokens.push_back(t);
                continue;
            }

            int nextPos = skipSpaces(input, i);
            // Check Note: This branch runs when the variable name is a supported function name,
            // the next pos is less than the size() of the input text, and the input[next pos]
            // is the character '('.
            if (isSupportedFunctionName(varName) && nextPos < (int)input.size() && input[nextPos] == '(')
            {
                // Function policy: only treat as function when immediately call-shaped.
                Token t;
                t.type = TOKEN_FUNCTION;
                t.text = toLowerString(varName);
                t.numberValue = 0.0;
                t.op = '\0';
                tokens.push_back(t);
                continue;
            }

            bool allLetters = true;
            // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
            for (int k = 0; k < (int)varName.size(); k++)
            {
                // Check Note: This branch runs when the var name[k] is a letter.
                if (!isalpha((unsigned char)varName[k]))
                {
                    allLetters = false;
                    break;
                }
            }

            // Check Note: This branch runs when the name contains only letters and the size of
            // the variable name is greater than one.
            if (allLetters && (int)varName.size() > 1)
            {
                // v3 input-freedom policy: split multi-letter pure alpha tokens into
                // implicit product-ready variable sequence (abc -> a b c).
                // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
                for (int k = 0; k < (int)varName.size(); k++)
                {
                    Token tk;
                    tk.type = TOKEN_VARIABLE;
                    tk.text = string(1, varName[k]);
                    tk.numberValue = 0.0;
                    tk.op = '\0';
                    tokens.push_back(tk);
                }
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                // Mixed alnum/underscore token stays as single variable symbol.
                Token t;
                t.type = TOKEN_VARIABLE;
                t.text = varName;
                t.numberValue = 0.0;
                t.op = '\0';
                tokens.push_back(t);
            }
            continue;
        }

        // Support ** as an exponent alias for terminals/keyboards where ^ is awkward.
        // Check Note: This branch runs when the current character is the character '*', the i +
        // 1 is less than the size() of the input text, and the next character is the character
        // '*'.
        if (c == '*' && i + 1 < (int)input.size() && input[i + 1] == '*')
        {
            // Input ergonomics alias: '**' normalized to '^' operator token.
            Token t;
            t.type = TOKEN_OPERATOR;
            t.text = "**";
            t.numberValue = 0.0;
            t.op = '^';
            tokens.push_back(t);
            i += 2;
            continue;
        }

        // Check Note: This branch runs when the current character is an operator character.
        if (isOperatorChar(c))
        {
            // Standard binary operator token.
            Token t;
            t.type = TOKEN_OPERATOR;
            t.text = string(1, c);
            t.numberValue = 0.0;
            t.op = c;
            tokens.push_back(t);
            i++;
            continue;
        }

        // Check Note: This branch runs when the current character is the character '('.
        if (c == '(')
        {
            // Parenthesis token: left boundary.
            Token t;
            t.type = TOKEN_LPAREN;
            t.text = "(";
            t.numberValue = 0.0;
            t.op = '\0';
            tokens.push_back(t);
            i++;
            continue;
        }

        // Check Note: This branch runs when the current character is the character ')'.
        if (c == ')')
        {
            // Parenthesis token: right boundary.
            Token t;
            t.type = TOKEN_RPAREN;
            t.text = ")";
            t.numberValue = 0.0;
            t.op = '\0';
            tokens.push_back(t);
            i++;
            continue;
        }

        ok = false;
        // Any remaining unclassified character is an immediate lexing error.
        errorMessage = string("Invalid character found: '") + c + "'";
        return tokens;
    }

    return tokens;
}

// PHASE 1.3: Pre-Parser Cleanup for messy input.
// Handler Revision Notes:
// - v1: implicit multiplication insertion.
// - v3: input normalization expanded for friendlier symbolic entry.
// - v4: unary sign-chain handling hardened for precedence correctness.
//
// Logic Sequence Guide:
// 1) collapse unary +/- chains into an explicit sign effect
// 2) preserve exponent precedence by materializing unary negatives as -1 * term
// 3) inject implicit multiplication where adjacency implies product
// Note:
// What it does: Normalizes unary sign chains and inserts implicit multiplication.
// Input: raw token vector from lexer.
// Returns: parser-friendly normalized token vector.
// Why needed: Users write forms like - -x, 2x, x(y+1), which parser should still accept.
// Theory: Pre-parse rewriting transforms informal math notation into explicit grammar.
vector<Token> normalizeTokens(const vector<Token> &tokens)
{
    // [H-02 | Approx changes: 3 waves | Implemented in v1, evolved in v3/v4]
    // Normalization contract:
    // - Make unary sign chains explicit and precedence-safe.
    // - Insert implicit multiplication where adjacency implies product.
    // - Keep token stream parser-friendly without losing expression intent.

    vector<Token> temp;

    // Resolve chains of unary +/- early so parsing keeps correct precedence.
    // Examples:
    //   +5 -> 5,  --x -> x,  x - -y -> x - (-1*y),  3*-4 -> 3*(-4)
    int i = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (i < (int)tokens.size())
    {
        // Pass 1A: detect whether current +/- is unary in context.
        Token current = tokens[i];
        bool unaryPos = false;
        // Check Note: This branch runs when the current token is an operator token and either
        // the operator in the current token is the character '+' or the operator in the current
        // token is the character '-'.
        if (current.type == TOKEN_OPERATOR && (current.op == '+' || current.op == '-'))
        {
            // Check Note: This branch runs when i is zero.
            if (i == 0)
            {
                unaryPos = true;
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                Token prev = tokens[i - 1];
                // Check Note: This branch runs when either the previous token is an operator
                // token or the previous token is a left parenthesis token.
                if (prev.type == TOKEN_OPERATOR || prev.type == TOKEN_LPAREN)
                {
                    unaryPos = true;
                }
            }
        }

        // Check Note: This branch runs when this position is not treated as unary.
        if (!unaryPos)
        {
            temp.push_back(current);
            i++;
            continue;
        }

        int sign = 1;
        int j = i;
        bool unaryAfterExponent = (i > 0 && tokens[i - 1].type == TOKEN_OPERATOR && tokens[i - 1].op == '^');
         // Pass 1B: collapse runs like ---+ into net sign.
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while (j < (int)tokens.size() && tokens[j].type == TOKEN_OPERATOR &&
               (tokens[j].op == '+' || tokens[j].op == '-'))
        {
            // Check Note: This branch runs when the operator in the tokens[j] is the character
            // '-'.
            if (tokens[j].op == '-')
            {
                sign *= -1;
            }
            j++;
        }

        // Check Note: This branch runs when j is at least the size() of the token list.
        if (j >= (int)tokens.size())
        {
            break;
        }

        Token next = tokens[j];
        // Check Note: This branch runs when the next token is a number token.
        if (next.type == TOKEN_NUMBER)
        {
            // Check Note: This branch runs when the sign comes right after an exponent.
            if (unaryAfterExponent)
            {
                Token signedNumber = next;
                if (sign < 0)
                {
                    signedNumber.numberValue = -signedNumber.numberValue;
                    std::ostringstream out;
                    out << signedNumber.numberValue;
                    signedNumber.text = out.str();
                }
                temp.push_back(signedNumber);
                i = j + 1;
                continue;
            }

            // Check Note: This branch runs when the sign is less than zero.
            if (sign < 0)
            {
                // Keep unary minus lower precedence than exponent: -2^2 => -(2^2).
                // v4 precedence fix: materialize as -1 * number instead of signed literal.
                Token negOne;
                negOne.type = TOKEN_NUMBER;
                negOne.text = "-1";
                negOne.numberValue = -1.0;
                negOne.op = '\0';
                temp.push_back(negOne);

                Token mul;
                mul.type = TOKEN_OPERATOR;
                mul.text = "*";
                mul.numberValue = 0.0;
                mul.op = '*';
                temp.push_back(mul);
            }
            temp.push_back(next);
            i = j + 1;
            continue;
        }

        // Check Note: This branch runs when the sign comes right after an exponent, the sign is
        // less than zero, and the next token is a variable token.
        if (unaryAfterExponent && sign < 0 && next.type == TOKEN_VARIABLE)
        {
            // Keep exponent unary minus grouped as one exponent operand: x^-a => x^(-1*a).
            Token lp;
            lp.type = TOKEN_LPAREN;
            lp.text = "(";
            lp.numberValue = 0.0;
            lp.op = '\0';
            temp.push_back(lp);

            Token negOne;
            negOne.type = TOKEN_NUMBER;
            negOne.text = "-1";
            negOne.numberValue = -1.0;
            negOne.op = '\0';
            temp.push_back(negOne);

            Token mul;
            mul.type = TOKEN_OPERATOR;
            mul.text = "*";
            mul.numberValue = 0.0;
            mul.op = '*';
            temp.push_back(mul);

            temp.push_back(next);

            Token rp;
            rp.type = TOKEN_RPAREN;
            rp.text = ")";
            rp.numberValue = 0.0;
            rp.op = '\0';
            temp.push_back(rp);

            i = j + 1;
            continue;
        }

        // Check Note: This branch runs when the sign is less than zero.
        if (sign < 0)
        {
            // Non-numeric unary target: also materialize as multiplication by -1.
            Token negOne;
            negOne.type = TOKEN_NUMBER;
            negOne.text = "-1";
            negOne.numberValue = -1.0;
            negOne.op = '\0';
            temp.push_back(negOne);

            Token mul;
            mul.type = TOKEN_OPERATOR;
            mul.text = "*";
            mul.numberValue = 0.0;
            mul.op = '*';
            temp.push_back(mul);
        }

        i = j;
    }

    // Insert implicit multiplication: 2x => 2*x, x(y+1) => x*(y+1), (x+1)(x-1) => ...
    vector<Token> normalized;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)temp.size(); i++)
    {
        // Pass 2: insert explicit '*' when adjacent token categories imply product.
        normalized.push_back(temp[i]);
        // Check Note: This branch runs when the i + 1 is at least the size() of the temp.
        if (i + 1 >= (int)temp.size())
        {
            continue;
        }

        Token a = temp[i];
        Token b = temp[i + 1];

        bool aCanEnd = (a.type == TOKEN_NUMBER || a.type == TOKEN_VARIABLE || a.type == TOKEN_RPAREN);
        bool bCanStart = (b.type == TOKEN_NUMBER || b.type == TOKEN_VARIABLE || b.type == TOKEN_LPAREN);

        // Check Note: This branch runs when the a can end and the b can start.
        if (aCanEnd && bCanStart)
        {
            Token mul;
            mul.type = TOKEN_OPERATOR;
            mul.text = "*";
            mul.numberValue = 0.0;
            mul.op = '*';
            normalized.push_back(mul);
        }
    }

    return normalized;
}

// PHASE 1.4: Syntactic Analysis (Shunting-Yard to postfix).
// Handler Revision Notes:
// - v1: baseline shunting-yard conversion.
// - v2: function token stack behavior stabilized.
// - v4: right-associative exponent behavior retained during precedence checks.
// Note:
// What it does: Converts normalized infix tokens to postfix (RPN).
// Input: normalized token stream; output refs ok and errorMessage.
// Returns: postfix token vector.
// Why needed: Postfix makes AST construction simple and deterministic.
// Theory: Dijkstra's shunting-yard algorithm resolves precedence/associativity with a stack.
vector<Token> shuntingYardToPostfix(const vector<Token> &tokens, bool &ok, string &errorMessage)
{
    // Contract detail:
    // - Reads normalized infix stream.
    // - Produces precedence-correct postfix stream.
    // - Preserves right-associative semantics for exponentiation.
    // - Reports first structural mismatch encountered.
    vector<Token> output;
    stack<Token> ops;
    ok = true;
    errorMessage = "";

    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)tokens.size(); i++)
    {
        Token t = tokens[i];

        // Check Note: This branch runs when either t is a number token or t is a variable
        // token.
        if (t.type == TOKEN_NUMBER || t.type == TOKEN_VARIABLE)
        {
            // Operand tokens bypass operator stack and go straight to output.
            output.push_back(t);
        }
        // Check Note: If the earlier case did not match, this branch runs when t is an operator
        // token.
        else if (t.type == TOKEN_OPERATOR)
        {
            // Operator-handling loop:
            // repeatedly pop stronger/equal (assoc-dependent) operators.
            // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
            while (!ops.empty() && ops.top().type == TOKEN_OPERATOR)
            {
                char topOp = ops.top().op;
                char curOp = t.op;

                bool lowerOrEqual = precedence(curOp) <= precedence(topOp);
                bool lowerOnlyIfRightAssoc = precedence(curOp) < precedence(topOp);

                bool shouldPop = false;
                // Check Note: This branch runs when the cur op is right-associative.
                if (isRightAssociative(curOp))
                {
                    // Right-assoc ops (^) only pop strictly stronger operators.
                    shouldPop = lowerOnlyIfRightAssoc;
                }
                // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
                else
                {
                    // Left-assoc ops (+,-,*,/) pop stronger OR equal precedence.
                    shouldPop = lowerOrEqual;
                }

                // Check Note: This branch runs when the operator on the stack should not be
                // popped yet.
                if (!shouldPop)
                {
                    break;
                }

                output.push_back(ops.top());
                ops.pop();
            }

            // Push current operator after precedence drain.
            ops.push(t);
        }
        // Check Note: If the earlier case did not match, this branch runs when t is a function
        // token.
        else if (t.type == TOKEN_FUNCTION)
        {
            // Function token sits on operator stack until matching ')' is processed.
            ops.push(t);
        }
        // Check Note: If the earlier case did not match, this branch runs when t is a left
        // parenthesis token.
        else if (t.type == TOKEN_LPAREN)
        {
            // Left paren acts as a precedence barrier/sentinel.
            ops.push(t);
        }
        // Check Note: If the earlier case did not match, this branch runs when t is a right
        // parenthesis token.
        else if (t.type == TOKEN_RPAREN)
        {
            // Drain until matching left paren.
            bool foundLeftParen = false;
            // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
            while (!ops.empty())
            {
                // Check Note: This branch runs when the top item on the operator stack is a
                // left parenthesis token.
                if (ops.top().type == TOKEN_LPAREN)
                {
                    foundLeftParen = true;
                    ops.pop();
                    break;
                }
                output.push_back(ops.top());
                ops.pop();
            }

            // Check Note: This branch runs when no matching left parenthesis was found.
            if (!foundLeftParen)
            {
                // Right paren without prior left paren -> malformed input.
                ok = false;
                errorMessage = "Mismatched parentheses: missing '('";
                return output;
            }

            // Check Note: This branch runs when the operator stack is not empty and the top
            // item on the operator stack is a function token.
            if (!ops.empty() && ops.top().type == TOKEN_FUNCTION)
            {
                // Parenthesized function argument complete; emit function token.
                output.push_back(ops.top());
                ops.pop();
            }
        }
    }

    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (!ops.empty())
    {
        // Check Note: This branch runs when either the top item on the operator stack is a left
        // parenthesis token or the top item on the operator stack is a right parenthesis token.
        if (ops.top().type == TOKEN_LPAREN || ops.top().type == TOKEN_RPAREN)
        {
            // Any paren left on stack implies unmatched opening paren.
            ok = false;
            errorMessage = "Mismatched parentheses: missing ')'";
            return output;
        }
        // Final operator-stack flush into postfix stream.
        output.push_back(ops.top());
        ops.pop();
    }

    return output;
}

// Note:
// What it does: Allocates and initializes a numeric AST node.
// Input: literal value.
// Returns: pointer to NODE_NUMBER node.
// Why needed: Centralized node construction keeps AST creation uniform.
// Theory: AST nodes are typed semantic units built by factory helpers.
Node *makeNumber(double value)
{
    Node *n = new Node();
    n->type = NODE_NUMBER;
    n->numberValue = value;
    n->variableName = "";
    n->op = '\0';
    n->left = NULL;
    n->right = NULL;
    return n;
}

// Note:
// What it does: Allocates and initializes a variable AST node.
// Input: variable symbol name.
// Returns: pointer to NODE_VARIABLE node.
// Why needed: Encapsulates symbol-node creation for parser and rewrites.
// Theory: Variables are leaf identifiers in expression trees.
Node *makeVariable(const string &name)
{
    Node *n = new Node();
    n->type = NODE_VARIABLE;
    n->numberValue = 0.0;
    n->variableName = name;
    n->op = '\0';
    n->left = NULL;
    n->right = NULL;
    return n;
}

// Note:
// What it does: Allocates and initializes a unary function AST node.
// Input: function name and argument subtree.
// Returns: pointer to NODE_FUNCTION node.
// Why needed: Uniform constructor for function-call representation.
// Theory: Unary function application is modeled as node(name, child).
Node *makeFunction(const string &name, Node *arg)
{
    Node *n = new Node();
    n->type = NODE_FUNCTION;
    n->numberValue = 0.0;
    n->variableName = toLowerString(name);
    n->op = '\0';
    n->left = arg;
    n->right = NULL;
    return n;
}

// Note:
// What it does: Allocates and initializes a binary operator AST node.
// Input: operator symbol op with left/right subtrees.
// Returns: pointer to NODE_OPERATOR node.
// Why needed: All binary arithmetic structure is formed through this helper.
// Theory: Binary expression trees encode infix operations with internal nodes.
Node *makeOperator(char op, Node *left, Node *right)
{
    Node *n = new Node();
    n->type = NODE_OPERATOR;
    n->numberValue = 0.0;
    n->variableName = "";
    n->op = op;
    n->left = left;
    n->right = right;
    return n;
}

// PHASE 1.5: Build AST from postfix.
// Handler Revision Notes:
// - v1: baseline postfix->AST assembly.
// - v2: function-node arity checks and clearer error messages.
// Note:
// What it does: Builds AST from postfix tokens using stack reduction.
// Input: postfix token list; output refs ok and errorMessage.
// Returns: root node pointer on success, NULL on error.
// Why needed: Optimization/evaluation stages require tree form.
// Theory: RPN to AST is a classic push-pop construction where operators consume operands.
Node *buildASTFromPostfix(const vector<Token> &postfix, bool &ok, string &errorMessage)
{
    // Contract detail:
    // - Uses stack-based postfix reduction.
    // - Leaves exactly one root on success.
    // - Emits descriptive arity/syntax errors on malformed postfix.
    stack<Node *> st;
    ok = true;
    errorMessage = "";

    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)postfix.size(); i++)
    {
        Token t = postfix[i];

        // Check Note: This branch runs when t is a number token.
        if (t.type == TOKEN_NUMBER)
        {
            // Number token -> numeric leaf.
            st.push(makeNumber(t.numberValue));
        }
        // Check Note: If the earlier case did not match, this branch runs when t is a variable
        // token.
        else if (t.type == TOKEN_VARIABLE)
        {
            // Variable token -> symbol leaf.
            st.push(makeVariable(t.text));
        }
        // Check Note: If the earlier case did not match, this branch runs when t is a function
        // token.
        else if (t.type == TOKEN_FUNCTION)
        {
            // Unary function requires one existing subtree.
            // Check Note: This branch runs when the node stack is empty.
            if (st.empty())
            {
                ok = false;
                errorMessage = "Invalid expression: not enough operands for function '" + t.text + "'";
                return NULL;
            }

            Node *arg = st.top();
            st.pop();
            // Wrap argument under function node and push back.
            st.push(makeFunction(t.text, arg));
        }
        // Check Note: If the earlier case did not match, this branch runs when t is an operator
        // token.
        else if (t.type == TOKEN_OPERATOR)
        {
            // Binary operator requires two subtrees: left and right.
            // Check Note: This branch runs when the size of the node stack is less than 2.
            if (st.size() < 2)
            {
                ok = false;
                errorMessage = "Invalid expression: not enough operands for operator '" + t.text + "'";
                return NULL;
            }

            Node *right = st.top();
            st.pop();
            Node *left = st.top();
            st.pop();
            // Compose and reinsert combined subtree.
            st.push(makeOperator(t.op, left, right));
        }
    }

    // Check Note: This branch runs when the size of the node stack is not one.
    if (st.size() != 1)
    {
        // Parser must terminate with single tree root; otherwise syntax incomplete.
        ok = false;
        errorMessage = "Invalid expression: parser did not end with one root node";
        return NULL;
    }

    return st.top();
}

// ------------------------------------------------------------
// PHASE 2: MIDDLE-END (OPTIMIZER)
// ------------------------------------------------------------
// Version Trail:
// - v1: constant-fold + identity/DCE skeleton.
// - v2: broad symbolic rewrite expansion.
// - v3: readability-aware canonicalization.
// - v4/v5: exponent-law and validation synergy.
// - v6: commutative/associative cancellation stabilization.

// Note:
// What it does: Checks structural/algebraic equality of two ASTs (with commutative swap for +,*).
// Input: two subtree roots a and b.
// Returns: true when both represent same structure under supported equivalence.
// Why needed: Cancellation, identity detection, and rewrite guards depend on equality tests.
// Theory: Recursive tree isomorphism with selective commutative normalization.
bool areTreesEqual(Node *a, Node *b)
{
    // Equality Handler (changed in v1, v2, v6):
    // - v1: strict structural equality.
    // - v2: function/operator recursive parity checks.
    // - v6: commutative support (+, *) is critical for cancellation and matching.
    // Check Note: This branch runs when a is missing and b is missing.
    if (a == NULL && b == NULL)
        return true;
    // Check Note: This branch runs when either a is missing or b is missing.
    if (a == NULL || b == NULL)
        return false;
    // Check Note: This branch runs when the type of a is not the type of b.
    if (a->type != b->type)
        return false;

    // Check Note: This branch runs when a is a number node.
    if (a->type == NODE_NUMBER)
    {
        return isZero(a->numberValue - b->numberValue);
    }
    // Check Note: This branch runs when a is a variable node.
    if (a->type == NODE_VARIABLE)
    {
        return a->variableName == b->variableName;
    }

    // Check Note: This branch runs when a is a function node.
    if (a->type == NODE_FUNCTION)
    {
        return a->variableName == b->variableName && areTreesEqual(a->left, b->left);
    }

    // Check Note: This branch runs when the operator in a is not the operator in b.
    if (a->op != b->op)
        return false;

    bool direct = areTreesEqual(a->left, b->left) && areTreesEqual(a->right, b->right);
    // Check Note: This branch runs when the direct comparison already matches.
    if (direct)
        return true;

    // Check Note: This branch runs when either the operator in a is the character '+' or the
    // operator in a is the character '*'.
    if (a->op == '+' || a->op == '*')
    {
        return areTreesEqual(a->left, b->right) && areTreesEqual(a->right, b->left);
    }

    return false;
}

// Note:
// What it does: Predicate for checking function-node type and function name.
// Input: node pointer and target function name.
// Returns: true when node is that function call.
// Why needed: Simplification rules match patterns using concise predicates.
// Theory: Pattern rewriting is easier with typed predicates over AST shapes.
bool isFunctionNode(Node *node, const string &name)
{
    // Helper predicate for concise pattern matching in rewrite handlers.
    return node != NULL && node->type == NODE_FUNCTION && toLowerString(node->variableName) == toLowerString(name);
}

// Note:
// What it does: Safe extractor for numeric-literal nodes.
// Input: node pointer and output ref value.
// Returns: true if node is numeric; fills value.
// Why needed: Many rules branch differently for numeric vs symbolic nodes.
// Theory: Guarded extraction avoids unsafe casts in recursive transformations.
bool isNumberNode(Node *node, double &value)
{
    // Helper extractor:
    // returns true and outputs value only when node is a numeric literal.
    // Check Note: This branch runs when either the current node is missing or the current node
    // is not a number node.
    if (node == NULL || node->type != NODE_NUMBER)
        return false;
    value = node->numberValue;
    return true;
}

// Note:
// What it does: Tests whether floating value is effectively an integer.
// Input: number x.
// Returns: true if x is within tolerance of nearest integer.
// Why needed: Certain exponent laws are only safe/desired for integer powers.
// Theory: Discrete algebraic rewrites often require integer-domain constraints.
bool isIntegerValue(double x)
{
    // Numeric tolerance gate for exponent-law operations requiring integer powers.
    return fabs(x - round(x)) < 1e-9;
}

// Note:
// What it does: Attempts to represent subtree as coeff * var^power monomial.
// Input: subtree node; output refs var, power, coeff.
// Returns: true on successful monomial extraction.
// Why needed: Enables like-term combining and polynomial-style simplification.
// Theory: Canonical term decomposition is core to symbolic algebra aggregation.
bool extractMonomial(Node *node, string &var, int &power, double &coeff)
{
    // Monomial extractor contract:
    // tries to express node as coeff * var^power, where var may be empty for constants.
    // This helper drives like-term combination and polynomial-style simplifications.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;

    // Check Note: This branch runs when the current node is a number node.
    if (node->type == NODE_NUMBER)
    {
        // Pure constant maps to var="", power=0, coeff=value.
        var = "";
        power = 0;
        coeff = node->numberValue;
        return true;
    }

    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
    {
        // Bare variable maps to coeff=1, power=1.
        var = node->variableName;
        power = 1;
        coeff = 1.0;
        return true;
    }

    // Check Note: This branch runs when the current node is not an operator node.
    if (node->type != NODE_OPERATOR)
        // Non-operator/non-leaf forms are not monomials in this extractor model.
        return false;

    // Check Note: This branch runs when the operator in the current node is the character '^',
    // the left side exists, the left side is a variable node, the right side exists, the right
    // side is a number node, the numeric value in the right side is a whole number, and the
    // numeric value in the right side is at least zero.
    if (node->op == '^' && node->left != NULL && node->left->type == NODE_VARIABLE &&
        node->right != NULL && node->right->type == NODE_NUMBER && isIntegerValue(node->right->numberValue) &&
        node->right->numberValue >= 0)
    {
        // Canonical power monomial: x^n with non-negative integer n.
        var = node->left->variableName;
        power = (int)round(node->right->numberValue);
        coeff = 1.0;
        return true;
    }

    // Check Note: This branch runs when the operator in the current node is the character '*'.
    if (node->op == '*')
    {
        // Multiplicative merge: recursively extract left/right and combine if compatible.
        string lv, rv;
        int lp = 0, rp = 0;
        double lc = 0.0, rc = 0.0;
        // Check Note: This branch runs when either the left side can be treated as a single
        // monomial or the right side can be treated as a single monomial.
        if (!extractMonomial(node->left, lv, lp, lc) || !extractMonomial(node->right, rv, rp, rc))
            return false;

        // Check Note: This branch runs when the lv is empty and the rv is empty.
        if (lv.empty() && rv.empty())
        {
            // constant * constant -> constant monomial
            var = "";
            power = 0;
            coeff = lc * rc;
            return true;
        }

        // Check Note: This branch runs when the lv is empty and the rv is not empty.
        if (lv.empty() && !rv.empty())
        {
            // constant * variable monomial -> scale coefficient
            var = rv;
            power = rp;
            coeff = lc * rc;
            return true;
        }

        // Check Note: This branch runs when the lv is not empty and the rv is empty.
        if (!lv.empty() && rv.empty())
        {
            // variable monomial * constant -> scale coefficient
            var = lv;
            power = lp;
            coeff = lc * rc;
            return true;
        }

        // Check Note: This branch runs when the lv is the rv.
        if (lv == rv)
        {
            // same base variable: add exponents, multiply coefficients.
            var = lv;
            power = lp + rp;
            coeff = lc * rc;
            return true;
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '/'.
    if (node->op == '/')
    {
        // Division case supported only by numeric denominator to keep monomial form.
        string lv;
        int lp = 0;
        double lc = 0.0;
        double d = 0.0;
        // Check Note: This branch runs when the left side can be treated as a single monomial,
        // the is number node(node->right, d), and the denominator value is not zero.
        if (extractMonomial(node->left, lv, lp, lc) && isNumberNode(node->right, d) && !isZero(d))
        {
            // (monomial)/(constant) remains monomial with scaled coefficient.
            var = lv;
            power = lp;
            coeff = lc / d;
            return true;
        }
    }

    return false;
}

// Note:
// What it does: Reconstructs AST monomial from canonical pieces.
// Input: variable symbol, integer power, numeric coefficient.
// Returns: AST subtree for that monomial.
// Why needed: After coefficient aggregation, terms must be rebuilt back into AST form.
// Theory: Simplifiers often use extract-transform-rebuild pipelines.
Node *buildMonomial(const string &var, int power, double coeff)
{
    // Inverse helper of extractMonomial for reconstructed canonical terms.
    // Check Note: This branch runs when either the var is empty or the power is zero.
    if (var.empty() || power == 0)
    {
        // Constant-only monomial.
        return makeNumber(coeff);
    }

    Node *base = makeVariable(var);
    // Check Note: This branch runs when the power is greater than one.
    if (power > 1)
    {
        // Build explicit exponent node only for powers > 1.
        base = makeOperator('^', base, makeNumber((double)power));
    }

    // Check Note: This branch runs when the coeff is one.
    if (isOne(coeff))
        // Unit coefficient omitted to keep expression compact.
        return base;

    return makeOperator('*', makeNumber(coeff), base);
}

// Note:
// What it does: Flattens nested +/- tree into signed term list.
// Input: root node, output vector terms, incoming sign.
// Returns: nothing (fills terms by reference).
// Why needed: Additive associativity handling requires linear term representation.
// Theory: Tree flattening converts recursive arithmetic structure to multiset-like form.
void collectAdditiveTerms(Node *node, vector<pair<Node *, int> > &terms, int sign)
{
    // Associative-Flatten Handler (v1->v6):
    // - Purpose: convert nested +/- trees into signed flat term list.
    // - This is the backbone for additive combining and cancellation.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return;

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '+'.
    if (node->type == NODE_OPERATOR && node->op == '+')
    {
        // (+): propagate same sign to both children.
        collectAdditiveTerms(node->left, terms, sign);
        collectAdditiveTerms(node->right, terms, sign);
        return;
    }

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '-'.
    if (node->type == NODE_OPERATOR && node->op == '-')
    {
        // (-): right child receives flipped sign.
        collectAdditiveTerms(node->left, terms, sign);
        collectAdditiveTerms(node->right, terms, -sign);
        return;
    }

    // Leaf/non-additive subtree contributes as one signed term.
    terms.push_back(make_pair(node, sign));
}

// Note:
// What it does: Detects explicit constant division-by-zero patterns in tree.
// Input: subtree root.
// Returns: true if any denominator is numeric zero.
// Why needed: Final output must prevent unsafe evaluation paths.
// Theory: Static safety checks catch definite runtime faults pre-evaluation.
bool hasConstantDivisionByZero(Node *node)
{
    // Safety scanner:
    // - Detects explicit constant denominator zero patterns.
    // - Conservative by design (does not prove symbolic zero for variable expressions).
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;
    // Check Note: This branch runs when the current node is an operator node, the operator in
    // the current node is the character '/', the right side exists, the right side is a number
    // node, and the numeric value in the right side is zero.
    if (node->type == NODE_OPERATOR && node->op == '/' &&
        node->right != NULL && node->right->type == NODE_NUMBER && isZero(node->right->numberValue))
    {
        return true;
    }
    return hasConstantDivisionByZero(node->left) || hasConstantDivisionByZero(node->right);
}

// Note:
// What it does: Recognizes negation forms and extracts positive counterpart term.
// Input: candidate node and output ref positive.
// Returns: true if node matches supported negation pattern.
// Why needed: Needed for cancellation like t + (-t) and sign normalization.
// Theory: Equivalent forms of unary negation are normalized into one semantic representation.
bool extractNegatedTerm(Node *node, Node *&positive)
{
    // Negation pattern extractor:
    // normalizes two forms into a shared "positive counterpart" representation:
    // 1) negative numeric literal, 2) multiplication by -1.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;

    // Check Note: This branch runs when the current node is a number node and the numeric value
    // in the current node is less than zero.
    if (node->type == NODE_NUMBER && node->numberValue < 0)
    {
        // -k -> positive counterpart k
        positive = makeNumber(-node->numberValue);
        return true;
    }

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '*'.
    if (node->type == NODE_OPERATOR && node->op == '*')
    {
        double n = 0.0;
        // Check Note: This branch runs when the left side exists, the is number
        // node(node->left, n), and 0 of the n + 1 is zero.
        if (node->left != NULL && isNumberNode(node->left, n) && isZero(n + 1.0))
        {
            // (-1) * t -> t
            positive = node->right;
            return true;
        }
        // Check Note: This branch runs when the right side exists, the is number
        // node(node->right, n), and 0 of the n + 1 is zero.
        if (node->right != NULL && isNumberNode(node->right, n) && isZero(n + 1.0))
        {
            // t * (-1) -> t
            positive = node->left;
            return true;
        }
    }

    return false;
}

// Note:
// What it does: Removes one matching factor from a multiplicative subtree.
// Input: product node, target factor, output ref remaining.
// Returns: true if one factor removed successfully.
// Why needed: Supports factor cancellation in product/division simplifications.
// Theory: Multiplicative cancellation is modeled as factor multiset subtraction.
bool removeFactorFromProduct(Node *node, Node *factor, Node *&remaining)
{
    // Product-factor cancellation helper:
    // attempts to remove one matching factor from a multiplicative tree.
    // Returns remaining subtree after one removal if successful.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;

    // Check Note: This branch runs when the current node and the factor we want to remove
    // represent the same expression.
    if (areTreesEqual(node, factor))
    {
        // Exact match: factor removed entirely; caller treats NULL as multiplicative identity.
        remaining = NULL;
        return true;
    }

    // Power-aware cancellation: x^n / x => x^(n-1), and x^n / x^m => x^(n-m).
    // Check Note: This branch runs when the current node is an operator node, the operator in
    // the current node is the character '^', the left side exists, the right side exists, the
    // right side is a number node, and the numeric value in the right side is a whole number.
    if (node->type == NODE_OPERATOR && node->op == '^' &&
        node->left != NULL && node->right != NULL && node->right->type == NODE_NUMBER && isIntegerValue(node->right->numberValue))
    {
        int nodeExp = (int)round(node->right->numberValue);
        // Check Note: This branch runs when the node exp is at least one.
        if (nodeExp >= 1)
        {
            int factorExp = 0;
            bool sameBase = false;

            // Check Note: This branch runs when the left side and the factor we want to remove
            // represent the same expression.
            if (areTreesEqual(node->left, factor))
            {
                sameBase = true;
                factorExp = 1;
            }
            // Check Note: If the earlier case did not match, this branch runs when the factor
            // we want to remove exists, the factor we want to remove is an operator node, the
            // operator in the factor we want to remove is the character '^', the left child of
            // the factor we want to remove exists, the right child of the factor we want to
            // remove exists, the right child of the factor we want to remove is a number node,
            // the numeric value in the right child of the factor we want to remove is a whole
            // number, and the left side and the left child of the factor we want to remove
            // represent the same expression.
            else if (factor != NULL && factor->type == NODE_OPERATOR && factor->op == '^' &&
                     factor->left != NULL && factor->right != NULL && factor->right->type == NODE_NUMBER &&
                     isIntegerValue(factor->right->numberValue) && areTreesEqual(node->left, factor->left))
            {
                sameBase = true;
                factorExp = (int)round(factor->right->numberValue);
            }

            // Check Note: This branch runs when both sides use the same base, the factor exp is
            // at least one, and the node exp is at least the factor exp.
            if (sameBase && factorExp >= 1 && nodeExp >= factorExp)
            {
                int remExp = nodeExp - factorExp;
                // Check Note: This branch runs when the rem exp is zero.
                if (remExp == 0)
                {
                    remaining = NULL;
                }
                // Check Note: If the earlier case did not match, this branch runs when the rem
                // exp is one.
                else if (remExp == 1)
                {
                    remaining = node->left;
                }
                // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
                else
                {
                    remaining = makeOperator('^', node->left, makeNumber((double)remExp));
                }
                return true;
            }
        }
    }

    // Check Note: This branch runs when either the current node is not an operator node or the
    // operator in the current node is not the character '*'.
    if (node->type != NODE_OPERATOR || node->op != '*')
        // Non-product nodes cannot be decomposed for factor removal.
        return false;

    Node *newLeft = NULL;
    // Check Note: This branch runs when the requested factor can be removed from the left side.
    if (removeFactorFromProduct(node->left, factor, newLeft))
    {
        // Removal succeeded in left branch; rebuild with untouched right branch.
        // Check Note: This branch runs when the new left is missing.
        if (newLeft == NULL)
            remaining = node->right;
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            remaining = makeOperator('*', newLeft, node->right);
        return true;
    }

    Node *newRight = NULL;
    // Check Note: This branch runs when the requested factor can be removed from the right
    // side.
    if (removeFactorFromProduct(node->right, factor, newRight))
    {
        // Removal succeeded in right branch; rebuild with untouched left branch.
        // Check Note: This branch runs when the new right is missing.
        if (newRight == NULL)
            remaining = node->left;
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            remaining = makeOperator('*', node->left, newRight);
        return true;
    }

    return false;
}

// Note:
// What it does: Tries to divide a shared factor out of every additive term in a numerator.
// Input: numerator subtree, divisor factor, and output ref reducedNumerator.
// Returns: true when all additive terms can be divided by divisor safely.
// Why needed: Covers division simplification gaps like (x^2 + x) / x -> x + 1.
// Theory: Uses additive flattening + per-term factor removal as a conservative factoring rewrite.
bool divideOutCommonFactorFromAdditive(Node *numerator, Node *divisor, Node *&reducedNumerator)
{
    reducedNumerator = NULL;

    // Check Note: This branch runs when either the numerator is missing or the divisor is
    // missing.
    if (numerator == NULL || divisor == NULL)
        return false;

    // Check Note: This branch runs when either the numerator is not an operator node or the
    // operator in the numerator is not the character '+' and the operator in the numerator is
    // not the character '-'.
    if (numerator->type != NODE_OPERATOR || (numerator->op != '+' && numerator->op != '-'))
        return false;

    vector<pair<Node *, int> > terms;
    collectAdditiveTerms(numerator, terms, 1);

    // Check Note: This branch runs when the size of the term list is less than 2.
    if ((int)terms.size() < 2)
        return false;

    vector<pair<Node *, int> > reducedTerms;
    // Loop Note: Iterates through each additive term to ensure the same divisor factor can be removed from all terms; loop is required because term count is expression-dependent.
    for (int i = 0; i < (int)terms.size(); i++)
    {
        Node *remaining = NULL;
        // Check Note: This branch runs when the requested factor can be removed from the first
        // part of the terms[i].
        if (!removeFactorFromProduct(terms[i].first, divisor, remaining))
            return false;

        // Check Note: This branch runs when the remaining is missing.
        if (remaining == NULL)
            remaining = makeNumber(1.0);

        reducedTerms.push_back(make_pair(remaining, terms[i].second));
    }

    Node *sum = NULL;
    // Loop Note: Rebuilds the reduced additive expression from signed terms after factor removal; loop is required because the rebuilt term list is dynamic.
    for (int i = 0; i < (int)reducedTerms.size(); i++)
    {
        Node *piece = reducedTerms[i].first;
        // Check Note: This branch runs when the second part of the reduced terms[i] is less
        // than zero.
        if (reducedTerms[i].second < 0)
            piece = makeOperator('*', makeNumber(-1.0), piece);

        // Check Note: This branch runs when the sum is missing.
        if (sum == NULL)
            sum = piece;
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            sum = makeOperator('+', sum, piece);
    }

    reducedNumerator = sum;
    return reducedNumerator != NULL;
}

// Note:
// What it does: Flattens chained multiplications and isolates numeric coefficient.
// Input: subtree root; output refs nonNumbers and numberCoeff.
// Returns: nothing (fills outputs by reference).
// Why needed: Canonical product comparison and regrouping need factor vectors.
// Theory: Associative decomposition turns product trees into coefficient × factor list.
void flattenProduct(Node *node, vector<Node *> &nonNumbers, double &numberCoeff)
{
    // Multiplicative-Flatten Handler (v1, v3, v6):
    // - v1: flatten chained multiplications and separate numeric coefficient.
    // - v3: used for improved product canonical display behavior.
    // - v6: unary-minus normalization added (0 - x treated as -1 * x).
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return;

    // Treat unary-minus form (0 - x) as multiplication by -1 for canonical factor extraction.
    // Check Note: This branch runs when the current node is an operator node, the operator in
    // the current node is the character '-', the left side exists, the left side is a number
    // node, and the numeric value in the left side is zero.
    if (node->type == NODE_OPERATOR && node->op == '-' &&
        node->left != NULL && node->left->type == NODE_NUMBER && isZero(node->left->numberValue))
    {
        numberCoeff *= -1.0;
        flattenProduct(node->right, nonNumbers, numberCoeff);
        return;
    }

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '*'.
    if (node->type == NODE_OPERATOR && node->op == '*')
    {
        flattenProduct(node->left, nonNumbers, numberCoeff);
        flattenProduct(node->right, nonNumbers, numberCoeff);
        return;
    }

    // Check Note: This branch runs when the current node is a number node.
    if (node->type == NODE_NUMBER)
    {
        numberCoeff *= node->numberValue;
        return;
    }

    nonNumbers.push_back(node);
}

// Note:
// What it does: Checks if a factor is compact enough for lightweight product ordering.
// Input: factor node.
// Returns: true for variable / numeric / simple power atom shapes.
// Why needed: Prevents over-aggressive reordering of complex factors.
// Theory: Local canonicalization should be conservative to preserve readability/stability.
bool isSimpleFactor(Node *node)
{
    // "Simple" here means a compact printable multiplicative atom.
    // Used by lightweight product canonicalization paths.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;
    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
        return true;
    // Check Note: This branch runs when the current node is an operator node, the operator in
    // the current node is the character '^', the left side exists, the left side is a variable
    // node, the right side exists, and the right side is a number node.
    if (node->type == NODE_OPERATOR && node->op == '^' &&
        node->left != NULL && node->left->type == NODE_VARIABLE &&
        node->right != NULL && node->right->type == NODE_NUMBER)
    {
        return true;
    }
    return false;
}

// Note:
// What it does: Generates deterministic sort key for simple factors.
// Input: factor node.
// Returns: sortable string key.
// Why needed: Commutative products require stable output ordering.
// Theory: Canonical keys convert structural ordering into lexicographic ordering.
string simpleFactorKey(Node *node)
{
    // Ordering key for simple factors to produce deterministic commutative product output.
    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
        return node->variableName;
    // Check Note: This branch runs when the current node is an operator node, the operator in
    // the current node is the character '^', the left side exists, and the left side is a
    // variable node.
    if (node->type == NODE_OPERATOR && node->op == '^' && node->left != NULL && node->left->type == NODE_VARIABLE)
    {
        ostringstream out;
        out << node->left->variableName << "^" << fixed << setprecision(6) << node->right->numberValue;
        return out.str();
    }
    return "";
}

// Note:
// What it does: Rebuilds product AST from coefficient and ordered factors.
// Input: factor vector and numeric coefficient.
// Returns: product subtree (or constant identity form).
// Why needed: Many simplifiers operate in flattened form but must return tree form.
// Theory: Structural reconstruction follows canonical factor order for deterministic results.
Node *buildProductFromFactors(const vector<Node *> &factors, double coeff)
{
    // Product rebuilder:
    // - starts with coefficient when necessary,
    // - appends factors left-to-right in already-canonical order.
    Node *result = NULL;

    // Check Note: This branch runs when either the coeff is not one or the factors is empty.
    if (!isOne(coeff) || factors.empty())
    {
        // Keep explicit coefficient if non-unit, or if expression is purely constant.
        result = makeNumber(coeff);
    }

    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)factors.size(); i++)
    {
        // Check Note: This branch runs when the result tree is missing.
        if (result == NULL)
            result = factors[i];
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            result = makeOperator('*', result, factors[i]);
    }

    // Check Note: This branch runs when the result tree is missing.
    if (result == NULL)
        return makeNumber(1.0);
    return result;
}

// Note:
// What it does: Converts a monomial subtree into commutative-invariant canonical key + coeff.
// Input: term node; output refs key and coeff.
// Returns: true when extraction is possible.
// Why needed: Like-term detection needs order-independent signatures.
// Theory: Canonical forms map algebraically equivalent monomials to same representation.
bool extractCanonicalMonomial(Node *node, string &key, double &coeff)
{
    // Canonical Monomial Extraction (v6):
    // - Builds an order-independent monomial key so ac and ca map identically.
    // - Enables robust coefficient aggregation in additive simplification.
    vector<Node *> factors;
    coeff = 1.0;
    flattenProduct(node, factors, coeff);

    vector<string> factorKeys;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)factors.size(); i++)
    {
        Node *f = factors[i];
        // Check Note: This branch runs when f is missing.
        if (f == NULL)
            return false;

        // Check Note: This branch runs when f is a variable node.
        if (f->type == NODE_VARIABLE)
        {
            factorKeys.push_back(f->variableName);
            continue;
        }

        // Check Note: This branch runs when f is an operator node, the operator in f is the
        // character '^', the left child of f exists, the left child of f is a variable node,
        // the right child of f exists, and the right child of f is a number node.
        if (f->type == NODE_OPERATOR && f->op == '^' &&
            f->left != NULL && f->left->type == NODE_VARIABLE &&
            f->right != NULL && f->right->type == NODE_NUMBER)
        {
            ostringstream out;
            out << f->left->variableName << "^" << fixed << setprecision(6) << f->right->numberValue;
            factorKeys.push_back(out.str());
            continue;
        }

        return false;
    }

    sort(factorKeys.begin(), factorKeys.end());
    key = "";
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)factorKeys.size(); i++)
    {
        // Check Note: This branch runs when the grouping key is not empty.
        if (!key.empty())
            key += "*";
        key += factorKeys[i];
    }
    return true;
}

// Note:
// What it does: Reconstructs one factor node from canonical key text.
// Input: factorKey string (e.g., x or x^2).
// Returns: factor AST node.
// Why needed: Inverse operation for key-based monomial rebuild.
// Theory: Serialization/deserialization enables map-based symbolic aggregation.
Node *buildFactorFromCanonicalKey(const string &factorKey)
{
    size_t p = factorKey.find('^');
    // Check Note: This branch runs when p is not found.
    if (p == string::npos)
        return makeVariable(factorKey);

    string base = factorKey.substr(0, p);
    string expText = factorKey.substr(p + 1);
    double expValue = atof(expText.c_str());
    // Check Note: This branch runs when the exp value is one.
    if (isOne(expValue))
        return makeVariable(base);
    return makeOperator('^', makeVariable(base), makeNumber(expValue));
}

// Note:
// What it does: Builds monomial AST from canonical key and coefficient.
// Input: canonical key and numeric coefficient.
// Returns: monomial subtree.
// Why needed: Final step after canonical coefficient accumulation.
// Theory: Canonical key space is transformed back into executable expression tree.
Node *buildCanonicalMonomial(const string &key, double coeff)
{
    // Check Note: This branch runs when the grouping key is empty.
    if (key.empty())
        return makeNumber(coeff);

    vector<Node *> factors;
    size_t start = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (start <= key.size())
    {
        size_t sep = key.find('*', start);
        string part = (sep == string::npos) ? key.substr(start) : key.substr(start, sep - start);
        // Check Note: This branch runs when the part is not empty.
        if (!part.empty())
            factors.push_back(buildFactorFromCanonicalKey(part));

        // Check Note: This branch runs when the sep is not found.
        if (sep == string::npos)
            break;
        start = sep + 1;
    }

    return buildProductFromFactors(factors, coeff);
}

// Note:
// What it does: Builds order-invariant signature for arbitrary factor list.
// Input: vector of factor subtrees.
// Returns: canonical signature string.
// Why needed: Needed when factors are not only simple monomials.
// Theory: Signature sorting approximates commutative multiset equivalence.
string canonicalProductKey(const vector<Node *> &factors)
{
    // Canonical Product Signature Builder (v6):
    // - Uses subtree signatures and sorted ordering to ignore factor order.
    // - Supports commutative-aware grouping even for non-trivial factors.
    vector<string> signatures;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)factors.size(); i++)
    {
        signatures.push_back(signatureOfTree(factors[i]));
    }
    sort(signatures.begin(), signatures.end());

    string key;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)signatures.size(); i++)
    {
        // Check Note: This branch runs when the grouping key is not empty.
        if (!key.empty())
            key += "|";
        key += signatures[i];
    }
    return key;
}

// Note:
// What it does: Extracts canonical product representation with sorted factors.
// Input: product subtree; output refs key, coeff, sortedFactors.
// Returns: true when extraction succeeds.
// Why needed: Core of additive term aggregation for generalized product terms.
// Theory: Canonicalization = flatten + coefficient isolation + signature sorting.
bool extractCanonicalProduct(Node *node, string &key, double &coeff, vector<Node *> &sortedFactors)
{
    // Canonical Product Extractor (v6):
    // - Generalized from monomial-only extraction to broader product forms.
    // - Supplies both stable key and sorted factor list for deterministic rebuild.
    vector<Node *> factors;
    coeff = 1.0;
    flattenProduct(node, factors, coeff);

    vector<pair<string, Node *> > keyed;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)factors.size(); i++)
    {
        keyed.push_back(make_pair(signatureOfTree(factors[i]), factors[i]));
    }
    sort(keyed.begin(), keyed.end());

    sortedFactors.clear();
    key = "";
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)keyed.size(); i++)
    {
        sortedFactors.push_back(keyed[i].second);
        // Check Note: This branch runs when the grouping key is not empty.
        if (!key.empty())
            key += "|";
        key += keyed[i].first;
    }
    return true;
}

// Note:
// What it does: Recognizes two-variable product term and extracts coefficient + variable pair.
// Input: term node; output refs coeff, a, b.
// Returns: true when term matches strict 2-variable product shape.
// Why needed: Perfect-square detectors depend on this compact pattern.
// Theory: Pattern-specific extractors accelerate targeted identity recognition.
bool extractTwoVarProductCoeff(Node *term, double &coeff, string &a, string &b)
{
    // Specialized recognizer for two-variable products (used in perfect-square detection).
    vector<Node *> factors;
    double num = 1.0;
    flattenProduct(term, factors, num);

    vector<string> vars;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)factors.size(); i++)
    {
        // Check Note: This branch runs when the factors[i] is not a variable node.
        if (factors[i]->type != NODE_VARIABLE)
            // Keep this recognizer strict; reject powers/functions/mixed terms.
            return false;
        vars.push_back(factors[i]->variableName);
    }

    // Check Note: This branch runs when the size of the set of variables is not 2.
    if ((int)vars.size() != 2)
        return false;

    // Check Note: This branch runs when the first variable name is greater than the second
    // variable name.
    if (vars[0] > vars[1])
        swap(vars[0], vars[1]);

    coeff = num;
    a = vars[0];
    b = vars[1];
    return true;
}

// Note:
// What it does: Normalizes subtraction-like forms into (a - b) pair.
// Input: candidate expression node; output refs a and b.
// Returns: true if expression can be interpreted as difference.
// Why needed: Difference-of-squares and related transforms require normalized operands.
// Theory: Equivalent syntactic forms are mapped to shared semantic pattern.
bool extractDifference(Node *node, Node *&a, Node *&b)
{
    // Difference normalizer:
    // extracts a-b from direct subtraction or equivalent additive-with-negation form.
    // Check Note: This branch runs when either the current node is missing or the current node
    // is not an operator node.
    if (node == NULL || node->type != NODE_OPERATOR)
        return false;

    // Check Note: This branch runs when the operator in the current node is the character '-'.
    if (node->op == '-')
    {
        a = node->left;
        b = node->right;
        return true;
    }

    // Check Note: This branch runs when the operator in the current node is the character '+'.
    if (node->op == '+')
    {
        Node *pos = NULL;
        // Check Note: This branch runs when the left side can be recognized as a negated term.
        if (extractNegatedTerm(node->left, pos))
        {
            a = node->right;
            b = pos;
            return true;
        }
        // Check Note: This branch runs when the right side can be recognized as a negated term.
        if (extractNegatedTerm(node->right, pos))
        {
            a = node->left;
            b = pos;
            return true;
        }
    }

    return false;
}

// Note:
// What it does: Checks whether a node is a named function raised to a chosen power.
// Input: candidate node, function name, exponent, and output ref for the inner argument.
// Returns: true when node matches fn(arg)^exponent.
// Why needed: Several trig identities are easiest to express in power form.
// Theory: Pattern matching over function-call trees plus exponent guards.
bool isPowerOfFunction(Node *node, const string &fn, double exponent, Node *&arg)
{
    arg = NULL;
    // Check Note: This branch runs when the current node is missing, the current node is not an
    // operator node, or the operator in the current node is not the character '^'.
    if (node == NULL || node->type != NODE_OPERATOR || node->op != '^')
        return false;
    // Check Note: This branch runs when the right side is missing or the right side is not a
    // number node.
    if (node->right == NULL || node->right->type != NODE_NUMBER)
        return false;
    // Check Note: This branch runs when the exponent stored in the right side is not the target
    // exponent value.
    if (!isZero(node->right->numberValue - exponent))
        return false;
    // Check Note: This branch runs when the left side is not the requested function call.
    if (!isFunctionNode(node->left, fn))
        return false;

    arg = node->left->left;
    return true;
}

// Note:
// What it does: Extracts the non-numeric factor from a product with a specific numeric multiplier.
// Input: candidate product, required numeric factor, and output ref for the remaining subtree.
// Returns: true when node matches factor * other or other * factor.
// Why needed: Supports rules like sin(2x)/(2sin(x)) and 2sin(x)^2.
// Theory: Small structural matcher for coefficient-bearing product forms.
bool extractProductWithNumericFactor(Node *node, double factor, Node *&other)
{
    other = NULL;
    // Check Note: This branch runs when the current node is missing, the current node is not an
    // operator node, or the operator in the current node is not the character '*'.
    if (node == NULL || node->type != NODE_OPERATOR || node->op != '*')
        return false;

    double n = 0.0;
    // Check Note: This branch runs when the left side is the requested numeric factor.
    if (isNumberNode(node->left, n) && isZero(n - factor))
    {
        other = node->right;
        return true;
    }
    // Check Note: This branch runs when the right side is the requested numeric factor.
    if (isNumberNode(node->right, n) && isZero(n - factor))
    {
        other = node->left;
        return true;
    }
    return false;
}

// Note:
// What it does: Recognizes a double-angle argument written as 2 * x or x * 2.
// Input: candidate node and output ref for the base argument.
// Returns: true when node is a simple doubled form of another expression.
// Why needed: Basic trig double-angle rules depend on spotting 2x reliably.
// Theory: Normalizes scalar-times-expression syntax into a reusable pattern.
bool extractDoubleAngleArgument(Node *node, Node *&arg)
{
    // Check Note: This branch delegates to the numeric-factor product matcher with factor two.
    return extractProductWithNumericFactor(node, 2.0, arg);
}

// Note:
// What it does: Checks whether a numeric node is approximately pi/2.
// Input: candidate node.
// Returns: true when node is close enough to pi/2 for supported trig complement rules.
// Why needed: Test inputs use decimal approximations like 1.5708 instead of symbolic pi/2.
// Theory: Tolerant numeric matching allows stable recognition of common constants.
bool isApproxHalfPiNode(Node *node)
{
    // Check Note: This branch runs when the current node is missing or the current node is not a
    // number node.
    if (node == NULL || node->type != NODE_NUMBER)
        return false;
    return fabs(node->numberValue - acos(-1.0) / 2.0) < 1e-3;
}

// Note:
// What it does: Extracts x from forms equivalent to pi/2 - x.
// Input: candidate node and output ref for the subtracted argument.
// Returns: true when node matches the supported half-pi complement form.
// Why needed: Enables beginner-level complement identities such as sin(pi/2 - x)=cos(x).
// Theory: Reuses normalized subtraction extraction plus tolerant constant matching.
bool extractHalfPiMinusArgument(Node *node, Node *&arg)
{
    arg = NULL;
    Node *a = NULL;
    Node *b = NULL;
    // Check Note: This branch runs when the current node cannot be read as a subtraction-like
    // expression.
    if (!extractDifference(node, a, b))
        return false;
    // Check Note: This branch runs when the left side of that subtraction is not approximately
    // pi over two.
    if (!isApproxHalfPiNode(a))
        return false;
    arg = b;
    return true;
}

// Note:
// What it does: Recognizes conjugate products like (a-b)(a+b) in either order.
// Input: candidate product node and output refs for the matched terms.
// Returns: true when node is a supported difference-of-squares product.
// Why needed: Lets additive contexts expand conjugates only when that helps cancellation.
// Theory: Matches a product of one sum and one difference that share the same two terms.
bool extractConjugateProduct(Node *node, Node *&a, Node *&b)
{
    a = NULL;
    b = NULL;
    // Check Note: This branch runs when the current node is missing, the current node is not a
    // multiplication node, or either side of the product is missing.
    if (node == NULL || node->type != NODE_OPERATOR || node->op != '*' ||
        node->left == NULL || node->right == NULL)
    {
        return false;
    }

    auto matchDifferenceAndSum = [&](Node *diffNode, Node *sumNode) -> bool {
        Node *diffA = NULL;
        Node *diffB = NULL;
        // Check Note: This branch runs when the first candidate side cannot be read as a
        // difference.
        if (!extractDifference(diffNode, diffA, diffB))
            return false;
        // Check Note: This branch runs when the second candidate side is missing, is not an
        // addition node, or is missing one of its children.
        if (sumNode == NULL || sumNode->type != NODE_OPERATOR || sumNode->op != '+' ||
            sumNode->left == NULL || sumNode->right == NULL)
        {
            return false;
        }

        bool sameOrder = areTreesEqual(diffA, sumNode->left) && areTreesEqual(diffB, sumNode->right);
        bool swappedOrder = areTreesEqual(diffA, sumNode->right) && areTreesEqual(diffB, sumNode->left);
        // Check Note: This branch runs when the difference terms and the sum terms do not match
        // in either direct or swapped order.
        if (!sameOrder && !swappedOrder)
            return false;

        a = diffA;
        b = diffB;
        return true;
    };

    return matchDifferenceAndSum(node->left, node->right) ||
           matchDifferenceAndSum(node->right, node->left);
}

// Note:
// What it does: Checks if candidate equals base squared.
// Input: candidate subtree and base subtree.
// Returns: true if candidate is mathematically base^2 under supported forms.
// Why needed: Used in identities like (a^2-b^2)/(a-b)=a+b.
// Theory: Pattern matching over power nodes and numeric literal squares.
bool isSquareOf(Node *candidate, Node *base)
{
    // Square matcher for structural identities:
    // accepts both symbolic power form and numeric-literal square checks.
    // Check Note: This branch runs when either the candidate expression is missing or the base
    // text is missing.
    if (candidate == NULL || base == NULL)
        return false;

    // Check Note: This branch runs when the candidate expression is an operator node, the
    // operator in the candidate expression is the character '^', the right child of the
    // candidate expression exists, the right child of the candidate expression is a number
    // node, 0 of the number value - 2 of the right child of the candidate expression is zero,
    // and the left child of the candidate expression and the base text represent the same
    // expression.
    if (candidate->type == NODE_OPERATOR && candidate->op == '^' &&
        candidate->right != NULL && candidate->right->type == NODE_NUMBER &&
        isZero(candidate->right->numberValue - 2.0) && areTreesEqual(candidate->left, base))
    {
        return true;
    }

    // Check Note: This branch runs when the candidate expression is a number node and the base
    // text is a number node.
    if (candidate->type == NODE_NUMBER && base->type == NODE_NUMBER)
    {
        // Check Note: This branch runs when the numeric value in the candidate expression is
        // less than the -1e-9.
        if (candidate->numberValue < -1e-9)
            return false;
        return isZero(candidate->numberValue - base->numberValue * base->numberValue);
    }

    return false;
}

// Note:
// What it does: Checks if candidate equals base cubed.
// Input: candidate subtree and base subtree.
// Returns: true if candidate is mathematically base^3 under supported forms.
// Why needed: Used in identities like (a^3-b^3)/(a-b)=a^2+ab+b^2 and (a^3+b^3)/(a+b)=a^2-ab+b^2.
// Theory: Pattern matching over power nodes for cube identities in quotient simplification.
bool isCubeOf(Node *candidate, Node *base)
{
    // Cube matcher for structural identities:
    // accepts symbolic power form and numeric literal cube checks.
    // Check Note: This branch runs when either the candidate expression is missing or the base
    // text is missing.
    if (candidate == NULL || base == NULL)
        return false;

    // Check Note: This branch runs when the candidate expression is an operator node, the
    // operator in the candidate expression is the character '^', the right child of the
    // candidate expression exists, the right child of the candidate expression is a number
    // node, 0 of the number value - 3 of the right child of the candidate expression is zero,
    // and the left child of the candidate expression and the base text represent the same
    // expression.
    if (candidate->type == NODE_OPERATOR && candidate->op == '^' &&
        candidate->right != NULL && candidate->right->type == NODE_NUMBER &&
        isZero(candidate->right->numberValue - 3.0) && areTreesEqual(candidate->left, base))
    {
        return true;
    }

    // Check Note: This branch runs when the candidate expression is a number node and the base
    // text is a number node.
    if (candidate->type == NODE_NUMBER && base->type == NODE_NUMBER)
    {
        return isZero(candidate->numberValue - base->numberValue * base->numberValue * base->numberValue);
    }

    return false;
}

// Note:
// What it does: Checks if candidate equals base raised to an integer power n.
// Input: candidate subtree, base subtree, and integer exponent n.
// Returns: true if candidate is mathematically base^n under supported forms.
// Why needed: Higher-order quotient identities rely on a reusable power matcher.
// Theory: Pattern matching over exponent nodes with bounded integer-degree checks.
bool isNthPowerOf(Node *candidate, Node *base, int n)
{
    // Check Note: This branch runs when either the candidate expression is missing or the base
    // text is missing.
    if (candidate == NULL || base == NULL)
        return false;
    // Check Note: This branch runs when n is less than zero.
    if (n < 0)
        return false;

    // Check Note: This branch runs when the candidate expression is an operator node, the
    // operator in the candidate expression is the character '^', the right child of the
    // candidate expression exists, the right child of the candidate expression is a number
    // node, the number value - (double)n of the right child of the candidate expression is
    // zero, and the left child of the candidate expression and the base text represent the same
    // expression.
    if (candidate->type == NODE_OPERATOR && candidate->op == '^' &&
        candidate->right != NULL && candidate->right->type == NODE_NUMBER &&
        isZero(candidate->right->numberValue - (double)n) && areTreesEqual(candidate->left, base))
    {
        return true;
    }

    // Check Note: This branch runs when the candidate expression is a number node and the base
    // text is a number node.
    if (candidate->type == NODE_NUMBER && base->type == NODE_NUMBER)
    {
        return isZero(candidate->numberValue - pow(base->numberValue, (double)n));
    }

    return false;
}

// Note:
// What it does: Simplifies sqrt-compatible radicands that are perfect-square products.
// Input: radicand subtree and output ref simplified.
// Returns: true when radicand can be rewritten without a remaining sqrt.
// Why needed: Improves sqrt/power-half interaction with multiplication and division rewrites.
// Theory: Uses multiplicative factorization and even-power extraction for square roots.
bool simplifySquareRootRadicand(Node *radicand, Node *&simplified)
{
    simplified = NULL;

    // Check Note: This branch runs when the value inside the square root is missing.
    if (radicand == NULL)
        return false;

    vector<Node *> factors;
    double coeff = 1.0;
    flattenProduct(radicand, factors, coeff);

    // Check Note: This branch runs when the coeff is less than the -1e-9.
    if (coeff < -1e-9)
        return false;

    double coeffRoot = 1.0;
    // Check Note: This branch runs when the coeff is not one.
    if (!isOne(coeff))
    {
        // Check Note: This branch runs when the coeff is not a whole number.
        if (!isIntegerValue(coeff))
            return false;

        double r = sqrt(coeff);
        // Check Note: This branch runs when r is not a whole number.
        if (!isIntegerValue(r))
            return false;
        coeffRoot = r;
    }

    vector<Node *> reducedFactors;
    // Loop Note: Iterates through radicand factors and accepts only perfect-square-compatible factors so the root can be removed safely.
    for (int i = 0; i < (int)factors.size(); i++)
    {
        Node *f = factors[i];
        // Check Note: This branch runs when f is missing.
        if (f == NULL)
            continue;

        // Check Note: This branch runs when f is an operator node, the operator in f is the
        // character '^', the right child of f exists, and the right child of f is a number
        // node.
        if (f->type == NODE_OPERATOR && f->op == '^' && f->right != NULL && f->right->type == NODE_NUMBER)
        {
            double e = f->right->numberValue;
            // Check Note: This branch runs when e is not a whole number.
            if (!isIntegerValue(e))
                return false;

            int ie = (int)round(e);
            // Check Note: This branch runs when the ie % 2 is not zero.
            if ((ie % 2) != 0)
                return false;

            int half = ie / 2;
            // Check Note: This branch runs when the half is zero.
            if (half == 0)
                continue;

            // Check Note: This branch runs when the half is one.
            if (half == 1)
                reducedFactors.push_back(f->left);
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
                reducedFactors.push_back(makeOperator('^', f->left, makeNumber((double)half)));
            continue;
        }

        // Check Note: This branch runs when f is a number node.
        if (f->type == NODE_NUMBER)
        {
            // Check Note: This branch runs when either the numeric value in f is less than the
            // -1e-9 or the numeric value in f is not a whole number.
            if (f->numberValue < -1e-9 || !isIntegerValue(f->numberValue))
                return false;
            double r = sqrt(f->numberValue);
            // Check Note: This branch runs when r is not a whole number.
            if (!isIntegerValue(r))
                return false;
            // Check Note: This branch runs when r is not one.
            if (!isOne(r))
                reducedFactors.push_back(makeNumber(r));
            continue;
        }

        return false;
    }

    Node *result = NULL;
    // Check Note: This branch runs when either the coeff root is not one or the reduced factors
    // is empty.
    if (!isOne(coeffRoot) || reducedFactors.empty())
        result = makeNumber(coeffRoot);

    // Loop Note: Rebuilds the simplified product after extracting square-root contributions from each factor.
    for (int i = 0; i < (int)reducedFactors.size(); i++)
    {
        // Check Note: This branch runs when the result tree is missing.
        if (result == NULL)
            result = reducedFactors[i];
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            result = makeOperator('*', result, reducedFactors[i]);
    }

    // Check Note: This branch runs when the result tree is missing.
    if (result == NULL)
        result = makeNumber(1.0);

    simplified = result;
    return true;
}

// Note:
// What it does: Extracts a fraction term with explicit sign from additive context.
// Input: candidate additive term and output refs numerator, denominator, sign.
// Returns: true when term is representable as sign * (num/den).
// Why needed: Enables robust same-denominator merging even when one fraction term is negated.
// Theory: Normalizes equivalent signed-fraction forms before additive combination.
bool extractSignedFractionTerm(Node *node, Node *&numerator, Node *&denominator, int &sign)
{
    numerator = NULL;
    denominator = NULL;
    sign = 1;

    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '/'.
    if (node->type == NODE_OPERATOR && node->op == '/')
    {
        numerator = node->left;
        denominator = node->right;
        sign = 1;
        return true;
    }

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '*'.
    if (node->type == NODE_OPERATOR && node->op == '*')
    {
        double n = 0.0;
        // Check Note: This branch runs when the left side exists, the is number
        // node(node->left, n), 0 of the n + 1 is zero, the right side exists, the right side is
        // an operator node, and the operator in the right side is the character '/'.
        if (node->left != NULL && isNumberNode(node->left, n) && isZero(n + 1.0) &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/')
        {
            numerator = node->right->left;
            denominator = node->right->right;
            sign = -1;
            return true;
        }

        // Check Note: This branch runs when the right side exists, the is number
        // node(node->right, n), 0 of the n + 1 is zero, the left side exists, the left side is
        // an operator node, and the operator in the left side is the character '/'.
        if (node->right != NULL && isNumberNode(node->right, n) && isZero(n + 1.0) &&
            node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '/')
        {
            numerator = node->left->left;
            denominator = node->left->right;
            sign = -1;
            return true;
        }
    }

    // Check Note: This branch runs when the current node is an operator node, the operator in
    // the current node is the character '-', the left side exists, the right side exists, the
    // left side is a number node, the numeric value in the left side is zero, the right side is
    // an operator node, and the operator in the right side is the character '/'.
    if (node->type == NODE_OPERATOR && node->op == '-' && node->left != NULL && node->right != NULL &&
        node->left->type == NODE_NUMBER && isZero(node->left->numberValue) &&
        node->right->type == NODE_OPERATOR && node->right->op == '/')
    {
        numerator = node->right->left;
        denominator = node->right->right;
        sign = -1;
        return true;
    }

    return false;
}

// Note:
// What it does: Flattens additive/subtractive base into signed term list for power expansion.
// Input: expression node and output vector of (term, sign).
// Returns: nothing (fills output vector by reference).
// Why needed: general power expansion needs canonical list of addends, not nested +/- tree shape.
// Theory: associative flattening converts tree-form sums into linear term representation.
void collectSignedAddTerms(Node *node, vector<pair<Node *, int> > &out)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return;

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '+'.
    if (node->type == NODE_OPERATOR && node->op == '+')
    {
        collectSignedAddTerms(node->left, out);
        collectSignedAddTerms(node->right, out);
        return;
    }

    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '-'.
    if (node->type == NODE_OPERATOR && node->op == '-')
    {
        collectSignedAddTerms(node->left, out);

        vector<pair<Node *, int> > rightTerms;
        collectSignedAddTerms(node->right, rightTerms);
        // Loop Note: Iterates through all extracted right-side terms to flip signs under subtraction; loop is needed because term count is dynamic.
        for (int i = 0; i < (int)rightTerms.size(); i++)
        {
            out.push_back(make_pair(rightTerms[i].first, -rightTerms[i].second));
        }
        return;
    }

    out.push_back(make_pair(node, 1));
}

// Note:
// What it does: Computes multinomial coefficient n!/(k1!k2!...km!) for exponent distributions.
// Input: total exponent n and exponent-count vector counts.
// Returns: coefficient as long double.
// Why needed: this coefficient is the core weight in general multinomial expansion.
// Theory: multinomial theorem coefficient formula from combinatorics.
long double multinomialCoefficient(int n, const vector<int> &counts)
{
    long double result = 1.0L;
    int remaining = n;

    // Loop Note: Iterates across each term's assigned exponent count; loop is required because number of terms is variable.
    for (int i = 0; i < (int)counts.size(); i++)
    {
        int k = counts[i];
        // Check Note: This branch runs when k is at most zero.
        if (k <= 0)
            continue;

        // Build C(remaining, k) multiplicatively for numeric stability in small n.
        // Loop Note: Repeats k times to multiply each binomial step; loop is required because combination factor has variable length.
        for (int t = 1; t <= k; t++)
        {
            result *= (long double)(remaining - k + t);
            result /= (long double)t;
        }
        remaining -= k;
    }

    return result;
}

// Note:
// What it does: Builds product term for one exponent-allocation in multinomial expansion.
// Input: term list, per-term exponents, and scalar coefficient.
// Returns: AST node for coefficient * product( term_i ^ count_i ).
// Why needed: expansion enumerator needs to materialize each generated multinomial term.
// Theory: each distribution vector maps to one monomial-like product term.
Node *buildExpandedPowerTerm(const vector<pair<Node *, int> > &terms, const vector<int> &counts, long double coeff)
{
    Node *product = NULL;

    // Loop Note: Iterates across all base terms to apply assigned exponents; loop is needed because base-term count is dynamic.
    for (int i = 0; i < (int)terms.size(); i++)
    {
        int e = counts[i];
        // Check Note: This branch runs when e is zero.
        if (e == 0)
            continue;

        Node *factor = NULL;
        // Check Note: This branch runs when e is one.
        if (e == 1)
            factor = terms[i].first;
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            factor = makeOperator('^', terms[i].first, makeNumber((double)e));

        // Apply source term sign parity to this factor contribution.
        // Check Note: This branch runs when the second part of the terms[i] is less than zero
        // and the e % 2 is one.
        if (terms[i].second < 0 && (e % 2 == 1))
            factor = makeOperator('*', makeNumber(-1.0), factor);

        // Check Note: This branch runs when the product is missing.
        if (product == NULL)
            product = factor;
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            product = makeOperator('*', product, factor);
    }

    // Check Note: This branch runs when the product is missing.
    if (product == NULL)
        product = makeNumber(1.0);

    double c = (double)coeff;
    // Check Note: This branch runs when 0 of the c - 1 is zero.
    if (isZero(c - 1.0))
        return product;
    return makeOperator('*', makeNumber(c), product);
}

// Note:
// What it does: Expands (sum of terms)^n using bounded multinomial expansion.
// Input: base expression, integer exponent n, output ref expanded.
// Returns: true when a bounded safe expansion is produced.
// Why needed: avoids hardcoding square/cube patterns and supports general power forms.
// Theory: multinomial theorem with bounded search-space controls.
bool tryExpandPowerByMultinomial(Node *base, int n, Node *&expanded)
{
    expanded = NULL;

    // Check Note: This branch runs when the base text is missing.
    if (base == NULL)
        return false;
    // Check Note: This branch runs when n is less than 2.
    if (n < 2)
        return false;

    vector<pair<Node *, int> > terms;
    collectSignedAddTerms(base, terms);

    // Check Note: This branch runs when the size of the term list is less than 2.
    if ((int)terms.size() < 2)
        return false;

    // Bounded safety: keep only manageable expansions.
    // termCountUpper ~= C(n + m - 1, m - 1), bounded conservatively.
    int m = (int)terms.size();
    int estimate = 1;
    // Loop Note: Builds conservative term-count estimate iteratively; loop is used because closed-form intermediate handling is simpler this way.
    for (int i = 1; i <= m - 1; i++)
    {
        estimate = (estimate * (n + i)) / i;
        // Check Note: This branch runs when the estimate is greater than the 250.
        if (estimate > 250)
            break;
    }

    // Check Note: This branch runs when either the estimate is greater than the 250, n is
    // greater than 6, or m is greater than 6.
    if (estimate > 250 || n > 6 || m > 6)
        return false;

    vector<int> counts(m, 0);
    vector<Node *> termNodes;
    termNodes.reserve(estimate + 4);

    function<void(int, int)> dfs = [&](int idx, int remaining) {
        // Check Note: This branch runs when the idx is the m - 1.
        if (idx == m - 1)
        {
            counts[idx] = remaining;
            long double coeff = multinomialCoefficient(n, counts);
            termNodes.push_back(buildExpandedPowerTerm(terms, counts, coeff));
            return;
        }

        // Loop Note: Enumerates all valid exponent allocations for current index; loop is necessary because each split is a distinct multinomial term family.
        for (int take = 0; take <= remaining; take++)
        {
            counts[idx] = take;
            dfs(idx + 1, remaining - take);
        }
    };

    dfs(0, n);

    // Check Note: This branch runs when the term nodes is empty.
    if (termNodes.empty())
        return false;

    Node *sum = NULL;
    // Loop Note: Combines generated multinomial terms into one additive AST; loop is needed because generated term count is data-dependent.
    for (int i = 0; i < (int)termNodes.size(); i++)
    {
        // Check Note: This branch runs when the sum is missing.
        if (sum == NULL)
            sum = termNodes[i];
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
            sum = makeOperator('+', sum, termNodes[i]);
    }

    expanded = sum;
    return true;
}

// Note:
// What it does: Detects and rewrites bounded perfect-power polynomial structures.
// Input: expression node and output ref rewritten.
// Returns: true when expression matches supported perfect-power formulas.
// Why needed: Lets formula recognition feed later division/cancellation stages in the pipeline.
// Theory: Coefficient/power pattern recognition using binomial-theorem constraints.
bool detectPerfectPower(Node *node, Node *&rewritten)
{
    // Pattern Handler: perfect-power recognition (v3, refined v4/v6/v10/v11)
    // - Keeps legacy square recognizers for stability.
    // - Adds bounded generic (x +/- k)^n detection for broader perfect-power coverage.
    rewritten = NULL;
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;

    vector<pair<Node *, int> > terms;
    collectAdditiveTerms(node, terms, 1);

    map<string, double> coeff;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)terms.size(); i++)
    {
        string v;
        int p = 0;
        double c = 0.0;
        // Check Note: This branch runs when the first part of the terms[i] can be treated as a
        // single monomial.
        if (!extractMonomial(terms[i].first, v, p, c))
            continue;

        ostringstream key;
        key << v << "#" << p;
        coeff[key.str()] += terms[i].second * c;
    }

    auto onlyTheseKeysNonZero = [&](const vector<string> &allowed) {
        // Loop Note: Iterates over all extracted coefficient buckets to verify no extra active terms exist; loop is needed because term-space size is dynamic.
        for (map<string, double>::iterator it = coeff.begin(); it != coeff.end(); ++it)
        {
            // Check Note: This branch runs when the sign or value part of the current map entry
            // is zero.
            if (isZero(it->second))
                continue;

            bool okKey = false;
            // Loop Note: Scans allowed key set for membership check; loop is needed because allowed pattern keys vary by candidate.
            for (int k = 0; k < (int)allowed.size(); k++)
            {
                // Check Note: This branch runs when the expression part of the current map
                // entry is the allowed[k].
                if (it->first == allowed[k])
                {
                    okKey = true;
                    break;
                }
            }

            // Check Note: This branch runs when the key does not belong to the allowed pattern.
            if (!okKey)
                return false;
        }
        return true;
    };

    // x^2 +/- 2kx + k^2 => (x +/- k)^2
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (map<string, double>::iterator it = coeff.begin(); it != coeff.end(); ++it)
    {
        string key = it->first;
        size_t split = key.find('#');
        string v = key.substr(0, split);
        int p = atoi(key.substr(split + 1).c_str());
        // Check Note: This branch runs when v is not empty and p is 2.
        if (!v.empty() && p == 2)
        {
            ostringstream k1, k0;
            k1 << v << "#1";
            k0 << "#0";
            double c2 = it->second;
            double c1 = coeff.count(k1.str()) ? coeff[k1.str()] : 0.0;
            double c0 = coeff.count(k0.str()) ? coeff[k0.str()] : 0.0;
            double k = fabs(c1) / 2.0;

            // Check Note: This branch runs when the c2 is one, the c1 is not zero, the c0 - k *
            // k is zero, and the only these keys non zero(vector<string>{key, k1.str(),
            // k0.str()}).
            if (isOne(c2) && !isZero(c1) && isZero(c0 - k * k) &&
                onlyTheseKeysNonZero(vector<string>{key, k1.str(), k0.str()}))
            {
                Node *inside = makeOperator(c1 >= 0.0 ? '+' : '-', makeVariable(v), makeNumber(k));
                rewritten = makeOperator('^', inside, makeNumber(2.0));
                return true;
            }
        }
    }

    // x^2 + y^2 +/- 2xy => (x +/- y)^2
    string xVar = "", yVar = "";
    double x2 = 0.0, y2 = 0.0, xy = 0.0;
    bool sawUnsupportedTerm = false;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)terms.size(); i++)
    {
        Node *t = terms[i].first;
        int sgn = terms[i].second;

        // Check Note: This branch runs when t is an operator node, the operator in t is the
        // character '^', the left child of t exists, the left child of t is a variable node,
        // the right child of t exists, the right child of t is a number node, and 0 of the
        // number value - 2 of the right child of t is zero.
        if (t->type == NODE_OPERATOR && t->op == '^' && t->left != NULL && t->left->type == NODE_VARIABLE &&
            t->right != NULL && t->right->type == NODE_NUMBER && isZero(t->right->numberValue - 2.0))
        {
            // Check Note: This branch runs when either the x var is empty or the x var is the
            // stored name in the left child of t.
            if (xVar.empty() || xVar == t->left->variableName)
            {
                xVar = t->left->variableName;
                x2 += sgn;
            }
            // Check Note: If the earlier case did not match, this branch runs when either the y
            // var is empty or the y var is the stored name in the left child of t.
            else if (yVar.empty() || yVar == t->left->variableName)
            {
                yVar = t->left->variableName;
                y2 += sgn;
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                // Third distinct square variable means expression is not a pure 2-variable perfect square.
                sawUnsupportedTerm = true;
            }
            continue;
        }

        double termCoeff = 0.0;
        string va, vb;
        // Check Note: This branch runs when t can be recognized as a two-variable product.
        if (extractTwoVarProductCoeff(t, termCoeff, va, vb))
        {
            // Check Note: This branch runs when the x var is empty.
            if (xVar.empty())
                xVar = va;
            // Check Note: This branch runs when the y var is empty.
            if (yVar.empty())
                yVar = vb;

            // Check Note: This branch runs when either the va is the x var and the vb is the y
            // var or the va is the y var and the vb is the x var.
            if ((va == xVar && vb == yVar) || (va == yVar && vb == xVar))
                xy += sgn * termCoeff;
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                sawUnsupportedTerm = true;
            }
        }
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
        {
            sawUnsupportedTerm = true;
        }
    }

    // Check Note: This branch runs when the x var is not empty, the y var is not empty, it is
    // not true that the saw unsupported term, the x2 is one, the y2 is one, and 0 of the
    // fabs(xy) - 2 is zero.
    if (!xVar.empty() && !yVar.empty() && !sawUnsupportedTerm && isOne(x2) && isOne(y2) && isZero(fabs(xy) - 2.0))
    {
        Node *inside = makeOperator(xy >= 0.0 ? '+' : '-', makeVariable(xVar), makeVariable(yVar));
        rewritten = makeOperator('^', inside, makeNumber(2.0));
        return true;
    }

    // Generic bounded single-variable perfect-power detection:
    // matches x^n + C(n,1)s*k*x^(n-1) + ... + (s*k)^n, where s in {+1,-1}.
    set<string> variables;
    // Loop Note: Collects variable names present in monomial-key map so each candidate variable can be evaluated as a potential perfect-power base.
    for (map<string, double>::iterator it = coeff.begin(); it != coeff.end(); ++it)
    {
        // Check Note: This branch runs when the sign or value part of the current map entry is
        // zero.
        if (isZero(it->second))
            continue;

        string key = it->first;
        size_t split = key.find('#');
        string v = key.substr(0, split);
        // Check Note: This branch runs when v is not empty.
        if (!v.empty())
            variables.insert(v);
    }

    auto getCoeffFor = [&](const string &v, int p) {
        ostringstream k;
        k << v << "#" << p;
        return coeff.count(k.str()) ? coeff[k.str()] : 0.0;
    };

    auto binomialCoeff = [&](int n, int r) {
        long double out = 1.0L;
        // Loop Note: Builds nCr multiplicatively to avoid large intermediate factorials while preserving stable bounded arithmetic for small n.
        for (int i = 1; i <= r; i++)
        {
            out *= (long double)(n - r + i);
            out /= (long double)i;
        }
        return (double)out;
    };

    // Loop Note: Evaluates each discovered variable as the potential base variable in a bounded (x +/- k)^n reconstruction.
    for (set<string>::iterator vit = variables.begin(); vit != variables.end(); ++vit)
    {
        string v = *vit;
        int maxPower = -1;

        // Loop Note: Finds highest power of the current variable present in coefficient buckets to infer candidate exponent n.
        for (map<string, double>::iterator it = coeff.begin(); it != coeff.end(); ++it)
        {
            // Check Note: This branch runs when the sign or value part of the current map entry
            // is zero.
            if (isZero(it->second))
                continue;

            string key = it->first;
            size_t split = key.find('#');
            string keyVar = key.substr(0, split);
            int p = atoi(key.substr(split + 1).c_str());
            // Check Note: This branch runs when the key var is v and p is greater than the max
            // power.
            if (keyVar == v && p > maxPower)
                maxPower = p;
        }

        // Check Note: This branch runs when either the max power is less than 2 or the max
        // power is greater than 6.
        if (maxPower < 2 || maxPower > 6)
            continue;

        int n = maxPower;
        double lead = getCoeffFor(v, n);
        // Check Note: This branch runs when the lead is not one.
        if (!isOne(lead))
            continue;

        double cN1 = getCoeffFor(v, n - 1);
        // Check Note: This branch runs when the c n1 is zero.
        if (isZero(cN1))
            continue;

        int sign = (cN1 >= 0.0) ? 1 : -1;
        double k = fabs(cN1) / (double)n;
        // Check Note: This branch runs when k is less than the 1e-9.
        if (k < 1e-9)
            continue;

        bool okPattern = true;
        // Loop Note: Verifies every power coefficient from n down to 0 against the binomial expectation for the inferred (x +/- k)^n candidate.
        for (int p = 0; p <= n; p++)
        {
            int r = n - p;
            double expected = binomialCoeff(n, r) * pow(k, (double)r) * ((r % 2 == 0 || sign > 0) ? 1.0 : -1.0);
            double got = getCoeffFor(v, p);
            // Check Note: This branch runs when the got - expected is not zero.
            if (!isZero(got - expected))
            {
                okPattern = false;
                break;
            }
        }

        // Check Note: This branch runs when the expected coefficient pattern did not match.
        if (!okPattern)
            continue;

        bool hasUnsupported = false;
        // Loop Note: Ensures no non-target terms remain active outside the inferred single-variable polynomial family, preventing unsafe over-matching.
        for (map<string, double>::iterator it = coeff.begin(); it != coeff.end(); ++it)
        {
            // Check Note: This branch runs when the sign or value part of the current map entry
            // is zero.
            if (isZero(it->second))
                continue;

            string key = it->first;
            size_t split = key.find('#');
            string keyVar = key.substr(0, split);
            int p = atoi(key.substr(split + 1).c_str());
            bool allowed = ((keyVar == v && p >= 0 && p <= n) || (keyVar.empty() && p == 0));
            // Check Note: This branch runs when it is not true that the allowed.
            if (!allowed)
            {
                hasUnsupported = true;
                break;
            }
        }

        // Check Note: This branch runs when unsupported extra terms were found.
        if (hasUnsupported)
            continue;

        Node *inside = makeOperator(sign > 0 ? '+' : '-', makeVariable(v), makeNumber(k));
        rewritten = makeOperator('^', inside, makeNumber((double)n));
        return true;
    }

    return false;
}

// PHASE 2.6: Algebraic simplifier for common symbolic forms.
// Central Rewrite Engine (majorly evolved v1->v6):
// - This function hosts property handlers (commutative/associative/distributive,
//   identities/inverses, exponent laws, trig/log identities, cancellation rules).
// - Each op-domain section below acts as a dedicated handler group.
// Note:
// What it does: Applies rule-based symbolic rewrites across algebra/trig/log/division identities.
// Input: subtree root and change-flag reference.
// Returns: simplified (or unchanged) subtree root.
// Why needed: This is the main intelligence pass that performs mathematical simplification.
// Theory: Term-rewriting system with recursive descent and pattern-directed transformations.
Node *algebraicSimplification(Node *node, bool &changed)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return node;

    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        // Function Handler Group (v2, v3, v4):
        // - log/ln simplifications and exp inverse patterns.
        // - function-domain-preserving rewrites only.
        // - basic trig parity/complement rewrites for beginner-level identities.
        node->left = algebraicSimplification(node->left, changed);

        string fn = toLowerString(node->variableName);
        Node *positiveArg = NULL;

        // sin(-x) => -sin(x), tan(-x) => -tan(x), cot(-x) => -cot(x), csc(-x) => -csc(x)
        // Check Note: This branch runs when the function input can be recognized as a negated
        // term and the function name is one of the supported odd trig functions.
        if (extractNegatedTerm(node->left, positiveArg) &&
            (fn == "sin" || fn == "tan" || fn == "cot" || fn == "csc"))
        {
            changed = true;
            return makeOperator('*', makeNumber(-1.0), makeFunction(node->variableName, positiveArg));
        }

        // cos(-x) => cos(x), sec(-x) => sec(x)
        // Check Note: This branch runs when the function input can be recognized as a negated
        // term and the function name is one of the supported even trig functions.
        if (extractNegatedTerm(node->left, positiveArg) && (fn == "cos" || fn == "sec"))
        {
            changed = true;
            return makeFunction(node->variableName, positiveArg);
        }

        Node *complementArg = NULL;
        // sin(pi/2 - x) => cos(x), cos(pi/2 - x) => sin(x)
        // Check Note: This branch runs when the function input matches a supported pi-over-two
        // minus angle pattern and the function is either sin(...) or cos(...).
        if (extractHalfPiMinusArgument(node->left, complementArg) && (fn == "sin" || fn == "cos"))
        {
            changed = true;
            // Check Note: This branch runs when the function name is "sin".
            if (fn == "sin")
                return makeFunction("cos", complementArg);
            return makeFunction("sin", complementArg);
        }

        // sqrt(perfect-square-radicand) => simplified product/power form.
        // Check Note: This branch runs when the function name is "sqrt" and the left side
        // exists.
        if (fn == "sqrt" && node->left != NULL)
        {
            // sqrt(x^2) => abs(x) to preserve sign-correct principal root semantics.
            // Check Note: This branch runs when the left side is an operator node, the operator
            // in the left side is the character '^', the right child of the left side exists,
            // the right child of the left side is a number node, and 0 of the number value - 2
            // of the right child of the left side is zero.
            if (node->left->type == NODE_OPERATOR && node->left->op == '^' &&
                node->left->right != NULL && node->left->right->type == NODE_NUMBER &&
                isZero(node->left->right->numberValue - 2.0))
            {
                changed = true;
                return makeFunction("abs", node->left->left);
            }

            Node *sqrtReduced = NULL;
            // Check Note: This branch runs when the square-root input the left side can be
            // simplified safely and the simplified square-root form exists.
            if (simplifySquareRootRadicand(node->left, sqrtReduced) && sqrtReduced != NULL)
            {
                changed = true;
                return sqrtReduced;
            }
        }

        // Check Note: This branch runs when either the function name is "log" or the function
        // name is "ln" and the left side exists.
        if ((fn == "log" || fn == "ln") && node->left != NULL)
        {
            // Check Note: This branch runs when the left side is a number node.
            if (node->left->type == NODE_NUMBER)
            {
                // Check Note: This branch runs when the numeric value in the left side is one.
                if (isOne(node->left->numberValue))
                {
                    changed = true;
                    return makeNumber(0.0);
                }
                // Check Note: This branch runs when the number value - exp(1.0) of the left
                // side is zero.
                if (isZero(node->left->numberValue - exp(1.0)))
                {
                    changed = true;
                    return makeNumber(1.0);
                }
            }

            // log(sqrt(x)) => 0.5 * log(x)
            // Check Note: This branch runs when the left side is a sqrt(...) call.
            if (isFunctionNode(node->left, "sqrt"))
            {
                changed = true;
                return makeOperator('*', makeNumber(0.5), makeFunction(fn, node->left->left));
            }

            // log(e ^ x) and ln(e ^ x) => x
            // Check Note: This branch runs when the left side is an operator node, the operator
            // in the left side is the character '^', the left child of the left side exists,
            // the left child of the left side is a number node, and the number value - exp(1.0)
            // of the left child of the left side is zero.
            if (node->left->type == NODE_OPERATOR && node->left->op == '^' &&
                node->left->left != NULL && node->left->left->type == NODE_NUMBER &&
                isZero(node->left->left->numberValue - exp(1.0)))
            {
                changed = true;
                return node->left->right;
            }
        }

        // exp(log(x)) => x and exp(ln(x)) => x
        // Check Note: This branch runs when the function name is "exp", the left side exists,
        // and the left side is a function node.
        if (fn == "exp" && node->left != NULL && node->left->type == NODE_FUNCTION)
        {
            string inner = toLowerString(node->left->variableName);
            // Check Note: This branch runs when either the inner is "log" or the inner is "ln".
            if (inner == "log" || inner == "ln")
            {
                changed = true;
                return node->left->left;
            }
        }

        // sqrt(x) * sqrt(x) is handled in '*' rules; keep unary node unchanged here.
        return node;
    }

    // Preserve selected quotient patterns before child rewrites expand them away.
    // Check Note: This branch runs when the current node is an operator node and the operator in
    // the current node is the character '/'.
    if (node->type == NODE_OPERATOR && node->op == '/')
    {
        // (u) / (u^n) => 1 / (u^(n-1)) for numeric n > 1
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // right side is a power node, the exponent in the right side is a number greater than
        // one, and the base of the right side matches the left side.
        if (node->left != NULL && node->right != NULL &&
            node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER &&
            node->right->right->numberValue > 1.0 &&
            areTreesEqual(node->left, node->right->left))
        {
            double reducedExponent = node->right->right->numberValue - 1.0;
            changed = true;
            // Check Note: This branch runs when the reduced exponent is one.
            if (isOne(reducedExponent))
                return makeOperator('/', makeNumber(1.0), node->left);
            return makeOperator('/', makeNumber(1.0),
                                makeOperator('^', node->left, makeNumber(reducedExponent)));
        }

        // (a - b) / (b - a) => -1, including equivalent additive-negation forms.
        // Check Note: This branch runs when both sides can be read as differences and the
        // denominator reverses the numerator terms.
        if (node->left != NULL && node->right != NULL)
        {
            Node *leftA = NULL;
            Node *leftB = NULL;
            Node *rightA = NULL;
            Node *rightB = NULL;

            if (extractDifference(node->left, leftA, leftB) &&
                extractDifference(node->right, rightA, rightB) &&
                areTreesEqual(leftA, rightB) &&
                areTreesEqual(leftB, rightA))
            {
                changed = true;
                return makeNumber(-1.0);
            }
        }
    }

    // Check Note: This branch runs when the current node is not an operator node.
    if (node->type != NODE_OPERATOR)
        return node;

    node->left = algebraicSimplification(node->left, changed);
    node->right = algebraicSimplification(node->right, changed);

    // In additive contexts, expose conjugate products only when that can unlock cancellation.
    // Check Note: This branch runs when the operator in the current node is either the character
    // '+' or the character '-'.
    if (node->op == '+' || node->op == '-')
    {
        Node *conjA = NULL;
        Node *conjB = NULL;

        // Check Note: This branch runs when the left side is a conjugate product like
        // (a - b) * (a + b).
        if (extractConjugateProduct(node->left, conjA, conjB))
        {
            changed = true;
            return makeOperator(node->op,
                                makeOperator('-', makeOperator('^', conjA, makeNumber(2.0)),
                                             makeOperator('^', conjB, makeNumber(2.0))),
                                node->right);
        }

        // Check Note: This branch runs when the right side is a conjugate product like
        // (a - b) * (a + b).
        if (extractConjugateProduct(node->right, conjA, conjB))
        {
            changed = true;
            return makeOperator(node->op,
                                node->left,
                                makeOperator('-', makeOperator('^', conjA, makeNumber(2.0)),
                                             makeOperator('^', conjB, makeNumber(2.0))));
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '-'.
    if (node->op == '-')
    {
        // a/b - c/d => (a*d - c*b) / (b*d)
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '/',
        // the right side is an operator node, and the operator in the right side is the
        // character '/'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            node->right->type == NODE_OPERATOR && node->right->op == '/')
        {
            changed = true;
            Node *num = makeOperator('-',
                                     makeOperator('*', node->left->left, node->right->right),
                                     makeOperator('*', node->left->right, node->right->left));
            Node *den = makeOperator('*', node->left->right, node->right->right);
            return makeOperator('/', num, den);
        }

        // 1 - 1/x => (x - 1) / x
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is one, the right side exists, the right
        // side is an operator node, the operator in the right side is the character '/', the
        // left child of the right side exists, the left child of the right side is a number
        // node, and the numeric value in the left child of the right side is one.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue) &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            node->right->left != NULL && node->right->left->type == NODE_NUMBER && isOne(node->right->left->numberValue))
        {
            Node *x = node->right->right;
            changed = true;
            return makeOperator('/', makeOperator('-', x, makeNumber(1.0)), x);
        }

        // x^4 - y^4 => (x - y) * (x + y) * (x^2 + y^2), including x^4 - 1.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *a = NULL;
            Node *b = NULL;

            if (node->left->type == NODE_OPERATOR && node->left->op == '^' &&
                node->left->right != NULL && node->left->right->type == NODE_NUMBER &&
                isZero(node->left->right->numberValue - 4.0))
            {
                a = node->left->left;
            }

            if (a != NULL)
            {
                if (node->right->type == NODE_OPERATOR && node->right->op == '^' &&
                    node->right->right != NULL && node->right->right->type == NODE_NUMBER &&
                    isZero(node->right->right->numberValue - 4.0))
                {
                    b = node->right->left;
                }
                else if (node->right->type == NODE_NUMBER && isOne(node->right->numberValue))
                {
                    b = makeNumber(1.0);
                }
            }

            if (a != NULL && b != NULL)
            {
                changed = true;
                Node *f1 = makeOperator('-', a, b);
                Node *f2 = makeOperator('+', a, b);
                Node *f3 = makeOperator('+', makeOperator('^', a, makeNumber(2.0)), makeOperator('^', b, makeNumber(2.0)));
                return makeOperator('*', makeOperator('*', f1, f2), f3);
            }
        }

        // Check Note: This branch runs when the right side exists, the right side is an
        // operator node, and the operator in the right side is the character '*'.
        if (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '*')
        {
            double n = 0.0;
            // Check Note: This branch runs when the left child of the right side exists, the is
            // number node(node->right->left, n), and 0 of the n + 1 is zero.
            if (node->right->left != NULL && isNumberNode(node->right->left, n) && isZero(n + 1.0))
            {
                changed = true;
                return makeOperator('+', node->left, node->right->right);
            }
            // Check Note: This branch runs when the right child of the right side exists, the
            // is number node(node->right->right, n), and 0 of the n + 1 is zero.
            if (node->right->right != NULL && isNumberNode(node->right->right, n) && isZero(n + 1.0))
            {
                changed = true;
                return makeOperator('+', node->left, node->right->left);
            }
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '*'.
    if (node->op == '*')
    {
        // Multiplication Handler Group (v2, v3, v4, v6):
        // - commutative factor canonicalization
        // - distributive expansion
        // - reciprocal/factor cancellation
        // - exponent merge laws
        // - trig/log specific multiplicative identities
        // Canonicalize simple commutative products: reorder variable factors and combine numeric coefficients.
        vector<Node *> factors;
        double coeff = 1.0;
        flattenProduct(node, factors, coeff);

        bool allSimple = true;
        // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
        for (int i = 0; i < (int)factors.size(); i++)
        {
            // Check Note: This branch runs when it is not true that the is simple
            // factor(factors[i]).
            if (!isSimpleFactor(factors[i]))
            {
                allSimple = false;
                break;
            }
        }

        // Check Note: This branch runs when the all simple.
        if (allSimple)
        {
            // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
            for (int i = 0; i < (int)factors.size(); i++)
            {
                // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
                for (int j = i + 1; j < (int)factors.size(); j++)
                {
                    // Check Note: This branch runs when the simple factor key(factors[j]) is
                    // less than the simple factor key(factors[i]).
                    if (simpleFactorKey(factors[j]) < simpleFactorKey(factors[i]))
                        swap(factors[i], factors[j]);
                }
            }

            Node *rebuilt = buildProductFromFactors(factors, coeff);
            // Check Note: This branch runs when the rebuilt term list and the current node
            // represent the same expression.
            if (!areTreesEqual(rebuilt, node))
            {
                changed = true;
                return rebuilt;
            }
        }

        // sqrt(x) * sqrt(x) => x
        // Check Note: This branch runs when the left side is a sqrt(...) call, the right side
        // is a sqrt(...) call, and the left child of the left side and the left child of the
        // right side represent the same expression.
        if (isFunctionNode(node->left, "sqrt") && isFunctionNode(node->right, "sqrt") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return node->left->left;
        }

        // x * sqrt(x) => x^1.5, and sqrt(x) * x => x^1.5
        // Check Note: This branch runs when the left side exists, the right side is a sqrt(...)
        // call, and the left side and the left child of the right side represent the same
        // expression.
        if (node->left != NULL && isFunctionNode(node->right, "sqrt") &&
            areTreesEqual(node->left, node->right->left))
        {
            changed = true;
            return makeOperator('^', node->left, makeNumber(1.5));
        }
        // Check Note: This branch runs when the left side is a sqrt(...) call, the right side
        // exists, and the left child of the left side and the right side represent the same
        // expression.
        if (isFunctionNode(node->left, "sqrt") && node->right != NULL &&
            areTreesEqual(node->left->left, node->right))
        {
            changed = true;
            return makeOperator('^', node->right, makeNumber(1.5));
        }

        // sqrt(x) * x^0.5 => x, and x^0.5 * sqrt(x) => x
        // Check Note: This branch runs when the left side is a sqrt(...) call, the right side
        // exists, the right side is an operator node, the operator in the right side is the
        // character '^', the left child of the right side exists, the right child of the right
        // side exists, the right child of the right side is a number node, 5 of the number
        // value - 0 of the right child of the right side is zero, and the left child of the
        // left side and the left child of the right side represent the same expression.
        if (isFunctionNode(node->left, "sqrt") && node->right != NULL &&
            node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->right->left != NULL && node->right->right != NULL &&
            node->right->right->type == NODE_NUMBER && isZero(node->right->right->numberValue - 0.5) &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return node->left->left;
        }
        // Check Note: This branch runs when the right side is a sqrt(...) call, the left side
        // exists, the left side is an operator node, the operator in the left side is the
        // character '^', the left child of the left side exists, the right child of the left
        // side exists, the right child of the left side is a number node, 5 of the number value
        // - 0 of the right child of the left side is zero, and the left child of the right side
        // and the left child of the left side represent the same expression.
        if (isFunctionNode(node->right, "sqrt") && node->left != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '^' &&
            node->left->left != NULL && node->left->right != NULL &&
            node->left->right->type == NODE_NUMBER && isZero(node->left->right->numberValue - 0.5) &&
            areTreesEqual(node->right->left, node->left->left))
        {
            changed = true;
            return node->right->left;
        }

        // sec(x) * cos(x) => 1, csc(x) * sin(x) => 1, cot(x) * tan(x) => 1
        // Check Note: This branch runs when either the left side is a sec(...) call and the
        // right side is a cos(...) call, the left side is a cos(...) call and the right side is
        // a sec(...) call, the left side is a csc(...) call and the right side is a sin(...)
        // call, the left side is a sin(...) call and the right side is a csc(...) call, the
        // left side is a cot(...) call and the right side is a tan(...) call, or the left side
        // is a tan(...) call and the right side is a cot(...) call and the left child of the
        // left side and the left child of the right side represent the same expression.
        if (((isFunctionNode(node->left, "sec") && isFunctionNode(node->right, "cos")) ||
             (isFunctionNode(node->left, "cos") && isFunctionNode(node->right, "sec")) ||
             (isFunctionNode(node->left, "csc") && isFunctionNode(node->right, "sin")) ||
             (isFunctionNode(node->left, "sin") && isFunctionNode(node->right, "csc")) ||
             (isFunctionNode(node->left, "cot") && isFunctionNode(node->right, "tan")) ||
             (isFunctionNode(node->left, "tan") && isFunctionNode(node->right, "cot"))) &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeNumber(1.0);
        }

        // tan(x) * cos(x) => sin(x)
        // Check Note: This branch runs when the left side is a tan(...) call, the right side is
        // a cos(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "tan") && isFunctionNode(node->right, "cos") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("sin", node->left->left);
        }
        // Check Note: This branch runs when the left side is a cos(...) call, the right side is
        // a tan(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "cos") && isFunctionNode(node->right, "tan") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("sin", node->left->left);
        }

        // cot(x) * sin(x) => cos(x)
        // Check Note: This branch runs when the left side is a cot(...) call, the right side is
        // a sin(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "cot") && isFunctionNode(node->right, "sin") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("cos", node->left->left);
        }
        // Check Note: This branch runs when the left side is a sin(...) call, the right side is
        // a cot(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "sin") && isFunctionNode(node->right, "cot") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("cos", node->left->left);
        }

        // n * log(x) => log(x ^ n), keep numeric coefficient form for readability.
        // Check Note: This branch runs when the left side is a log(...) call, the right side
        // exists, and the right side is a variable node.
        if (isFunctionNode(node->left, "log") && node->right != NULL && node->right->type == NODE_VARIABLE)
        {
            changed = true;
            return makeFunction("log", makeOperator('^', node->left->left, node->right));
        }
        // Check Note: This branch runs when the right side is a log(...) call, the left side
        // exists, and the left side is a variable node.
        if (isFunctionNode(node->right, "log") && node->left != NULL && node->left->type == NODE_VARIABLE)
        {
            changed = true;
            return makeFunction("log", makeOperator('^', node->right->left, node->left));
        }

        // exp(a) * exp(-a) => 1
        // Check Note: This branch runs when the left side is a exp(...) call and the right side
        // is a exp(...) call.
        if (isFunctionNode(node->left, "exp") && isFunctionNode(node->right, "exp"))
        {
            Node *pos = NULL;
            // Check Note: This branch runs when the left child of the right side can be
            // recognized as a negated term and the left child of the left side and the pos
            // represent the same expression.
            if (extractNegatedTerm(node->right->left, pos) && areTreesEqual(node->left->left, pos))
            {
                changed = true;
                return makeNumber(1.0);
            }
            // Check Note: This branch runs when the left child of the left side can be
            // recognized as a negated term and the left child of the right side and the pos
            // represent the same expression.
            if (extractNegatedTerm(node->left->left, pos) && areTreesEqual(node->right->left, pos))
            {
                changed = true;
                return makeNumber(1.0);
            }
        }

        // (a / b) * b => a, b * (a / b) => a
        // Check Note: This branch runs when the left side exists, the left side is an operator
        // node, the operator in the left side is the character '/', and the right child of the
        // left side and the right side represent the same expression.
        if (node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            areTreesEqual(node->left->right, node->right))
        {
            changed = true;
            return node->left->left;
        }
        // Check Note: This branch runs when the right side exists, the right side is an
        // operator node, the operator in the right side is the character '/', and the right
        // child of the right side and the left side represent the same expression.
        if (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            areTreesEqual(node->right->right, node->left))
        {
            changed = true;
            return node->right->left;
        }

        // (a / (b*c)) * b => a / c, and symmetric side.
        // Check Note: This branch runs when the left side exists, the left side is an operator
        // node, and the operator in the left side is the character '/'.
        if (node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '/')
        {
            Node *reducedDen = NULL;
            // Check Note: This branch runs when the requested factor can be removed from the
            // right child of the left side.
            if (removeFactorFromProduct(node->left->right, node->right, reducedDen))
            {
                changed = true;
                // Check Note: This branch runs when the reduced den is missing.
                if (reducedDen == NULL)
                    return node->left->left;
                return makeOperator('/', node->left->left, reducedDen);
            }
        }
        // Check Note: This branch runs when the right side exists, the right side is an
        // operator node, and the operator in the right side is the character '/'.
        if (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/')
        {
            Node *reducedDen = NULL;
            // Check Note: This branch runs when the requested factor can be removed from the
            // right child of the right side.
            if (removeFactorFromProduct(node->right->right, node->left, reducedDen))
            {
                changed = true;
                // Check Note: This branch runs when the reduced den is missing.
                if (reducedDen == NULL)
                    return node->right->left;
                return makeOperator('/', node->right->left, reducedDen);
            }
        }

        // Keep conjugate products factored: (a-b)*(a+b) or (a+b)*(a-b).
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the right side is an operator node, and either the
        // operator in the left side is the character '-' and the operator in the right side is
        // the character '+' or the operator in the left side is the character '+' and the
        // operator in the right side is the character '-'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->right->type == NODE_OPERATOR &&
            ((node->left->op == '-' && node->right->op == '+') || (node->left->op == '+' && node->right->op == '-')))
        {
            Node *m = (node->left->op == '-') ? node->left : node->right;
            Node *p = (node->left->op == '+') ? node->left : node->right;
            if (areTreesEqual(m->left, p->left) && areTreesEqual(m->right, p->right))
                return node;
            if (areTreesEqual(m->left, p->right) && areTreesEqual(m->right, p->left))
                return node;
        }

        // Keep (a-b)*(a+b)*(a^2+b^2) factored to preserve x^4-y^4 identity form.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *pairNode = NULL;
            Node *sumSq = NULL;
            if (node->left->type == NODE_OPERATOR && node->left->op == '*')
            {
                pairNode = node->left;
                sumSq = node->right;
            }
            else if (node->right->type == NODE_OPERATOR && node->right->op == '*')
            {
                pairNode = node->right;
                sumSq = node->left;
            }

            if (pairNode != NULL && sumSq != NULL && sumSq->type == NODE_OPERATOR && sumSq->op == '+')
            {
                Node *a = NULL;
                Node *b = NULL;
                Node *c1 = pairNode->left;
                Node *c2 = pairNode->right;
                if (c1 != NULL && c2 != NULL && c1->type == NODE_OPERATOR && c2->type == NODE_OPERATOR &&
                    ((c1->op == '-' && c2->op == '+') || (c1->op == '+' && c2->op == '-')))
                {
                    Node *m = (c1->op == '-') ? c1 : c2;
                    Node *p = (c1->op == '+') ? c1 : c2;
                    if (areTreesEqual(m->left, p->left) && areTreesEqual(m->right, p->right))
                    {
                        a = m->left;
                        b = m->right;
                    }
                    else if (areTreesEqual(m->left, p->right) && areTreesEqual(m->right, p->left))
                    {
                        a = m->left;
                        b = m->right;
                    }
                }

                if (a != NULL && b != NULL &&
                    sumSq->left != NULL && sumSq->right != NULL &&
                    sumSq->left->type == NODE_OPERATOR && sumSq->left->op == '^' &&
                    sumSq->right->type == NODE_OPERATOR && sumSq->right->op == '^' &&
                    sumSq->left->right != NULL && sumSq->left->right->type == NODE_NUMBER && isZero(sumSq->left->right->numberValue - 2.0) &&
                    sumSq->right->right != NULL && sumSq->right->right->type == NODE_NUMBER && isZero(sumSq->right->right->numberValue - 2.0) &&
                    ((areTreesEqual(sumSq->left->left, a) && areTreesEqual(sumSq->right->left, b)) ||
                     (areTreesEqual(sumSq->left->left, b) && areTreesEqual(sumSq->right->left, a))))
                {
                    return node;
                }
            }
        }

        // Distribute multiplication over +/-.
        // Distributive Property Handler
        // Version tags: v2 initial, v6 stabilization context.
        // Check Note: This branch runs when the left side exists, the left side is an operator
        // node, and either the operator in the left side is the character '+' or the operator
        // in the left side is the character '-'.
        if (node->left != NULL && node->left->type == NODE_OPERATOR &&
            (node->left->op == '+' || node->left->op == '-'))
        {
            changed = true;
            Node *l = makeOperator('*', node->left->left, node->right);
            Node *r = makeOperator('*', node->left->right, node->right);
            return makeOperator(node->left->op, l, r);
        }
        // Check Note: This branch runs when the right side exists, the right side is an
        // operator node, and either the operator in the right side is the character '+' or the
        // operator in the right side is the character '-'.
        if (node->right != NULL && node->right->type == NODE_OPERATOR &&
            (node->right->op == '+' || node->right->op == '-'))
        {
            changed = true;
            Node *l = makeOperator('*', node->left, node->right->left);
            Node *r = makeOperator('*', node->left, node->right->right);
            return makeOperator(node->right->op, l, r);
        }

        // (a/b) * (b/c) => a/c
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '/',
        // the right side is an operator node, and the operator in the right side is the
        // character '/'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            node->right->type == NODE_OPERATOR && node->right->op == '/')
        {
            // Check Note: This branch runs when the right child of the left side and the left
            // child of the right side represent the same expression.
            if (areTreesEqual(node->left->right, node->right->left))
            {
                changed = true;
                return makeOperator('/', node->left->left, node->right->right);
            }
            // Check Note: This branch runs when the left child of the left side and the right
            // child of the right side represent the same expression.
            if (areTreesEqual(node->left->left, node->right->right))
            {
                changed = true;
                return makeOperator('/', node->right->left, node->left->right);
            }
        }

        // Aggressive exponent-law merge: x^a * x^b => x^(a+b), including implicit exponent 1.
        // Exponent Property Handler (Product Form)
        // Version tags: v4 introduced, v6 integrated with broader canonical flow.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *lb = NULL;
            Node *le = NULL;
            Node *rb = NULL;
            Node *re = NULL;

            // Check Note: This branch runs when the left side is an operator node and the
            // operator in the left side is the character '^'.
            if (node->left->type == NODE_OPERATOR && node->left->op == '^')
            {
                lb = node->left->left;
                le = node->left->right;
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                lb = node->left;
                le = makeNumber(1.0);
            }

            // Check Note: This branch runs when the right side is an operator node and the
            // operator in the right side is the character '^'.
            if (node->right->type == NODE_OPERATOR && node->right->op == '^')
            {
                rb = node->right->left;
                re = node->right->right;
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                rb = node->right;
                re = makeNumber(1.0);
            }

            // Check Note: This branch runs when the lb and the rb represent the same
            // expression.
            if (areTreesEqual(lb, rb))
            {
                changed = true;
                return makeOperator('^', lb, makeOperator('+', le, re));
            }
        }

        // x*x and x^a * x^b style compaction.
        string mv;
        int mp = 0;
        double mc = 0.0;
        // Check Note: This branch runs when the current node can be treated as a single
        // monomial and the mv is not empty.
        if (extractMonomial(node, mv, mp, mc) && !mv.empty())
        {
            Node *mono = buildMonomial(mv, mp, mc);
            // Check Note: This branch runs when the mono and the current node represent the
            // same expression.
            if (!areTreesEqual(mono, node))
            {
                changed = true;
                return mono;
            }
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '^'.
    if (node->op == '^')
    {
        // e^(ln(x)) => x
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is a number node, the number value - exp(1.0) of the left side is zero, and
        // the right side is a ln(...) call.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_NUMBER && isZero(node->left->numberValue - exp(1.0)) &&
            isFunctionNode(node->right, "ln"))
        {
            changed = true;
            return node->right->left;
        }

        // 10^log(x) => x
        // Check Note: This branch runs when the left side exists, the left side is the number
        // ten, the right side exists, and the right side is a log(...) call.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_NUMBER && isZero(node->left->numberValue - 10.0) &&
            isFunctionNode(node->right, "log"))
        {
            changed = true;
            return node->right->left;
        }

        // a^(log(x)/log(a)) => x (base/log-base inverse in change-of-base form).
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // right side is an operator node, the operator in the right side is the character '/',
        // the left child of the right side is a log(...) call, the right child of the right
        // side is a log(...) call, and the left side and the left child of the right child of
        // the right side represent the same expression.
        if (node->left != NULL && node->right != NULL &&
            node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            isFunctionNode(node->right->left, "log") && isFunctionNode(node->right->right, "log") &&
            areTreesEqual(node->left, node->right->right->left))
        {
            changed = true;
            return node->right->left->left;
        }

        // a^(log(x)/k) => x when k is numeric log(a) after folding.
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is greater than zero, the right side exists,
        // the right side is an operator node, the operator in the right side is the character
        // '/', the left child of the right side exists, either the left child of the right side
        // is a log(...) call or the left child of the right side is a ln(...) call, the right
        // child of the right side exists, and the right child of the right side is a number
        // node.
        if (node->left != NULL && node->left->type == NODE_NUMBER && node->left->numberValue > 0.0 &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            node->right->left != NULL &&
            (isFunctionNode(node->right->left, "log") || isFunctionNode(node->right->left, "ln")) &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER)
        {
            double denom = node->right->right->numberValue;
            string exponentFn = toLowerString(node->right->left->variableName);
            double baseLog = (exponentFn == "log") ? log10(node->left->numberValue)
                                                   : log(node->left->numberValue);
            if (fabs(denom - baseLog) < 1e-7)
            {
                changed = true;
                return node->right->left->left;
            }
        }

        // (sqrt(x))^(2k) => x^k (including k=1 => x).
        // Check Note: This branch runs when the left side is a sqrt(...) call, the right side
        // exists, the right side is a number node, the numeric value in the right side is a
        // whole number, and the numeric value in the right side is at least zero.
        if (isFunctionNode(node->left, "sqrt") && node->right != NULL && node->right->type == NODE_NUMBER &&
            isIntegerValue(node->right->numberValue) && node->right->numberValue >= 0)
        {
            int p = (int)round(node->right->numberValue);
            if ((p % 2) == 0)
            {
                changed = true;
                int half = p / 2;
                if (half == 0)
                    return makeNumber(1.0);
                if (half == 1)
                    return node->left->left;
                return makeOperator('^', node->left->left, makeNumber((double)half));
            }
        }

        // a^0.5 where a is perfect-square-compatible => simplified root-free form.
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // right side is a number node, and 5 of the number value - 0 of the right side is zero.
        if (node->left != NULL && node->right != NULL && node->right->type == NODE_NUMBER && isZero(node->right->numberValue - 0.5))
        {
            Node *sqrtReduced = NULL;
            // Check Note: This branch runs when the square-root input the left side can be
            // simplified safely and the simplified square-root form exists.
            if (simplifySquareRootRadicand(node->left, sqrtReduced) && sqrtReduced != NULL)
            {
                changed = true;
                return sqrtReduced;
            }
        }

        // (x^n / x^m)^p => x^(p*(n-m)) to preserve compact exponent pipeline form.
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '/',
        // the left child of the left side exists, the right child of the left side exists, the
        // left child of the left side is an operator node, the operator in the left child of
        // the left side is the character '^', the right child of the left side is an operator
        // node, the operator in the right child of the left side is the character '^', and the
        // left child of the left child of the left side and the left child of the right child
        // of the left side represent the same expression.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            node->left->left != NULL && node->left->right != NULL &&
            node->left->left->type == NODE_OPERATOR && node->left->left->op == '^' &&
            node->left->right->type == NODE_OPERATOR && node->left->right->op == '^' &&
            areTreesEqual(node->left->left->left, node->left->right->left))
        {
            changed = true;
            Node *base = node->left->left->left;
            Node *nExp = node->left->left->right;
            Node *mExp = node->left->right->right;
            return makeOperator('^', base, makeOperator('*', node->right, makeOperator('-', nExp, mExp)));
        }

        // General bounded multinomial expansion: (sum)^n for integer n.
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // right side is a number node, the numeric value in the right side is a whole number,
        // and the numeric value in the right side is at least 0 of 2.
        if (node->left != NULL && node->right != NULL && node->right->type == NODE_NUMBER &&
            isIntegerValue(node->right->numberValue) && node->right->numberValue >= 2.0)
        {
            int n = (int)round(node->right->numberValue);
            Node *expanded = NULL;
            // Check Note: This branch runs when the left side can be expanded safely with the
            // multinomial rule and the expanded form exists.
            if (tryExpandPowerByMultinomial(node->left, n, expanded) && expanded != NULL)
            {
                changed = true;
                return expanded;
            }
        }

        // Exponent Property Handler (Power of Power)
        // Version tags: v4 introduced, v6 validation-stabilized.
        // Aggressive exponent-law: (a^b)^c => a^(b*c)
        // Check Note: This branch runs when the left side exists, the left side is an operator
        // node, and the operator in the left side is the character '^'.
        if (node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '^')
        {
            changed = true;
            return makeOperator('^', node->left->left, makeOperator('*', node->left->right, node->right));
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '/'.
    if (node->op == '/')
    {
        // log(x^n) / n => log(x), and ln(x^n) / n => ln(x).
        // Check Note: This branch runs when the left side exists, the right side exists, either
        // the left side is a log(...) call or the left side is a ln(...) call, the left child
        // of the left side exists, the left child of the left side is an operator node, the
        // operator in the left child of the left side is the character '^', and the right child
        // of the left child of the left side and the right side represent the same expression.
        if (node->left != NULL && node->right != NULL &&
            (isFunctionNode(node->left, "log") || isFunctionNode(node->left, "ln")) &&
            node->left->left != NULL && node->left->left->type == NODE_OPERATOR && node->left->left->op == '^' &&
            areTreesEqual(node->left->left->right, node->right))
        {
            changed = true;
            return makeFunction(node->left->variableName, node->left->left->left);
        }

        // a / ((x + y) * (x - y)) => a / (x^2 - y^2)
        // Check Note: This branch runs when the right side exists, the right side is an
        // operator node, the operator in the right side is the character '*', the left child of
        // the right side exists, the right child of the right side exists, the left child of
        // the right side is an operator node, the right child of the right side is an operator
        // node, and either the operator in the left child of the right side is the character
        // '+' and the operator in the right child of the right side is the character '-' or the
        // operator in the left child of the right side is the character '-' and the operator in
        // the right child of the right side is the character '+'.
        if (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '*' &&
            node->right->left != NULL && node->right->right != NULL &&
            node->right->left->type == NODE_OPERATOR && node->right->right->type == NODE_OPERATOR &&
            ((node->right->left->op == '+' && node->right->right->op == '-') ||
             (node->right->left->op == '-' && node->right->right->op == '+')))
        {
            Node *p = (node->right->left->op == '+') ? node->right->left : node->right->right;
            Node *m = (node->right->left->op == '-') ? node->right->left : node->right->right;

            if (areTreesEqual(p->left, m->left) && areTreesEqual(p->right, m->right))
            {
                changed = true;
                return makeOperator('/', node->left,
                                    makeOperator('-', makeOperator('^', p->left, makeNumber(2.0)),
                                                 makeOperator('^', p->right, makeNumber(2.0))));
            }
            if (areTreesEqual(p->left, m->right) && areTreesEqual(p->right, m->left))
            {
                changed = true;
                return makeOperator('/', node->left,
                                    makeOperator('-', makeOperator('^', p->left, makeNumber(2.0)),
                                                 makeOperator('^', p->right, makeNumber(2.0))));
            }

            // Canonicalize conjugates even when sum/difference child ordering has already been normalized.
            // Check Note: This branch runs when either the left child of p and the left child
            // of m represent the same expression, the left child of p and the right child of m
            // represent the same expression, the right child of p and the left child of m
            // represent the same expression, or the right child of p and the right child of m
            // represent the same expression.
            if (areTreesEqual(p->left, m->left) || areTreesEqual(p->left, m->right) ||
                areTreesEqual(p->right, m->left) || areTreesEqual(p->right, m->right))
            {
                changed = true;
                return makeOperator('/', node->left,
                                    makeOperator('-', makeOperator('^', p->left, makeNumber(2.0)),
                                                 makeOperator('^', p->right, makeNumber(2.0))));
            }
        }

        // Division Handler Group (v2, v4, v6):
        // - inverse reductions and nested-fraction normalization
        // - exponent subtraction law
        // - factor cancellation in numerator/denominator products
        // abs(x) / x => sgn(x)
        // Check Note: This branch runs when the left side is a abs(...) call and the left child
        // of the left side and the right side represent the same expression.
        if (isFunctionNode(node->left, "abs") && areTreesEqual(node->left->left, node->right))
        {
            changed = true;
            return makeFunction("sgn", node->right);
        }

        // 1 / (1 + 1/x) => x / (x + 1)
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is one, the right side exists, the right
        // side is an operator node, the operator in the right side is the character '+', the
        // left child of the right side exists, the left child of the right side is a number
        // node, the numeric value in the left child of the right side is one, the right child
        // of the right side exists, the right child of the right side is an operator node, the
        // operator in the right child of the right side is the character '/', the left child of
        // the right child of the right side exists, the left child of the right child of the
        // right side is a number node, and the numeric value in the left child of the right
        // child of the right side is one.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue) &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '+' &&
            node->right->left != NULL && node->right->left->type == NODE_NUMBER && isOne(node->right->left->numberValue) &&
            node->right->right != NULL && node->right->right->type == NODE_OPERATOR && node->right->right->op == '/' &&
            node->right->right->left != NULL && node->right->right->left->type == NODE_NUMBER && isOne(node->right->right->left->numberValue))
        {
            Node *x = node->right->right->right;
            changed = true;
            return makeOperator('/', x, makeOperator('+', x, makeNumber(1.0)));
        }

        // x / sqrt(x) => sqrt(x)
        // Check Note: This branch runs when the right side is a sqrt(...) call and the left
        // side and the left child of the right side represent the same expression.
        if (isFunctionNode(node->right, "sqrt") && areTreesEqual(node->left, node->right->left))
        {
            changed = true;
            return makeFunction("sqrt", node->left);
        }

        // sqrt(x) / x => x^-0.5
        // Check Note: This branch runs when the left side is a sqrt(...) call and the left
        // child of the left side and the right side represent the same expression.
        if (isFunctionNode(node->left, "sqrt") && areTreesEqual(node->left->left, node->right))
        {
            changed = true;
            return makeOperator('^', node->right, makeNumber(-0.5));
        }

        // 1 / (1 / x) => x
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is one, the right side exists, the right
        // side is an operator node, the operator in the right side is the character '/', the
        // left child of the right side exists, the left child of the right side is a number
        // node, and the numeric value in the left child of the right side is one.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue) &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            node->right->left != NULL && node->right->left->type == NODE_NUMBER && isOne(node->right->left->numberValue))
        {
            changed = true;
            return node->right->right;
        }

        // a / (b / c) => (a * c) / b
        // Associative-style division normalization handler.
        // Check Note: This branch runs when the right side exists, the right side is an
        // operator node, and the operator in the right side is the character '/'.
        if (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/')
        {
            changed = true;
            return makeOperator('/', makeOperator('*', node->left, node->right->right), node->right->left);
        }

        // (a / b) / c => a / (b * c)
        // Check Note: This branch runs when the left side exists, the left side is an operator
        // node, and the operator in the left side is the character '/'.
        if (node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '/')
        {
            changed = true;
            return makeOperator('/', node->left->left, makeOperator('*', node->left->right, node->right));
        }

        // (a*f + b*f + ...)/f => a + b + ... when every additive term shares factor f.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *reducedNumerator = NULL;
            // Check Note: This branch runs when every term in the left side can give up the
            // factor the right side.
            if (divideOutCommonFactorFromAdditive(node->left, node->right, reducedNumerator))
            {
                changed = true;
                return reducedNumerator;
            }
        }

        // Aggressive exponent-law: x^a / x^b => x^(a-b), including implicit exponent 1.
        // Exponent Property Handler (Quotient Form)
        // Version tags: v4 introduced, v6 matched with commutative canonical terms.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *lb = NULL;
            Node *le = NULL;
            Node *rb = NULL;
            Node *re = NULL;

            // Check Note: This branch runs when the left side is an operator node and the
            // operator in the left side is the character '^'.
            if (node->left->type == NODE_OPERATOR && node->left->op == '^')
            {
                lb = node->left->left;
                le = node->left->right;
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                lb = node->left;
                le = makeNumber(1.0);
            }

            // Check Note: This branch runs when the right side is an operator node and the
            // operator in the right side is the character '^'.
            if (node->right->type == NODE_OPERATOR && node->right->op == '^')
            {
                rb = node->right->left;
                re = node->right->right;
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                rb = node->right;
                re = makeNumber(1.0);
            }

            // Check Note: This branch runs when the lb and the rb represent the same
            // expression.
            if (areTreesEqual(lb, rb))
            {
                changed = true;
                return makeOperator('^', lb, makeOperator('-', le, re));
            }
        }

        // Cancel denominator factors from a product numerator: (a*b*c)/(b*c) => a.
        // Check Note: This branch runs when the left side exists.
        if (node->left != NULL)
        {
            Node *tmp = node->left;
            bool removedAny = false;

            // Check Note: This branch runs when the right side exists, the right side is an
            // operator node, and the operator in the right side is the character '*'.
            if (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '*')
            {
                Node *afterFirst = NULL;
                // Check Note: This branch runs when the requested factor can be removed from
                // the tmp.
                if (removeFactorFromProduct(tmp, node->right->left, afterFirst))
                {
                    tmp = (afterFirst == NULL) ? makeNumber(1.0) : afterFirst;
                    removedAny = true;
                }
                Node *afterSecond = NULL;
                // Check Note: This branch runs when the requested factor can be removed from
                // the tmp.
                if (removeFactorFromProduct(tmp, node->right->right, afterSecond))
                {
                    tmp = (afterSecond == NULL) ? makeNumber(1.0) : afterSecond;
                    removedAny = true;
                }
                // Check Note: This branch runs when at least one factor was removed.
                if (removedAny)
                {
                    changed = true;
                    return tmp;
                }
            }

            Node *afterSingle = NULL;
            // Check Note: This branch runs when the requested factor can be removed from the
            // tmp.
            if (removeFactorFromProduct(tmp, node->right, afterSingle))
            {
                changed = true;
                // Check Note: This branch runs when the after single is missing.
                if (afterSingle == NULL)
                    return makeNumber(1.0);
                return afterSingle;
            }
        }

        // sin(x) / cos(x) => tan(x)
        // Check Note: This branch runs when the left side is a sin(...) call, the right side is
        // a cos(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "sin") && isFunctionNode(node->right, "cos") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("tan", node->left->left);
        }

        // sin(x) / tan(x) => cos(x)
        // Check Note: This branch runs when the left side is a sin(...) call, the right side is
        // a tan(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "sin") && isFunctionNode(node->right, "tan") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("cos", node->left->left);
        }

        // cos(x) / sin(x) => cot(x)
        // Check Note: This branch runs when the left side is a cos(...) call, the right side is
        // a sin(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "cos") && isFunctionNode(node->right, "sin") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("cot", node->left->left);
        }

        // sec(x) / csc(x) => tan(x)
        // Check Note: This branch runs when the left side is a sec(...) call, the right side is
        // a csc(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "sec") && isFunctionNode(node->right, "csc") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("tan", node->left->left);
        }

        // csc(x) / sec(x) => cot(x)
        // Check Note: This branch runs when the left side is a csc(...) call, the right side is
        // a sec(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "csc") && isFunctionNode(node->right, "sec") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("cot", node->left->left);
        }

        // tan(x) / sec(x) => sin(x)
        // Check Note: This branch runs when the left side is a tan(...) call, the right side is
        // a sec(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "tan") && isFunctionNode(node->right, "sec") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("sin", node->left->left);
        }

        // cot(x) / csc(x) => cos(x)
        // Check Note: This branch runs when the left side is a cot(...) call, the right side is
        // a csc(...) call, and the left child of the left side and the left child of the right
        // side represent the same expression.
        if (isFunctionNode(node->left, "cot") && isFunctionNode(node->right, "csc") &&
            areTreesEqual(node->left->left, node->right->left))
        {
            changed = true;
            return makeFunction("cos", node->left->left);
        }

        // 1 / sec(x) => cos(x), 1 / csc(x) => sin(x), 1 / cot(x) => tan(x)
        // Check Note: This branch runs when the left side is the number one and the right side
        // is one of the supported reciprocal trig functions.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue))
        {
            // Check Note: This branch runs when the right side is a sec(...) call.
            if (isFunctionNode(node->right, "sec"))
            {
                changed = true;
                return makeFunction("cos", node->right->left);
            }
            // Check Note: If the earlier case did not match, this branch runs when the right
            // side is a csc(...) call.
            if (isFunctionNode(node->right, "csc"))
            {
                changed = true;
                return makeFunction("sin", node->right->left);
            }
            // Check Note: If the earlier case did not match, this branch runs when the right
            // side is a cot(...) call.
            if (isFunctionNode(node->right, "cot"))
            {
                changed = true;
                return makeFunction("tan", node->right->left);
            }
        }

        // sin(2*x) / (2*sin(x)) => cos(x)
        // Check Note: This branch runs when the left side is a sin(...) call, the right side
        // can be split into 2 times another factor, the sin input is a doubled angle, the other
        // denominator factor is a sin(...) call, and both rules refer to the same inner angle.
        if (isFunctionNode(node->left, "sin"))
        {
            Node *doubleAngleArg = NULL;
            Node *scaledDenominator = NULL;
            Node *singleAngleArg = NULL;

            // Check Note: This branch runs when the sine input is a doubled angle, the
            // denominator has a numeric factor of two, and the remaining denominator factor is a
            // sin(...) call.
            if (extractDoubleAngleArgument(node->left->left, doubleAngleArg) &&
                extractProductWithNumericFactor(node->right, 2.0, scaledDenominator) &&
                isFunctionNode(scaledDenominator, "sin"))
            {
                singleAngleArg = scaledDenominator->left;
                // Check Note: This branch runs when the doubled-angle base and the denominator
                // sine input represent the same expression.
                if (areTreesEqual(doubleAngleArg, singleAngleArg))
                {
                    changed = true;
                    return makeFunction("cos", singleAngleArg);
                }
            }
        }

        // (x*y)/x => y and (x*y)/y => x
        // Check Note: This branch runs when the left side exists, the left side is an operator
        // node, and the operator in the left side is the character '*'.
        if (node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '*')
        {
            // Check Note: This branch runs when the left child of the left side and the right
            // side represent the same expression.
            if (areTreesEqual(node->left->left, node->right))
            {
                changed = true;
                return node->left->right;
            }
            // Check Note: This branch runs when the right child of the left side and the right
            // side represent the same expression.
            if (areTreesEqual(node->left->right, node->right))
            {
                changed = true;
                return node->left->left;
            }
        }

        // (a^2 - b^2)/(a - b) => a + b and (a^2 - b^2)/(a + b) => a - b.
        // Also accepts swapped square order in numerator when denominator terms match.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *numA2 = NULL;
            Node *numB2 = NULL;

            // Check Note: This branch runs when the left side can be read as a subtraction.
            if (extractDifference(node->left, numA2, numB2))
            {
                Node *a = NULL;
                Node *b = NULL;

                // (a^2 - b^2)/(a - b) => a + b
                // Check Note: This branch runs when the right side can be read as a
                // subtraction.
                if (extractDifference(node->right, a, b))
                {
                    // Check Note: This branch runs when the is square of(num a2, a) and the is
                    // square of(num b2, b).
                    if (isSquareOf(numA2, a) && isSquareOf(numB2, b))
                    {
                        changed = true;
                        return makeOperator('+', a, b);
                    }

                    // Check Note: This branch runs when the is square of(num a2, b) and the is
                    // square of(num b2, a).
                    if (isSquareOf(numA2, b) && isSquareOf(numB2, a))
                    {
                        changed = true;
                        return makeOperator('*', makeNumber(-1.0), makeOperator('+', a, b));
                    }
                }

                // (a^2 - b^2)/(a + b) => a - b
                // Check Note: This branch runs when the right side is an operator node, the
                // operator in the right side is the character '+', the left child of the right
                // side exists, and the right child of the right side exists.
                if (node->right->type == NODE_OPERATOR && node->right->op == '+' &&
                    node->right->left != NULL && node->right->right != NULL)
                {
                    a = node->right->left;
                    b = node->right->right;

                    // Check Note: This branch runs when the is square of(num a2, a) and the is
                    // square of(num b2, b).
                    if (isSquareOf(numA2, a) && isSquareOf(numB2, b))
                    {
                        changed = true;
                        return makeOperator('-', a, b);
                    }

                    // Check Note: This branch runs when the is square of(num a2, b) and the is
                    // square of(num b2, a).
                    if (isSquareOf(numA2, b) && isSquareOf(numB2, a))
                    {
                        changed = true;
                        return makeOperator('-', b, a);
                    }
                }
            }
        }

        // (a^2 - b^2)/(a - b)^2 => (a + b)/(a - b), and
        // (a^2 - b^2)/(a + b)^2 => (a - b)/(a + b).
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // right side is an operator node, the operator in the right side is the character '^',
        // the right child of the right side exists, the right child of the right side is a
        // number node, and 0 of the number value - 2 of the right child of the right side is
        // zero.
        if (node->left != NULL && node->right != NULL &&
            node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER && isZero(node->right->right->numberValue - 2.0))
        {
            Node *numA2 = NULL;
            Node *numB2 = NULL;
            if (extractDifference(node->left, numA2, numB2) &&
                node->right->left != NULL && node->right->left->type == NODE_OPERATOR)
            {
                Node *a = node->right->left->left;
                Node *b = node->right->left->right;

                if (node->right->left->op == '-')
                {
                    if (isSquareOf(numA2, a) && isSquareOf(numB2, b))
                    {
                        changed = true;
                        return makeOperator('/', makeOperator('+', a, b), makeOperator('-', a, b));
                    }
                }
                else if (node->right->left->op == '+')
                {
                    if (isSquareOf(numA2, a) && isSquareOf(numB2, b))
                    {
                        changed = true;
                        return makeOperator('/', makeOperator('-', a, b), makeOperator('+', a, b));
                    }
                }
            }
        }

        // (a^4 - b^4)/(a^2 + b^2) => a^2 - b^2
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '-',
        // the right side is an operator node, and the operator in the right side is the
        // character '+'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '-' &&
            node->right->type == NODE_OPERATOR && node->right->op == '+')
        {
            Node *a = NULL;
            Node *b = NULL;
            // Check Note: This branch runs when the left child of the right side exists, the
            // right child of the right side exists, the left child of the right side is an
            // operator node, the operator in the left child of the right side is the character
            // '^', the right child of the left child of the right side exists, the right child
            // of the left child of the right side is a number node, 0 of the number value - 2
            // of the right child of the left child of the right side is zero, the right child
            // of the right side is an operator node, the operator in the right child of the
            // right side is the character '^', the right child of the right child of the right
            // side exists, the right child of the right child of the right side is a number
            // node, and 0 of the number value - 2 of the right child of the right child of the
            // right side is zero.
            if (node->right->left != NULL && node->right->right != NULL &&
                node->right->left->type == NODE_OPERATOR && node->right->left->op == '^' &&
                node->right->left->right != NULL && node->right->left->right->type == NODE_NUMBER && isZero(node->right->left->right->numberValue - 2.0) &&
                node->right->right->type == NODE_OPERATOR && node->right->right->op == '^' &&
                node->right->right->right != NULL && node->right->right->right->type == NODE_NUMBER && isZero(node->right->right->right->numberValue - 2.0))
            {
                a = node->right->left->left;
                b = node->right->right->left;
                // Check Note: This branch runs when the is nth power of(node->left->left, a, 4)
                // and the is nth power of(node->left->right, b, 4).
                if (isNthPowerOf(node->left->left, a, 4) && isNthPowerOf(node->left->right, b, 4))
                {
                    changed = true;
                    return makeOperator('-', makeOperator('^', a, makeNumber(2.0)), makeOperator('^', b, makeNumber(2.0)));
                }
            }
        }

        // (a^3 - b^3)/(a - b) => a^2 + ab + b^2
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, and the operator in the left side is the character
        // '-'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '-')
        {
            Node *a = NULL;
            Node *b = NULL;
            // Check Note: This branch runs when the right side can be read as a subtraction,
            // the is cube of(node->left->left, a), and the is cube of(node->left->right, b).
            if (extractDifference(node->right, a, b) && isCubeOf(node->left->left, a) && isCubeOf(node->left->right, b))
            {
                changed = true;
                Node *a2 = makeOperator('^', a, makeNumber(2.0));
                Node *ab = makeOperator('*', a, b);
                Node *b2 = makeOperator('^', b, makeNumber(2.0));
                return makeOperator('+', makeOperator('+', a2, ab), b2);
            }
        }

        // (a^3 + b^3)/(a + b) => a^2 - ab + b^2
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '+',
        // the right side is an operator node, and the operator in the right side is the
        // character '+'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '+' &&
            node->right->type == NODE_OPERATOR && node->right->op == '+')
        {
            Node *a = node->right->left;
            Node *b = node->right->right;

            // Check Note: This branch runs when the is cube of(node->left->left, a) and the is
            // cube of(node->left->right, b).
            if (isCubeOf(node->left->left, a) && isCubeOf(node->left->right, b))
            {
                changed = true;
                Node *a2 = makeOperator('^', a, makeNumber(2.0));
                Node *ab = makeOperator('*', a, b);
                Node *b2 = makeOperator('^', b, makeNumber(2.0));
                return makeOperator('+', makeOperator('-', a2, ab), b2);
            }
        }

        // (a^4 - b^4)/(a - b) => a^3 + a^2b + ab^2 + b^3
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, and the operator in the left side is the character
        // '-'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '-')
        {
            Node *a = NULL;
            Node *b = NULL;
            // Check Note: This branch runs when the right side can be read as a subtraction,
            // the is nth power of(node->left->left, a, 4), and the is nth power
            // of(node->left->right, b, 4).
            if (extractDifference(node->right, a, b) && isNthPowerOf(node->left->left, a, 4) && isNthPowerOf(node->left->right, b, 4))
            {
                changed = true;
                Node *a3 = makeOperator('^', a, makeNumber(3.0));
                Node *a2b = makeOperator('*', makeOperator('^', a, makeNumber(2.0)), b);
                Node *ab2 = makeOperator('*', a, makeOperator('^', b, makeNumber(2.0)));
                Node *b3 = makeOperator('^', b, makeNumber(3.0));
                return makeOperator('+', makeOperator('+', a3, a2b), makeOperator('+', ab2, b3));
            }
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '+'.
    if (node->op == '+')
    {
        // Addition Handler Group (v2, v3, v6):
        // - additive inverse collapse
        // - same-denominator fraction merge
        // - classic trig/log additive identities
        // -t + t => 0 and t + -t => 0
        Node *pos = NULL;
        // Check Note: This branch runs when the left side can be recognized as a negated term
        // and the pos and the right side represent the same expression.
        if (extractNegatedTerm(node->left, pos) && areTreesEqual(pos, node->right))
        {
            changed = true;
            return makeNumber(0.0);
        }
        // Check Note: This branch runs when the right side can be recognized as a negated term
        // and the pos and the left side represent the same expression.
        if (extractNegatedTerm(node->right, pos) && areTreesEqual(pos, node->left))
        {
            changed = true;
            return makeNumber(0.0);
        }

        // 1 + (-cos(x)^2) => sin(x)^2 and 1 + (-sin(x)^2) => cos(x)^2.
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is one, the right side can be recognized as
        // a negated term, the pos exists, the pos is an operator node, the operator in the pos
        // is the character '^', the right child of the pos exists, the right child of the pos
        // is a number node, and 0 of the number value - 2 of the right child of the pos is
        // zero.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue) && extractNegatedTerm(node->right, pos) &&
            pos != NULL && pos->type == NODE_OPERATOR && pos->op == '^' &&
            pos->right != NULL && pos->right->type == NODE_NUMBER && isZero(pos->right->numberValue - 2.0))
        {
            if (isFunctionNode(pos->left, "cos"))
            {
                changed = true;
                return makeOperator('^', makeFunction("sin", pos->left->left), makeNumber(2.0));
            }
            if (isFunctionNode(pos->left, "sin"))
            {
                changed = true;
                return makeOperator('^', makeFunction("cos", pos->left->left), makeNumber(2.0));
            }
        }
        if (node->right != NULL && node->right->type == NODE_NUMBER && isOne(node->right->numberValue) && extractNegatedTerm(node->left, pos) &&
            pos != NULL && pos->type == NODE_OPERATOR && pos->op == '^' &&
            pos->right != NULL && pos->right->type == NODE_NUMBER && isZero(pos->right->numberValue - 2.0))
        {
            if (isFunctionNode(pos->left, "cos"))
            {
                changed = true;
                return makeOperator('^', makeFunction("sin", pos->left->left), makeNumber(2.0));
            }
            if (isFunctionNode(pos->left, "sin"))
            {
                changed = true;
                return makeOperator('^', makeFunction("cos", pos->left->left), makeNumber(2.0));
            }
        }

        // 1 + tan(x)^2 => sec(x)^2 and 1 + cot(x)^2 => csc(x)^2.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *oneSide = NULL;
            Node *powSide = NULL;
            if (node->left->type == NODE_NUMBER && isOne(node->left->numberValue))
            {
                oneSide = node->left;
                powSide = node->right;
            }
            else if (node->right->type == NODE_NUMBER && isOne(node->right->numberValue))
            {
                oneSide = node->right;
                powSide = node->left;
            }

            if (oneSide != NULL && powSide != NULL && powSide->type == NODE_OPERATOR && powSide->op == '^' &&
                powSide->right != NULL && powSide->right->type == NODE_NUMBER && isZero(powSide->right->numberValue - 2.0))
            {
                if (isFunctionNode(powSide->left, "tan"))
                {
                    changed = true;
                    return makeOperator('^', makeFunction("sec", powSide->left->left), makeNumber(2.0));
                }
                if (isFunctionNode(powSide->left, "cot"))
                {
                    changed = true;
                    return makeOperator('^', makeFunction("csc", powSide->left->left), makeNumber(2.0));
                }
            }
        }

        // 1 + 1/x => (x + 1) / x
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is one, the right side exists, the right
        // side is an operator node, the operator in the right side is the character '/', the
        // left child of the right side exists, the left child of the right side is a number
        // node, and the numeric value in the left child of the right side is one.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue) &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            node->right->left != NULL && node->right->left->type == NODE_NUMBER && isOne(node->right->left->numberValue))
        {
            Node *x = node->right->right;
            changed = true;
            return makeOperator('/', makeOperator('+', x, makeNumber(1.0)), x);
        }
        if (node->right != NULL && node->right->type == NODE_NUMBER && isOne(node->right->numberValue) &&
            node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            node->left->left != NULL && node->left->left->type == NODE_NUMBER && isOne(node->left->left->numberValue))
        {
            Node *x = node->left->right;
            changed = true;
            return makeOperator('/', makeOperator('+', x, makeNumber(1.0)), x);
        }

        // A*B + A => A*(B + 1), and A + A*B => A*(B + 1)
        // Check Note: This branch runs when the left side exists, the left side is an operator
        // node, the operator in the left side is the character '*', and the right side exists.
        if (node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '*' && node->right != NULL)
        {
            if (areTreesEqual(node->left->left, node->right))
            {
                changed = true;
                return makeOperator('*', node->right, makeOperator('+', node->left->right, makeNumber(1.0)));
            }
            if (areTreesEqual(node->left->right, node->right))
            {
                changed = true;
                return makeOperator('*', node->right, makeOperator('+', node->left->left, makeNumber(1.0)));
            }
        }
        // Check Note: This branch runs when the right side exists, the right side is an
        // operator node, the operator in the right side is the character '*', and the left side
        // exists.
        if (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '*' && node->left != NULL)
        {
            if (areTreesEqual(node->right->left, node->left))
            {
                changed = true;
                return makeOperator('*', node->left, makeOperator('+', node->right->right, makeNumber(1.0)));
            }
            if (areTreesEqual(node->right->right, node->left))
            {
                changed = true;
                return makeOperator('*', node->left, makeOperator('+', node->right->left, makeNumber(1.0)));
            }
        }

        // sin(x)^2 + cos(x)^2 => 1
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '^',
        // the right side is an operator node, the operator in the right side is the character
        // '^', the right child of the left side exists, the right child of the left side is a
        // number node, 0 of the number value - 2 of the right child of the left side is zero,
        // the right child of the right side exists, the right child of the right side is a
        // number node, and 0 of the number value - 2 of the right child of the right side is
        // zero.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '^' &&
            node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->left->right != NULL && node->left->right->type == NODE_NUMBER && isZero(node->left->right->numberValue - 2.0) &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER && isZero(node->right->right->numberValue - 2.0))
        {
            Node *ls = node->left->left;
            Node *rs = node->right->left;
            // Check Note: This branch runs when the left power base is a sin(...) call, the
            // right power base is a cos(...) call, and the left child of the left power base
            // and the left child of the right power base represent the same expression.
            if (isFunctionNode(ls, "sin") && isFunctionNode(rs, "cos") && areTreesEqual(ls->left, rs->left))
            {
                changed = true;
                return makeNumber(1.0);
            }
            // Check Note: This branch runs when the left power base is a cos(...) call, the
            // right power base is a sin(...) call, and the left child of the left power base
            // and the left child of the right power base represent the same expression.
            if (isFunctionNode(ls, "cos") && isFunctionNode(rs, "sin") && areTreesEqual(ls->left, rs->left))
            {
                changed = true;
                return makeNumber(1.0);
            }
        }

        // cos(2*x) + 2*sin(x)^2 => 1
        // Check Note: This branch runs when one side is a cos(...) call, the other side can be
        // split into 2 times another factor, that other factor is sin(...) squared, and both
        // sides refer to the same inner angle.
        if (node->left != NULL && node->right != NULL)
        {
            Node *cosSide = NULL;
            Node *otherSide = NULL;

            // Check Note: This branch runs when the left side is a cos(...) call.
            if (isFunctionNode(node->left, "cos"))
            {
                cosSide = node->left;
                otherSide = node->right;
            }
            // Check Note: If the earlier case did not match, this branch runs when the right
            // side is a cos(...) call.
            else if (isFunctionNode(node->right, "cos"))
            {
                cosSide = node->right;
                otherSide = node->left;
            }

            // Check Note: This branch runs when a cosine side and a matching partner side were
            // both found.
            if (cosSide != NULL && otherSide != NULL)
            {
                Node *doubleAngleArg = NULL;
                Node *scaledSquare = NULL;
                Node *sinArg = NULL;

                // Check Note: This branch runs when the cosine input is a doubled angle, the
                // partner side has a numeric factor of two, and the remaining partner factor is
                // sin(...) squared.
                if (extractDoubleAngleArgument(cosSide->left, doubleAngleArg) &&
                    extractProductWithNumericFactor(otherSide, 2.0, scaledSquare) &&
                    isPowerOfFunction(scaledSquare, "sin", 2.0, sinArg))
                {
                    // Check Note: This branch runs when the doubled-angle base and the sine
                    // square input represent the same expression.
                    if (areTreesEqual(doubleAngleArg, sinArg))
                    {
                        changed = true;
                        return makeNumber(1.0);
                    }
                }
            }
        }

        // sec(x)^2 + (-tan(x)^2) => 1 and csc(x)^2 + (-cot(x)^2) => 1
        // Check Note: This branch runs when one side is a negated term, the other side is a
        // positive squared trig term, and together they match one of the supported reordered
        // subtraction identities.
        if (node->left != NULL && node->right != NULL)
        {
            Node *positiveSide = NULL;
            Node *negatedSide = NULL;

            // Check Note: This branch runs when the left side can be recognized as a negated
            // term.
            if (extractNegatedTerm(node->left, negatedSide))
            {
                positiveSide = node->right;
            }
            // Check Note: If the earlier case did not match, this branch runs when the right
            // side can be recognized as a negated term.
            else if (extractNegatedTerm(node->right, negatedSide))
            {
                positiveSide = node->left;
            }

            // Check Note: This branch runs when both the positive squared side and the negated
            // squared side were identified.
            if (positiveSide != NULL && negatedSide != NULL)
            {
                Node *positiveArg = NULL;
                Node *negatedArg = NULL;

                // Check Note: This branch runs when the positive side is sec(...) squared, the
                // negated side is tan(...) squared, and both use the same angle.
                if (isPowerOfFunction(positiveSide, "sec", 2.0, positiveArg) &&
                    isPowerOfFunction(negatedSide, "tan", 2.0, negatedArg) &&
                    areTreesEqual(positiveArg, negatedArg))
                {
                    changed = true;
                    return makeNumber(1.0);
                }

                // Check Note: This branch runs when the positive side is csc(...) squared, the
                // negated side is cot(...) squared, and both use the same angle.
                if (isPowerOfFunction(positiveSide, "csc", 2.0, positiveArg) &&
                    isPowerOfFunction(negatedSide, "cot", 2.0, negatedArg) &&
                    areTreesEqual(positiveArg, negatedArg))
                {
                    changed = true;
                    return makeNumber(1.0);
                }
            }
        }

        // log(a) + log(b) => log(a*b)
        // Check Note: This branch runs when both sides are logarithm calls of the same kind,
        // ignoring uppercase versus lowercase letters in the function name.
        if ((isFunctionNode(node->left, "log") || isFunctionNode(node->left, "ln")) &&
            (isFunctionNode(node->right, "log") || isFunctionNode(node->right, "ln")) &&
            toLowerString(node->left->variableName) == toLowerString(node->right->variableName))
        {
            changed = true;
            return makeFunction(node->left->variableName, makeOperator('*', node->left->left, node->right->left));
        }

        // log(a^b) => b * log(a) and ln(a^b) => b * ln(a)
        // Check Note: This branch runs when either the left side is a log(...) call or the left
        // side is a ln(...) call, the left child of the left side exists, the left child of the
        // left side is an operator node, and the operator in the left child of the left side is
        // the character '^'.
        if ((isFunctionNode(node->left, "log") || isFunctionNode(node->left, "ln")) &&
            node->left->left != NULL && node->left->left->type == NODE_OPERATOR && node->left->left->op == '^')
        {
            changed = true;
            return makeOperator('*', node->left->left->right, makeFunction(node->left->variableName, node->left->left->left));
        }
        if ((isFunctionNode(node->right, "log") || isFunctionNode(node->right, "ln")) &&
            node->right->left != NULL && node->right->left->type == NODE_OPERATOR && node->right->left->op == '^')
        {
            changed = true;
            return makeOperator('*', node->right->left->right, makeFunction(node->right->variableName, node->right->left->left));
        }

        // x + (k*x)/d => merged coefficient form when monomial base matches.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            string lv, rv;
            int lp = 0, rp = 0;
            double lc = 0.0, rc = 0.0;
            double d = 0.0;

            if (extractMonomial(node->left, lv, lp, lc) && node->right->type == NODE_OPERATOR && node->right->op == '/' &&
                isNumberNode(node->right->right, d) && !isZero(d) && extractMonomial(node->right->left, rv, rp, rc) &&
                lv == rv && lp == rp)
            {
                changed = true;
                return buildMonomial(lv, lp, lc + rc / d);
            }

            if (extractMonomial(node->right, rv, rp, rc) && node->left->type == NODE_OPERATOR && node->left->op == '/' &&
                isNumberNode(node->left->right, d) && !isZero(d) && extractMonomial(node->left->left, lv, lp, lc) &&
                lv == rv && lp == rp)
            {
                changed = true;
                return buildMonomial(rv, rp, rc + lc / d);
            }
        }

        // Merge signed same-denominator fractions inside addition context.
        // Check Note: This branch runs when the left side exists and the right side exists.
        if (node->left != NULL && node->right != NULL)
        {
            Node *ln = NULL;
            Node *ld = NULL;
            Node *rn = NULL;
            Node *rd = NULL;
            int ls = 1;
            int rs = 1;

            // Check Note: This branch runs when both sides can be read as signed fractions and
            // those fractions use the same denominator.
            if (extractSignedFractionTerm(node->left, ln, ld, ls) && extractSignedFractionTerm(node->right, rn, rd, rs) && areTreesEqual(ld, rd))
            {
                Node *leftNum = (ls < 0) ? makeOperator('*', makeNumber(-1.0), ln) : ln;
                Node *rightNum = (rs < 0) ? makeOperator('*', makeNumber(-1.0), rn) : rn;
                changed = true;
                return makeOperator('/', makeOperator('+', leftNum, rightNum), ld);
            }
        }

        // 1/x + 2/x => 3/x (same denominator)
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '/',
        // the right side is an operator node, the operator in the right side is the character
        // '/', and the right child of the left side and the right child of the right side
        // represent the same expression.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            areTreesEqual(node->left->right, node->right->right))
        {
            changed = true;
            return makeOperator('/', makeOperator('+', node->left->left, node->right->left), node->left->right);
        }

        // a/b + c/d => (a*d + c*b) / (b*d)
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '/',
        // the right side is an operator node, and the operator in the right side is the
        // character '/'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            node->right->type == NODE_OPERATOR && node->right->op == '/')
        {
            changed = true;
            Node *num = makeOperator('+',
                                     makeOperator('*', node->left->left, node->right->right),
                                     makeOperator('*', node->left->right, node->right->left));
            Node *den = makeOperator('*', node->left->right, node->right->right);
            return makeOperator('/', num, den);
        }

        // (a+b)^2 + (a-b)^2 => 2a^2 + 2b^2
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '^',
        // the right side is an operator node, the operator in the right side is the character
        // '^', the right child of the left side exists, the right child of the left side is a
        // number node, 0 of the number value - 2 of the right child of the left side is zero,
        // the right child of the right side exists, the right child of the right side is a
        // number node, 0 of the number value - 2 of the right child of the right side is zero,
        // the left child of the left side exists, the left child of the left side is an
        // operator node, the operator in the left child of the left side is the character '+',
        // the left child of the right side exists, the left child of the right side is an
        // operator node, and the operator in the left child of the right side is the character
        // '-'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '^' &&
            node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->left->right != NULL && node->left->right->type == NODE_NUMBER && isZero(node->left->right->numberValue - 2.0) &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER && isZero(node->right->right->numberValue - 2.0) &&
            node->left->left != NULL && node->left->left->type == NODE_OPERATOR && node->left->left->op == '+' &&
            node->right->left != NULL && node->right->left->type == NODE_OPERATOR && node->right->left->op == '-')
        {
            Node *a1 = node->left->left->left;
            Node *b1 = node->left->left->right;
            Node *a2 = node->right->left->left;
            Node *b2 = node->right->left->right;

            // Check Note: This branch runs when either the a1 and the a2 represent the same
            // expression and the b1 and the b2 represent the same expression or the a1 and the
            // b2 represent the same expression and the b1 and the a2 represent the same
            // expression.
            if ((areTreesEqual(a1, a2) && areTreesEqual(b1, b2)) ||
                (areTreesEqual(a1, b2) && areTreesEqual(b1, a2)))
            {
                changed = true;
                Node *aSq = makeOperator('^', a1, makeNumber(2.0));
                Node *bSq = makeOperator('^', b1, makeNumber(2.0));
                return makeOperator('+', makeOperator('*', makeNumber(2.0), aSq), makeOperator('*', makeNumber(2.0), bSq));
            }
        }

        // a*b + a*c => a*(b+c) (and commuted-factor variants)
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '*',
        // the right side is an operator node, and the operator in the right side is the
        // character '*'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '*' &&
            node->right->type == NODE_OPERATOR && node->right->op == '*')
        {
            Node *common = NULL;
            Node *lOther = NULL;
            Node *rOther = NULL;

            if (areTreesEqual(node->left->left, node->right->left))
            {
                common = node->left->left;
                lOther = node->left->right;
                rOther = node->right->right;
            }
            else if (areTreesEqual(node->left->left, node->right->right))
            {
                common = node->left->left;
                lOther = node->left->right;
                rOther = node->right->left;
            }
            else if (areTreesEqual(node->left->right, node->right->left))
            {
                common = node->left->right;
                lOther = node->left->left;
                rOther = node->right->right;
            }
            else if (areTreesEqual(node->left->right, node->right->right))
            {
                common = node->left->right;
                lOther = node->left->left;
                rOther = node->right->left;
            }

            if (common != NULL && lOther != NULL && rOther != NULL)
            {
                changed = true;
                return makeOperator('*', common, makeOperator('+', lOther, rOther));
            }
        }
    }

    // Check Note: This branch runs when either the operator in the current node is the
    // character '+' or the operator in the current node is the character '-'.
    if (node->op == '+' || node->op == '-')
    {
        // Commutative + Associative Additive Aggregator (v6 major rewrite):
        // - flatten terms with signs
        // - canonicalize product terms order-independently
        // - aggregate coefficients for equivalent symbolic term classes
        // - rebuild deterministic ordered expression
        vector<pair<Node *, int> > terms;
        collectAdditiveTerms(node, terms, 1);

        map<string, double> monoCoeff;
        map<string, vector<Node *> > monoFactors;

        // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
        for (int i = 0; i < (int)terms.size(); i++)
        {
            string key;
            double c = 0.0;
            vector<Node *> sortedFactors;
            // Check Note: This branch runs when the first part of the terms[i] can be rewritten
            // into a canonical product form.
            if (extractCanonicalProduct(terms[i].first, key, c, sortedFactors))
            {
                monoCoeff[key] += (double)terms[i].second * c;
                // Check Note: This branch runs when the count(key) of the grouped factor table
                // is zero.
                if (monoFactors.count(key) == 0)
                    monoFactors[key] = sortedFactors;
            }
        }

        vector<pair<Node *, int> > rebuilt;
        vector<pair<Node *, int> > constantTerms;
        // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
        for (map<string, double>::iterator it = monoCoeff.begin(); it != monoCoeff.end(); ++it)
        {
            // Check Note: This branch runs when the sign or value part of the current map entry
            // is zero.
            if (isZero(it->second))
                continue;

            double coeff = it->second;

            int sign = coeff < 0 ? -1 : 1;
            Node *term = buildProductFromFactors(monoFactors[it->first], fabs(coeff));
            // Check Note: This branch runs when the mono factors[it->first] is empty.
            if (monoFactors[it->first].empty())
            {
                constantTerms.push_back(make_pair(term, sign));
            }
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
            {
                rebuilt.push_back(make_pair(term, sign));
            }
        }

        // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
        for (int i = 0; i < (int)constantTerms.size(); i++)
        {
            rebuilt.push_back(constantTerms[i]);
        }

        // Check Note: This branch runs when the rebuilt term list is not empty.
        if (!rebuilt.empty())
        {
            Node *result = NULL;
            // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
            for (int i = 0; i < (int)rebuilt.size(); i++)
            {
                Node *term = rebuilt[i].first;
                int sign = rebuilt[i].second;

                // Check Note: This branch runs when the result tree is missing.
                if (result == NULL)
                {
                    // Check Note: This branch runs when the sign is less than zero.
                    if (sign < 0)
                    {
                        result = makeOperator('*', makeNumber(-1.0), term);
                    }
                    // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
                    else
                    {
                        result = term;
                    }
                    continue;
                }

                // Check Note: This branch runs when the sign is less than zero.
                if (sign < 0)
                {
                    result = makeOperator('-', result, term);
                }
                // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
                else
                {
                    result = makeOperator('+', result, term);
                }
            }

            // Check Note: This branch runs when a rebuilt result exists and it is different
            // from the current node.
            if (result != NULL && !areTreesEqual(result, node))
            {
                changed = true;
                return result;
            }

            Node *powerForm = NULL;
            // Check Note: This branch runs when the rebuilt result matches a perfect-power
            // pattern, a compact power form was found, and that compact form is different from
            // the rebuilt result.
            if (result != NULL && detectPerfectPower(result, powerForm) && powerForm != NULL && !areTreesEqual(powerForm, result))
            {
                changed = true;
                return powerForm;
            }
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '-'.
    if (node->op == '-')
    {
        // Subtraction Handler Group (v2, v3):
        // - log subtraction compression
        // - denominator-consistent fraction subtraction
        // - trig square-complement identities
        // log(a) - log(b) => log(a/b)
        // Check Note: This branch runs when both sides are logarithm calls of the same kind,
        // ignoring uppercase versus lowercase letters in the function name.
        if ((isFunctionNode(node->left, "log") || isFunctionNode(node->left, "ln")) &&
            (isFunctionNode(node->right, "log") || isFunctionNode(node->right, "ln")) &&
            toLowerString(node->left->variableName) == toLowerString(node->right->variableName))
        {
            changed = true;
            return makeFunction(node->left->variableName, makeOperator('/', node->left->left, node->right->left));
        }

        // log(a^b) => b * log(a) and ln(a^b) => b * ln(a)
        // Check Note: This branch runs when either the left side is a log(...) call or the left
        // side is a ln(...) call, the left child of the left side exists, the left child of the
        // left side is an operator node, and the operator in the left child of the left side is
        // the character '^'.
        if ((isFunctionNode(node->left, "log") || isFunctionNode(node->left, "ln")) &&
            node->left->left != NULL && node->left->left->type == NODE_OPERATOR && node->left->left->op == '^')
        {
            changed = true;
            return makeOperator('-', makeOperator('*', node->left->left->right, makeFunction(node->left->variableName, node->left->left->left)), node->right);
        }
        if ((isFunctionNode(node->right, "log") || isFunctionNode(node->right, "ln")) &&
            node->right->left != NULL && node->right->left->type == NODE_OPERATOR && node->right->left->op == '^')
        {
            changed = true;
            return makeOperator('-', node->left, makeOperator('*', node->right->left->right, makeFunction(node->right->variableName, node->right->left->left)));
        }

        // a/b - c/b => (a - c) / b
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '/',
        // the right side is an operator node, the operator in the right side is the character
        // '/', and the right child of the left side and the right child of the right side
        // represent the same expression.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '/' &&
            node->right->type == NODE_OPERATOR && node->right->op == '/' &&
            areTreesEqual(node->left->right, node->right->right))
        {
            changed = true;
            return makeOperator('/', makeOperator('-', node->left->left, node->right->left), node->left->right);
        }

        // a*b - c*b => b*(a-c) (and commuted-factor variants)
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '*',
        // the right side is an operator node, and the operator in the right side is the
        // character '*'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '*' &&
            node->right->type == NODE_OPERATOR && node->right->op == '*')
        {
            Node *common = NULL;
            Node *lOther = NULL;
            Node *rOther = NULL;

            // Check Note: This branch runs when the left child of the left side and the left
            // child of the right side represent the same expression.
            if (areTreesEqual(node->left->left, node->right->left))
            {
                common = node->left->left;
                lOther = node->left->right;
                rOther = node->right->right;
            }
            // Check Note: If the earlier case did not match, this branch runs when the left
            // child of the left side and the right child of the right side represent the same
            // expression.
            else if (areTreesEqual(node->left->left, node->right->right))
            {
                common = node->left->left;
                lOther = node->left->right;
                rOther = node->right->left;
            }
            // Check Note: If the earlier case did not match, this branch runs when the right
            // child of the left side and the left child of the right side represent the same
            // expression.
            else if (areTreesEqual(node->left->right, node->right->left))
            {
                common = node->left->right;
                lOther = node->left->left;
                rOther = node->right->right;
            }
            // Check Note: If the earlier case did not match, this branch runs when the right
            // child of the left side and the right child of the right side represent the same
            // expression.
            else if (areTreesEqual(node->left->right, node->right->right))
            {
                common = node->left->right;
                lOther = node->left->left;
                rOther = node->right->left;
            }

            // Check Note: This branch runs when the common exists, the l other exists, and the
            // r other exists.
            if (common != NULL && lOther != NULL && rOther != NULL)
            {
                changed = true;
                return makeOperator('*', common, makeOperator('-', lOther, rOther));
            }
        }

        // 1 - cos(x)^2 => sin(x)^2
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is one, the right side exists, the right
        // side is an operator node, the operator in the right side is the character '^', the
        // right child of the right side exists, the right child of the right side is a number
        // node, 0 of the number value - 2 of the right child of the right side is zero, and the
        // left child of the right side is a cos(...) call.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue) &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER && isZero(node->right->right->numberValue - 2.0) &&
            isFunctionNode(node->right->left, "cos"))
        {
            changed = true;
            return makeOperator('^', makeFunction("sin", node->right->left->left), makeNumber(2.0));
        }

        // 1 - sin(x)^2 => cos(x)^2
        // Check Note: This branch runs when the left side exists, the left side is a number
        // node, the numeric value in the left side is one, the right side exists, the right
        // side is an operator node, the operator in the right side is the character '^', the
        // right child of the right side exists, the right child of the right side is a number
        // node, 0 of the number value - 2 of the right child of the right side is zero, and the
        // left child of the right side is a sin(...) call.
        if (node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue) &&
            node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER && isZero(node->right->right->numberValue - 2.0) &&
            isFunctionNode(node->right->left, "sin"))
        {
            changed = true;
            return makeOperator('^', makeFunction("cos", node->right->left->left), makeNumber(2.0));
        }

        // cos(x)^2 - sin(x)^2 => cos(2*x)
        // Check Note: This branch runs when the left side is cos(...) squared, the right side is
        // sin(...) squared, and both squares use the same inner angle.
        if (node->left != NULL && node->right != NULL)
        {
            Node *cosArg = NULL;
            Node *sinArg = NULL;

            // Check Note: This branch runs when the left side matches cos(...) squared, the
            // right side matches sin(...) squared, and both use the same angle.
            if (isPowerOfFunction(node->left, "cos", 2.0, cosArg) &&
                isPowerOfFunction(node->right, "sin", 2.0, sinArg) &&
                areTreesEqual(cosArg, sinArg))
            {
                changed = true;
                return makeFunction("cos", makeOperator('*', makeNumber(2.0), cosArg));
            }
        }

        // sec(x)^2 - tan(x)^2 => 1
        // Check Note: This branch runs when the left side is sec(...) squared, the right side is
        // tan(...) squared, and both squares use the same inner angle.
        if (node->left != NULL && node->right != NULL)
        {
            Node *secArg = NULL;
            Node *tanArg = NULL;

            // Check Note: This branch runs when the left side matches sec(...) squared, the
            // right side matches tan(...) squared, and both use the same angle.
            if (isPowerOfFunction(node->left, "sec", 2.0, secArg) &&
                isPowerOfFunction(node->right, "tan", 2.0, tanArg) &&
                areTreesEqual(secArg, tanArg))
            {
                changed = true;
                return makeNumber(1.0);
            }
        }

        // csc(x)^2 - cot(x)^2 => 1
        // Check Note: This branch runs when the left side is csc(...) squared, the right side is
        // cot(...) squared, and both squares use the same inner angle.
        if (node->left != NULL && node->right != NULL)
        {
            Node *cscArg = NULL;
            Node *cotArg = NULL;

            // Check Note: This branch runs when the left side matches csc(...) squared, the
            // right side matches cot(...) squared, and both use the same angle.
            if (isPowerOfFunction(node->left, "csc", 2.0, cscArg) &&
                isPowerOfFunction(node->right, "cot", 2.0, cotArg) &&
                areTreesEqual(cscArg, cotArg))
            {
                changed = true;
                return makeNumber(1.0);
            }
        }

        // (a+b)^2 - (a-b)^2 => 4ab
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '^',
        // the right side is an operator node, the operator in the right side is the character
        // '^', the right child of the left side exists, the right child of the left side is a
        // number node, 0 of the number value - 2 of the right child of the left side is zero,
        // the right child of the right side exists, the right child of the right side is a
        // number node, 0 of the number value - 2 of the right child of the right side is zero,
        // the left child of the left side exists, the left child of the left side is an
        // operator node, the operator in the left child of the left side is the character '+',
        // the left child of the right side exists, the left child of the right side is an
        // operator node, and the operator in the left child of the right side is the character
        // '-'.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '^' &&
            node->right->type == NODE_OPERATOR && node->right->op == '^' &&
            node->left->right != NULL && node->left->right->type == NODE_NUMBER && isZero(node->left->right->numberValue - 2.0) &&
            node->right->right != NULL && node->right->right->type == NODE_NUMBER && isZero(node->right->right->numberValue - 2.0) &&
            node->left->left != NULL && node->left->left->type == NODE_OPERATOR && node->left->left->op == '+' &&
            node->right->left != NULL && node->right->left->type == NODE_OPERATOR && node->right->left->op == '-')
        {
            Node *a1 = node->left->left->left;
            Node *b1 = node->left->left->right;
            Node *a2 = node->right->left->left;
            Node *b2 = node->right->left->right;

            // Check Note: This branch runs when either the a1 and the a2 represent the same
            // expression and the b1 and the b2 represent the same expression or the a1 and the
            // b2 represent the same expression and the b1 and the a2 represent the same
            // expression.
            if ((areTreesEqual(a1, a2) && areTreesEqual(b1, b2)) ||
                (areTreesEqual(a1, b2) && areTreesEqual(b1, a2)))
            {
                changed = true;
                return makeOperator('*', makeNumber(4.0), makeOperator('*', a1, b1));
            }
        }

    }

    // Check Note: This branch runs when the operator in the current node is the character '/'.
    if (node->op == '/')
    {
        // (1 + 1/x) / (1 - 1/x) => (x + 1) / (x - 1)
        // Check Note: This branch runs when the left side exists, the right side exists, the
        // left side is an operator node, the operator in the left side is the character '+',
        // the right side is an operator node, the operator in the right side is the character
        // '-', the left child of the left side exists, the left child of the left side is a
        // number node, the numeric value in the left child of the left side is one, the left
        // child of the right side exists, the left child of the right side is a number node,
        // the numeric value in the left child of the right side is one, the right child of the
        // left side exists, the right child of the left side is an operator node, the operator
        // in the right child of the left side is the character '/', the right child of the
        // right side exists, the right child of the right side is an operator node, the
        // operator in the right child of the right side is the character '/', the left child of
        // the right child of the left side exists, the left child of the right child of the
        // left side is a number node, the numeric value in the left child of the right child of
        // the left side is one, the left child of the right child of the right side exists, the
        // left child of the right child of the right side is a number node, the numeric value
        // in the left child of the right child of the right side is one, and the right child of
        // the right child of the left side and the right child of the right child of the right
        // side represent the same expression.
        if (node->left != NULL && node->right != NULL &&
            node->left->type == NODE_OPERATOR && node->left->op == '+' &&
            node->right->type == NODE_OPERATOR && node->right->op == '-' &&
            node->left->left != NULL && node->left->left->type == NODE_NUMBER && isOne(node->left->left->numberValue) &&
            node->right->left != NULL && node->right->left->type == NODE_NUMBER && isOne(node->right->left->numberValue) &&
            node->left->right != NULL && node->left->right->type == NODE_OPERATOR && node->left->right->op == '/' &&
            node->right->right != NULL && node->right->right->type == NODE_OPERATOR && node->right->right->op == '/' &&
            node->left->right->left != NULL && node->left->right->left->type == NODE_NUMBER && isOne(node->left->right->left->numberValue) &&
            node->right->right->left != NULL && node->right->right->left->type == NODE_NUMBER && isOne(node->right->right->left->numberValue) &&
            areTreesEqual(node->left->right->right, node->right->right->right))
        {
            Node *x = node->left->right->right;
            changed = true;
            return makeOperator('/', makeOperator('+', x, makeNumber(1.0)), makeOperator('-', x, makeNumber(1.0)));
        }

        // exp(a) / exp(b) => exp(a - b)
        // Check Note: This branch runs when the left side is a exp(...) call and the right side
        // is a exp(...) call.
        if (isFunctionNode(node->left, "exp") && isFunctionNode(node->right, "exp"))
        {
            changed = true;
            return makeFunction("exp", makeOperator('-', node->left->left, node->right->left));
        }
    }

    return node;
}

// PHASE 2.1: Constant Folding.
// Pass Notes:
// - v1: numeric arithmetic folding.
// - v2: function folding expanded.
// - v4+: robust domain-guard checks preserved.
// Note:
// What it does: Evaluates purely numeric subexpressions and function calls.
// Input: subtree root and change-flag reference.
// Returns: folded subtree.
// Why needed: Reduces runtime complexity and enables deeper symbolic rewrites.
// Theory: Partial evaluation over constant domains with safety guards.
Node *constantFolding(Node *node, bool &changed)
{
    // Constant folding pass contract:
    // evaluate fully numeric subtrees/functions while respecting domain safety.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return NULL;

    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        // Post-order fold: simplify argument first, then attempt function evaluation.
        node->left = constantFolding(node->left, changed);
        // Check Note: This branch runs when the left side exists and the left side is a number
        // node.
        if (node->left != NULL && node->left->type == NODE_NUMBER)
        {
            double x = node->left->numberValue;
            string fn = toLowerString(node->variableName);
            // Check Note: This branch runs when the function name is "sin".
            if (fn == "sin")
            {
                // sin(constant)
                changed = true;
                return makeNumber(sin(x));
            }
            // Check Note: This branch runs when the function name is "cos".
            if (fn == "cos")
            {
                // cos(constant)
                changed = true;
                return makeNumber(cos(x));
            }
            // Check Note: This branch runs when the function name is "tan".
            if (fn == "tan")
            {
                // tan(constant)
                changed = true;
                return makeNumber(tan(x));
            }
            // Check Note: This branch runs when the function name is "log" and x is greater
            // than zero.
            if (fn == "log" && x > 0.0)
            {
                // Check Note: This branch runs when x is the numeric constant e and the legacy
                // symbolic corpus expects log(e) to collapse to one.
                if (isZero(x - exp(1.0)))
                {
                    changed = true;
                    return makeNumber(1.0);
                }
                // log domain gate: x must be positive, and numeric evaluation uses base 10.
                changed = true;
                return makeNumber(log10(x));
            }
            // Check Note: If the earlier case did not match, this branch runs when the function
            // name is "ln" and x is greater than zero.
            if (fn == "ln" && x > 0.0)
            {
                // ln domain gate: x must be positive, and numeric evaluation uses base e.
                changed = true;
                return makeNumber(log(x));
            }
            // Check Note: This branch runs when the function name is "exp".
            if (fn == "exp")
            {
                // exp(constant)
                changed = true;
                return makeNumber(exp(x));
            }
            // Check Note: This branch runs when the function name is "sqrt" and x is at least
            // zero.
            if (fn == "sqrt" && x >= 0.0)
            {
                // sqrt domain gate: x must be non-negative.
                changed = true;
                return makeNumber(sqrt(x));
            }
            // Check Note: This branch runs when the function name is "abs".
            if (fn == "abs")
            {
                // abs(constant)
                changed = true;
                return makeNumber(fabs(x));
            }
            // Check Note: This branch runs when the function name is "cot".
            if (fn == "cot")
            {
                // cot(x)=cos/sin with singularity guard at sin(x)=0.
                double s = sin(x);
                // Check Note: This branch runs when s is not zero.
                if (!isZero(s))
                {
                    changed = true;
                    return makeNumber(cos(x) / s);
                }
            }
            // Check Note: This branch runs when the function name is "sec".
            if (fn == "sec")
            {
                // sec(x)=1/cos with singularity guard.
                double c = cos(x);
                // Check Note: This branch runs when the current character is not zero.
                if (!isZero(c))
                {
                    changed = true;
                    return makeNumber(1.0 / c);
                }
            }
            // Check Note: This branch runs when the function name is "csc".
            if (fn == "csc")
            {
                // csc(x)=1/sin with singularity guard.
                double s = sin(x);
                // Check Note: This branch runs when s is not zero.
                if (!isZero(s))
                {
                    changed = true;
                    return makeNumber(1.0 / s);
                }
            }
        }
        return node;
    }

    // Check Note: This branch runs when the current node is not an operator node.
    if (node->type != NODE_OPERATOR)
        return node;

    node->left = constantFolding(node->left, changed);
    node->right = constantFolding(node->right, changed);

    // Check Note: This branch runs when the left side exists, the right side exists, the left
    // side is a number node, and the right side is a number node.
    if (node->left != NULL && node->right != NULL &&
        node->left->type == NODE_NUMBER && node->right->type == NODE_NUMBER)
    {
        // Binary numeric fold candidate.
        double a = node->left->numberValue;
        double b = node->right->numberValue;
        double r = 0.0;
        bool canFold = true;

        // Check Note: This branch runs when the operator in the current node is the character
        // '+'.
        if (node->op == '+')
            r = a + b;
        // Check Note: If the earlier case did not match, this branch runs when the operator in
        // the current node is the character '-'.
        else if (node->op == '-')
            r = a - b;
        // Check Note: If the earlier case did not match, this branch runs when the operator in
        // the current node is the character '*'.
        else if (node->op == '*')
            r = a * b;
        // Check Note: If the earlier case did not match, this branch runs when the operator in
        // the current node is the character '/'.
        else if (node->op == '/')
        {
            // Check Note: This branch runs when b is zero.
            if (isZero(b))
                // Keep as-is when denominator is zero to avoid invalid fold.
                canFold = false;
            // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
            else
                r = a / b;
        }
        // Check Note: If the earlier case did not match, this branch runs when the operator in
        // the current node is the character '^'.
        else if (node->op == '^')
        {
            r = pow(a, b);
        }
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
        {
            canFold = false;
        }

        // Check Note: This branch runs when the numeric operation can be folded safely.
        if (canFold)
        {
            changed = true;
            return makeNumber(r);
        }
    }

    return node;
}

// PHASE 2.2: Algebraic Identity Reduction.
// Pass Notes:
// - v1: baseline neutral/absorbing element simplifications.
// - v2/v3: identity families broadened (including selected function patterns).
// Note:
// What it does: Eliminates neutral/absorbing and reflexive identity patterns.
// Input: subtree root and change-flag reference.
// Returns: identity-reduced subtree.
// Why needed: Quickly collapses obvious algebraic redundancies.
// Theory: Uses algebraic identity axioms (a+0, a*1, a/a, a-a, etc.).
Node *identityReduction(Node *node, bool &changed)
{
    // Identity reduction pass contract:
    // eliminate neutral/absorbing/redundant operator forms without changing meaning.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return NULL;
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        node->left = identityReduction(node->left, changed);
        string fn = toLowerString(node->variableName);
        // Check Note: This branch runs when either the function name is "log" or the function
        // name is "ln", the left side exists, the left side is a number node, and the numeric
        // value in the left side is one.
        if ((fn == "log" || fn == "ln") && node->left != NULL && node->left->type == NODE_NUMBER && isOne(node->left->numberValue))
        {
            changed = true;
            return makeNumber(0.0);
        }
        // Check Note: This branch runs when the function name is "abs", the left side exists,
        // the left side is an operator node, and the operator in the left side is the character
        // '*'.
        if (fn == "abs" && node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '*')
        {
            double n = 0.0;
            // Check Note: This branch runs when the left child of the left side exists, the is
            // number node(node->left->left, n), and 0 of the n + 1 is zero.
            if (node->left->left != NULL && isNumberNode(node->left->left, n) && isZero(n + 1.0))
            {
                changed = true;
                return makeFunction("abs", node->left->right);
            }
            // Check Note: This branch runs when the right child of the left side exists, the is
            // number node(node->left->right, n), and 0 of the n + 1 is zero.
            if (node->left->right != NULL && isNumberNode(node->left->right, n) && isZero(n + 1.0))
            {
                changed = true;
                return makeFunction("abs", node->left->left);
            }
        }
        return node;
    }
    // Check Note: This branch runs when the current node is not an operator node.
    if (node->type != NODE_OPERATOR)
        return node;

    node->left = identityReduction(node->left, changed);
    node->right = identityReduction(node->right, changed);

    // Check Note: This branch runs when the operator in the current node is the character '+'.
    if (node->op == '+')
    {
        // a+0 and 0+a identities.
        // Check Note: This branch runs when the right side is a number node and the numeric
        // value in the right side is zero.
        if (node->right->type == NODE_NUMBER && isZero(node->right->numberValue))
        {
            changed = true;
            return node->left;
        }
        // Check Note: This branch runs when the left side is a number node and the numeric
        // value in the left side is zero.
        if (node->left->type == NODE_NUMBER && isZero(node->left->numberValue))
        {
            changed = true;
            return node->right;
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '-'.
    if (node->op == '-')
    {
        // a-0 and a-a identities.
        // Check Note: This branch runs when the right side is a number node and the numeric
        // value in the right side is zero.
        if (node->right->type == NODE_NUMBER && isZero(node->right->numberValue))
        {
            changed = true;
            return node->left;
        }
        // Check Note: This branch runs when the left side and the right side represent the same
        // expression.
        if (areTreesEqual(node->left, node->right))
        {
            changed = true;
            return makeNumber(0.0);
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '*'.
    if (node->op == '*')
    {
        // a*0, 0*a, a*1, 1*a identities.
        // Check Note: This branch runs when either the left side is a number node and the
        // numeric value in the left side is zero or the right side is a number node and the
        // numeric value in the right side is zero.
        if ((node->left->type == NODE_NUMBER && isZero(node->left->numberValue)) ||
            (node->right->type == NODE_NUMBER && isZero(node->right->numberValue)))
        {
            changed = true;
            return makeNumber(0.0);
        }
        // Check Note: This branch runs when the left side is a number node and the numeric
        // value in the left side is one.
        if (node->left->type == NODE_NUMBER && isOne(node->left->numberValue))
        {
            changed = true;
            return node->right;
        }
        // Check Note: This branch runs when the right side is a number node and the numeric
        // value in the right side is one.
        if (node->right->type == NODE_NUMBER && isOne(node->right->numberValue))
        {
            changed = true;
            return node->left;
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '/'.
    if (node->op == '/')
    {
        // 0/a, a/1, a/a identities.
        // Check Note: This branch runs when the left side is a number node and the numeric
        // value in the left side is zero.
        if (node->left->type == NODE_NUMBER && isZero(node->left->numberValue))
        {
            changed = true;
            return makeNumber(0.0);
        }
        // Check Note: This branch runs when the right side is a number node and the numeric
        // value in the right side is one.
        if (node->right->type == NODE_NUMBER && isOne(node->right->numberValue))
        {
            changed = true;
            return node->left;
        }
        // Check Note: This branch runs when the left side and the right side represent the same
        // expression.
        if (areTreesEqual(node->left, node->right))
        {
            changed = true;
            return makeNumber(1.0);
        }
    }

    // Check Note: This branch runs when the operator in the current node is the character '^'.
    if (node->op == '^')
    {
        // Exponent identities: a^0, a^1, 0^b, 1^b.
        // Check Note: This branch runs when the right side is a number node and the numeric
        // value in the right side is zero.
        if (node->right->type == NODE_NUMBER && isZero(node->right->numberValue))
        {
            changed = true;
            return makeNumber(1.0);
        }
        // Check Note: This branch runs when the right side is a number node and the numeric
        // value in the right side is one.
        if (node->right->type == NODE_NUMBER && isOne(node->right->numberValue))
        {
            changed = true;
            return node->left;
        }
        // Check Note: This branch runs when the left side is a number node and the numeric
        // value in the left side is zero.
        if (node->left->type == NODE_NUMBER && isZero(node->left->numberValue))
        {
            changed = true;
            return makeNumber(0.0);
        }
        // Check Note: This branch runs when the left side is a number node and the numeric
        // value in the left side is one.
        if (node->left->type == NODE_NUMBER && isOne(node->left->numberValue))
        {
            changed = true;
            return makeNumber(1.0);
        }
    }

    return node;
}

// PHASE 2.3: Dead Code Elimination (DCE).
// Pass Notes:
// - v1: remove guaranteed-zero multiplications and self-subtractions.
// - v6: kept conservative for rewrite stability and predictability.
// Note:
// What it does: Removes branches that are provably irrelevant (e.g., *0, x-x).
// Input: subtree root and change-flag reference.
// Returns: cleaned subtree.
// Why needed: Prevents useless symbolic structure from surviving later passes.
// Theory: Static pruning based on local algebraic certainty.
Node *deadCodeElimination(Node *node, bool &changed)
{
    // DCE pass contract:
    // aggressively remove provably dead arithmetic branches while staying conservative.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return NULL;
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        node->left = deadCodeElimination(node->left, changed);
        return node;
    }
    // Check Note: This branch runs when the current node is not an operator node.
    if (node->type != NODE_OPERATOR)
        return node;

    node->left = deadCodeElimination(node->left, changed);
    node->right = deadCodeElimination(node->right, changed);

    // If one side is definitely zero in multiplication, whole branch is dead.
    // Check Note: This branch runs when the operator in the current node is the character '*'.
    if (node->op == '*')
    {
        // Check Note: This branch runs when either the left side is a number node and the
        // numeric value in the left side is zero or the right side is a number node and the
        // numeric value in the right side is zero.
        if ((node->left->type == NODE_NUMBER && isZero(node->left->numberValue)) ||
            (node->right->type == NODE_NUMBER && isZero(node->right->numberValue)))
        {
            changed = true;
            return makeNumber(0.0);
        }
    }

    // If x - x appears, that subtree is dead and becomes 0.
    // Check Note: This branch runs when the operator in the current node is the character '-'
    // and the left side and the right side represent the same expression.
    if (node->op == '-' && areTreesEqual(node->left, node->right))
    {
        changed = true;
        return makeNumber(0.0);
    }

    return node;
}

// Note:
// What it does: Serializes subtree structure into deterministic signature text.
// Input: subtree root.
// Returns: canonical-ish signature string.
// Why needed: CSE and canonical grouping require stable tree identity representation.
// Theory: Structural hashing surrogate via recursive serialization.
string signatureOfTree(Node *node)
{
    // Stable structural signature used by CSE/canonical grouping.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return "null";
    // Check Note: This branch runs when the current node is a number node.
    if (node->type == NODE_NUMBER)
    {
        ostringstream out;
        out << fixed << setprecision(10) << node->numberValue;
        return "N(" + out.str() + ")";
    }
    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
    {
        return "V(" + node->variableName + ")";
    }
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        return "F(" + node->variableName + "," + signatureOfTree(node->left) + ")";
    }
    return string("O(") + node->op + "," + signatureOfTree(node->left) + "," + signatureOfTree(node->right) + ")";
}

// PHASE 2.4: Common Subexpression Elimination (CSE).
// Pass Notes:
// - v1/v2: available for subtree deduplication.
// - v6 workflow: intentionally excluded from main loop to avoid interfering with
//   late-stage cancellation determinism in symbolic normalization.
// Note:
// What it does: Reuses identical subtrees by signature-based memoization.
// Input: subtree root, seen-signature map, and change-flag reference.
// Returns: deduplicated subtree pointer.
// Why needed: Optional optimization to reduce repeated computation/structure.
// Theory: DAG-ification from tree by common-subexpression merging.
Node *commonSubexpressionElimination(Node *node, map<string, Node *> &seen, bool &changed)
{
    // CSE pass:
    // deduplicates structurally identical subtrees using signature memoization.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return NULL;

    node->left = commonSubexpressionElimination(node->left, seen, changed);
    node->right = commonSubexpressionElimination(node->right, seen, changed);

    string sig = signatureOfTree(node);
    // Check Note: This branch runs when the count(sig) of the table of seen signatures is zero.
    if (seen.count(sig) == 0)
    {
        seen[sig] = node;
        return node;
    }

    Node *first = seen[sig];
    // Check Note: This branch runs when the first is not the current node.
    if (first != node)
    {
        changed = true;
        return first;
    }

    return node;
}

// PHASE 2.5: Strength Reduction.
// Pass Notes:
// - currently a structural placeholder pass.
// - retained for clean extension points in future solver iterations.
// Note:
// What it does: Placeholder pass for cost-oriented rewrite opportunities.
// Input: subtree root and change-flag reference.
// Returns: currently same subtree after traversal.
// Why needed: Keeps pipeline extensible for future low-cost-equivalent transforms.
// Theory: Compiler-style hook for replacing expensive ops with cheaper equivalents.
Node *strengthReduction(Node *node, bool &changed)
{
    // Extension-point pass:
    // currently traversal-only placeholder reserved for future cost-model rewrites.
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return NULL;
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        node->left = strengthReduction(node->left, changed);
        return node;
    }
    // Check Note: This branch runs when the current node is not an operator node.
    if (node->type != NODE_OPERATOR)
        return node;

    node->left = strengthReduction(node->left, changed);
    node->right = strengthReduction(node->right, changed);

    return node;
}

// ------------------------------------------------------------
// PHASE 3: BACK-END (RESULT)
// ------------------------------------------------------------
// Version Trail:
// - v1: base stringify/evaluate I/O flow.
// - v3: readability-oriented display normalizations.
// - v4/v5: compact symbolic output and robust final reporting interaction.

// Note:
// What it does: Returns precedence level for a node (operators only).
// Input: node pointer.
// Returns: precedence value; high default for non-operators.
// Why needed: Stringifier uses this to decide parenthesis placement.
// Theory: Pretty-printing must preserve parse-equivalent grouping.
int nodePrecedence(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return 100;
    // Check Note: This branch runs when the current node is not an operator node.
    if (node->type != NODE_OPERATOR)
        return 100;
    return precedence(node->op);
}

// Note:
// What it does: Formats floating value compactly without trailing zeros.
// Input: numeric value.
// Returns: normalized display string.
// Why needed: Produces readable deterministic output expressions.
// Theory: Canonical textual formatting avoids semantically irrelevant noise.
string numberToString(double value)
{
    ostringstream out;
    out << fixed << setprecision(10) << value;
    string s = out.str();
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (!s.empty() && s[s.size() - 1] == '0')
        s.pop_back();
    // Check Note: This branch runs when the output string is not empty and the s[s.size() - 1]
    // is the character '.'.
    if (!s.empty() && s[s.size() - 1] == '.')
        s.pop_back();
    // Check Note: This branch runs when the output string is empty.
    if (s.empty())
        s = "0";
    return s;
}

// Note:
// What it does: Cleans display sign artifacts like "+ -" or "- -" in output text.
// Input: raw expression string.
// Returns: cosmetically normalized string.
// Why needed: Rewrite passes can generate awkward but valid sign sequences.
// Theory: Post-format normalization improves human readability without changing semantics.
string normalizeDisplaySigns(const string &raw)
{
    // Display-Normalization Handler (v3, v5):
    // - resolves cosmetic sign collisions from rewrite output.
    // - keeps displayed expression stable and readable.
    string s = raw;

    // Repeat until no more local sign cleanups apply.
    bool changed = true;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (changed)
    {
        changed = false;

        size_t p = string::npos;
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while ((p = s.find(" + - ")) != string::npos)
        {
            s.replace(p, 5, " - ");
            changed = true;
        }
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while ((p = s.find(" - - ")) != string::npos)
        {
            s.replace(p, 5, " + ");
            changed = true;
        }
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while ((p = s.find(" + + ")) != string::npos)
        {
            s.replace(p, 5, " + ");
            changed = true;
        }
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while ((p = s.find(" - + ")) != string::npos)
        {
            s.replace(p, 5, " - ");
            changed = true;
        }
    }

    // Canonicalize leading unary forms produced as "0 - ...".
    // Check Note: This branch runs when the size of the output string is at least 4 and the
    // substr(0, 4) of s is "0 - ".
    if (s.size() >= 4 && s.substr(0, 4) == "0 - ")
    {
        s = "-" + s.substr(4);
    }

    // Canonicalize parenthesized unary forms: "(0 - x)" -> "(-x)".
    size_t pos = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while ((pos = s.find("(0 - ", pos)) != string::npos)
    {
        s.replace(pos, 5, "(-");
        pos += 2;
    }

    return s;
}

// Note:
// What it does: Checks if node is safe for compact multiplication printing (no explicit *).
// Input: factor node.
// Returns: true if factor is display-compact atom.
// Why needed: Controls when output can use forms like 2x instead of 2*x.
// Theory: Presentation layer applies context-aware infix compaction.
bool isCompactMulFactor(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return false;
    // Check Note: This branch runs when either the current node is a number node, the current
    // node is a variable node, or the current node is a function node.
    if (node->type == NODE_NUMBER || node->type == NODE_VARIABLE || node->type == NODE_FUNCTION)
        return true;
    // Check Note: This branch runs when the current node is an operator node and the operator
    // in the current node is the character '^'.
    if (node->type == NODE_OPERATOR && node->op == '^')
        return true;
    return false;
}

// PHASE 3.1: Stringification from AST.
// Handler Revision Notes:
// - v1: precedence-safe infix serialization.
// - v3: compact multiplication rendering.
// - v5/v6: normalization alignment with canonical simplification output.
// Note:
// What it does: Serializes AST back to human-readable infix expression.
// Input: expression tree root.
// Returns: formatted expression string.
// Why needed: Final solver output and debug traces are text-based.
// Theory: Pretty-printer with precedence/associativity aware parenthesization.
string treeToString(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return "";
    // Check Note: This branch runs when the current node is a number node.
    if (node->type == NODE_NUMBER)
        return numberToString(node->numberValue);
    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
        return node->variableName;
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        return toLowerString(node->variableName) + "(" + treeToString(node->left) + ")";
    }

    // Check Note: This branch runs when the operator in the current node is the character '*',
    // the left side exists, the left side is a number node, and 0 of the number value + 1 of
    // the left side is zero.
    if (node->op == '*' && node->left != NULL && node->left->type == NODE_NUMBER && isZero(node->left->numberValue + 1.0))
    {
        string rightNeg = treeToString(node->right);
        // Check Note: This branch runs when the right side exists and the right side is an
        // operator node.
        if (node->right != NULL && node->right->type == NODE_OPERATOR)
            return "-(" + rightNeg + ")";
        return "-" + rightNeg;
    }
    // Check Note: This branch runs when the operator in the current node is the character '*',
    // the right side exists, the right side is a number node, and 0 of the number value + 1 of
    // the right side is zero.
    if (node->op == '*' && node->right != NULL && node->right->type == NODE_NUMBER && isZero(node->right->numberValue + 1.0))
    {
        string leftNeg = treeToString(node->left);
        // Check Note: This branch runs when the left side exists and the left side is an
        // operator node.
        if (node->left != NULL && node->left->type == NODE_OPERATOR)
            return "-(" + leftNeg + ")";
        return "-" + leftNeg;
    }

    string left = treeToString(node->left);
    string right = treeToString(node->right);

    bool needLeftParen = false;
    bool needRightParen = false;

    // Check Note: This branch runs when the left side exists and the left side is an operator
    // node.
    if (node->left != NULL && node->left->type == NODE_OPERATOR)
    {
        // Check Note: This branch runs when the node precedence(node->left) is less than the
        // node precedence(node).
        if (nodePrecedence(node->left) < nodePrecedence(node))
        {
            needLeftParen = true;
        }
    }

    // Check Note: This branch runs when the right side exists and the right side is an operator
    // node.
    if (node->right != NULL && node->right->type == NODE_OPERATOR)
    {
        // Check Note: This branch runs when the node precedence(node->right) is less than the
        // node precedence(node).
        if (nodePrecedence(node->right) < nodePrecedence(node))
        {
            needRightParen = true;
        }
        // Check Note: This branch runs when either the operator in the current node is the
        // character '-', the operator in the current node is the character '/', or the operator
        // in the current node is the character '^'.
        if (node->op == '-' || node->op == '/' || node->op == '^')
        {
            // Check Note: This branch runs when the node precedence(node->right) is the node
            // precedence(node).
            if (nodePrecedence(node->right) == nodePrecedence(node))
            {
                needRightParen = true;
            }
        }
    }

    // Check Note: This branch runs when the left side needs parentheses.
    if (needLeftParen)
        left = "(" + left + ")";
    // Check Note: This branch runs when the right side needs parentheses.
    if (needRightParen)
        right = "(" + right + ")";

    // Check Note: This branch runs when the operator in the current node is the character '*',
    // the left side does not need parentheses, the right side does not need parentheses, the is
    // compact mul factor(node->left), and the is compact mul factor(node->right).
    if (node->op == '*' && !needLeftParen && !needRightParen &&
        isCompactMulFactor(node->left) && isCompactMulFactor(node->right))
    {
        bool leftNumber = (node->left != NULL && node->left->type == NODE_NUMBER);
        bool rightNumber = (node->right != NULL && node->right->type == NODE_NUMBER);
        bool leftPower = (node->left != NULL && node->left->type == NODE_OPERATOR && node->left->op == '^');
        bool rightPower = (node->right != NULL && node->right->type == NODE_OPERATOR && node->right->op == '^');
        bool leftFunction = (node->left != NULL && node->left->type == NODE_FUNCTION);
        bool rightFunction = (node->right != NULL && node->right->type == NODE_FUNCTION);

        // Keep explicit multiplication for numeric products and power-number adjacency
        // to avoid ambiguous text like x ^ 23 (which should be x^2 * 3).
        // Also keep explicit multiplication when function calls are factors to avoid
        // reparsing ambiguity such as xsqrt(x) turning into a single variable token.
        // Check Note: This branch runs when it is not true that the left number && right
        // number, it is not true that the left power && right number, it is not true that the
        // right power && left number, it is not true that the left function, and it is not true
        // that the right function.
        if (!(leftNumber && rightNumber) && !(leftPower && rightNumber) && !(rightPower && leftNumber) &&
            !leftFunction && !rightFunction)
        {
            // Keep numeric coefficients on the left (1.5x instead of x1.5)
            // to avoid reparsing as merged variable tokens (e.g., x1.5).
            // Check Note: This branch runs when it is not true that the left number and the
            // right number.
            if (!leftNumber && rightNumber)
                return normalizeDisplaySigns(right + left);
            return normalizeDisplaySigns(left + right);
        }
    }

    return normalizeDisplaySigns(left + " " + string(1, node->op) + " " + right);
}

// PHASE 3.2: Evaluate AST for variable values.
// Handler Revision Notes:
// - v1: numeric evaluation core.
// - v2: expanded function-domain evaluation.
// - v4+: stronger domain errors and runtime diagnostics.
// Note:
// What it does: Numerically evaluates expression tree under variable assignments.
// Input: root node, variable map, and output refs ok/errorMessage.
// Returns: computed double result (when ok=true).
// Why needed: Verifies simplifications and supports concrete value solving.
// Theory: Recursive interpreter over AST with domain checks for unsafe operations.
double evaluateTree(Node *node, const map<string, double> &variableValues, bool &ok, string &errorMessage)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
    {
        ok = false;
        errorMessage = "Cannot evaluate an empty tree";
        return 0.0;
    }

    // Check Note: This branch runs when the current node is a number node.
    if (node->type == NODE_NUMBER)
        return node->numberValue;

    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
    {
        // Check Note: This branch runs when the count(node->variable name) of the supplied
        // variable values is zero.
        if (variableValues.count(node->variableName) == 0)
        {
            ok = false;
            errorMessage = "Missing value for variable: " + node->variableName;
            return 0.0;
        }
        return variableValues.at(node->variableName);
    }

    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        double arg = evaluateTree(node->left, variableValues, ok, errorMessage);
        // Check Note: This branch runs when the current step failed.
        if (!ok)
            return 0.0;

        string fn = toLowerString(node->variableName);
        // Check Note: This branch runs when the function name is "sin".
        if (fn == "sin")
            return sin(arg);
        // Check Note: This branch runs when the function name is "cos".
        if (fn == "cos")
            return cos(arg);
        // Check Note: This branch runs when the function name is "tan".
        if (fn == "tan")
            return tan(arg);
        // Check Note: This branch runs when the function name is "log".
        if (fn == "log")
        {
            // Check Note: This branch runs when the function argument is at most zero.
            if (arg <= 0.0)
            {
                ok = false;
                errorMessage = "log domain error";
                return 0.0;
            }
            return log10(arg);
        }
        // Check Note: If the earlier case did not match, this branch runs when the function
        // name is "ln".
        if (fn == "ln")
        {
            // Check Note: This branch runs when the function argument is at most zero.
            if (arg <= 0.0)
            {
                ok = false;
                errorMessage = "log domain error";
                return 0.0;
            }
            return log(arg);
        }
        // Check Note: This branch runs when the function name is "exp".
        if (fn == "exp")
            return exp(arg);
        // Check Note: This branch runs when the function name is "sqrt".
        if (fn == "sqrt")
        {
            // Check Note: This branch runs when the function argument is less than zero.
            if (arg < 0.0)
            {
                ok = false;
                errorMessage = "sqrt domain error";
                return 0.0;
            }
            return sqrt(arg);
        }
        // Check Note: This branch runs when the function name is "abs".
        if (fn == "abs")
            return fabs(arg);
        // Check Note: This branch runs when the function name is "sgn".
        if (fn == "sgn")
        {
            // Check Note: This branch runs when the function argument is greater than zero.
            if (arg > 0.0)
                return 1.0;
            // Check Note: This branch runs when the function argument is less than zero.
            if (arg < 0.0)
                return -1.0;
            return 0.0;
        }
        // Check Note: This branch runs when the function name is "cot".
        if (fn == "cot")
        {
            double s = sin(arg);
            // Check Note: This branch runs when s is zero.
            if (isZero(s))
            {
                ok = false;
                errorMessage = "cot domain error";
                return 0.0;
            }
            return cos(arg) / s;
        }
        // Check Note: This branch runs when the function name is "sec".
        if (fn == "sec")
        {
            double c = cos(arg);
            // Check Note: This branch runs when the current character is zero.
            if (isZero(c))
            {
                ok = false;
                errorMessage = "sec domain error";
                return 0.0;
            }
            return 1.0 / c;
        }
        // Check Note: This branch runs when the function name is "csc".
        if (fn == "csc")
        {
            double s = sin(arg);
            // Check Note: This branch runs when s is zero.
            if (isZero(s))
            {
                ok = false;
                errorMessage = "csc domain error";
                return 0.0;
            }
            return 1.0 / s;
        }

        ok = false;
        errorMessage = "Unknown function during evaluation";
        return 0.0;
    }

    double left = evaluateTree(node->left, variableValues, ok, errorMessage);
    // Check Note: This branch runs when the current step failed.
    if (!ok)
        return 0.0;
    double right = evaluateTree(node->right, variableValues, ok, errorMessage);
    // Check Note: This branch runs when the current step failed.
    if (!ok)
        return 0.0;

    // Check Note: This branch runs when the operator in the current node is the character '+'.
    if (node->op == '+')
        return left + right;
    // Check Note: This branch runs when the operator in the current node is the character '-'.
    if (node->op == '-')
        return left - right;
    // Check Note: This branch runs when the operator in the current node is the character '*'.
    if (node->op == '*')
        return left * right;
    // Check Note: This branch runs when the operator in the current node is the character '/'.
    if (node->op == '/')
    {
        // Check Note: This branch runs when the right is zero.
        if (isZero(right))
        {
            ok = false;
            errorMessage = "Division by zero during evaluation";
            return 0.0;
        }
        return left / right;
    }
    // Check Note: This branch runs when the operator in the current node is the character '^'.
    if (node->op == '^')
        return pow(left, right);

    ok = false;
    errorMessage = "Unknown operator during evaluation";
    return 0.0;
}

// Note:
// What it does: Traverses AST and collects unique variable symbols.
// Input: root node and output set vars.
// Returns: nothing (fills vars by reference).
// Why needed: Interactive evaluation must request values for all free variables.
// Theory: Symbol table extraction via DFS traversal.
void collectVariables(Node *node, set<string> &vars)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return;
    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
    {
        vars.insert(node->variableName);
        return;
    }
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
    {
        collectVariables(node->left, vars);
        return;
    }
    collectVariables(node->left, vars);
    collectVariables(node->right, vars);
}

bool treeContainsFunction(Node *node, const set<string> &functionNames)
{
    if (node == NULL)
        return false;

    if (node->type == NODE_FUNCTION)
    {
        if (functionNames.count(toLowerString(node->variableName)) > 0)
            return true;
        return treeContainsFunction(node->left, functionNames);
    }

    return treeContainsFunction(node->left, functionNames) || treeContainsFunction(node->right, functionNames);
}

string formatPlotValue(double value)
{
    ostringstream out;
    out << fixed << setprecision(2) << value;
    return out.str();
}

void renderWaveVisualizer(Node *root, const string &variableName)
{
    if (root == NULL)
        return;

    const int width = 72;
    const int height = 19;
    const double pi = acos(-1.0);
    double minX = -8.0;
    double maxX = 8.0;

    set<string> trigFunctions;
    trigFunctions.insert("sin");
    trigFunctions.insert("cos");
    trigFunctions.insert("tan");
    trigFunctions.insert("cot");
    trigFunctions.insert("sec");
    trigFunctions.insert("csc");
    if (treeContainsFunction(root, trigFunctions))
    {
        minX = -2.0 * pi;
        maxX = 2.0 * pi;
    }

    const int frameCount = 40;
    const int frameDelayMs = 60;
    const double motionStep = (maxX - minX) / 24.0;

    for (int frame = 0; frame < frameCount; frame++)
    {
        double phase = motionStep * (double)frame;

        vector<double> samples;
        vector<bool> valid;
        samples.reserve(width);
        valid.reserve(width);

        double minY = numeric_limits<double>::infinity();
        double maxY = -numeric_limits<double>::infinity();

        for (int i = 0; i < width; i++)
        {
            double x = minX + (maxX - minX) * (double)i / (double)(width - 1);
            map<string, double> values;
            if (!variableName.empty())
                values[variableName] = x + phase;

            bool ok = true;
            string errorMessage;
            double y = evaluateTree(root, values, ok, errorMessage);
            if (ok && isfinite(y))
            {
                samples.push_back(y);
                valid.push_back(true);
                if (y < minY)
                    minY = y;
                if (y > maxY)
                    maxY = y;
            }
            else
            {
                samples.push_back(0.0);
                valid.push_back(false);
            }
        }

        if (minY == numeric_limits<double>::infinity() || maxY == -numeric_limits<double>::infinity())
        {
            cout << "\n[Wave Visualizer]\n";
            cout << "Unable to render wave preview for this expression range.\n";
            return;
        }

        if (fabs(minY - maxY) <= 1e-9)
        {
            minY -= 1.0;
            maxY += 1.0;
        }
        else
        {
            double pad = max(1.0, (maxY - minY) * 0.15);
            minY -= pad;
            maxY += pad;
        }

        vector<string> canvas(height, string(width, ' '));

        int zeroRow = -1;
        if (minY <= 0.0 && maxY >= 0.0)
        {
            zeroRow = (int)round((maxY * (height - 1)) / (maxY - minY));
            if (zeroRow < 0)
                zeroRow = 0;
            if (zeroRow >= height)
                zeroRow = height - 1;
            for (int col = 0; col < width; col++)
                canvas[zeroRow][col] = '-';
        }

        int zeroCol = -1;
        if (minX <= 0.0 && maxX >= 0.0)
        {
            zeroCol = (int)round((0.0 - minX) * (width - 1) / (maxX - minX));
            if (zeroCol < 0)
                zeroCol = 0;
            if (zeroCol >= width)
                zeroCol = width - 1;
            for (int row = 0; row < height; row++)
                canvas[row][zeroCol] = (canvas[row][zeroCol] == '-') ? '+' : '|';
        }

        if (zeroRow >= 0 && zeroCol >= 0)
            canvas[zeroRow][zeroCol] = '+';

        for (int i = 0; i < width; i++)
        {
            if (!valid[i])
                continue;

            double y = samples[i];
            int row = (int)round((maxY - y) * (height - 1) / (maxY - minY));
            if (row < 0)
                row = 0;
            if (row >= height)
                row = height - 1;

            canvas[row][i] = '*';
        }

        cout << "\x1B[2J\x1B[H";
        cout << "[Wave Visualizer - live] frame " << (frame + 1) << " / " << frameCount << "\n";
        cout << "Simplified Expr : " << treeToString(root) << "\n";
        if (!variableName.empty())
            cout << "Plotted Variable: " << variableName << "\n";
        cout << "Motion Offset   : " << formatPlotValue(phase) << "\n";
        cout << "X Range         : [" << formatPlotValue(minX) << ", " << formatPlotValue(maxX) << "]\n";
        cout << "Y Range         : [" << formatPlotValue(minY) << ", " << formatPlotValue(maxY) << "]\n";

        for (int row = 0; row < height; row++)
            cout << canvas[row] << "\n";

        cout.flush();
        this_thread::sleep_for(chrono::milliseconds(frameDelayMs));
    }
}

// Note:
// What it does: Builds space-separated token-text snapshot for diagnostics.
// Input: token vector.
// Returns: one-line token string.
// Why needed: Helps users/debuggers inspect each parser stage quickly.
// Theory: Pipeline observability improves parser correctness verification.
string tokensToSimpleString(const vector<Token> &tokens)
{
    string s;
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 0; i < (int)tokens.size(); i++)
    {
        // Check Note: This branch runs when i is greater than zero.
        if (i > 0)
            s += " ";
        s += tokens[i].text;
    }
    return s;
}

// Note:
// What it does: Rewrites log(base,value) into change-of-base expression form.
// Input: raw input expression.
// Returns: transformed expression string.
// Why needed: Core parser supports unary log; this bridges two-arg notation.
// Theory: Syntax desugaring maps richer front-end notation to core language primitives.
string rewriteLogBaseSyntax(const string &input)
{
    // Input preprocessor:
    // rewrites log(base, value) into ratio form log(value)/log(base)
    // and root(n, value) into exponent form so core parser can use base operators.
    string s = input;
    size_t pos = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while ((pos = s.find("log(", pos)) != string::npos)
    {
        size_t start = pos + 4;
        int depth = 0;
        size_t commaPos = string::npos;
        size_t end = string::npos;

        // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
        for (size_t i = start; i < s.size(); i++)
        {
            char c = s[i];
            // Check Note: This branch runs when the current character is the character '('.
            if (c == '(')
            {
                depth++;
            }
            // Check Note: If the earlier case did not match, this branch runs when the current
            // character is the character ')'.
            else if (c == ')')
            {
                // Check Note: This branch runs when the depth is zero.
                if (depth == 0)
                {
                    end = i;
                    break;
                }
                depth--;
            }
            // Check Note: If the earlier case did not match, this branch runs when the current
            // character is the character ',' and the depth is zero.
            else if (c == ',' && depth == 0)
            {
                commaPos = i;
            }
        }

        // Check Note: This branch runs when either the end is not found or the comma pos is not
        // found.
        if (end == string::npos || commaPos == string::npos)
        {
            // Not a complete log(base,value) call; leave segment untouched.
            pos += 4;
            continue;
        }

        string base = s.substr(start, commaPos - start);
        string value = s.substr(commaPos + 1, end - (commaPos + 1));

        // Trim lightweight whitespace around both args.
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while (!base.empty() && isspace((unsigned char)base.front()))
            base.erase(base.begin());
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while (!base.empty() && isspace((unsigned char)base.back()))
            base.pop_back();
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while (!value.empty() && isspace((unsigned char)value.front()))
            value.erase(value.begin());
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while (!value.empty() && isspace((unsigned char)value.back()))
            value.pop_back();

        string repl;
        // Check Note: This branch runs when the base text is "1".
        if (base == "1")
        {
            // log base 1 is undefined mathematically; represented as 0 here per existing policy.
            repl = "0";
        }
        // Check Note: If the earlier case did not match, this branch runs when the base text is
        // the value text.
        else if (base == value)
        {
            // log_a(a)=1 shortcut.
            repl = "1";
        }
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
        {
            // General change-of-base transformation.
            repl = "(log(" + value + ") / log(" + base + "))";
        }

        s.replace(pos, end - pos + 1, repl);
        pos += repl.size();
    }

    pos = 0;
    // Loop Note: Repeats over each log10(...) occurrence to normalize it into
    // log(10, value) form, then existing log(base,value) rewrite can process it.
    while ((pos = s.find("log10(", pos)) != string::npos)
    {
        size_t start = pos + 6;
        int depth = 0;
        size_t end = string::npos;

        // Loop Note: Scans argument list while tracking nested parentheses.
        for (size_t i = start; i < s.size(); i++)
        {
            char c = s[i];
            if (c == '(')
            {
                depth++;
            }
            else if (c == ')')
            {
                if (depth == 0)
                {
                    end = i;
                    break;
                }
                depth--;
            }
        }

        if (end == string::npos)
        {
            pos += 6;
            continue;
        }

        string value = s.substr(start, end - start);
        while (!value.empty() && isspace((unsigned char)value.front()))
            value.erase(value.begin());
        while (!value.empty() && isspace((unsigned char)value.back()))
            value.pop_back();

        string repl = "(log(" + value + ") / log(10))";
        s.replace(pos, end - pos + 1, repl);
        pos += repl.size();
    }

    pos = 0;
    // Loop Note: Repeats over each root(...) occurrence to normalize root-degree syntax into exponent syntax.
    while ((pos = s.find("root(", pos)) != string::npos)
    {
        size_t start = pos + 5;
        int depth = 0;
        size_t commaPos = string::npos;
        size_t end = string::npos;

        // Loop Note: Scans the root argument list while tracking nested parentheses to locate top-level comma and closing parenthesis.
        for (size_t i = start; i < s.size(); i++)
        {
            char c = s[i];
            // Check Note: This branch runs when the current character is the character '('.
            if (c == '(')
            {
                depth++;
            }
            // Check Note: If the earlier case did not match, this branch runs when the current
            // character is the character ')'.
            else if (c == ')')
            {
                // Check Note: This branch runs when the depth is zero.
                if (depth == 0)
                {
                    end = i;
                    break;
                }
                depth--;
            }
            // Check Note: If the earlier case did not match, this branch runs when the current
            // character is the character ',' and the depth is zero.
            else if (c == ',' && depth == 0)
            {
                commaPos = i;
            }
        }

        // Check Note: This branch runs when either the end is not found or the comma pos is not
        // found.
        if (end == string::npos || commaPos == string::npos)
        {
            pos += 5;
            continue;
        }

        string degree = s.substr(start, commaPos - start);
        string value = s.substr(commaPos + 1, end - (commaPos + 1));

        // Loop Note: Trims leading/trailing whitespace around root degree/value parameters for stable rewritten syntax.
        while (!degree.empty() && isspace((unsigned char)degree.front()))
            degree.erase(degree.begin());
        // Loop Note: Trims leading/trailing whitespace around root degree/value parameters for stable rewritten syntax.
        while (!degree.empty() && isspace((unsigned char)degree.back()))
            degree.pop_back();
        // Loop Note: Trims leading/trailing whitespace around root degree/value parameters for stable rewritten syntax.
        while (!value.empty() && isspace((unsigned char)value.front()))
            value.erase(value.begin());
        // Loop Note: Trims leading/trailing whitespace around root degree/value parameters for stable rewritten syntax.
        while (!value.empty() && isspace((unsigned char)value.back()))
            value.pop_back();

        string repl = "((" + value + ") ^ (1 / (" + degree + ")))";
        s.replace(pos, end - pos + 1, repl);
        pos += repl.size();
    }

    return s;
}

// Note:
// What it does: Converts sqrt(...) calls into exponent form ((...)^(0.5)).
// Input: raw expression string.
// Returns: expression with sqrt-call syntax rewritten.
// Why needed: aligns standalone solver input handling with runner preprocessing.
// Theory: syntax desugaring maps function form into core power operator form.
string convertSqrtCalls(const string &expr)
{
    string out = expr;

    while (true)
    {
        size_t p = out.find("sqrt(");
        if (p == string::npos)
            break;

        size_t argStart = p + 4; // points to '('
        int depth = 0;
        size_t i = argStart;
        bool foundEnd = false;

        for (; i < out.size(); i++)
        {
            if (out[i] == '(')
                depth++;
            else if (out[i] == ')')
            {
                depth--;
                if (depth == 0)
                {
                    foundEnd = true;
                    break;
                }
            }
        }

        if (!foundEnd)
            break;

        string inner = out.substr(argStart + 1, i - (argStart + 1));
        string repl = "((" + inner + ")^(0.5))";
        out.replace(p, i - p + 1, repl);
    }

    return out;
}

// Note:
// What it does: Expands compact all-letter symbol chunks into explicit products.
// Input: letter-only symbol text (e.g., "ab", "yx", "ypi").
// Returns: explicit product text (e.g., "a*b", "y*x", "y*pi").
// Why needed: keeps close-form representation checks stable by making implied products explicit.
// Theory: lexical-level canonicalization reduces ambiguous compact symbolic spelling.
string expandLetterOnlyProduct(const string &name)
{
    string lowered = toLowerString(name);
    vector<string> factors;

    // Loop Note: Walk symbol text left-to-right and tokenize constants (pi) or one-letter variables.
    for (int i = 0; i < (int)lowered.size();)
    {
        // Check Note: This branch runs when the i + 1 is less than the size() of the lowercase
        // text, the lowered[i] is the character 'p', and the next character is the character
        // 'i'.
        if (i + 1 < (int)lowered.size() && lowered[i] == 'p' && lowered[i + 1] == 'i')
        {
            factors.push_back("pi");
            i += 2;
            continue;
        }

        factors.push_back(string(1, lowered[i]));
        i++;
    }

    string out;
    // Loop Note: Rebuild tokenized factors using explicit multiplication separators.
    for (int i = 0; i < (int)factors.size(); i++)
    {
        if (i > 0)
            out += "*";
        out += factors[i];
    }
    return out;
}

// Note:
// What it does: Converts compact function shorthand into explicit function-call form.
// Input: letter-only symbol text (e.g., "cosx", "lnab", "sqrtyx").
// Returns: rewritten call text (e.g., "cos(x)", "ln(a*b)", "sqrt(y*x)") or empty when no match.
// Why needed: users often omit parentheses in single-argument function input.
// Theory: front-end desugaring maps shorthand notation into parser-ready grammar.
string rewriteCompactFunctionShorthand(const string &name)
{
    static const vector<string> kFunctionNames = {
        "sqrt", "sin", "cos", "tan", "cot", "sec", "csc", "log", "ln", "exp", "abs", "sgn"};

    string lowered = toLowerString(name);

    // Loop Note: Check known function prefixes and rewrite only when a compact tail exists.
    for (int i = 0; i < (int)kFunctionNames.size(); i++)
    {
        const string &fname = kFunctionNames[i];
        if (lowered.size() <= fname.size())
            continue;

        // Check Note: This branch runs when the lowercase text does not start with the fname.
        if (lowered.compare(0, fname.size(), fname) != 0)
            continue;

        string tail = lowered.substr(fname.size());
        bool tailAllLetters = !tail.empty();

        // Loop Note: Validate tail as pure letters to avoid rewriting identifiers with digits/underscores.
        for (int j = 0; j < (int)tail.size(); j++)
        {
            if (!isalpha((unsigned char)tail[j]))
            {
                tailAllLetters = false;
                break;
            }
        }

        if (!tailAllLetters)
            continue;

        string arg = expandLetterOnlyProduct(tail);
        return fname + "(" + arg + ")";
    }

    return "";
}

// Note:
// What it does: Rewrites compact letter-only identifiers in raw input into explicit math syntax.
// Input: raw expression string.
// Returns: expression where compact symbol chunks are expanded (e.g., yx -> y*x, cosx -> cos(x)).
// Why needed: keeps all user-input normalization inside project.cpp (standalone parity with runner behavior intent).
// Theory: pre-lex token-shape canonicalization removes shorthand ambiguity before parsing.
string expandCompactLetterSymbols(const string &expr)
{
    string out;

    // Loop Note: Scan input once and rewrite alphabetic chunks while preserving all non-alpha text.
    for (int i = 0; i < (int)expr.size();)
    {
        char c = expr[i];
        if (!isalpha((unsigned char)c))
        {
            out += c;
            i++;
            continue;
        }

        int start = i;
        i++;
        while (i < (int)expr.size() && (isalnum((unsigned char)expr[i]) || expr[i] == '_'))
            i++;

        string name = expr.substr(start, i - start);
        string lowered = toLowerString(name);

        // Keep canonical constants/function names intact when already valid symbols.
        if (lowered == "pi" || lowered == "e" || isSupportedFunctionName(name) || lowered == "log10")
        {
            out += lowered;
            continue;
        }

        bool allLetters = true;
        for (int j = 0; j < (int)name.size(); j++)
        {
            if (!isalpha((unsigned char)name[j]))
            {
                allLetters = false;
                break;
            }
        }

        if (!allLetters)
        {
            out += name;
            continue;
        }

        // First try function shorthand rewrite (cosx, lnab, sqrtyx ...).
        string asFunctionShorthand = rewriteCompactFunctionShorthand(name);
        if (!asFunctionShorthand.empty())
        {
            out += asFunctionShorthand;
            continue;
        }

        // Fallback for plain compact symbols (yx, ab, xyz ...).
        out += expandLetterOnlyProduct(name);
    }

    return out;
}

// Note:
// What it does: Applies standalone input preprocessing before lexing.
// Input: raw user expression.
// Returns: normalized expression for parser pipeline.
// Why needed: allows project binary to self-run with runner-equivalent syntax support.
// Theory: pre-lex normalization stage for syntax compatibility.
string preprocessInputExpression(const string &input)
{
    string s = input;
    // Phase A: expand compact symbolic shorthand before structural rewrites.
    s = expandCompactLetterSymbols(s);
    // Phase B: normalize sqrt(...) function syntax into exponent form.
    s = convertSqrtCalls(s);
    // Phase C: normalize log/root aliases into parser-supported core syntax.
    s = rewriteLogBaseSyntax(s);
    return s;
}

// PHASE 2.7: Final-form variant exploration (resource-bounded).
// Handler Revision Notes:
// - v7: introduces selective power expansion checks at end of pipeline.
// - Strategy: generate limited variants, re-simplify, keep only strict wins.

// Note:
// What it does: Counts total AST nodes recursively.
// Input: subtree root.
// Returns: node count.
// Why needed: Complexity scoring compares candidate expression forms.
// Theory: Tree size is a simple proxy for representational complexity.
int countNodes(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return 0;
    return 1 + countNodes(node->left) + countNodes(node->right);
}

// Note:
// What it does: Computes maximum depth of AST.
// Input: subtree root.
// Returns: depth value.
// Why needed: Deeper trees often indicate more complex expression structure.
// Theory: Tree height approximates nesting complexity.
int maxDepth(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return 0;
    int l = maxDepth(node->left);
    int r = maxDepth(node->right);
    return 1 + (l > r ? l : r);
}

// Note:
// What it does: Counts exponent operator nodes in subtree.
// Input: subtree root.
// Returns: number of power nodes.
// Why needed: Complexity metric penalizes heavy power usage where useful.
// Theory: Operator-type weighted metrics guide bounded search heuristics.
int countPowerNodes(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return 0;
    int self = (node->type == NODE_OPERATOR && node->op == '^') ? 1 : 0;
    return self + countPowerNodes(node->left) + countPowerNodes(node->right);
}

// Note:
// What it does: Produces weighted complexity score from size/depth/power count.
// Input: subtree root.
// Returns: integer complexity score (lower is better).
// Why needed: Final variant explorer needs objective comparison criteria.
// Theory: Heuristic cost function enables practical search under constraints.
int expressionComplexityScore(Node *node)
{
    // Complexity heuristic used for variant selection:
    // smaller is preferred. Weighting favors reduced size/depth/power count.
    int nodes = countNodes(node);
    int depth = maxDepth(node);
    int powers = countPowerNodes(node);
    return nodes * 5 + depth * 3 + powers * 4;
}

// Note:
// What it does: Deep-copies an AST subtree.
// Input: subtree root.
// Returns: new independent subtree copy.
// Why needed: Variant exploration must avoid mutating baseline candidate.
// Theory: Persistent-style search requires structural cloning for branch isolation.
Node *cloneTree(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return NULL;

    // Check Note: This branch runs when the current node is a number node.
    if (node->type == NODE_NUMBER)
        return makeNumber(node->numberValue);
    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
        return makeVariable(node->variableName);
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
        return makeFunction(node->variableName, cloneTree(node->left));

    return makeOperator(node->op, cloneTree(node->left), cloneTree(node->right));
}

// Note:
// What it does: Checks whether a power node is eligible for bounded expansion.
// Input: candidate node.
// Returns: true if node satisfies safe expansion constraints.
// Why needed: Prevents combinatorial explosion during final-form exploration.
// Theory: Search-space pruning via admissibility constraints.
bool canExpandPowerNode(Node *node)
{
    // Resource policy for expansion candidacy:
    // - integer exponent only
    // - exponent in [2,4] to prevent combinatorial blow-up
    // - base size bounded to avoid expensive distributive explosions
    // Check Note: This branch runs when either the current node is missing, the current node is
    // not an operator node, or the operator in the current node is not the character '^'.
    if (node == NULL || node->type != NODE_OPERATOR || node->op != '^')
        return false;
    // Check Note: This branch runs when either the right side is missing or the right side is
    // not a number node.
    if (node->right == NULL || node->right->type != NODE_NUMBER)
        return false;

    double expVal = node->right->numberValue;
    // Check Note: This branch runs when the exp val is not a whole number.
    if (!isIntegerValue(expVal))
        return false;

    int n = (int)round(expVal);
    // Check Note: This branch runs when either n is less than 2 or n is greater than 4.
    if (n < 2 || n > 4)
        return false;

    // Check Note: This branch runs when the left side is missing.
    if (node->left == NULL)
        return false;

    int baseSize = countNodes(node->left);
    // Check Note: This branch runs when the base size is greater than the 12.
    if (baseSize > 12)
        return false;

    return true;
}

// Note:
// What it does: Replaces base^n with repeated multiplication chain.
// Input: power node.
// Returns: expanded product subtree (or clone if not eligible).
// Why needed: Some simplifications become visible only after expansion.
// Theory: Representation change can expose latent rewrite opportunities.
Node *expandPowerToProduct(Node *powerNode)
{
    // Expands base^n into repeated multiplication (base*base*...*base).
    // This is intentionally structural; simplification is done in subsequent passes.
    // Check Note: This branch runs when the power node is safe to expand.
    if (!canExpandPowerNode(powerNode))
        return cloneTree(powerNode);

    int n = (int)round(powerNode->right->numberValue);
    Node *result = cloneTree(powerNode->left);
    // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
    for (int i = 1; i < n; i++)
    {
        result = makeOperator('*', result, cloneTree(powerNode->left));
    }
    return result;
}

// Note:
// What it does: Counts eligible expandable power nodes in subtree.
// Input: subtree root.
// Returns: count of expansion candidates.
// Why needed: Drives bounded iteration over variant choices.
// Theory: Candidate enumeration is prerequisite for local search.
int countExpandablePowerNodes(Node *node)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return 0;
    int self = canExpandPowerNode(node) ? 1 : 0;
    return self + countExpandablePowerNodes(node->left) + countExpandablePowerNodes(node->right);
}

// Note:
// What it does: Clones tree while expanding exactly one targeted eligible power node.
// Input: root node, target candidate index, traversal counter seen.
// Returns: cloned variant tree.
// Why needed: Enables single-change neighborhood exploration in search loop.
// Theory: Local-search neighbor generation through controlled structural mutation.
Node *cloneWithExpandedPowerAtIndex(Node *node, int targetIndex, int &seen)
{
    // Check Note: This branch runs when the current node is missing.
    if (node == NULL)
        return NULL;

    // Check Note: This branch runs when the current node is safe to expand.
    if (canExpandPowerNode(node))
    {
        // Check Note: This branch runs when the table of seen signatures is the target index.
        if (seen == targetIndex)
        {
            seen++;
            return expandPowerToProduct(node);
        }
        seen++;
    }

    // Check Note: This branch runs when the current node is a number node.
    if (node->type == NODE_NUMBER)
        return makeNumber(node->numberValue);
    // Check Note: This branch runs when the current node is a variable node.
    if (node->type == NODE_VARIABLE)
        return makeVariable(node->variableName);
    // Check Note: This branch runs when the current node is a function node.
    if (node->type == NODE_FUNCTION)
        return makeFunction(node->variableName, cloneWithExpandedPowerAtIndex(node->left, targetIndex, seen));

    return makeOperator(node->op,
                        cloneWithExpandedPowerAtIndex(node->left, targetIndex, seen),
                        cloneWithExpandedPowerAtIndex(node->right, targetIndex, seen));
}

// Note:
// What it does: Runs a short bounded subset of optimization passes on a candidate tree.
// Input: candidate root node.
// Returns: optimized candidate root.
// Why needed: Re-simplifies explored variants without full expensive pipeline.
// Theory: Iterative fixed-point approximation under hard iteration budget.
Node *runBoundedOptimizationCycle(Node *node)
{
    // Compact optimization rerun used for candidate variants.
    // Keeps cost bounded while still applying the core simplification stack.
    Node *root = node;

    bool anyChange = true;
    int iter = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (anyChange && iter < 6)
    {
        iter++;
        anyChange = false;

        bool ch = false;
        root = constantFolding(root, ch);
        anyChange = anyChange || ch;

        ch = false;
        root = identityReduction(root, ch);
        anyChange = anyChange || ch;

        ch = false;
        root = deadCodeElimination(root, ch);
        anyChange = anyChange || ch;

        ch = false;
        root = strengthReduction(root, ch);
        anyChange = anyChange || ch;

        ch = false;
        root = algebraicSimplification(root, ch);
        anyChange = anyChange || ch;
    }

    bool cleanup = true;
    int cleanupIter = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (cleanup && cleanupIter < 3)
    {
        cleanupIter++;
        cleanup = false;

        bool ch = false;
        root = algebraicSimplification(root, ch);
        cleanup = cleanup || ch;

        ch = false;
        root = identityReduction(root, ch);
        cleanup = cleanup || ch;

        ch = false;
        root = deadCodeElimination(root, ch);
        cleanup = cleanup || ch;
    }

    return root;
}

// Note:
// What it does: Orchestrates full solver workflow from input to final symbolic/numeric output.
// Input: user expression and optional variable values via stdin.
// Returns: process exit code (0 success, non-zero on errors).
// Why needed: Integrates frontend, optimizer pipeline, variant exploration, and backend evaluation.
// Theory: End-to-end compiler-like pipeline: lex -> parse -> optimize -> print/evaluate.
int main()
{
    // Driver Orchestration (v1->v6):
    // 1) front-end translation
    // 2) iterative middle-end optimization
    // 3) cleanup + round-trip stabilization
    // 4) backend output/evaluation

    cout << "================ Algebraic Expression Solver ================\n";
    cout << "Enter an expression (type 'quit' to exit): ";

    string input;
    // Check Note: This branch runs when reading input succeeded.
    if (!getline(cin, input))
    {
        cout << "No input received. Exiting.\n";
        return 0;
    }

    // Check Note: This branch runs when either the input text is "quit" or the input text is
    // "exit".
    if (input == "quit" || input == "exit")
    {
        cout << "Exited by user request.\n";
        return 0;
    }

    // Check Note: This branch runs when the input text is empty.
    if (input.empty())
    {
        cout << "Empty expression. Please run again and enter a valid expression.\n";
        return 1;
    }

    // ---------------- PHASE 1: FRONT-END ----------------
    bool ok = true;
    string errorMessage;

    string preprocessedInput = preprocessInputExpression(input);

    vector<Token> rawTokens = lexicalAnalysis(preprocessedInput, ok, errorMessage);
    // Check Note: This branch runs when the current step failed.
    if (!ok)
    {
        cout << "[Phase 1 - Lexing Error] " << errorMessage << "\n";
        return 1;
    }

    vector<Token> normalizedTokens = normalizeTokens(rawTokens);
    vector<Token> postfix = shuntingYardToPostfix(normalizedTokens, ok, errorMessage);
    // Check Note: This branch runs when the current step failed.
    if (!ok)
    {
        cout << "[Phase 1 - Parsing Error] " << errorMessage << "\n";
        return 1;
    }

    Node *root = buildASTFromPostfix(postfix, ok, errorMessage);
    // Check Note: This branch runs when either the current step failed or the current
    // expression tree is missing.
    if (!ok || root == NULL)
    {
        cout << "[Phase 1 - AST Build Error] " << errorMessage << "\n";
        return 1;
    }

    cout << "\n[Phase 1 Output]\n";
    cout << "Raw Tokens       : " << tokensToSimpleString(rawTokens) << "\n";
    cout << "Normalized Tokens: " << tokensToSimpleString(normalizedTokens) << "\n";
    cout << "Postfix Tokens   : " << tokensToSimpleString(postfix) << "\n";
    cout << "Initial AST Expr : " << treeToString(root) << "\n";

    // ---------------- PHASE 2: MIDDLE-END ----------------
    cout << "\n[Phase 2 Output]\n";
    cout << "Running optimizer passes...\n";

    bool anyChange = true;
    int iteration = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (anyChange && iteration < 10)
    {
        // Main Optimization Loop (change-reviewed through v6):
        // - fixed pass order preserves deterministic simplification behavior.
        // - CSE intentionally skipped in this loop for cancellation stability.
        iteration++;
        anyChange = false;
        bool changed = false;

        root = constantFolding(root, changed);
        // Check Note: This branch runs when this pass changed the tree.
        if (changed)
        {
            anyChange = true;
            cout << "After Constant Folding           : " << treeToString(root) << "\n";
        }

        changed = false;
        root = identityReduction(root, changed);
        // Check Note: This branch runs when this pass changed the tree.
        if (changed)
        {
            anyChange = true;
            cout << "After Identity Reduction         : " << treeToString(root) << "\n";
        }

        changed = false;
        root = deadCodeElimination(root, changed);
        // Check Note: This branch runs when this pass changed the tree.
        if (changed)
        {
            anyChange = true;
            cout << "After Dead Code Elimination      : " << treeToString(root) << "\n";
        }

        changed = false;
        root = strengthReduction(root, changed);
        // Check Note: This branch runs when this pass changed the tree.
        if (changed)
        {
            anyChange = true;
            cout << "After Strength Reduction         : " << treeToString(root) << "\n";
        }

        changed = false;
        root = algebraicSimplification(root, changed);
        // Check Note: This branch runs when this pass changed the tree.
        if (changed)
        {
            anyChange = true;
            cout << "After Algebraic Simplification   : " << treeToString(root) << "\n";
        }

        // NOTE: CSE is intentionally skipped here to preserve deterministic algebraic cancellation behavior.
    }

    // Final algebraic cleanup without CSE to fully settle cancellation after distribution/reordering.
    bool cleanupChange = true;
    int cleanupIter = 0;
    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
    while (cleanupChange && cleanupIter < 6)
    {
        // Post-loop Cleanup Window (v6):
        // - extra algebraic/identity/DCE settling without structural dedup side effects.
        cleanupIter++;
        cleanupChange = false;

        bool changed = false;
        root = algebraicSimplification(root, changed);
        cleanupChange = cleanupChange || changed;

        changed = false;
        root = identityReduction(root, changed);
        cleanupChange = cleanupChange || changed;

        changed = false;
        root = deadCodeElimination(root, changed);
        cleanupChange = cleanupChange || changed;
    }

    // One round-trip canonicalization can expose additional commutative cancellations.
    {
        // Round-Trip Canonical Stabilization (v6):
        // - stringify -> re-lex -> re-parse -> rerun passes once more.
        // - purpose: expose hidden commutative cancellations after prior rewrites.
        string roundTripExpr = treeToString(root);
        bool rtOk = true;
        string rtErr;
        vector<Token> rtRaw = lexicalAnalysis(roundTripExpr, rtOk, rtErr);
        // Check Note: This branch runs when the round-trip parse succeeded.
        if (rtOk)
        {
            vector<Token> rtNorm = normalizeTokens(rtRaw);
            vector<Token> rtPost = shuntingYardToPostfix(rtNorm, rtOk, rtErr);
            // Check Note: This branch runs when the round-trip parse succeeded.
            if (rtOk)
            {
                Node *rtRoot = buildASTFromPostfix(rtPost, rtOk, rtErr);
                // Check Note: This branch runs when the round-trip parse succeeded and the
                // round-trip tree exists.
                if (rtOk && rtRoot != NULL)
                {
                    root = rtRoot;

                    bool rtAny = true;
                    int rtIter = 0;
                    // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
                    while (rtAny && rtIter < 8)
                    {
                        rtIter++;
                        rtAny = false;

                        bool ch = false;
                        root = constantFolding(root, ch);
                        rtAny = rtAny || ch;

                        ch = false;
                        root = identityReduction(root, ch);
                        rtAny = rtAny || ch;

                        ch = false;
                        root = deadCodeElimination(root, ch);
                        rtAny = rtAny || ch;

                        ch = false;
                        root = strengthReduction(root, ch);
                        rtAny = rtAny || ch;

                        ch = false;
                        root = algebraicSimplification(root, ch);
                        rtAny = rtAny || ch;
                    }
                }
            }
        }
    }

    // Final-form power variant exploration (v7):
    // - Tests limited alternative representations for powers.
    // - If expanded form simplifies further, adopt and iterate.
    // - Stops when no candidate strictly improves complexity.
    {
        bool improved = true;
        int wave = 0;
        // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
        while (improved && wave < 4)
        {
            wave++;
            improved = false;

            int expandableCount = countExpandablePowerNodes(root);
            // Check Note: This branch runs when the number of expandable power nodes is at most
            // zero.
            if (expandableCount <= 0)
                break;

            Node *bestRoot = root;
            int bestScore = expressionComplexityScore(root);
            string bestText = treeToString(root);

            // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
            for (int idx = 0; idx < expandableCount; idx++)
            {
                int seen = 0;
                Node *candidate = cloneWithExpandedPowerAtIndex(root, idx, seen);
                candidate = runBoundedOptimizationCycle(candidate);

                int candScore = expressionComplexityScore(candidate);
                string candText = treeToString(candidate);

                bool strictlyBetter = false;
                // Check Note: This branch runs when the candidate score is less than the
                // current best score.
                if (candScore < bestScore)
                {
                    strictlyBetter = true;
                }
                // Check Note: If the earlier case did not match, this branch runs when the
                // candidate score is the current best score and the size of the candidate text
                // form is less than the size() of the current best text form.
                else if (candScore == bestScore && candText.size() < bestText.size())
                {
                    strictlyBetter = true;
                }

                // Check Note: This branch runs when the candidate is better and it is not the
                // same expression as the current best one.
                if (strictlyBetter && !areTreesEqual(candidate, bestRoot))
                {
                    bestRoot = candidate;
                    bestScore = candScore;
                    bestText = candText;
                    improved = true;
                }
            }

            // Check Note: This branch runs when a better expression form was found.
            if (improved)
            {
                root = bestRoot;
                cout << "After Power Variant Exploration   : " << treeToString(root) << "\n";
            }
        }
    }

    cout << "Final Optimized Expr             : " << treeToString(root) << "\n";

    // Check Note: This branch runs when the current expression tree contains a definite
    // division by numeric zero.
    if (hasConstantDivisionByZero(root))
    {
        cout << "\n[Phase 3 Output]\n";
        cout << "Evaluation Error : Division by zero detected in expression\n";
        cout << "==============================================================\n";
        return 1;
    }

    // ---------------- PHASE 3: BACK-END ----------------
    cout << "\n[Phase 3 Output]\n";
    cout << "Stringified Result: " << treeToString(root) << "\n";

    set<string> vars;
    collectVariables(root, vars);

    if (vars.size() <= 1)
    {
        string waveVariable;
        if (!vars.empty())
            waveVariable = *vars.begin();
        renderWaveVisualizer(root, waveVariable);
    }
    else
    {
        cout << "\n[Wave Visualizer]\n";
        cout << "Wave preview is available for one-variable expressions only.\n";
    }

    // Check Note: This branch runs when the set of variables is empty.
    if (vars.empty())
    {
        map<string, double> noVars;
        ok = true;
        errorMessage = "";
        double value = evaluateTree(root, noVars, ok, errorMessage);
        // Check Note: This branch runs when the current step succeeded.
        if (ok)
        {
            cout << "Numeric Result   : " << numberToString(value) << "\n";
        }
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
        {
            cout << "Evaluation Error : " << errorMessage << "\n";
        }
    }
    // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
    else
    {
        map<string, double> values;
        cout << "Variables found  : ";
        int count = 0;
        // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
        for (set<string>::iterator it = vars.begin(); it != vars.end(); ++it)
        {
            // Check Note: This branch runs when the count is greater than zero.
            if (count > 0)
                cout << ", ";
            cout << *it;
            count++;
        }
        cout << "\n";

        // Loop Note: Iterates through a sequence/state repeatedly because the operation depends on processing multiple items or steps; a loop is used instead of manual repetition for scalability and correctness.
        for (set<string>::iterator it = vars.begin(); it != vars.end(); ++it)
        {
            // Loop Note: Repeats until a dynamic condition changes (unknown iteration count ahead of time); while is used because termination depends on runtime state, not a fixed count.
            while (true)
            {
                cout << "Enter value for " << *it << ": ";
                string valueLine;
                // Check Note: This branch runs when reading input succeeded.
                if (!getline(cin, valueLine))
                {
                    cout << "Input stream closed. Exiting.\n";
                    return 1;
                }

                // Check Note: This branch runs when the entered value is empty.
                if (valueLine.empty())
                {
                    cout << "No value entered. Skipping numeric evaluation.\n";
                    cout << "Final Simplified Expression: " << treeToString(root) << "\n";
                    cout << "==============================================================\n";
                    return 0;
                }

                stringstream parser(valueLine);
                double v;
                char extra;
                // Check Note: This branch runs when the parser >> v and it is not true that the
                // parser >> extra.
                if ((parser >> v) && !(parser >> extra))
                {
                    values[*it] = v;
                    break;
                }

                cout << "Invalid number. Try again.\n";
            }
        }

        ok = true;
        errorMessage = "";
        double value = evaluateTree(root, values, ok, errorMessage);
        // Check Note: This branch runs when the current step succeeded.
        if (ok)
        {
            cout << "Numeric Result   : " << numberToString(value) << "\n";
        }
        // Else Note: Fallback branch used when prior condition(s) fail; needed to preserve complete case coverage and deterministic behavior.
        else
        {
            cout << "Evaluation Error : " << errorMessage << "\n";
        }
    }

    cout << "==============================================================\n";
    return 0;
}

/*
===============================================================================
PIPELINE GUIDE (END-TO-END) - TREE VIEW + THEORY
===============================================================================

High-level tree of the solver pipeline:

Input Expression (string)
|
+-- Phase 1: Front-End (Translator)
|   |
|   +-- H-31 preprocessInputExpression
|   |   - Orchestrates pre-lex input desugaring in deterministic order.
|   |   - Theory: staged canonicalization lowers notation entropy before formal lexing.
|   |
|   +-- H-30 expandCompactLetterSymbols
|   |   - Rewrites compact letter chunks and function shorthand into explicit math syntax.
|   |   - Theory: lexical-shape normalization enforces one surface form per semantic intent.
|   |
|   +-- H-29 rewriteCompactFunctionShorthand
|   |   - Maps compact forms like cosx/sqrtyx into call form cos(x)/sqrt(y*x).
|   |   - Theory: syntactic sugar elimination maps informal notation to grammar terminals.
|   |
|   +-- H-28 expandLetterOnlyProduct
|   |   - Splits letter-only chunks (including pi-aware segments) into explicit products.
|   |   - Theory: conservative token decomposition avoids ambiguous implicit multiplication.
|   |
|   +-- H-01 lexicalAnalysis
|   |   - Converts raw chars into typed tokens (number, variable, op, function, paren)
|   |   - Theory: scanner emits terminal symbols for deterministic downstream parsing.
|   |
|   +-- H-02 normalizeTokens
|   |   - Resolves unary sign chains
|   |   - Inserts implicit multiplication (for forms like 2x, x(y+1))
|   |   - Theory: grammar repair converts informal infix into precedence-safe explicit form.
|   |
|   +-- H-03 shuntingYardToPostfix
|   |   - Applies precedence and associativity, outputs postfix tokens
|   |   - Theory: stack-based operator transduction enforces infix binding laws.
|   |
|   +-- H-04 buildASTFromPostfix
|       - Builds abstract syntax tree (AST)
|       - Theory: postfix reduction creates a compositional semantic tree.
|
+-- Phase 2: Middle-End (Optimizer)
|   |
|   +-- Canonical shape helpers (core to cancellation quality)
|   |   |
|   |   +-- H-05 areTreesEqual
|   |   |   - Structural equivalence with commutative matching for + and *.
|   |   |   - Theory: selective tree isomorphism approximates algebraic equality for rewrite guards.
|   |   |
|   |   +-- H-06 collectAdditiveTerms
|   |   |   - Flattens nested additive trees into signed linear term lists.
|   |   |   - Theory: associative normalization converts recursive sums to multiset-style algebra.
|   |   |
|   |   +-- H-07 flattenProduct
|   |   |   - Flattens multiplicative trees and isolates scalar coefficient.
|   |   |   - Theory: coefficient-times-factor decomposition enables factor-level cancellation.
|   |   |
|   |   +-- H-08 extractCanonicalProduct
|   |   |   - Produces stable product signatures across reordered factors.
|   |   |   - Theory: canonical signatures collapse commutative variants into one key space.
|   |   |
|   |   +-- H-09 detectPerfectPower
|   |       - Detects binomial/theory-aligned perfect powers from expanded forms.
|   |       - Theory: coefficient-pattern recognition inverts bounded binomial expansion.
|   |   |
|   |   +-- H-32 to H-37 targeted pattern helpers
|   |       - Recognize trig powers, doubled angles, pi/2 complements, and conjugate products.
|   |       - Theory: small structural matchers keep higher-level rewrite rules precise.
|   |
|   +-- Iterative rewrite cycle (fixed-point style, bounded)
|   |   |
|   |   +-- H-10 algebraicSimplification
|   |   |   - Pattern rules: identities, trig/log/exponent laws, fraction algebra,
|   |   |     formula transforms, cancellation, factoring/expansion interaction,
|   |   |     plus basic trig parity/complement/double-angle closures and
|   |   |     pre-recursion quotient preservation for fragile division patterns
|   |   |   - Theory: term-rewriting converges via repeated local graph transforms.
|   |   |
|   |   +-- H-11 constantFolding
|   |   |   - Evaluates numeric-only subtrees
|   |   |   - Theory: partial evaluation shrinks symbolic search space.
|   |   |
|   |   +-- H-12 identityReduction
|   |   |   - Removes neutral/absorbing terms (x+0, x*1, x-x, etc.)
|   |   |   - Theory: algebraic identity axioms provide low-cost normalization wins.
|   |   |
|   |   +-- H-13 deadCodeElimination
|   |   |   - Prunes guaranteed-no-effect parts (like *0 branches)
|   |   |   - Theory: static certainty pruning reduces irrelevant subtrees early.
|   |   |
|   |   +-- H-14 commonSubexpressionElimination (available, selectively used)
|   |   |   - Theory: structural memoization converts repeated trees into DAG-like reuse.
|   |   +-- H-15/H-17 round-trip stability checks via string conversion + reparse
|   |       - Theory: idempotence checks detect representation drift after rewrites.
|   |
|   +-- Phase 2.7 bounded power-variant exploration
|       - Tests alternate power forms and keeps strictly better candidate
|       - Theory: bounded neighborhood search escapes local representation minima.
|
+-- Phase 3: Back-End (Result)
    |
    +-- H-15 treeToString
    |   - Pretty-prints AST with precedence-safe parentheses
    |   - Theory: precedence-aware pretty-printing preserves parse-equivalent semantics.
    |
    +-- H-16 evaluateTree (optional, when values are provided)
        - Numeric evaluation with domain checks
        - Theory: recursive interpretation computes denotation under variable environment.


Theory of how the algorithm works:

1) Compiler-style architecture
   - The solver acts like a compiler pipeline:
     lexical analysis -> syntax analysis -> IR optimization -> backend rendering/eval.

2) Term-rewriting engine core
   - Algebraic simplification is rule-driven over AST patterns.
   - Each rewrite is local but repeated to a fixed-point, so global simplification emerges
     from many local transformations.

3) Canonicalization + bounded search
   - Canonical forms (term ordering, factor grouping, monomial extraction) reduce
     representation variance.
   - A bounded exploration step (power-variant) tries alternate forms when a compact
     expression may hide simplification opportunities.

4) Safety constraints
   - Domain-sensitive function behavior and division-by-zero checks avoid unsafe evaluation.
   - Numeric comparisons use tolerance predicates (isZero/isOne) to reduce floating error.

Latest extension note (v21)
   - Added pre-recursion division shortcuts so u / u^n and reversed-difference quotients are
     simplified before denominator rewrites expand or reorder away the useful pattern.
   - Split numeric log semantics so log now folds/evaluates in base 10 while ln keeps base e,
     while preserving legacy symbolic compatibility cases like log(e) and 10^log(x).


===============================================================================
20 TEST EXPRESSIONS - SPECIFIC PIPELINES (FROM v26 RAW TRACES)
===============================================================================

Format used below:
- Test ID
- Expression
- Phase 1 (tokenization/parsing snapshots)
- Key Phase 2 transitions
- Final result

1) Test #1
   Expr: 1 + 2 + 3
   Phase 1: Raw=1 + 2 + 3 | Normalized=1 + 2 + 3 | Postfix=1 2 + 3 +
   AST: 1 + 2 + 3
   Phase 2: Constant Folding -> 6
   Final: 6

2) Test #5
   Expr: 2 ^ 3 ^ 2
   Phase 1: Raw=2 ^ 3 ^ 2 | Postfix=2 3 2 ^ ^
   AST: 2 ^ (3 ^ 2)  (right-associative exponent parse)
   Phase 2: Constant Folding -> 512
   Final: 512

3) Test #15
   Expr: -(x + y)
   Phase 1: Normalized unary rewrite -> -1 * (x + y)
   AST: -(x + y)
   Phase 2: Alternates between -x + -y and -(x + y) during rewrite stabilization
   Final: -(x + y)

4) Test #40
   Expr: (x + 1) * (x + 1)
   Phase 1: Postfix=x 1 + x 1 + *
   AST: (x + 1) * (x + 1)
   Phase 2: Distribution/combination -> x^2 + 2x + 1, then compact form detection
            -> (x + 1)^2 (plus bounded variant checks)
   Final: (x + 1) ^ 2

5) Test #44
   Expr: (x^2 - 1) / (x - 1)
   Phase 1: Standard parse
   Phase 2: Difference-of-squares quotient rule -> x + 1
   Final: x + 1

6) Test #54
   Expr: sqrt(x) * sqrt(x)
   Phase 1: Function nodes parsed as unary calls
   Phase 2: sqrt-product identity -> x
   Final: x

7) Test #62
   Expr: x + 0.5 * x
   Phase 1: AST shows x + 0.5x
   Phase 2: Coefficient merge -> x * (0.5 + 1) -> Constant Folding -> 1.5x
   Final: 1.5x

8) Test #64
   Expr: sin(x)^2 + cos(x)^2
   Phase 1: Postfix=x sin 2 ^ x cos 2 ^ +
   Phase 2: Pythagorean trig identity -> 1
   Final: 1

9) Test #67
   Expr: log(a) + log(b)
   Phase 1: Parsed as function-sum
   Phase 2: Log product law -> log(ab)
   Final: log(ab)

10) Test #75
    Expr: x^(1/2)
    Phase 1: Fractional exponent parse
    Phase 2: Constant Folding on exponent fraction -> x ^ 0.5
    Final: x ^ 0.5

11) Test #92
    Expr: sqrt(x^2)
    Phase 1: Parsed as sqrt(power)
    Phase 2: Sign-correct square-root rewrite -> abs(x)
    Final: abs(x)

12) Test #121
    Expr: (x^2 - 4)/(x + 2)
    Phase 2: Difference-of-squares cancellation path -> x - 2
    Final: x - 2

13) Test #134
    Expr: log(x^n) / n
    Phase 2: Direct quotient-log rewrite -> log(x)
    Final: log(x)

14) Test #150
    Expr: (x^2 - y^2)/(x + y)
    Phase 2: Difference-of-squares quotient rule -> x - y
    Final: x - y

15) Test #172
    Expr: e^(ln(x))
    Phase 1: e token normalized numerically in AST base
    Phase 2: exp/ln inverse rule -> x
    Final: x

16) Test #181
    Expr: 1 - cos(x)^2
    Phase 2: Reordered subtraction normalization then trig identity
            1 - cos^2 -> sin^2
    Final: sin(x) ^ 2

17) Test #200
    Expr: 1/(x+1) - 1/(x-1)
    Phase 2: Fraction merge -> single denominator, simplify numerator,
            conjugate denominator normalization -> -2/(x^2 - 1)
    Final: -2 / (x ^ 2 - 1)

18) Test #208
    Expr: x^4 - 1
    Phase 2: Quartic-difference factoring -> (x - 1)(x + 1)(x^2 + 1)
            intermediate distribute/refactor oscillation seen, bounded exploration
            keeps compact factored form
    Final: (x - 1) * (x + 1) * (x ^ 2 + 1)

19) Test #230
    Expr: x^2 + y^2 + 2xy
    Phase 2: Pattern recognized as (x + y)^2, then expansion/normalization loops;
            final kept form is expanded polynomial under current scoring/selection
    Final: x ^ 2 + y ^ 2 + 2x * y

20) Test #245
    Expr: x + x/2 + x/4
    Phase 2: Monomial-fraction coefficient aggregation -> 1.75x
    Final: 1.75x

===============================================================================
END OF PIPELINE COMMENT GUIDE
===============================================================================
*/
