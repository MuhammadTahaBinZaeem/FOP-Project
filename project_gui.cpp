#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#define main solver_console_main
#include "project.cpp"
#undef main

#include <windows.h>

struct AppState
{
    HWND inputEdit = NULL;
    HWND statusText = NULL;
    HWND simplifyButton = NULL;
    Node *root = NULL;
    std::string simplifiedExpression;
    std::string variableName;
    std::string errorMessage;
    bool canAnimate = false;
    double phase = 0.0;
};

static const wchar_t *kWindowClassName = L"ProjectWaveVisualizerWindow";
static const wchar_t *kWindowTitle = L"Project Wave Visualizer";
static const int kInputId = 1001;
static const int kButtonId = 1002;
static const int kTimerId = 1003;

static AppState *getState(HWND hwnd)
{
    return reinterpret_cast<AppState *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

static void setStatus(AppState *state, const std::string &text)
{
    state->errorMessage = text;
    if (state->statusText != NULL)
        SetWindowTextA(state->statusText, text.c_str());
}

static void destroyTree(Node *node)
{
    if (node == NULL)
        return;
    destroyTree(node->left);
    destroyTree(node->right);
    delete node;
}

static bool buildSimplifiedTree(const std::string &input, Node *&root, std::string &errorMessage)
{
    bool ok = true;
    std::string err;
    std::string preprocessedInput = preprocessInputExpression(input);

    std::vector<Token> rawTokens = lexicalAnalysis(preprocessedInput, ok, err);
    if (!ok)
    {
        errorMessage = err;
        return false;
    }

    std::vector<Token> normalizedTokens = normalizeTokens(rawTokens);
    std::vector<Token> postfix = shuntingYardToPostfix(normalizedTokens, ok, err);
    if (!ok)
    {
        errorMessage = err;
        return false;
    }

    root = buildASTFromPostfix(postfix, ok, err);
    if (!ok || root == NULL)
    {
        errorMessage = err;
        return false;
    }

    bool anyChange = true;
    int iteration = 0;
    while (anyChange && iteration < 10)
    {
        iteration++;
        anyChange = false;

        bool changed = false;
        root = constantFolding(root, changed);
        anyChange = anyChange || changed;

        changed = false;
        root = identityReduction(root, changed);
        anyChange = anyChange || changed;

        changed = false;
        root = deadCodeElimination(root, changed);
        anyChange = anyChange || changed;

        changed = false;
        root = strengthReduction(root, changed);
        anyChange = anyChange || changed;

        changed = false;
        root = algebraicSimplification(root, changed);
        anyChange = anyChange || changed;
    }

    bool cleanupChange = true;
    int cleanupIter = 0;
    while (cleanupChange && cleanupIter < 6)
    {
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

    {
        std::string roundTripExpr = treeToString(root);
        bool rtOk = true;
        std::string rtErr;
        std::vector<Token> rtRaw = lexicalAnalysis(roundTripExpr, rtOk, rtErr);
        if (rtOk)
        {
            std::vector<Token> rtNorm = normalizeTokens(rtRaw);
            std::vector<Token> rtPost = shuntingYardToPostfix(rtNorm, rtOk, rtErr);
            if (rtOk)
            {
                Node *rtRoot = buildASTFromPostfix(rtPost, rtOk, rtErr);
                if (rtOk && rtRoot != NULL)
                {
                    root = rtRoot;

                    bool rtAny = true;
                    int rtIter = 0;
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

    {
        bool improved = true;
        int wave = 0;
        while (improved && wave < 4)
        {
            wave++;
            improved = false;

            int expandableCount = countExpandablePowerNodes(root);
            if (expandableCount <= 0)
                break;

            Node *bestRoot = root;
            int bestScore = expressionComplexityScore(root);
            std::string bestText = treeToString(root);

            for (int idx = 0; idx < expandableCount; idx++)
            {
                int seen = 0;
                Node *candidate = cloneWithExpandedPowerAtIndex(root, idx, seen);
                candidate = runBoundedOptimizationCycle(candidate);

                int candScore = expressionComplexityScore(candidate);
                std::string candText = treeToString(candidate);

                bool strictlyBetter = false;
                if (candScore < bestScore)
                {
                    strictlyBetter = true;
                }
                else if (candScore == bestScore && candText.size() < bestText.size())
                {
                    strictlyBetter = true;
                }

                if (strictlyBetter && !areTreesEqual(candidate, bestRoot))
                {
                    bestRoot = candidate;
                    bestScore = candScore;
                    bestText = candText;
                    improved = true;
                }
            }

            if (improved)
                root = bestRoot;
        }
    }

    if (hasConstantDivisionByZero(root))
    {
        errorMessage = "Division by zero detected in expression";
        return false;
    }

    errorMessage.clear();
    return true;
}

static double mapToScreenY(double value, double minY, double maxY, int top, int bottom)
{
    if (std::fabs(maxY - minY) < 1e-9)
        return (top + bottom) / 2.0;
    double t = (maxY - value) / (maxY - minY);
    return top + t * (bottom - top);
}

static void drawWave(HDC hdc, RECT clientRect, AppState *state)
{
    const int marginLeft = 24;
    const int marginRight = 24;
    const int marginTop = 130;
    const int marginBottom = 34;

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    int graphLeft = marginLeft;
    int graphRight = width - marginRight;
    int graphTop = marginTop;
    int graphBottom = height - marginBottom;

    HBRUSH bg = CreateSolidBrush(RGB(10, 14, 24));
    FillRect(hdc, &clientRect, bg);
    DeleteObject(bg);

    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(31, 43, 63));
    HPEN axisPen = CreatePen(PS_SOLID, 1, RGB(96, 120, 168));
    HPEN wavePen = CreatePen(PS_SOLID, 2, RGB(48, 219, 234));
    HPEN waveGlow = CreatePen(PS_SOLID, 4, RGB(18, 90, 104));

    HGDIOBJ oldPen = SelectObject(hdc, gridPen);

    for (int y = graphTop; y <= graphBottom; y += 40)
        MoveToEx(hdc, graphLeft, y, NULL), LineTo(hdc, graphRight, y);
    for (int x = graphLeft; x <= graphRight; x += 80)
        MoveToEx(hdc, x, graphTop, NULL), LineTo(hdc, x, graphBottom);

    SelectObject(hdc, axisPen);

    if (graphBottom > graphTop)
    {
        MoveToEx(hdc, graphLeft, (graphTop + graphBottom) / 2, NULL);
        LineTo(hdc, graphRight, (graphTop + graphBottom) / 2);
    }
    if (graphRight > graphLeft)
    {
        MoveToEx(hdc, (graphLeft + graphRight) / 2, graphTop, NULL);
        LineTo(hdc, (graphLeft + graphRight) / 2, graphBottom);
    }

    if (state->root == NULL)
    {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(234, 239, 244));
        TextOutA(hdc, 24, 24, "Enter an expression and click Simplify & Animate.", 49);
        SelectObject(hdc, oldPen);
        DeleteObject(gridPen);
        DeleteObject(axisPen);
        DeleteObject(wavePen);
        DeleteObject(waveGlow);
        return;
    }

    std::vector<std::string> functionNames = {"sin", "cos", "tan", "cot", "sec", "csc"};
    std::set<std::string> trigFunctions(functionNames.begin(), functionNames.end());
    double pi = std::acos(-1.0);
    double minX = -8.0;
    double maxX = 8.0;
    if (treeContainsFunction(state->root, trigFunctions))
    {
        minX = -2.0 * pi;
        maxX = 2.0 * pi;
    }

    int plotWidth = std::max(2, graphRight - graphLeft);
    std::vector<POINT> points;
    std::vector<POINT> segment;
    double minY = 0.0;
    double maxY = 0.0;
    bool haveValues = false;

    std::vector<double> values;
    std::vector<bool> valid;
    values.reserve(plotWidth);
    valid.reserve(plotWidth);

    for (int i = 0; i < plotWidth; i++)
    {
        double x = minX + (maxX - minX) * (double)i / (double)(plotWidth - 1);
        std::map<std::string, double> vars;
        if (!state->variableName.empty())
            vars[state->variableName] = x + state->phase;

        bool ok = true;
        std::string err;
        double y = evaluateTree(state->root, vars, ok, err);
        if (ok && std::isfinite(y) && std::fabs(y) < 1e6)
        {
            values.push_back(y);
            valid.push_back(true);
            if (!haveValues)
            {
                minY = maxY = y;
                haveValues = true;
            }
            else
            {
                if (y < minY)
                    minY = y;
                if (y > maxY)
                    maxY = y;
            }
        }
        else
        {
            values.push_back(0.0);
            valid.push_back(false);
        }
    }

    if (!haveValues)
    {
        SelectObject(hdc, oldPen);
        DeleteObject(gridPen);
        DeleteObject(axisPen);
        DeleteObject(wavePen);
        DeleteObject(waveGlow);
        return;
    }

    if (std::fabs(maxY - minY) < 1e-9)
    {
        minY -= 1.0;
        maxY += 1.0;
    }
    else
    {
        double pad = std::max(1.0, (maxY - minY) * 0.15);
        minY -= pad;
        maxY += pad;
    }

    int axisY = -1;
    if (minY <= 0.0 && maxY >= 0.0)
        axisY = (int)std::round(mapToScreenY(0.0, minY, maxY, graphTop, graphBottom));

    SelectObject(hdc, waveGlow);

    for (int i = 0; i < plotWidth; i++)
    {
        if (!valid[i])
        {
            if (segment.size() >= 2)
                Polyline(hdc, segment.data(), (int)segment.size());
            segment.clear();
            continue;
        }

        double y = values[i];
        int sx = graphLeft + i;
        int sy = (int)std::round(mapToScreenY(y, minY, maxY, graphTop, graphBottom));

        if (!segment.empty())
        {
            double previousY = values[i - 1];
            if (valid[i - 1] && std::fabs(previousY - y) > (maxY - minY) * 0.65)
            {
                if (segment.size() >= 2)
                    Polyline(hdc, segment.data(), (int)segment.size());
                segment.clear();
            }
        }

        segment.push_back(POINT{sx, sy});
    }

    if (segment.size() >= 2)
        Polyline(hdc, segment.data(), (int)segment.size());

    SelectObject(hdc, wavePen);
    segment.clear();

    for (int i = 0; i < plotWidth; i++)
    {
        if (!valid[i])
        {
            if (segment.size() >= 2)
                Polyline(hdc, segment.data(), (int)segment.size());
            segment.clear();
            continue;
        }

        double y = values[i];
        int sx = graphLeft + i;
        int sy = (int)std::round(mapToScreenY(y, minY, maxY, graphTop, graphBottom));

        if (!segment.empty())
        {
            double previousY = values[i - 1];
            if (valid[i - 1] && std::fabs(previousY - y) > (maxY - minY) * 0.65)
            {
                if (segment.size() >= 2)
                    Polyline(hdc, segment.data(), (int)segment.size());
                segment.clear();
            }
        }

        segment.push_back(POINT{sx, sy});
    }

    if (segment.size() >= 2)
        Polyline(hdc, segment.data(), (int)segment.size());

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(237, 242, 247));

    HFONT titleFont = CreateFontA(26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    HFONT bodyFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    HFONT monoFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 FIXED_PITCH | FF_MODERN, "Consolas");

    HGDIOBJ oldFont = SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(246, 249, 252));
    TextOutA(hdc, 24, 20, "Project Wave Visualizer", 23);

    SelectObject(hdc, bodyFont);
    SetTextColor(hdc, RGB(165, 185, 208));
    TextOutA(hdc, 24, 56, "Enter an equation, simplify it, and watch the curve move in real time.", 74);

    std::string displayExpr = state->simplifiedExpression.empty() ? "Simplified: (waiting for input)" : "Simplified: " + state->simplifiedExpression;
    TextOutA(hdc, 24, 82, displayExpr.c_str(), (int)displayExpr.size());

    std::string motionText = "Phase offset: " + std::to_string(state->phase);
    TextOutA(hdc, 24, 106, motionText.c_str(), (int)motionText.size());

    if (!state->errorMessage.empty())
    {
        SetTextColor(hdc, RGB(255, 151, 151));
        TextOutA(hdc, 420, 82, state->errorMessage.c_str(), (int)state->errorMessage.size());
    }

    if (!state->variableName.empty())
    {
        std::string varText = "Variable: " + state->variableName;
        SetTextColor(hdc, RGB(120, 233, 220));
        TextOutA(hdc, 420, 106, varText.c_str(), (int)varText.size());
    }

    SelectObject(hdc, monoFont);
    SetTextColor(hdc, RGB(137, 255, 223));
    if (axisY >= graphTop && axisY <= graphBottom)
    {
        std::string axisText = "x-axis at y = 0";
        TextOutA(hdc, graphRight - 140, axisY - 18, axisText.c_str(), (int)axisText.size());
    }

    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
    DeleteObject(bodyFont);
    DeleteObject(monoFont);

    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
    DeleteObject(axisPen);
    DeleteObject(wavePen);
    DeleteObject(waveGlow);
}

static void simplifyCurrentExpression(HWND hwnd)
{
    AppState *state = getState(hwnd);
    if (state == NULL)
        return;

    char buffer[4096];
    GetWindowTextA(state->inputEdit, buffer, sizeof(buffer));
    std::string input = buffer;

    if (input.empty())
    {
        setStatus(state, "Enter an expression first.");
        if (state->root != NULL)
        {
            destroyTree(state->root);
            state->root = NULL;
        }
        state->simplifiedExpression.clear();
        state->variableName.clear();
        state->canAnimate = false;
        state->phase = 0.0;
        KillTimer(hwnd, kTimerId);
        InvalidateRect(hwnd, NULL, TRUE);
        return;
    }

    if (state->root != NULL)
    {
        destroyTree(state->root);
        state->root = NULL;
    }

    std::string errorMessage;
    if (!buildSimplifiedTree(input, state->root, errorMessage))
    {
        state->simplifiedExpression.clear();
        state->variableName.clear();
        state->canAnimate = false;
        state->phase = 0.0;
        KillTimer(hwnd, kTimerId);
        setStatus(state, "Simplify error: " + errorMessage);
        InvalidateRect(hwnd, NULL, TRUE);
        return;
    }

    std::set<std::string> vars;
    collectVariables(state->root, vars);
    state->simplifiedExpression = treeToString(state->root);
    state->variableName.clear();

    if (vars.size() <= 1)
    {
        if (!vars.empty())
            state->variableName = *vars.begin();
        state->canAnimate = true;
        state->phase = 0.0;
        SetTimer(hwnd, kTimerId, 16, NULL);
        setStatus(state, "Animating simplified wave preview.");
    }
    else
    {
        state->canAnimate = false;
        KillTimer(hwnd, kTimerId);
        setStatus(state, "Wave preview needs one variable or a constant expression.");
    }

    InvalidateRect(hwnd, NULL, TRUE);
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    AppState *state = getState(hwnd);

    switch (msg)
    {
    case WM_CREATE:
    {
        state = new AppState();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

        state->inputEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "sin(x)",
                                           WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                           24, 148, 620, 30, hwnd, (HMENU)kInputId,
                                           GetModuleHandle(NULL), NULL);
        state->simplifyButton = CreateWindowExA(0, "BUTTON", "Simplify & Animate",
                                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                                660, 146, 180, 34, hwnd, (HMENU)kButtonId,
                                                GetModuleHandle(NULL), NULL);
        state->statusText = CreateWindowExA(0, "STATIC", "Ready.",
                                            WS_CHILD | WS_VISIBLE,
                                            24, 188, 820, 22, hwnd, NULL,
                                            GetModuleHandle(NULL), NULL);

        SendMessageA(state->inputEdit, EM_SETLIMITTEXT, 4000, 0);
        SendMessageA(state->inputEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageA(state->simplifyButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageA(state->statusText, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

        SetFocus(state->inputEdit);
        SetTimer(hwnd, kTimerId, 16, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kButtonId)
            simplifyCurrentExpression(hwnd);
        return 0;
    case WM_TIMER:
        if (wParam == kTimerId && state != NULL && state->canAnimate)
        {
            state->phase += 0.14;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    case WM_SIZE:
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
        HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

        drawWave(memDC, clientRect, state);
        BitBlt(hdc, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        if (state != NULL)
        {
            destroyTree(state->root);
            delete state;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ProjectWaveVisualizerWindow";

    if (!RegisterClassA(&wc))
        return 1;

    HWND hwnd = CreateWindowExA(0, "ProjectWaveVisualizerWindow", "Project Wave Visualizer",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, 1000, 760,
                                NULL, NULL, instance, NULL);
    if (hwnd == NULL)
        return 1;

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}