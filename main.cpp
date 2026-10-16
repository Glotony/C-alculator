#include <cmath>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>
#include <cassert>
#include <utility>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

using Func = std::function<double(double)>;

namespace ErrorMessages
{
    inline constexpr char NonFiniteInput[] =
        "Input must be a finite number.";

    inline constexpr char NonFiniteResult[] =
        "Calculation produced NaN or infinity.";

    inline constexpr char InvalidFunction[] =
        "Invalid function selection.";

    inline constexpr char InvalidOperation[] =
        "Invalid operation selection.";

    inline constexpr char DivisionByZero[] =
        "Division by zero is not allowed.";

    inline constexpr char ModuloByZero[] =
        "Modulo by zero is not allowed.";

    inline constexpr char PowerDomain[] =
        "This power has no real-valued result for the given operands.";

    inline constexpr char ZeroToNonpositivePower[] =
        "Zero cannot be raised to a zero or negative power in this calculator.";

    inline constexpr char LogDomain[] =
        "ln(x) requires x > 0.";

    inline constexpr char ReciprocalDomain[] =
        "1/x is undefined at x = 0.";

    inline constexpr char SquareRootDomain[] =
        "sqrt(x) requires x >= 0.";

    inline constexpr char TangentPole[] =
        "tan(x) is at or too close to a singularity.";

    inline constexpr char InvalidBounds[] =
        "Bounds must be finite numbers.";

    inline constexpr char InvalidIntervals[] =
        "The interval count is invalid.";

    inline constexpr char SimpsonIntervals[] =
        "Simpson's rule requires an even interval count of at least 2.";

    inline constexpr char DerivativeFailed[] =
        "The derivative could not be evaluated reliably.";

    inline constexpr char SecondDerivativeFailed[] =
        "The second derivative could not be evaluated reliably.";

    inline constexpr char IntegrationFailed[] =
        "Integration failed because a sampled value was invalid.";

    inline constexpr char IntegrationSingularity[] =
        "The integration interval contains or crosses a singularity.";

    inline constexpr char LimitFailed[] =
        "The limit could not be established reliably.";

    inline constexpr char TaylorFailed[] =
        "Taylor approximation failed because a required derivative is invalid.";

    inline constexpr char NewtonInvalidGuess[] =
        "Newton's method requires a finite initial guess.";

    inline constexpr char NewtonDerivativeSmall[] =
        "Newton's method stopped because the derivative is too small.";

    inline constexpr char NewtonResidualTooLarge[] =
        "The step became small, but the function value is not close enough to zero.";

    inline constexpr char NewtonMaxIterations[] =
        "Newton's method did not converge within the iteration limit.";

    inline constexpr char ArcLengthFailed[] =
        "Arc length could not be evaluated reliably.";

    inline constexpr char ImGuiGlfwInit[] =
        "Failed to initialize the Dear ImGui GLFW backend.";

    inline constexpr char ImGuiOpenGLInit[] =
        "Failed to initialize the Dear ImGui OpenGL backend.";

    inline constexpr char UnexpectedError[] =
        "An unexpected error occurred while evaluating the operation.";
}

// ============================================================
// CALCULATOR MATHEMATICS — YOUR ORIGINAL OPERATIONS
// ============================================================

std::string numberText(double value)
{
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
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

// Analytic first/second derivatives are defined after evaluateFunction().

// Higher derivatives are calculated analytically for the 12 predefined functions.
double nthDerivativeByIndex(int index, double x, int n);
double taylorSeries(int functionIndex, double x, double center, int degree);

// Evaluate a quadrature sample and reject NaN/infinity immediately.
double finiteSample(const Func& f, double x)
{
    const double value = f(x);
    if (!std::isfinite(value))
        throw std::domain_error(ErrorMessages::IntegrationFailed);
    return value;
}

// Trapezoidal integration
double trapezoidal(const Func& f, double a, double b, int n = 1000)
{
    if (!std::isfinite(a) || !std::isfinite(b))
        throw std::invalid_argument(ErrorMessages::InvalidBounds);

    if (n < 1 || n > 1000000)
        throw std::invalid_argument(
            "Trapezoidal integration requires 1 to 1,000,000 intervals."
        );

    const double h = (b - a) / n;
    double sum = 0.5 * (finiteSample(f, a) + finiteSample(f, b));

    for (int i = 1; i < n; ++i)
        sum += finiteSample(f, a + i * h);

    const double result = sum * h;
    if (!std::isfinite(result))
        throw std::overflow_error(ErrorMessages::NonFiniteResult);
    return result;
}

// Simpson's integration: interval count must be even and at least 2.
double simpson(const Func& f, double a, double b, int n = 1000)
{
    if (!std::isfinite(a) || !std::isfinite(b))
        throw std::invalid_argument(ErrorMessages::InvalidBounds);

    if (n < 2 || n > 1000000 || n % 2 != 0)
        throw std::invalid_argument(ErrorMessages::SimpsonIntervals);

    const double h = (b - a) / n;
    double sum = finiteSample(f, a) + finiteSample(f, b);

    for (int i = 1; i < n; ++i)
    {
        const double weight = (i % 2 == 0) ? 2.0 : 4.0;
        sum += weight * finiteSample(f, a + i * h);
    }

    const double result = sum * h / 3.0;
    if (!std::isfinite(result))
        throw std::overflow_error(ErrorMessages::NonFiniteResult);
    return result;
}

// Adaptive Simpson integration with an estimated absolute error.
struct IntegrationResult
{
    double value = 0.0;
    double estimatedError = 0.0;
    int evaluations = 0;
    bool converged = false;
};

struct AdaptivePartial
{
    double value = 0.0;
    double error = 0.0;
    bool converged = false;
};

double simpsonEstimate(double a, double b, double fa, double fm, double fb)
{
    return (b - a) * (fa + 4.0 * fm + fb) / 6.0;
}

IntegrationResult adaptiveSimpson(
    const Func& f,
    double a,
    double b,
    int maxIntervals,
    double absoluteTolerance = 1e-9,
    double relativeTolerance = 1e-8)
{
    if (!std::isfinite(a) || !std::isfinite(b))
        throw std::invalid_argument(ErrorMessages::InvalidBounds);

    if (maxIntervals < 2 || maxIntervals > 1000000)
        throw std::invalid_argument(ErrorMessages::SimpsonIntervals);

    if (!std::isfinite(absoluteTolerance) || absoluteTolerance <= 0.0 ||
        !std::isfinite(relativeTolerance) || relativeTolerance < 0.0)
        throw std::invalid_argument("Integration tolerances are invalid.");

    if (a == b)
        return IntegrationResult{0.0, 0.0, 0, true};

    const bool reverse = a > b;
    if (reverse)
        std::swap(a, b);

    int evaluations = 0;
    auto eval = [&](double x)
    {
        ++evaluations;
        return finiteSample(f, x);
    };

    const double midpoint = a + (b - a) / 2.0;
    const double fa = eval(a);
    const double fm = eval(midpoint);
    const double fb = eval(b);
    const double whole = simpsonEstimate(a, b, fa, fm, fb);
    const double initialTolerance = absoluteTolerance + relativeTolerance * std::fabs(whole);

    // Use the existing interval input as a refinement budget. The binary subdivision
    // limit is rounded down so it never exceeds the requested budget by more than
    // the final two-panel estimate required to assess local error.
    int maxDepth = 0;
    int leaves = 1;
    while (leaves <= maxIntervals / 2)
    {
        leaves *= 2;
        ++maxDepth;
    }
    maxDepth = std::max(0, maxDepth - 1);

    std::function<AdaptivePartial(double, double, double, double, double, double, double, int)> recurse;
    recurse = [&](double left, double right, double fLeft, double fMid,
                  double fRight, double wholeEstimate, double tolerance, int depth) -> AdaptivePartial
    {
        const double mid = left + (right - left) / 2.0;
        const double leftMid = left + (mid - left) / 2.0;
        const double rightMid = mid + (right - mid) / 2.0;

        const double fLeftMid = eval(leftMid);
        const double fRightMid = eval(rightMid);

        const double leftEstimate = simpsonEstimate(left, mid, fLeft, fLeftMid, fMid);
        const double rightEstimate = simpsonEstimate(mid, right, fMid, fRightMid, fRight);
        const double refined = leftEstimate + rightEstimate;
        const double delta = refined - wholeEstimate;
        const double errorEstimate = std::fabs(delta) / 15.0;
        const double corrected = refined + delta / 15.0;

        if (!std::isfinite(corrected) || !std::isfinite(errorEstimate))
            throw std::overflow_error(ErrorMessages::NonFiniteResult);

        if (errorEstimate <= tolerance)
            return AdaptivePartial{corrected, errorEstimate, true};

        if (depth <= 0)
            return AdaptivePartial{corrected, errorEstimate, false};

        const AdaptivePartial leftPart = recurse(
            left, mid, fLeft, fLeftMid, fMid, leftEstimate, tolerance / 2.0, depth - 1);
        const AdaptivePartial rightPart = recurse(
            mid, right, fMid, fRightMid, fRight, rightEstimate, tolerance / 2.0, depth - 1);

        const double combinedValue = leftPart.value + rightPart.value;
        const double combinedError = leftPart.error + rightPart.error;
        if (!std::isfinite(combinedValue) || !std::isfinite(combinedError))
            throw std::overflow_error(ErrorMessages::NonFiniteResult);

        return AdaptivePartial{
            combinedValue,
            combinedError,
            leftPart.converged && rightPart.converged
        };
    };

    const AdaptivePartial partial = recurse(
        a, b, fa, fm, fb, whole, initialTolerance, maxDepth);

    IntegrationResult result;
    result.value = reverse ? -partial.value : partial.value;
    result.estimatedError = partial.error;
    result.evaluations = evaluations;
    result.converged = partial.converged;
    return result;
}

// Multi-scale numerical limit estimator. It samples both sides at shrinking
// distances and refuses to claim a limit unless several successive scales stabilize.
bool limit(
    const Func& f,
    double approach,
    double& result,
    std::string& note,
    double initialH = 1e-2)
{
    if (!std::isfinite(approach) || !std::isfinite(initialH) || initialH <= 0.0)
    {
        note = "The approach point or sampling step is invalid.";
        return false;
    }

    double h = initialH * std::max(1.0, std::fabs(approach));
    if (!std::isfinite(h) || h <= 0.0)
        h = initialH;

    bool havePrevious = false;
    double previousLeft = 0.0;
    double previousRight = 0.0;
    int stableScales = 0;
    bool sawInvalidSide = false;

    for (int i = 0; i < 12; ++i)
    {
        const double leftX = approach - h;
        const double rightX = approach + h;
        if (leftX == approach || rightX == approach)
            break; // Further shrinking is below the useful floating-point resolution.

        double left = 0.0;
        double right = 0.0;
        bool leftOK = false;
        bool rightOK = false;

        try
        {
            left = f(leftX);
            leftOK = std::isfinite(left);
        }
        catch (...) { leftOK = false; }

        try
        {
            right = f(rightX);
            rightOK = std::isfinite(right);
        }
        catch (...) { rightOK = false; }

        if (!leftOK || !rightOK)
        {
            sawInvalidSide = true;
            havePrevious = false;
            stableScales = 0;
            h *= 0.1;
            continue;
        }

        const double scale = std::max(std::fabs(left), std::fabs(right));
        const double tolerance = 1e-10 + 1e-7 * scale;
        const double sideGap = std::fabs(left - right);

        if (havePrevious)
        {
            const double leftChange = std::fabs(left - previousLeft);
            const double rightChange = std::fabs(right - previousRight);

            if (sideGap <= tolerance &&
                leftChange <= tolerance &&
                rightChange <= tolerance)
            {
                ++stableScales;
            }
            else
            {
                stableScales = 0;
            }

            if (stableScales >= 2)
            {
                result = left / 2.0 + right / 2.0;
                if (!std::isfinite(result))
                {
                    note = ErrorMessages::LimitFailed;
                    return false;
                }
                note = "Numerical limit estimate stabilized across shrinking scales.";
                return true;
            }
        }

        previousLeft = left;
        previousRight = right;
        havePrevious = true;
        h *= 0.1;
    }

    note = sawInvalidSide
        ? "The function was not finite on both sides at sufficiently small sampling distances; a two-sided limit could not be established."
        : "Left and right samples did not stabilize at shrinking distances; the limit could not be established reliably.";
    return false;
}

// Newton's method with both residual and step checks.
double newtonMethod(
    const Func& f,
    const Func& df,
    double x0,
    bool& converged,
    std::string& note,
    double tolerance = 1e-9,
    int maxIter = 100
)
{
    converged = false;
    note.clear();

    if (!std::isfinite(x0))
    {
        note = ErrorMessages::NewtonInvalidGuess;
        return x0;
    }

    if (!std::isfinite(tolerance) || tolerance <= 0.0 || maxIter < 1)
    {
        note = "Newton's method received an invalid tolerance or iteration limit.";
        return x0;
    }

    double x = x0;
    for (int i = 0; i < maxIter; ++i)
    {
        const double fx = f(x);
        if (!std::isfinite(fx))
        {
            note = "Function value is non-finite at the current estimate.";
            return x;
        }

        // Check the residual first: an existing root may have derivative zero.
        if (std::fabs(fx) <= tolerance)
        {
            converged = true;
            note = "Newton's method converged: function residual is within tolerance.";
            return x;
        }

        const double dfx = df(x);
        if (!std::isfinite(dfx))
        {
            note = "Derivative is non-finite at the current estimate.";
            return x;
        }

        if (std::fabs(dfx) < 1e-10)
        {
            note = ErrorMessages::NewtonDerivativeSmall;
            return x;
        }

        const double xNew = x - fx / dfx;
        if (!std::isfinite(xNew))
        {
            note = "Newton's method produced a non-finite estimate.";
            return x;
        }

        const double fNew = f(xNew);
        if (!std::isfinite(fNew))
        {
            note = "Function value is non-finite at the next estimate.";
            return x;
        }

        if (std::fabs(fNew) <= tolerance)
        {
            converged = true;
            note = "Newton's method converged: function residual is within tolerance.";
            return xNew;
        }

        const double stepTolerance = tolerance * (1.0 + std::fabs(xNew));
        if (std::fabs(xNew - x) <= stepTolerance)
        {
            note = ErrorMessages::NewtonResidualTooLarge;
            return xNew;
        }

        x = xNew;
    }

    note = ErrorMessages::NewtonMaxIterations;
    return x;
}

// Arc length using an analytic derivative and adaptive Simpson integration.
// The result is always nonnegative and includes an error estimate.
IntegrationResult arcLength(const Func& df, double a, double b, int maxIntervals = 1000)
{
    if (!std::isfinite(a) || !std::isfinite(b))
        throw std::invalid_argument(ErrorMessages::InvalidBounds);

    if (a > b)
        std::swap(a, b);

    auto integrand = [&df](double x)
    {
        const double d = df(x);
        if (!std::isfinite(d))
            throw std::domain_error(ErrorMessages::ArcLengthFailed);
        const double value = std::hypot(1.0, d);
        if (!std::isfinite(value))
            throw std::overflow_error(ErrorMessages::ArcLengthFailed);
        return value;
    };

    return adaptiveSimpson(integrand, a, b, maxIntervals);
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
double f10(double x)
{
    // Stable sigmoid: avoids exp(-x) overflow when x is very negative.
    if (x >= 0.0)
        return 1.0 / (1.0 + std::exp(-x));

    const double ex = std::exp(x);
    return ex / (1.0 + ex);
}
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
        default:
            throw std::invalid_argument(
                ErrorMessages::InvalidFunction
            );
    }
}

double evaluateFunction(int index, double x)
{
    if (!std::isfinite(x))
    {
        throw std::domain_error(
            ErrorMessages::NonFiniteInput
        );
    }

    switch (index)
    {
        case 3: // ln(x)
            if (x <= 0.0)
                throw std::domain_error(
                    ErrorMessages::LogDomain
                );
            break;

        case 4: // 1/x
            if (x == 0.0)
                throw std::domain_error(
                    ErrorMessages::ReciprocalDomain
                );
            break;

        case 7: // tan(x)
            // Practical guard against numerically unstable poles.
            if (std::fabs(std::cos(x)) < 1e-12)
                throw std::domain_error(
                    ErrorMessages::TangentPole
                );
            break;

        case 8: // sqrt(x)
            if (x < 0.0)
                throw std::domain_error(
                    ErrorMessages::SquareRootDomain
                );
            break;

        default:
            break;
    }

    const double value = getFunction(index)(x);

    if (!std::isfinite(value))
    {
        throw std::domain_error(
            ErrorMessages::NonFiniteResult
        );
    }

    return value;
}

// Reject known singularities before a numerical method samples the interval.
// This check is specific to the 12 predefined functions and does not claim to
// detect every possible numerical difficulty for arbitrary user-defined functions.
void validateFunctionInterval(int index, double a, double b, bool derivativeRequired = false)
{
    if (!std::isfinite(a) || !std::isfinite(b))
        throw std::invalid_argument(ErrorMessages::InvalidBounds);

    const double lo = std::min(a, b);
    const double hi = std::max(a, b);

    switch (index)
    {
        case 3: // ln(x)
            if (lo <= 0.0)
                throw std::domain_error(ErrorMessages::LogDomain);
            break;

        case 4: // 1/x
            if (lo <= 0.0 && hi >= 0.0)
                throw std::domain_error(ErrorMessages::IntegrationSingularity);
            break;

        case 7: // tan(x), poles at pi/2 + k*pi
        {
            const long double pi = std::acos(-1.0L);
            const long double halfPi = pi / 2.0L;
            const long double low = static_cast<long double>(lo);
            const long double high = static_cast<long double>(hi);
            const long double k = std::ceil((low - halfPi) / pi);
            const long double pole = halfPi + k * pi;
            if (pole >= low - 1e-12L && pole <= high + 1e-12L)
                throw std::domain_error(ErrorMessages::IntegrationSingularity);
            if (std::fabs(std::cos(lo)) < 1e-12 || std::fabs(std::cos(hi)) < 1e-12)
                throw std::domain_error(ErrorMessages::IntegrationSingularity);
            break;
        }

        case 8: // sqrt(x)
            if (lo < 0.0)
                throw std::domain_error(ErrorMessages::SquareRootDomain);
            if (derivativeRequired && lo <= 0.0)
                throw std::domain_error(
                    "Arc length for sqrt(x) requires a strictly positive lower bound because its derivative is undefined at x = 0.");
            break;

        case 0: case 1: case 2: case 5: case 6: case 9: case 10: case 11:
            break;

        default:
            throw std::invalid_argument(ErrorMessages::InvalidFunction);
    }
}

// Validate the domain at points where the derivative formula requires it.
void validateDerivativeDomain(int index, double x)
{
    if (!std::isfinite(x))
        throw std::domain_error(ErrorMessages::NonFiniteInput);

    switch (index)
    {
        case 3:
            if (x <= 0.0)
                throw std::domain_error(ErrorMessages::LogDomain);
            break;
        case 4:
            if (x == 0.0)
                throw std::domain_error(ErrorMessages::ReciprocalDomain);
            break;
        case 7:
            if (std::fabs(std::cos(x)) < 1e-12)
                throw std::domain_error(ErrorMessages::TangentPole);
            break;
        case 8:
            if (x <= 0.0)
                throw std::domain_error("The derivative of sqrt(x) requires x > 0.");
            break;
        case 0: case 1: case 2: case 5: case 6:
        case 9: case 10: case 11:
            break;
        default:
            throw std::invalid_argument(ErrorMessages::InvalidFunction);
    }
}

// Analytic first derivative for the 12 predefined functions.
double firstDerivativeByIndex(int index, double x)
{
    validateDerivativeDomain(index, x);
    double result = 0.0;

    switch (index)
    {
        case 0:  result = 2.0 * x; break;
        case 1:  result = std::cos(x); break;
        case 2:  result = std::exp(x); break;
        case 3:  result = 1.0 / x; break;
        case 4:  result = -1.0 / (x * x); break;
        case 5:  result = 3.0 * x * x + 4.0 * x + 3.0; break;
        case 6:  result = -std::sin(x); break;
        case 7:
        {
            const double c = std::cos(x);
            result = 1.0 / (c * c);
            break;
        }
        case 8:  result = 1.0 / (2.0 * std::sqrt(x)); break;
        case 9:
        {
            const double fx = evaluateFunction(index, x);
            result = fx * (1.0 - fx);
            break;
        }
        case 10: result = std::exp(x) * (x + 1.0); break;
        case 11: result = std::cos(2.0 * x); break;
        default:
            throw std::invalid_argument(ErrorMessages::InvalidFunction);
    }

    if (!std::isfinite(result))
        throw std::domain_error(ErrorMessages::DerivativeFailed);
    return result;
}

// Analytic second derivative for the 12 predefined functions.
double secondDerivativeByIndex(int index, double x)
{
    validateDerivativeDomain(index, x);
    double result = 0.0;

    switch (index)
    {
        case 0:  result = 2.0; break;
        case 1:  result = -std::sin(x); break;
        case 2:  result = std::exp(x); break;
        case 3:  result = -1.0 / (x * x); break;
        case 4:  result = 2.0 / (x * x * x); break;
        case 5:  result = 6.0 * x + 4.0; break;
        case 6:  result = -std::cos(x); break;
        case 7:
        {
            const double c = std::cos(x);
            result = 2.0 * std::tan(x) / (c * c);
            break;
        }
        case 8:  result = -1.0 / (4.0 * x * std::sqrt(x)); break;
        case 9:
        {
            const double fx = evaluateFunction(index, x);
            result = fx * (1.0 - fx) * (1.0 - 2.0 * fx);
            break;
        }
        case 10: result = std::exp(x) * (x + 2.0); break;
        case 11: result = -2.0 * std::sin(2.0 * x); break;
        default:
            throw std::invalid_argument(ErrorMessages::InvalidFunction);
    }

    if (!std::isfinite(result))
        throw std::domain_error(ErrorMessages::SecondDerivativeFailed);
    return result;
}

// Polynomial helpers used for analytic higher derivatives of tan(x) and sigmoid(x).
std::vector<long double> polynomialDerivative(const std::vector<long double>& c)
{
    if (c.size() <= 1)
        return {0.0L};

    std::vector<long double> result(c.size() - 1, 0.0L);
    for (std::size_t i = 1; i < c.size(); ++i)
        result[i - 1] = c[i] * static_cast<long double>(i);
    return result;
}

std::vector<long double> tanDerivativePolynomial(int order)
{
    // P_0(t) = t, P_(n+1)(t) = (1+t^2) * P'_n(t).
    std::vector<long double> p = {0.0L, 1.0L};
    for (int n = 0; n < order; ++n)
    {
        const std::vector<long double> d = polynomialDerivative(p);
        std::vector<long double> next(d.size() + 2, 0.0L);
        for (std::size_t i = 0; i < d.size(); ++i)
        {
            next[i] += d[i];
            next[i + 2] += d[i];
        }
        p = std::move(next);
    }
    return p;
}

std::vector<long double> sigmoidDerivativePolynomial(int order)
{
    // P_0(s) = s, P_(n+1)(s) = s(1-s) * P'_n(s).
    std::vector<long double> p = {0.0L, 1.0L};
    for (int n = 0; n < order; ++n)
    {
        const std::vector<long double> d = polynomialDerivative(p);
        std::vector<long double> next(d.size() + 2, 0.0L);
        for (std::size_t i = 0; i < d.size(); ++i)
        {
            next[i + 1] += d[i];
            next[i + 2] -= d[i];
        }
        p = std::move(next);
    }
    return p;
}

long double evaluatePolynomial(const std::vector<long double>& coefficients, long double x)
{
    long double result = 0.0L;
    for (auto it = coefficients.rbegin(); it != coefficients.rend(); ++it)
        result = result * x + *it;
    return result;
}

long double factorialLongDouble(int n)
{
    long double result = 1.0L;
    for (int i = 2; i <= n; ++i)
        result *= static_cast<long double>(i);
    return result;
}

// Analytic n-th derivative for the predefined function set (Taylor degree 0-8).
double nthDerivativeByIndex(int index, double x, int n)
{
    if (n < 0 || n > 8)
        throw std::invalid_argument("Derivative order must be between 0 and 8.");

    if (n == 0)
        return evaluateFunction(index, x);

    validateDerivativeDomain(index, x);
    long double value = 0.0L;

    switch (index)
    {
        case 0: // x^2
            value = (n == 1) ? 2.0L * x : (n == 2 ? 2.0L : 0.0L);
            break;

        case 1: // sin(x) cycles every four derivatives
            switch (n % 4)
            {
                case 0: value = std::sin(x); break;
                case 1: value = std::cos(x); break;
                case 2: value = -std::sin(x); break;
                default: value = -std::cos(x); break;
            }
            break;

        case 2: // e^x
            value = std::exp(static_cast<long double>(x));
            break;

        case 3: // ln(x), n >= 1
            value = ((n % 2) == 1 ? 1.0L : -1.0L) *
                    factorialLongDouble(n - 1) /
                    std::pow(static_cast<long double>(x), n);
            break;

        case 4: // 1/x
            value = ((n % 2) == 0 ? 1.0L : -1.0L) *
                    factorialLongDouble(n) /
                    std::pow(static_cast<long double>(x), n + 1);
            break;

        case 5: // cubic polynomial
            if (n == 1) value = 3.0L * x * x + 4.0L * x + 3.0L;
            else if (n == 2) value = 6.0L * x + 4.0L;
            else if (n == 3) value = 6.0L;
            else value = 0.0L;
            break;

        case 6: // cos(x) cycles every four derivatives
            switch (n % 4)
            {
                case 0: value = std::cos(x); break;
                case 1: value = -std::sin(x); break;
                case 2: value = -std::cos(x); break;
                default: value = std::sin(x); break;
            }
            break;

        case 7: // tan(x) derivative polynomials
            value = evaluatePolynomial(
                tanDerivativePolynomial(n),
                std::tan(static_cast<long double>(x)));
            break;

        case 8: // sqrt(x) falling factorial
        {
            if (x <= 0.0)
                throw std::domain_error(
                    "Higher derivatives of sqrt(x) require x > 0.");
            long double coefficient = 1.0L;
            for (int k = 0; k < n; ++k)
                coefficient *= (0.5L - static_cast<long double>(k));
            value = coefficient * std::pow(static_cast<long double>(x), 0.5L - n);
            break;
        }

        case 9: // sigmoid(x) derivative polynomials in s(x)
        {
            const long double sValue = static_cast<long double>(evaluateFunction(index, x));
            value = evaluatePolynomial(sigmoidDerivativePolynomial(n), sValue);
            break;
        }

        case 10: // x * e^x
            value = std::exp(static_cast<long double>(x)) *
                    (static_cast<long double>(x) + static_cast<long double>(n));
            break;

        case 11: // sin(x)cos(x) = 0.5 sin(2x)
            value = 0.5L * std::pow(2.0L, n) *
                    std::sin(2.0L * static_cast<long double>(x) +
                             static_cast<long double>(n) * std::acos(-1.0L) / 2.0L);
            break;

        default:
            throw std::invalid_argument(ErrorMessages::InvalidFunction);
    }

    const double result = static_cast<double>(value);
    if (!std::isfinite(result))
        throw std::domain_error(ErrorMessages::TaylorFailed);
    return result;
}

double taylorSeries(int functionIndex, double x, double center, int degree)
{
    if (!std::isfinite(x) || !std::isfinite(center))
        throw std::invalid_argument(ErrorMessages::NonFiniteInput);
    if (degree < 0 || degree > 8)
        throw std::invalid_argument("Taylor degree must be between 0 and 8.");

    const long double dx = static_cast<long double>(x) - center;
    long double dxPower = 1.0L;
    long double factorial = 1.0L;
    long double sum = 0.0L;
    long double compensation = 0.0L;

    for (int i = 0; i <= degree; ++i)
    {
        if (i > 0)
        {
            dxPower *= dx;
            factorial *= static_cast<long double>(i);
        }

        const long double term =
            static_cast<long double>(nthDerivativeByIndex(functionIndex, center, i)) *
            dxPower / factorial;

        // Kahan compensated summation reduces avoidable rounding in the series sum.
        const long double adjusted = term - compensation;
        const long double nextSum = sum + adjusted;
        compensation = (nextSum - sum) - adjusted;
        sum = nextSum;

        if (!std::isfinite(static_cast<double>(sum)))
            throw std::overflow_error(ErrorMessages::TaylorFailed);
    }

    const double result = static_cast<double>(sum);
    if (!std::isfinite(result))
        throw std::overflow_error(ErrorMessages::TaylorFailed);
    return result;
}

#ifndef MATHLAB_RUN_TESTS
// ============================================================
// GUI APPLICATION (unchanged layout and controls)
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

    // Initialize Dear ImGui backends and handle initialization failures.
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        std::fprintf(stderr, "%s\n", ErrorMessages::ImGuiGlfwInit);
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 130"))
    {
        std::fprintf(stderr, "%s\n", ErrorMessages::ImGuiOpenGLInit);
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

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
    // STEP 3.2: Validate the final calculation result.
    if (!std::isfinite(value))
    {
        reportError(
            "The operation produced NaN or infinity. "
            "Check the input domain and numeric range."
        );
        return;
    }

    // Store the valid result.
    lastResult = value;
    resultLabel = label;

    // Update the result panel state.
    resultReady = true;
    resultIsError = false;

    // Clear any previous error or status message.
    status.clear();
};

    // --------------------------------------------------------
    // BASIC CALCULATOR: uses your original arithmetic functions
    // --------------------------------------------------------

    auto calculateBasic = [&]()
{
    // STEP 3.1: Validate required inputs.
    if (!std::isfinite(numA))
    {
        reportError("First number must be finite.");
        return;
    }

    if (basicOperation != 6 && !std::isfinite(numB))
    {
        reportError("Second number must be finite.");
        return;
    }

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

        // STEP 3.1: Validate power operations.
        case 4:
        {
            if (numA == 0.0 && numB <= 0.0)
            {
                reportError(
                    "Zero cannot be raised to a zero or negative power."
                );
                break;
            }

            if (numA < 0.0 && std::trunc(numB) != numB)
            {
                reportError(
                    "This power has no real-valued result for these operands."
                );
                break;
            }

            setResult(
                power(numA, numB),
                numberText(numA) + " ^ " + numberText(numB)
            );
            break;
        }

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
        try
        {
            Func f = [functionIndex](double t)
            {
                return evaluateFunction(functionIndex, t);
            };

            Func df = [functionIndex](double t)
            {
                return firstDerivativeByIndex(functionIndex, t);
            };

            auto checkFiniteInput = [&](double value, const char* name)
            {
                if (!std::isfinite(value))
                {
                    reportError(std::string(name) + " must be a finite number.");
                    return false;
                }
                return true;
            };

            auto checkIntervals = [&]()
            {
                if (!std::isfinite(lower) || !std::isfinite(upper))
                {
                    reportError(ErrorMessages::InvalidBounds);
                    return false;
                }

                if (intervals > 1000000)
                {
                    reportError("Intervals must not exceed 1,000,000.");
                    return false;
                }

                // Simpson's rule is also used for arc length.
                if (calculusOperation == 3 || calculusOperation == 7)
                {
                    if (intervals < 2 || intervals % 2 != 0)
                    {
                        reportError(ErrorMessages::SimpsonIntervals);
                        return false;
                    }
                }
                else if (intervals < 1)
                {
                    reportError(ErrorMessages::InvalidIntervals);
                    return false;
                }

                return true;
            };

            switch (calculusOperation)
            {
                case 0:
                    if (!checkFiniteInput(x, "x value"))
                        return;
                    setResult(
                        firstDerivativeByIndex(functionIndex, x),
                        "Analytic first derivative at x = " + numberText(x)
                    );
                    break;

                case 1:
                    if (!checkFiniteInput(x, "x value"))
                        return;
                    setResult(
                        secondDerivativeByIndex(functionIndex, x),
                        "Analytic second derivative at x = " + numberText(x)
                    );
                    break;

                case 2:
                    if (!checkIntervals())
                        return;
                    validateFunctionInterval(functionIndex, lower, upper);
                    setResult(
                        trapezoidal(f, lower, upper, intervals),
                        "Trapezoidal integral estimate (fixed-step)"
                    );
                    if (resultReady)
                        status = "Fixed-step trapezoidal estimate; no automatic error bound is provided.";
                    break;

                case 3:
                    if (!checkIntervals())
                        return;
                    validateFunctionInterval(functionIndex, lower, upper);
                    {
                        const IntegrationResult integral =
                            adaptiveSimpson(f, lower, upper, intervals);
                        setResult(
                            integral.value,
                            "Adaptive Simpson integral estimate"
                        );
                        if (resultReady)
                        {
                            status = "Estimated absolute error: " +
                                numberText(integral.estimatedError) +
                                "; function evaluations: " +
                                std::to_string(integral.evaluations);
                            if (!integral.converged)
                            {
                                resultIsError = true;
                                status = "Tolerance was not reached within the refinement budget. " + status;
                            }
                        }
                    }
                    break;

                case 4:
                {
                    if (!checkFiniteInput(approach, "Approach point"))
                        return;

                    double value = 0.0;
                    std::string note;
                    if (!limit(f, approach, value, note))
                    {
                        reportError(note);
                        return;
                    }

                    setResult(
                        value,
                        "Numerical limit estimate as x approaches " + numberText(approach)
                    );
                    if (resultReady)
                        status = note;
                    break;
                }

                case 5:
                    if (!checkFiniteInput(x, "Evaluation point") ||
                        !checkFiniteInput(center, "Taylor center"))
                        return;

                    if (taylorDegree < 0 || taylorDegree > 8)
                    {
                        reportError("Taylor degree must be between 0 and 8.");
                        return;
                    }

                    {
                        const double approximation =
                            taylorSeries(functionIndex, x, center, taylorDegree);
                        const double actualValue = evaluateFunction(functionIndex, x);
                        setResult(
                            approximation,
                            "Taylor polynomial estimate at x = " + numberText(x)
                        );
                        if (resultReady)
                        {
                            const double observedDifference =
                                std::fabs(actualValue - approximation);
                            status = "Absolute difference from the selected function value: " +
                                numberText(observedDifference);
                        }
                    }
                    break;

                case 6:
                {
                    if (!checkFiniteInput(initialGuess, "Initial guess"))
                        return;

                    bool converged = false;
                    std::string note;
                    const double root = newtonMethod(
                        f, df, initialGuess, converged, note
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
                    validateFunctionInterval(functionIndex, lower, upper, true);
                    {
                        const IntegrationResult length =
                            arcLength(df, lower, upper, intervals);
                        setResult(length.value, "Adaptive arc-length estimate");
                        if (resultReady)
                        {
                            status = "Estimated absolute integration error: " +
                                numberText(length.estimatedError) +
                                "; function evaluations: " +
                                std::to_string(length.evaluations);
                            if (!length.converged)
                            {
                                resultIsError = true;
                                status = "Arc-length tolerance was not reached within the refinement budget. " + status;
                            }
                        }
                    }
                    break;

                default:
                    reportError(ErrorMessages::InvalidOperation);
                    break;
            }
        }
        catch (const std::exception& e)
        {
            reportError(e.what());
        }
        catch (...)
        {
            reportError(ErrorMessages::UnexpectedError);
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
                try
                {
                    calculateBasic();
                }
                catch (const std::exception& e)
                {
                    reportError(e.what());
                }
                catch (...)
                {
                    reportError(ErrorMessages::UnexpectedError);
                }
            }
        }
        else
        {
            ImGui::TextUnformatted("Calculus Calculator");

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
                "%.17g",
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
#else

int main()
{
    const auto near = [](double actual, double expected, double tolerance)
    {
        return std::isfinite(actual) && std::fabs(actual - expected) <= tolerance;
    };

    assert(near(firstDerivativeByIndex(0, 2.0), 4.0, 1e-12));
    assert(near(secondDerivativeByIndex(0, 2.0), 2.0, 1e-12));
    assert(near(nthDerivativeByIndex(1, 0.0, 4), 0.0, 1e-12));
    assert(near(nthDerivativeByIndex(2, 0.0, 5), 1.0, 1e-12));

    const IntegrationResult integral = adaptiveSimpson(
        [](double x) { return x * x; }, 0.0, 1.0, 1000);
    assert(near(integral.value, 1.0 / 3.0, 1e-10));
    assert(integral.converged);
    assert(integral.estimatedError >= 0.0);

    double limitValue = 0.0;
    std::string limitNote;
    assert(limit([](double x) { return x * x; }, 0.0, limitValue, limitNote));
    assert(std::fabs(limitValue) < 1e-8);
    assert(!limit([](double x) { return 1.0 / x; }, 0.0, limitValue, limitNote));

    bool converged = false;
    std::string newtonNote;
    const double root = newtonMethod(
        [](double x) { return x * x; },
        [](double x) { return 2.0 * x; },
        0.0, converged, newtonNote);
    assert(converged && near(root, 0.0, 1e-12));

    const double taylor = taylorSeries(2, 0.1, 0.0, 4);
    assert(std::fabs(taylor - std::exp(0.1)) < 1e-6);

    bool rejectedSingularity = false;
    try
    {
        validateFunctionInterval(4, -1.0, 1.0);
    }
    catch (const std::domain_error&)
    {
        rejectedSingularity = true;
    }
    assert(rejectedSingularity);

    bool rejectedTangentPole = false;
    try
    {
        validateFunctionInterval(7, 1.0, 2.0);
    }
    catch (const std::domain_error&)
    {
        rejectedTangentPole = true;
    }
    assert(rejectedTangentPole);

    const IntegrationResult reversed = adaptiveSimpson(
        [](double x) { return x * x; }, 1.0, 0.0, 1000);
    assert(reversed.converged);
    assert(near(reversed.value, -1.0 / 3.0, 1e-10));

    std::puts("MathLab regression tests passed.");
    return 0;
}

#endif
