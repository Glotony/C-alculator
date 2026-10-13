#include <cmath>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

using Func = std::function<double(double)>;

// ============================================================
// CALCULATOR MATHEMATICS — YOUR ORIGINAL OPERATIONS
// ============================================================

std::string numberText(double value)
{
    std::ostringstream out;
    out << std::setprecision(10) << value;
    return out.str();
}

// Basic arithmetic
double add(double a, double b)      { return a + b; }
double subtract(double a, double b) { return a - b; }
double multiply(double a, double b) { return a * b; }
double divide(double a, double b)   { return a / b; }
double power(double a, double b)    { return std::pow(a, b); }
double squareRoot(double a)         { return std::sqrt(a); }
double modulo(double a, double b)   { return std::fmod(a, b); }

// First derivative: central difference
double derivative(const Func& f, double x, double h = 1e-5)
{
    return (f(x + h) - f(x - h)) / (2.0 * h);
}

// Second derivative
double secondDerivative(const Func& f, double x, double h = 1e-4)
{
    return (f(x + h) - 2.0 * f(x) + f(x - h)) / (h * h);
}

// Nth derivative
double nthDerivative(const Func& f, double x, int n)
{
    if (n == 0)
        return f(x);

    double h = std::pow(2.2e-16, 1.0 / (n + 2));
    double sum = 0.0;
    double binom = 1.0;

    for (int k = 0; k <= n; ++k)
    {
        double sign = (k % 2 == 0) ? 1.0 : -1.0;

        sum += sign * binom *
               f(x + (n / 2.0 - k) * h);

        binom = binom * (n - k) / (k + 1);
    }

    return sum / std::pow(h, n);
}

// Trapezoidal integration
double trapezoidal(const Func& f, double a, double b, int n = 1000)
{
    if (n < 1)
        n = 1;

    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));

    for (int i = 1; i < n; ++i)
        sum += f(a + i * h);

    return sum * h;
}

// Simpson's integration
double simpson(const Func& f, double a, double b, int n = 1000)
{
    if (n < 2)
        n = 2;

    if (n % 2 != 0)
        ++n;

    double h = (b - a) / n;
    double sum = f(a) + f(b);

    for (int i = 1; i < n; ++i)
    {
        sum += (i % 2 == 0 ? 2.0 : 4.0) * f(a + i * h);
    }

    return sum * h / 3.0;
}

// Numerical limit, checking both sides
bool limit(
    const Func& f,
    double approach,
    double& result,
    std::string& note,
    double h = 1e-6)
{
    double left = f(approach - h);
    double right = f(approach + h);

    if (!std::isfinite(left) || !std::isfinite(right))
    {
        note = "The function is undefined or diverges near this point.";
        return false;
    }

    if (std::fabs(left - right) >
        1e-3 * (1.0 + std::fabs(left) + std::fabs(right)))
    {
        std::ostringstream out;
        out << "Left and right limits differ. Left = "
            << left << ", right = " << right;

        note = out.str();
        return false;
    }

    result = (left + right) / 2.0;
    return true;
}

// Taylor-series approximation
double taylorSeries(const Func& f, double x, double a, int n)
{
    double result = 0.0;
    double factorial = 1.0;

    for (int i = 0; i <= n; ++i)
    {
        if (i > 0)
            factorial *= i;

        result += nthDerivative(f, a, i) *
                  std::pow(x - a, i) / factorial;
    }

    return result;
}

// Newton's method.
// The GUI displays errors instead of writing them to cout.
double newtonMethod(
    const Func& f,
    double x0,
    bool& converged,
    std::string& note,
    double tolerance = 1e-9,
    int maxIter = 100)
{
    double x = x0;
    converged = false;
    note.clear();

    for (int i = 0; i < maxIter; ++i)
    {
        double fx = f(x);
        double dfx = derivative(f, x);

        if (!std::isfinite(fx) || !std::isfinite(dfx))
        {
            note = "Function or derivative is undefined at the current estimate.";
            return x;
        }

        if (std::fabs(dfx) < 1e-10)
        {
            note = "Derivative is too small. Newton's method stopped.";
            return x;
        }

        double xNew = x - fx / dfx;

        if (!std::isfinite(xNew))
        {
            note = "Newton's method produced a non-finite estimate.";
            return x;
        }

        if (std::fabs(xNew - x) < tolerance)
        {
            converged = true;
            note = "Newton's method converged.";
            return xNew;
        }

        x = xNew;
    }

    note = "Maximum iterations reached without convergence.";
    return x;
}

// Arc length
double arcLength(const Func& f, double a, double b, int n = 1000)
{
    auto integrand = [&f](double x)
    {
        double d = derivative(f, x);
        return std::sqrt(1.0 + d * d);
    };

    return simpson(integrand, a, b, n);
}

// ============================================================
// ALL 12 PREDEFINED FUNCTIONS
// ============================================================

double f1(double x)  { return x * x; }
double f2(double x)  { return std::sin(x); }
double f3(double x)  { return std::exp(x); }
double f4(double x)  { return std::log(x); }
double f5(double x)  { return 1.0 / x; }
double f6(double x)  { return x * x * x + 2 * x * x + 3 * x + 4; }
double f7(double x)  { return std::cos(x); }
double f8(double x)  { return std::tan(x); }
double f9(double x)  { return std::sqrt(x); }
double f10(double x) { return 1.0 / (1.0 + std::exp(-x)); }
double f11(double x) { return x * std::exp(x); }
double f12(double x) { return std::sin(x) * std::cos(x); }

const char* FUNCTION_NAMES[] =
{
    "x^2",
    "sin(x)",
    "e^x",
    "ln(x)",
    "1/x",
    "x^3 + 2x^2 + 3x + 4",
    "cos(x)",
    "tan(x)",
    "sqrt(x)",
    "sigmoid(x)",
    "x * e^x",
    "sin(x) * cos(x)"
};

Func getFunction(int index)
{
    switch (index)
    {
        case 0:  return f1;
        case 1:  return f2;
        case 2:  return f3;
        case 3:  return f4;
        case 4:  return f5;
        case 5:  return f6;
        case 6:  return f7;
        case 7:  return f8;
        case 8:  return f9;
        case 9:  return f10;
        case 10: return f11;
        case 11: return f12;
        default: return f1;
    }
}

// ============================================================
// GUI APPLICATION
// ============================================================

int main()
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "Failed to initialize GLFW.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(
        1100,
        760,
        "MathLab | Basic + Calculus",
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::fprintf(stderr, "Failed to create the application window.\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    // Dark desktop-app appearance
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 10.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowPadding = ImVec2(18.0f, 18.0f);
    style.FramePadding = ImVec2(10.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 10.0f);
    style.WindowBorderSize = 0.0f;

    style.Colors[ImGuiCol_WindowBg] =
        ImVec4(0.055f, 0.065f, 0.085f, 1.0f);

    style.Colors[ImGuiCol_ChildBg] =
        ImVec4(0.075f, 0.085f, 0.110f, 1.0f);

    style.Colors[ImGuiCol_FrameBg] =
        ImVec4(0.13f, 0.15f, 0.19f, 1.0f);

    style.Colors[ImGuiCol_Button] =
        ImVec4(0.20f, 0.32f, 0.55f, 1.0f);

    style.Colors[ImGuiCol_ButtonHovered] =
        ImVec4(0.27f, 0.42f, 0.70f, 1.0f);

    style.Colors[ImGuiCol_ButtonActive] =
        ImVec4(0.16f, 0.27f, 0.47f, 1.0f);

    // Initialize Dear ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Screen selection: 0 = basic, 1 = calculus
    int page = 0;

    // Basic calculator state
    int basicOperation = 0;
    double numA = 12.0;
    double numB = 4.0;

    // Calculus calculator state
    int calculusOperation = 0;
    int functionIndex = 0;

    double x = 2.0;
    double lower = 0.0;
    double upper = 1.0;
    double approach = 1.0;
    double center = 0.0;
    double initialGuess = 1.0;

    int intervals = 1000;
    int taylorDegree = 4;

    // Shared result panel state
    double lastResult = 0.0;
    bool resultReady = false;
    bool resultIsError = false;

    std::string resultLabel;
    std::string status;

    const char* BASIC_OPERATIONS[] =
    {
        "Addition (+)",
        "Subtraction (-)",
        "Multiplication (*)",
        "Division (/)",
        "Power (a^b)",
        "Modulo (%)",
        "Square root"
    };

    const char* CALCULUS_OPERATIONS[] =
    {
        "First derivative",
        "Second derivative",
        "Trapezoidal integral",
        "Simpson's integral",
        "Limit approximation",
        "Taylor series",
        "Newton's root finding",
        "Arc length"
    };

    auto reportError = [&](const std::string& message)
    {
        resultReady = false;
        resultIsError = true;
        resultLabel.clear();
        status = message;
    };

    auto setResult = [&](double value, const std::string& label)
    {
        if (!std::isfinite(value))
        {
            reportError(
                "Result is undefined or outside the finite numeric range."
            );
            return;
        }

        lastResult = value;
        resultLabel = label;
        resultReady = true;
        resultIsError = false;
        status.clear();
    };

    // --------------------------------------------------------
    // BASIC CALCULATOR: uses your original arithmetic functions
    // --------------------------------------------------------

    auto calculateBasic = [&]()
    {
        switch (basicOperation)
        {
            case 0:
                setResult(
                    add(numA, numB),
                    numberText(numA) + " + " + numberText(numB)
                );
                break;

            case 1:
                setResult(
                    subtract(numA, numB),
                    numberText(numA) + " - " + numberText(numB)
                );
                break;

            case 2:
                setResult(
                    multiply(numA, numB),
                    numberText(numA) + " * " + numberText(numB)
                );
                break;

            case 3:
                if (numB == 0.0)
                {
                    reportError("Division by zero is not allowed.");
                    break;
                }

                setResult(
                    divide(numA, numB),
                    numberText(numA) + " / " + numberText(numB)
                );
                break;

            case 4:
                setResult(
                    power(numA, numB),
                    numberText(numA) + " ^ " + numberText(numB)
                );
                break;

            case 5:
                if (numB == 0.0)
                {
                    reportError("Modulo by zero is not allowed.");
                    break;
                }

                setResult(
                    modulo(numA, numB),
                    numberText(numA) + " % " + numberText(numB)
                );
                break;

            case 6:
                if (numA < 0.0)
                {
                    reportError(
                        "Cannot calculate the real square root of a negative number."
                    );
                    break;
                }

                setResult(
                    squareRoot(numA),
                    "sqrt(" + numberText(numA) + ")"
                );
                break;

            default:
                reportError("Please select a valid arithmetic operation.");
                break;
        }
    };

    // --------------------------------------------------------
    // CALCULUS CALCULATOR: uses your original calculus functions
    // --------------------------------------------------------

    auto calculateCalculus = [&]()
    {
        Func f = getFunction(functionIndex);

        auto checkIntervals = [&]()
        {
            if (intervals < 1 || intervals > 1000000)
            {
                reportError(
                    "Intervals must be between 1 and 1,000,000."
                );
                return false;
            }

            return true;
        };

        switch (calculusOperation)
        {
            case 0:
                setResult(
                    derivative(f, x),
                    "First derivative at x = " + numberText(x)
                );
                break;

            case 1:
                setResult(
                    secondDerivative(f, x),
                    "Second derivative at x = " + numberText(x)
                );
                break;

            case 2:
                if (!checkIntervals())
                    return;

                setResult(
                    trapezoidal(f, lower, upper, intervals),
                    "Trapezoidal integration"
                );
                break;

            case 3:
                if (!checkIntervals())
                    return;

                setResult(
                    simpson(f, lower, upper, intervals),
                    "Simpson's integration"
                );
                break;

            case 4:
            {
                double value = 0.0;
                std::string note;

                if (!limit(f, approach, value, note))
                {
                    reportError(note);
                    return;
                }

                setResult(
                    value,
                    "Limit as x approaches " + numberText(approach)
                );
                break;
            }

            case 5:
                if (taylorDegree < 0 || taylorDegree > 8)
                {
                    reportError(
                        "Taylor degree must be between 0 and 8."
                    );
                    return;
                }

                setResult(
                    taylorSeries(f, x, center, taylorDegree),
                    "Taylor approximation at x = " + numberText(x)
                );
                break;

            case 6:
            {
                bool converged = false;
                std::string note;

                double root = newtonMethod(
                    f, initialGuess, converged, note
                );

                if (!std::isfinite(root))
                {
                    reportError(note.empty()
                        ? "Newton's method produced an invalid estimate."
                        : note);
                    return;
                }

                lastResult = root;
                resultLabel = "Newton's method estimate";
                resultReady = true;
                resultIsError = !converged;
                status = note;
                break;
            }

            case 7:
                if (!checkIntervals())
                    return;

                setResult(
                    arcLength(f, lower, upper, intervals),
                    "Arc length"
                );
                break;

            default:
                reportError("Please select a valid calculus operation.");
                break;
        }
    };

    // ========================================================
    // MAIN GUI LOOP
    // ========================================================

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main application fills the desktop window
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        ImGuiWindowFlags rootFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("MathLabRoot", nullptr, rootFlags);

        // LEFT SIDEBAR
        ImGui::BeginChild(
            "Sidebar",
            ImVec2(220.0f, 0.0f),
            ImGuiChildFlags_Borders
        );

        ImGui::Spacing();
        ImGui::TextColored(
            ImVec4(0.45f, 0.68f, 1.0f, 1.0f),
            "MATHLAB"
        );

        ImGui::TextDisabled("CALCULATOR SUITE");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("WORKSPACES");
        ImGui::Spacing();

        if (ImGui::Button("Basic Calculator", ImVec2(-1.0f, 44.0f)))
        {
            page = 0;
        }

        if (ImGui::Button("Calculus Calculator", ImVec2(-1.0f, 44.0f)))
        {
            page = 1;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("ENGINE");
        ImGui::TextWrapped("C++17");
        ImGui::TextWrapped("Dear ImGui");
        ImGui::TextWrapped("GLFW + OpenGL");

        ImGui::EndChild();

        ImGui::SameLine(0.0f, 16.0f);

        // RIGHT WORKSPACE
        ImGui::BeginChild(
            "Workspace",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_None
        );

        if (page == 0)
        {
            ImGui::TextUnformatted("Basic Calculator");
            ImGui::TextDisabled(
                "Arithmetic operations with immediate results."
            );

            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SetNextItemWidth(370.0f);

            ImGui::Combo(
                "Operation",
                &basicOperation,
                BASIC_OPERATIONS,
                IM_ARRAYSIZE(BASIC_OPERATIONS)
            );

            ImGui::Spacing();

            ImGui::SetNextItemWidth(370.0f);
            ImGui::InputDouble(
                "First number",
                &numA,
                0.1,
                1.0,
                "%.10g"
            );

            if (basicOperation != 6)
            {
                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble(
                    "Second number",
                    &numB,
                    0.1,
                    1.0,
                    "%.10g"
                );
            }

            ImGui::Spacing();

            if (ImGui::Button("Calculate", ImVec2(180.0f, 44.0f)))
            {
                calculateBasic();
            }
        }
        else
        {
            ImGui::TextUnformatted("Calculus Calculator");
            ImGui::TextDisabled(
                "Numerical differentiation, integration and analysis."
            );

            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SetNextItemWidth(370.0f);

            ImGui::Combo(
                "Operation##calculus",
                &calculusOperation,
                CALCULUS_OPERATIONS,
                IM_ARRAYSIZE(CALCULUS_OPERATIONS)
            );

            ImGui::Spacing();

            ImGui::SetNextItemWidth(470.0f);

            ImGui::Combo(
                "Function",
                &functionIndex,
                FUNCTION_NAMES,
                IM_ARRAYSIZE(FUNCTION_NAMES)
            );

            ImGui::TextDisabled(
                "Selected function: %s",
                FUNCTION_NAMES[functionIndex]
            );

            ImGui::Spacing();

            // Show only the inputs needed for the selected operation.
            if (calculusOperation == 0 || calculusOperation == 1)
            {
                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble("x value", &x, 0.1, 1.0, "%.10g");
            }
            else if (calculusOperation == 2 ||
                     calculusOperation == 3 ||
                     calculusOperation == 7)
            {
                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble(
                    "Lower limit (a)", &lower, 0.1, 1.0, "%.10g"
                );

                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble(
                    "Upper limit (b)", &upper, 0.1, 1.0, "%.10g"
                );

                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputInt("Intervals (n)", &intervals);
            }
            else if (calculusOperation == 4)
            {
                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble(
                    "Approach point", &approach, 0.1, 1.0, "%.10g"
                );
            }
            else if (calculusOperation == 5)
            {
                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble(
                    "Evaluate at x", &x, 0.1, 1.0, "%.10g"
                );

                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble(
                    "Center point (a)", &center, 0.1, 1.0, "%.10g"
                );

                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputInt("Taylor degree (0-8)", &taylorDegree);
            }
            else if (calculusOperation == 6)
            {
                ImGui::SetNextItemWidth(370.0f);
                ImGui::InputDouble(
                    "Initial guess (x0)",
                    &initialGuess,
                    0.1,
                    1.0,
                    "%.10g"
                );
            }

            ImGui::Spacing();

            if (ImGui::Button("Calculate", ImVec2(180.0f, 44.0f)))
            {
                calculateCalculus();
            }
        }

        // SHARED RESULT PANEL
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild(
            "ResultPanel",
            ImVec2(0.0f, 150.0f),
            ImGuiChildFlags_Borders
        );

        ImGui::TextDisabled("RESULT");
        ImGui::Spacing();

        if (resultReady)
        {
            ImGui::TextColored(
                ImVec4(0.43f, 0.88f, 0.68f, 1.0f),
                "%.10g",
                lastResult
            );

            ImGui::TextWrapped("%s", resultLabel.c_str());
        }
        else if (status.empty())
        {
            ImGui::TextDisabled(
                "Choose an operation, enter your values, and press Calculate."
            );
        }

        if (!status.empty())
        {
            ImVec4 statusColor = resultIsError
                ? ImVec4(1.0f, 0.38f, 0.38f, 1.0f)
                : ImVec4(0.43f, 0.88f, 0.68f, 1.0f);

            ImGui::TextColored(
                statusColor,
                "%s",
                status.c_str()
            );
        }

        ImGui::EndChild();
        ImGui::EndChild();
        ImGui::End();

        // Render the frame through OpenGL
        ImGui::Render();

        int displayWidth = 0;
        int displayHeight = 0;

        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);

        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.055f, 0.065f, 0.085f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Clean shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
