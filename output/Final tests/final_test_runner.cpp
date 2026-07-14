#include <chrono>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct TestCase
{
    int id;
    std::string expression;
    std::string expected;
};

struct RunResult
{
    int id;
    std::string originalExpression;
    std::string convertedExpression;
    std::string expected;
    std::string observedSymbolic;
    std::string observedNumeric;
    std::string observedError;
    std::string status;
    std::string notes;
    fs::path rawPath;
    fs::path screenshotPath;
};

std::string getLineValue(const std::string &text, const std::string &prefix);

std::string trim(const std::string &s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        start++;

    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        end--;

    return s.substr(start, end - start);
}

std::string replaceAll(std::string s, const std::string &from, const std::string &to)
{
    if (from.empty())
        return s;

    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos)
    {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

bool isNumericToken(const std::string &s)
{
    static const std::regex kNumeric(R"(^[-+]?[0-9]*\.?[0-9]+$)");
    return std::regex_match(trim(s), kNumeric);
}

bool almostEqual(double a, double b)
{
    return std::fabs(a - b) <= 1e-9;
}

bool isKnownFunctionName(const std::string &name)
{
    static const std::vector<std::string> kKnownFunctions = {
        "sin", "cos", "tan", "cot", "sec", "csc",
        "log", "ln", "exp", "sqrt", "abs", "sgn", "root", "log10"};
    for (const std::string &f : kKnownFunctions)
    {
        if (name == f)
            return true;
    }
    return false;
}

std::string expandImplicitMultiplication(const std::string &expr)
{
    std::string out;
    for (size_t i = 0; i < expr.size();)
    {
        if (std::isalpha(static_cast<unsigned char>(expr[i])))
        {
            size_t j = i;
            while (j < expr.size() && std::isalpha(static_cast<unsigned char>(expr[j])))
                j++;

            std::string word = expr.substr(i, j - i);
            std::string low = word;
            for (char &c : low)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (low == "pi" || isKnownFunctionName(low) || word.size() == 1)
            {
                out += word;
            }
            else
            {
                for (size_t k = 0; k < word.size(); k++)
                {
                    if (k > 0)
                        out += '*';
                    out += word[k];
                }
            }

            i = j;
            continue;
        }

        out += expr[i];
        i++;
    }

    std::string expanded;
    for (size_t i = 0; i < out.size(); i++)
    {
        expanded += out[i];
        if (i + 1 >= out.size())
            continue;

        char a = out[i];
        char b = out[i + 1];
        bool aNum = std::isdigit(static_cast<unsigned char>(a));
        bool bNum = std::isdigit(static_cast<unsigned char>(b));
        bool aAlpha = std::isalpha(static_cast<unsigned char>(a));
        bool bAlpha = std::isalpha(static_cast<unsigned char>(b));

        if ((aNum && bAlpha) ||
            (aAlpha && bNum) ||
            ((aNum || aAlpha || a == ')') && b == '(') ||
            (a == ')' && (bNum || bAlpha)))
        {
            expanded += '*';
        }
    }

    return expanded;
}

std::string normalizeSymbolic(std::string s)
{
    s = replaceAll(s, " ", "");
    s = replaceAll(s, "\t", "");
    s = expandImplicitMultiplication(s);

    auto stripOuterParens = [](std::string t) {
        while (t.size() >= 2 && t.front() == '(' && t.back() == ')')
        {
            int depth = 0;
            bool wrapsAll = true;
            for (size_t i = 0; i < t.size(); i++)
            {
                if (t[i] == '(')
                    depth++;
                else if (t[i] == ')')
                    depth--;
                if (depth == 0 && i + 1 < t.size())
                {
                    wrapsAll = false;
                    break;
                }
            }
            if (!wrapsAll)
                break;
            t = t.substr(1, t.size() - 2);
        }
        return t;
    };

    auto splitTopLevel = [](const std::string &expr, char op) {
        std::vector<std::string> parts;
        int depth = 0;
        size_t start = 0;
        for (size_t i = 0; i < expr.size(); i++)
        {
            if (expr[i] == '(')
                depth++;
            else if (expr[i] == ')')
                depth--;
            else if (depth == 0 && expr[i] == op)
            {
                parts.push_back(expr.substr(start, i - start));
                start = i + 1;
            }
        }
        parts.push_back(expr.substr(start));
        return parts;
    };

    // Normalize top-level additive term ordering for readability-equivalent forms.
    std::string expanded;
    for (size_t i = 0; i < s.size(); i++)
    {
        char c = s[i];
        if (c == '-')
        {
            if (i == 0)
                expanded += c;
            else
                expanded += "+-";
        }
        else
        {
            expanded += c;
        }
    }

    std::vector<std::string> terms = splitTopLevel(expanded, '+');
    for (size_t i = 0; i < terms.size(); i++)
    {
        std::string t = stripOuterParens(terms[i]);
        std::vector<std::string> factors = splitTopLevel(t, '*');
        std::sort(factors.begin(), factors.end());

        std::string rebuilt;
        for (size_t j = 0; j < factors.size(); j++)
        {
            if (!rebuilt.empty())
                rebuilt += "*";
            rebuilt += factors[j];
        }
        terms[i] = rebuilt;
    }
    std::sort(terms.begin(), terms.end());

    std::string normalized;
    for (size_t i = 0; i < terms.size(); i++)
    {
        if (terms[i].empty())
            continue;
        if (!normalized.empty())
            normalized += "+";
        normalized += terms[i];
    }

    return normalized;
}

std::string shellQuoteSingle(const std::string &s)
{
    std::string out = "'";
    for (char c : s)
    {
        if (c == '\'')
        {
            out += "'\"'\"'";
        }
        else
        {
            out += c;
        }
    }
    out += "'";
    return out;
}

std::string readCommandOutput(const std::string &command)
{
    std::string output;
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        return "[Runner Error] Failed to execute command.";
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        output += buffer;
    }

    int code = pclose(pipe);
    (void)code;
    return output;
}

std::string replaceTokenPi(const std::string &expr)
{
    std::string out;
    out.reserve(expr.size() + 32);

    for (size_t i = 0; i < expr.size();)
    {
        bool token = false;
        if (i + 1 < expr.size())
        {
            char c1 = static_cast<char>(std::tolower(static_cast<unsigned char>(expr[i])));
            char c2 = static_cast<char>(std::tolower(static_cast<unsigned char>(expr[i + 1])));
            if (c1 == 'p' && c2 == 'i')
            {
                char prev = (i == 0) ? '\0' : expr[i - 1];
                char next = (i + 2 >= expr.size()) ? '\0' : expr[i + 2];
                bool prevOk = (i == 0) || !(std::isalnum(static_cast<unsigned char>(prev)) || prev == '_');
                bool nextOk = (i + 2 >= expr.size()) || !(std::isalnum(static_cast<unsigned char>(next)) || next == '_');
                if (prevOk && nextOk)
                {
                    out += "3.141592653589793";
                    i += 2;
                    token = true;
                }
            }
        }

        if (!token)
        {
            out += expr[i];
            i++;
        }
    }

    return out;
}

std::vector<std::string> extractVariablesFromExpression(const std::string &expr)
{
    std::vector<std::string> vars;
    std::string token;

    auto flushToken = [&]() {
        if (token.empty())
            return;

        std::string low = token;
        for (char &c : low)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        bool isKnown = (low == "pi" || low == "e");
        if (!isKnown && isKnownFunctionName(low))
            isKnown = true;

        if (!isKnown)
            vars.push_back(token);
        token.clear();
    };

    for (char ch : expr)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')
        {
            token.push_back(ch);
        }
        else
        {
            flushToken();
        }
    }
    flushToken();

    std::sort(vars.begin(), vars.end());
    vars.erase(std::unique(vars.begin(), vars.end()), vars.end());
    return vars;
}

bool evaluateExpressionWithAssignments(const fs::path &solverBin,
                                      const std::string &expr,
                                      const std::vector<std::pair<std::string, double>> &assignments,
                                      double &valueOut)
{
    std::ostringstream input;
    input << expr << "\n";
    for (const auto &kv : assignments)
        input << kv.second << "\n";

    std::string runCmd = "printf %b " + shellQuoteSingle(input.str()) +
                         " | " + shellQuoteSingle(solverBin.string()) + " 2>&1";
    std::string output = readCommandOutput(runCmd);

    std::string observedNumeric = getLineValue(output, "Numeric Result   :");
    if (!observedNumeric.empty() && isNumericToken(observedNumeric))
    {
        valueOut = std::stod(observedNumeric);
        return true;
    }
    return false;
}

bool areExpressionsEquivalentBySampling(const fs::path &solverBin,
                                        const std::string &expectedExpr,
                                        const std::string &observedExpr)
{
    std::vector<std::string> varsA = extractVariablesFromExpression(expectedExpr);
    std::vector<std::string> varsB = extractVariablesFromExpression(observedExpr);
    varsA.insert(varsA.end(), varsB.begin(), varsB.end());
    std::sort(varsA.begin(), varsA.end());
    varsA.erase(std::unique(varsA.begin(), varsA.end()), varsA.end());

    static const std::vector<double> kSamples = {-2.5, -1.5, -0.5, 0.5, 1.5, 2.5};
    int compared = 0;

    for (size_t i = 0; i < kSamples.size(); i++)
    {
        std::vector<std::pair<std::string, double>> assigns;
        assigns.reserve(varsA.size());
        for (size_t j = 0; j < varsA.size(); j++)
        {
            double v = kSamples[(i + j) % kSamples.size()];
            assigns.push_back({varsA[j], v});
        }

        double eVal = 0.0;
        double oVal = 0.0;
        bool eOk = evaluateExpressionWithAssignments(solverBin, expectedExpr, assigns, eVal);
        bool oOk = evaluateExpressionWithAssignments(solverBin, observedExpr, assigns, oVal);
        if (!eOk || !oOk)
            continue;

        compared++;
        if (!almostEqual(eVal, oVal))
            return false;
    }

    return compared >= 2;
}

std::string convertSqrtCalls(const std::string &expr)
{
    std::string out = expr;

    while (true)
    {
        size_t p = out.find("sqrt(");
        if (p == std::string::npos)
            break;

        size_t argStart = p + 5;
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

        std::string inner = out.substr(argStart + 1, i - (argStart + 1));
        std::string repl = "((" + inner + ")^(0.5))";
        out.replace(p, i - p + 1, repl);
    }

    return out;
}

std::string preprocessExpression(const std::string &input)
{
    std::string e = trim(input);
    e = convertSqrtCalls(e);
    return e;
}

std::vector<TestCase> loadTests(const fs::path &filePath)
{
    std::vector<TestCase> tests;
    std::ifstream in(filePath);
    if (!in)
    {
        return tests;
    }

    std::string line;
    int id = 0;
    while (std::getline(in, line))
    {
        std::string t = trim(line);
        if (t.empty())
            continue;

        size_t sep = t.find('|');
        if (sep == std::string::npos)
            continue;

        std::string expr = trim(t.substr(0, sep));
        std::string expected = trim(t.substr(sep + 1));
        if (expr.empty() || expected.empty())
            continue;

        id++;
        tests.push_back({id, expr, expected});
    }

    return tests;
}

std::string getLineValue(const std::string &text, const std::string &prefix)
{
    std::istringstream in(text);
    std::string line;
    std::string value;
    while (std::getline(in, line))
    {
        if (line.rfind(prefix, 0) == 0)
        {
            value = trim(line.substr(prefix.size()));
        }
    }
    return value;
}

std::string getLastErrorLikeLine(const std::string &text)
{
    std::istringstream in(text);
    std::string line;
    std::string err;
    while (std::getline(in, line))
    {
        if (line.find("Error") != std::string::npos || line.find("Undefined") != std::string::npos)
        {
            err = trim(line);
        }
    }
    return err;
}

int nextVersion(const fs::path &finalOutDir)
{
    int mx = 0;
    std::regex dirPat(R"(^v([0-9]+)$)");
    std::regex mdPat(R"(^v([0-9]+)\.md$)");

    if (!fs::exists(finalOutDir))
        return 1;

    for (const auto &entry : fs::directory_iterator(finalOutDir))
    {
        std::string name = entry.path().filename().string();
        std::smatch m;
        if (entry.is_directory() && std::regex_match(name, m, dirPat))
        {
            mx = std::max(mx, std::stoi(m[1].str()));
        }
        else if (entry.is_regular_file() && std::regex_match(name, m, mdPat))
        {
            mx = std::max(mx, std::stoi(m[1].str()));
        }
    }

    return mx + 1;
}

bool makeScreenshot(const fs::path &rawPath, const fs::path &pngPath)
{
    std::string rawQ = shellQuoteSingle(rawPath.string());
    std::string pngQ = shellQuoteSingle(pngPath.string());

    std::string cmd1 = "convert -background '#0d1117' -fill '#e6edf3' -pointsize 30 -interline-spacing 8 "
                       "-size 3200x4200 caption:@" + rawQ + " " + pngQ + " 2>/dev/null";

    if (std::system(cmd1.c_str()) == 0)
        return true;

    std::string cmd2 = "convert -background white -fill black -pointsize 26 -interline-spacing 6 "
                       "-size 3000x4000 caption:@" + rawQ + " " + pngQ + " 2>/dev/null";

    return std::system(cmd2.c_str()) == 0;
}

int main()
{
    fs::path exeDir = fs::current_path();
    fs::path projectDir = exeDir;
    fs::path outputDir = projectDir / "output";
    fs::path finalTestsDir = outputDir / "Final tests";
    fs::path finalOutputDir = outputDir / "Final output";
    fs::path testsFile = finalTestsDir / "testing expressions.txt";
    fs::path solverBin = projectDir / "project";

    std::error_code ec;
    fs::create_directories(finalOutputDir, ec);

    if (!fs::exists(testsFile))
    {
        std::cerr << "Missing input file: " << testsFile << "\n";
        return 1;
    }

    std::vector<TestCase> tests = loadTests(testsFile);
    if (tests.empty())
    {
        std::cerr << "No tests loaded from: " << testsFile << "\n";
        return 1;
    }

    // Build solver binary from current project.cpp before running tests.
    {
        std::string buildCmd = "g++ -std=c++17 -O2 " + shellQuoteSingle((projectDir / "project.cpp").string()) +
                               " -o " + shellQuoteSingle(solverBin.string());
        int buildCode = std::system(buildCmd.c_str());
        if (buildCode != 0)
        {
            std::cerr << "Failed to build solver binary.\n";
            return 1;
        }
    }

    int version = nextVersion(finalOutputDir);
    std::string versionName = "v" + std::to_string(version);

    fs::path versionShotDir = finalOutputDir / versionName;
    fs::create_directories(versionShotDir, ec);

    fs::path versionRawDir = finalOutputDir / (versionName + "_raw");
    fs::create_directories(versionRawDir, ec);

    fs::path mdPath = finalOutputDir / (versionName + ".md");

    std::vector<RunResult> results;
    results.reserve(tests.size());

    int pass = 0;
    int fail = 0;

    for (const TestCase &t : tests)
    {
        std::string converted = preprocessExpression(t.expression);
        std::string exprQ = shellQuoteSingle(converted);
        std::string solverQ = shellQuoteSingle(solverBin.string());

        // Feed expression then blank line so solver prints simplified expression for symbolic cases.
        std::string runCmd = "printf %b " + shellQuoteSingle(converted + "\\n\\n") + " | " + solverQ + " 2>&1";
        std::string output = readCommandOutput(runCmd);

        fs::path rawPath = versionRawDir / ("test_" + std::to_string(t.id) + ".txt");
        std::ofstream rawOut(rawPath);
        rawOut << output;

        fs::path shotPath = versionShotDir / ("test_" + std::to_string(t.id) + ".png");
        bool shotOk = makeScreenshot(rawPath, shotPath);
        (void)shotOk;

        std::string observedSymbolic = getLineValue(output, "Final Simplified Expression:");
        if (observedSymbolic.empty())
            observedSymbolic = getLineValue(output, "Stringified Result:");

        std::string observedNumeric = getLineValue(output, "Numeric Result   :");
        std::string observedError = getLastErrorLikeLine(output);

        std::string status = "FAIL";
        std::string notes;

        std::string expected = t.expected;
        std::string expectedLower = expected;
        for (char &c : expectedLower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (expectedLower.find("undefined") != std::string::npos || expectedLower.find("error") != std::string::npos)
        {
            if (!observedError.empty())
            {
                status = "PASS";
                notes = "Error-like output found as expected.";
            }
            else
            {
                notes = "Expected undefined/error but none found.";
            }
        }
        else if (isNumericToken(expected) && !observedNumeric.empty() && isNumericToken(observedNumeric))
        {
            double e = std::stod(expected);
            double o = std::stod(observedNumeric);
            if (almostEqual(e, o))
            {
                status = "PASS";
                notes = "Numeric match.";
            }
            else
            {
                notes = "Numeric mismatch.";
            }
        }
        else
        {
            if (normalizeSymbolic(expected) == normalizeSymbolic(observedSymbolic))
            {
                status = "PASS";
                notes = "Symbolic match (normalized).";
            }
            else if (!observedSymbolic.empty() && areExpressionsEquivalentBySampling(solverBin, expected, observedSymbolic))
            {
                status = "PASS";
                notes = "Symbolic arrangement differs; sampled numeric equivalence confirmed.";
            }
            else
            {
                notes = "Symbolic mismatch.";
            }
        }

        if (status == "PASS")
            pass++;
        else
            fail++;

        results.push_back({t.id, t.expression, converted, t.expected, observedSymbolic, observedNumeric,
                           observedError, status, notes, rawPath, shotPath});
    }

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tmLocal{};
#ifdef _WIN32
    localtime_s(&tmLocal, &t);
#else
    localtime_r(&t, &tmLocal);
#endif

    std::ostringstream ts;
    ts << std::put_time(&tmLocal, "%Y-%m-%d %H:%M:%S");

    std::ofstream md(mdPath);
    md << "# Final Expression Test Report " << versionName << "\n\n";
    md << "Generated: " << ts.str() << "\n\n";
    md << "Input file: `output/Final tests/testing expressions.txt`\n\n";
    md << "## Summary\n";
    md << "- Total: " << results.size() << "\n";
    md << "- PASS: " << pass << "\n";
    md << "- FAIL: " << fail << "\n\n";
    md << "## Results\n\n";
    md << "| # | Expression | Converted Expression | Expected | Observed Symbolic | Observed Numeric | Status | Notes | Raw | Screenshot |\n";
    md << "|---:|---|---|---|---|---|---|---|---|---|\n";

    for (const RunResult &r : results)
    {
        fs::path rawRel = fs::relative(r.rawPath, finalOutputDir);
        fs::path shotRel = fs::relative(r.screenshotPath, finalOutputDir);

        auto esc = [](std::string s) {
            s = replaceAll(s, "|", "\\|");
            return s;
        };

        md << "| " << r.id
           << " | " << esc(r.originalExpression)
           << " | " << esc(r.convertedExpression)
           << " | " << esc(r.expected)
           << " | " << esc(r.observedSymbolic)
           << " | " << esc(r.observedNumeric)
           << " | " << r.status
           << " | " << esc(r.notes)
           << " | [" << rawRel.string() << "](" << rawRel.string() << ")"
           << " | [" << shotRel.string() << "](" << shotRel.string() << ") |\n";
    }

    md << "\n## Notes\n";
    md << "- Expressions are preprocessed before execution (for example token `pi` and `sqrt(...)`).\n";
    md << "- Screenshots are generated as PNG files in folder `" << versionName << "`.\n";
    md << "- Raw command outputs are stored in folder `" << versionName << "_raw`.\n";

    std::cout << "Done.\n";
    std::cout << "Version folder: " << versionShotDir << "\n";
    std::cout << "Report: " << mdPath << "\n";
    std::cout << "Total=" << results.size() << " PASS=" << pass << " FAIL=" << fail << "\n";
    return 0;
}
