#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
using Complex = std::complex<double>;
constexpr double pi = std::numbers::pi;
constexpr std::size_t sample_limit = 4096;
constexpr std::size_t order_limit = 256;

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(message);
}
std::vector<double> read_vector(const Json &input, std::size_t limit,
                                bool allow_empty = false) {
  std::vector<double> result;
  if (input.is_string()) {
    if (!trim(input.string()).empty())
      result = numbers(input.string(), limit);
  } else {
    if (input.array().size() > limit)
      fail("Too many samples or coefficients for this operation");
    for (const auto &entry : input.array())
      result.push_back(entry.number());
  }
  if (result.empty() && !allow_empty)
    fail("Enter at least one sample or coefficient");
  for (double value : result)
    if (!std::isfinite(value) || std::abs(value) > 1e12)
      fail("Signal values must be finite and no larger than 1e12 in magnitude");
  return result;
}
double bounded(const Json &data, const std::string &key, double fallback,
                double low, double high) {
  const double value = data.numeric(key, fallback);
  if (value < low || value > high)
    fail(key + " is outside the supported range [" + format(low) + ", " +
         format(high) + "]");
  return value;
}
int integer(const Json &data, const std::string &key, int fallback,
             int low, int high) {
  return data.find(key) ? data.at(key).integer(low, high) : fallback;
}
Json::Array plot_points(const std::vector<double> &samples, double interval = 1,
                         double start = 0) {
  Json::Array result;
  const auto stride = std::max(std::size_t{1}, (samples.size() + 511) / 512);
  for (std::size_t i = 0; i < samples.size(); i += stride)
    result.emplace_back(Json::Array{
        start + static_cast<double>(i) * interval, samples[i]});
  if (samples.size() > 1 && (samples.size() - 1) % stride)
    result.emplace_back(Json::Array{
        start + static_cast<double>(samples.size() - 1) * interval,
        samples.back()});
  return result;
}
std::string preview(const std::vector<double> &samples, std::size_t limit = 16) {
  std::string result;
  for (std::size_t i = 0; i < std::min(samples.size(), limit); ++i) {
    if (i)
      result += ", ";
    result += format(samples[i]);
  }
  if (samples.size() > limit)
    result += " … (all samples are included in the result data)";
  return result;
}
void checked(SolutionBundle &result, double residual, std::string method,
              std::string evidence, double tolerance = 1e-8) {
  if (!std::isfinite(residual))
    fail("Verification produced a nonfinite residual");
  result.verification = {
      residual <= tolerance ? VerificationStatus::verified_numerical
                            : VerificationStatus::verification_failed,
      std::move(method),
      std::move(evidence) + "; normalized residual=" + format(residual)};
  if (residual > tolerance) {
    result.status = "verification_failed";
    result.warnings.push_back(
        "The numerical cross-check did not meet tolerance. Review scaling "
        "and conditioning before relying on this result.");
  }
}
Complex polynomial_in_delay(const std::vector<double> &coefficients,
                             Complex delay) {
  Complex value = 0;
  for (auto it = coefficients.rbegin(); it != coefficients.rend(); ++it)
    value = value * delay + *it;
  return value;
}
struct Filter {
  std::vector<double> numerator;
  std::vector<double> denominator;
  std::size_t order{};
};
Filter read_filter(const Json &data) {
  Filter filter{read_vector(data.at("numerator"), order_limit + 1),
                read_vector(data.at("denominator"), 65), 0};
  const double leading = filter.denominator.front();
  if (leading == 0)
    fail("The a[0] denominator coefficient cannot be zero");
  for (auto &value : filter.numerator) {
    value /= leading;
    if (!std::isfinite(value) || std::abs(value) > 1e12)
      fail("Normalizing by a[0] makes a numerator coefficient too large");
  }
  for (auto &value : filter.denominator) {
    value /= leading;
    if (!std::isfinite(value) || std::abs(value) > 1e12)
      fail("Normalizing by a[0] makes a denominator coefficient too large");
  }
  // Trailing zero delays do not change a transfer function or recurrence order.
  while (filter.numerator.size() > 1 && filter.numerator.back() == 0)
    filter.numerator.pop_back();
  while (filter.denominator.size() > 1 && filter.denominator.back() == 0)
    filter.denominator.pop_back();
  filter.order = std::max(filter.numerator.size(), filter.denominator.size()) - 1;
  return filter;
}
struct Stability {
  std::string status;
  std::string reason;
  std::vector<double> reflection;
};
Stability schur_stability(const std::vector<double> &denominator) {
  // The delay-polynomial denominator a0+a1*z^-1+...+an*z^-n has
  // characteristic polynomial a0*z^n+a1*z^(n-1)+...+an. Schur reduction
  // checks that ALL characteristic roots lie strictly inside the unit disk.
  std::vector<long double> coefficients(denominator.begin(), denominator.end());
  Stability result;
  while (coefficients.size() > 1) {
    const long double leading = coefficients.front();
    if (std::abs(leading) < 1e-25L) {
      result.status = "indeterminate";
      result.reason = "Schur reduction is numerically degenerate; no stability "
                      "certification is made.";
      return result;
    }
    const long double reflection = coefficients.back() / leading;
    result.reflection.push_back(static_cast<double>(reflection));
    if (std::abs(reflection) > 1 + 1e-10L) {
      result.status = "not_strictly_stable";
      result.reason = "A Schur reflection coefficient exceeds unit magnitude; "
                      "the supplied recurrence is not strictly stable.";
      return result;
    }
    if (std::abs(reflection) >= 1 - 1e-10L) {
      result.status = "boundary_or_ill_conditioned";
      result.reason = "A Schur coefficient is near the unit boundary. Unit-circle "
                      "poles and ill-conditioned cases are not certified stable.";
      return result;
    }
    const std::size_t degree = coefficients.size() - 1;
    std::vector<long double> next(degree);
    const long double normalization = leading * (1 - reflection * reflection);
    for (std::size_t i = 0; i < degree; ++i)
      next[i] = (coefficients[i] - reflection * coefficients[degree - i]) /
                normalization;
    coefficients = std::move(next);
  }
  result.status = "strictly_stable";
  result.reason = "All Schur reflection magnitudes are below one within the "
                  "stated tolerance; the denominator roots lie inside the "
                  "unit circle. This concerns the supplied recurrence, not "
                  "possible exact pole-zero cancellations.";
  return result;
}
Json stability_json(const Stability &stability) {
  return Json::Object{{"status", stability.status},
                      {"reason", stability.reason},
                      {"reflection_coefficients", array_json(stability.reflection)}};
}
std::vector<double> history(const Json &data, const std::string &key,
                             std::size_t required) {
  std::vector<double> result(required);
  if (const auto supplied = data.find(key)) {
    const auto values = read_vector(*supplied, required, true);
    if (values.size() != required)
      fail(key + " must contain exactly " + std::to_string(required) +
           " values, most recent first (index −1, then −2, …)");
    result = values;
  }
  return result;
}
struct Filtered {
  std::vector<double> output;
  std::vector<double> state;
  double recurrence_error{};
};
Filtered filter_samples(const Filter &filter, const std::vector<double> &input,
                         const std::vector<double> &past_input,
                         const std::vector<double> &past_output) {
  Filtered result;
  result.state.assign(filter.order, 0);
  // Reconstruct the direct-form-II-transposed state from explicit input and
  // output histories. This avoids an undocumented implementation-specific zi.
  for (std::size_t i = 0; i < filter.order; ++i) {
    long double state = 0;
    for (std::size_t j = i + 1; j < filter.numerator.size(); ++j)
      state += static_cast<long double>(filter.numerator[j]) *
               past_input[j - i - 1];
    for (std::size_t j = i + 1; j < filter.denominator.size(); ++j)
      state -= static_cast<long double>(filter.denominator[j]) *
               past_output[j - i - 1];
    result.state[i] = static_cast<double>(state);
  }
  result.output.reserve(input.size());
  for (std::size_t n = 0; n < input.size(); ++n) {
    const double output = filter.numerator.front() * input[n] +
                           (filter.order ? result.state[0] : 0);
    if (!std::isfinite(output) || std::abs(output) > 1e100)
      fail("Filter output diverged or overflowed. Inspect its denominator, "
           "initial conditions and stability; use fewer samples.");
    result.output.push_back(output);
    for (std::size_t i = 0; i < filter.order; ++i) {
      double next = i + 1 < filter.order ? result.state[i + 1] : 0;
      if (i + 1 < filter.numerator.size())
        next += filter.numerator[i + 1] * input[n];
      if (i + 1 < filter.denominator.size())
        next -= filter.denominator[i + 1] * output;
      if (!std::isfinite(next))
        fail("Internal filter state overflowed");
      result.state[i] = next;
    }
    // Independent direct-form-I recurrence in long double checks every output
    // without reusing the transposed state or state-update equations.
    long double expected = 0, scale = 0;
    for (std::size_t delay = 0; delay < filter.numerator.size(); ++delay) {
      const double sample = delay <= n ? input[n - delay]
                                        : past_input[delay - n - 1];
      const auto term = static_cast<long double>(filter.numerator[delay]) * sample;
      expected += term;
      scale += std::abs(term);
    }
    for (std::size_t delay = 1; delay < filter.denominator.size(); ++delay) {
      const double sample = delay <= n ? result.output[n - delay]
                                        : past_output[delay - n - 1];
      const auto term = static_cast<long double>(filter.denominator[delay]) * sample;
      expected -= term;
      scale += std::abs(term);
    }
    const double residual = static_cast<double>(
        std::abs(expected - static_cast<long double>(output)) / std::max(1.L, scale));
    result.recurrence_error = std::max(result.recurrence_error, residual);
  }
  return result;
}
SolutionBundle difference_equation(const Json &data) {
  data.only({"operation", "numerator", "denominator", "x", "past_input",
             "past_output", "fs"});
  const auto filter = read_filter(data);
  const auto input = read_vector(data.at("x"), sample_limit);
  const auto initial_input = history(data, "past_input", filter.numerator.size() - 1);
  const auto initial_output = history(data, "past_output", filter.denominator.size() - 1);
  const auto fs = bounded(data, "fs", 1, 1e-12, 1e12);
  const auto output = filter_samples(filter, input, initial_input, initial_output);
  const auto stability = schur_stability(filter.denominator);
  SolutionBundle result;
  result.domain = "signals";
  result.topic = "difference_equation";
  result.answer = "Filtered " + std::to_string(input.size()) + " samples.\n" +
                  preview(output.output) + "\nRecurrence stability: " +
                  stability.status + ".";
  result.steps = {
      {"FILTER_COEFFICIENTS",
       "Interpret numerator and denominator lists in increasing powers of z⁻¹. "
       "Divide every coefficient by a[0] before evaluating the recurrence.",
       "a[0] y[n] = sum_k b[k]x[n−k] − sum_(k≥1) a[k]y[n−k]"},
      {"FILTER_INITIAL_HISTORY",
       "Use explicit past samples, most recent first; omitted histories are "
       "zero. Convert these histories into a direct-form-II-transposed state.",
       "past_input=[x[−1],x[−2],…]; past_output=[y[−1],y[−2],…]"},
      {"FILTER_LINEAR_WORK",
       "Apply the recurrence in O(samples × order) work and O(order) working "
       "state, without building a dense matrix or blocking the UI thread.",
       "order=" + std::to_string(filter.order) +
           "; samples=" + std::to_string(input.size())},
      {"FILTER_RECURRENCE_REPLAY",
       "For every output, independently evaluate the original difference "
       "equation using input/output histories and long-double accumulation.",
       "maximum normalized equation residual=" + format(output.recurrence_error)},
      {"FILTER_SCHUR_CHECK", stability.reason,
       "Schur reflection coefficients: " + preview(stability.reflection)}};
  result.assumptions = {
      "Real coefficients; causal finite history. The sequence starts at n=0. "
      "Numerator order is at most 256, denominator order at most 64, and input "
      "length at most 4096.",
      "A finite successful output does not establish stability. The stability "
      "check applies to the supplied denominator, without symbolic pole-zero "
      "cancellation or fixed-point quantization effects.",
      "The curve uses seconds when a sample rate is supplied. Plot data is "
      "decimated for display; the complete output remains in exported JSON."};
  if (stability.status != "strictly_stable")
    result.warnings.push_back(stability.reason);
  checked(result, output.recurrence_error, "independent direct-form-I recurrence",
            std::to_string(input.size()) + " output samples checked");
  result.visual_json =
      Json(Json::Object{{"kind", "signal"},
                        {"label", "Filtered output versus seconds"},
                        {"points", plot_points(output.output, 1 / fs)},
                        {"samples", array_json(output.output)},
                        {"final_state", array_json(output.state)},
                        {"numerator", array_json(filter.numerator)},
                        {"denominator", array_json(filter.denominator)},
                        {"stability", stability_json(stability)}}).dump();
  return result;
}

struct FrequencySample {
  double frequency{};
  Complex value;
  double phase{};
  double direct_error{};
};
std::vector<FrequencySample> digital_response_samples(
    const Filter &filter, double fs, double low, double high, int count) {
  std::vector<FrequencySample> result;
  double previous_phase = 0;
  for (int sample = 0; sample < count; ++sample) {
    const double frequency = low + (high - low) * sample / (count - 1);
    const double omega = 2 * pi * frequency / fs;
    const Complex delay = std::polar(1., -omega);
    const auto a = polynomial_in_delay(filter.denominator, delay);
    const auto b = polynomial_in_delay(filter.numerator, delay);
    if (std::abs(a) < 1e-14)
      fail("The requested digital frequency response hits, or approaches, a "
           "denominator zero on the unit circle. Change the range or inspect "
           "the filter before taking a ratio.");
    const Complex value = b / a;
    if (!std::isfinite(std::abs(value)))
      fail("Digital frequency response overflowed");
    double phase = std::arg(value) * 180 / pi;
    if (sample) {
      while (phase - previous_phase > 180)
        phase -= 360;
      while (phase - previous_phase < -180)
        phase += 360;
    }
    previous_phase = phase;
    // Independent trigonometric sums do not reuse Horner's complex multiply
    // chain. They also expose a sign reversal or coefficient-order mistake.
    std::complex<long double> direct_a{}, direct_b{};
    for (std::size_t k = 0; k < filter.numerator.size(); ++k) {
      const long double angle = -static_cast<long double>(omega) * k;
      direct_b += static_cast<long double>(filter.numerator[k]) *
                  std::complex<long double>{std::cos(angle), std::sin(angle)};
    }
    for (std::size_t k = 0; k < filter.denominator.size(); ++k) {
      const long double angle = -static_cast<long double>(omega) * k;
      direct_a += static_cast<long double>(filter.denominator[k]) *
                  std::complex<long double>{std::cos(angle), std::sin(angle)};
    }
    const auto direct = direct_b / direct_a;
    const double error = static_cast<double>(
        std::abs(direct - std::complex<long double>(value)) /
        std::max(1.L, std::abs(direct)));
    result.push_back({frequency, value, phase, error});
  }
  return result;
}
Json frequency_visual(const std::vector<FrequencySample> &samples) {
  Json::Array rows, magnitude, phase;
  for (const auto &sample : samples) {
    const double amplitude = std::abs(sample.value);
    const double db = amplitude == 0 ? -600 : 20 * std::log10(amplitude);
    rows.emplace_back(Json::Object{
        {"frequency_hz", sample.frequency},
        {"value", complex_json(sample.value)},
        {"gain_db", amplitude == 0 ? Json() : Json(db)},
        {"unwrapped_phase_deg", sample.phase},
        {"direct_sum_error", sample.direct_error}});
    magnitude.emplace_back(Json::Array{sample.frequency, db});
    phase.emplace_back(Json::Array{sample.frequency, sample.phase});
  }
  return Json::Object{
      {"kind", "signal"},
      {"label", "Digital filter magnitude in dB versus frequency in Hz"},
      {"points", magnitude},
      {"secondary_label", "Unwrapped phase in degrees versus frequency in Hz"},
      {"secondary_points", phase},
      {"response", rows}};
}
SolutionBundle digital_frequency_response(const Json &data) {
  data.only({"operation", "numerator", "denominator", "fs", "f_min", "f_max",
             "count"});
  const auto filter = read_filter(data);
  const double fs = bounded(data, "fs", 1000, 1e-12, 1e12);
  const double low = bounded(data, "f_min", 0, 0, fs / 2);
  const double high = bounded(data, "f_max", fs / 2, 0, fs / 2);
  const int count = integer(data, "count", 128, 2, 512);
  if (!(high > low))
    fail("Digital response maximum frequency must exceed its minimum");
  const auto samples = digital_response_samples(filter, fs, low, high, count);
  double error = 0;
  for (const auto &sample : samples)
    error = std::max(error, sample.direct_error);
  const auto stability = schur_stability(filter.denominator);
  SolutionBundle result;
  result.domain = "signals";
  result.topic = "digital_frequency_response";
  result.answer = "Evaluated H(exp(jω)) at " + std::to_string(count) +
                  " frequencies from " + format(low) + " to " + format(high) +
                  " Hz; fs=" + format(fs) + " Hz.\nDenominator stability: " +
                  stability.status + ".";
  result.steps = {
      {"DIGITAL_UNIT_CIRCLE",
       "For a uniformly sampled discrete-time system, substitute z=exp(jω). "
       "A unit delay contributes exp(−jω), not jω as in a continuous-time "
       "transfer function.",
       "ω = 2πf/fs; H(z)=sum b[k]z^(−k) / sum a[k]z^(−k)"},
      {"DIGITAL_HORNER",
       "Evaluate numerator and denominator delay polynomials by Horner's "
       "method, then divide the complex values.",
       "coefficients are b[0], b[1], … and a[0], a[1], …"},
      {"DIGITAL_DIRECT_CHECK",
       "Independently expand each polynomial into sine and cosine sums in "
       "long-double arithmetic at every requested frequency.",
       std::to_string(count) + " frequency points cross-checked"},
      {"DIGITAL_STABILITY", stability.reason,
       "reflection coefficients=" + preview(stability.reflection)}};
  result.assumptions = {
      "Real coefficient causal recurrence. The frequency axis is linear and "
      "restricted to 0…fs/2; the other half is its conjugate mirror.",
      "The frequency response of an unstable recurrence is a formal rational "
      "evaluation, not proof that a bounded steady-state response exists.",
      "Zero gain has −infinite dB; exported data uses null and its plot uses "
      "−600 dB. Phase near a response zero is not physically well-conditioned."};
  if (stability.status != "strictly_stable")
    result.warnings.push_back(stability.reason);
  checked(result, error, "independent trigonometric-sum frequency evaluation",
            "Every requested frequency checked", 1e-7);
  auto visual = frequency_visual(samples);
  visual.object()["stability"] = stability_json(stability);
  result.visual_json = visual.dump();
  return result;
}
double window_value(const std::string &kind, int index, int count) {
  const double angle = 2 * pi * index / (count - 1);
  if (kind == "rectangular")
    return 1;
  if (kind == "hann")
    return .5 - .5 * std::cos(angle);
  if (kind == "hamming")
    return .54 - .46 * std::cos(angle);
  if (kind == "blackman")
    return .42 - .5 * std::cos(angle) + .08 * std::cos(2 * angle);
  fail("FIR window must be rectangular, hann, hamming or blackman");
}
double lowpass_kernel(double normalized_cutoff, int offset) {
  if (offset == 0)
    return 2 * normalized_cutoff;
  return std::sin(2 * pi * normalized_cutoff * offset) / (pi * offset);
}
std::vector<double> design_fir(const std::string &kind, int taps, double fs,
                               double first_cutoff, double second_cutoff,
                               const std::string &window) {
  if (kind != "lowpass" && kind != "highpass" && kind != "bandpass" &&
      kind != "bandstop")
    fail("FIR design supports lowpass, highpass, bandpass and bandstop");
  if (taps < 3 || taps > 255 || taps % 2 == 0)
    fail("Use an odd FIR tap count from 3 through 255 (type-I linear phase)");
  if (!(fs > 0) || !(first_cutoff > 0 && first_cutoff < fs / 2))
    fail("FIR cutoff must lie strictly between zero and Nyquist");
  const bool band = kind == "bandpass" || kind == "bandstop";
  if (band && !(second_cutoff > first_cutoff && second_cutoff < fs / 2))
    fail("The upper band edge must exceed the lower edge and remain below Nyquist");
  const int middle = (taps - 1) / 2;
  std::vector<double> coefficients;
  coefficients.reserve(static_cast<std::size_t>(taps));
  for (int n = 0; n < taps; ++n) {
    const int offset = n - middle;
    const double lower = lowpass_kernel(first_cutoff / fs, offset);
    const double upper = band ? lowpass_kernel(second_cutoff / fs, offset) : 0;
    const double impulse = offset == 0 ? 1 : 0;
    const double value = kind == "lowpass" ? lower
                         : kind == "highpass" ? impulse - lower
                         : kind == "bandpass" ? upper - lower
                                                : impulse - upper + lower;
    coefficients.push_back(value * window_value(window, n, taps));
  }
  // Normalize at a passband reference: DC, Nyquist, or the band midpoint.
  const double reference = kind == "highpass" ? fs / 2
                             : kind == "bandpass" ? (first_cutoff + second_cutoff) / 2
                                                    : 0;
  const Complex delay = std::polar(1., -2 * pi * reference / fs);
  const double gain = std::abs(polynomial_in_delay(coefficients, delay));
  if (!(gain > 1e-12))
    fail("FIR passband normalization is degenerate. Increase taps or adjust cutoffs");
  for (auto &value : coefficients)
    value /= gain;
  return coefficients;
}
SolutionBundle fir_design(const Json &data) {
  data.only({"operation", "kind", "taps", "fs", "cutoff", "cutoff_high", "window"});
  const auto kind = data.text("kind", "lowpass");
  const int taps = integer(data, "taps", 31, 3, 255);
  const double fs = bounded(data, "fs", 1000, 1e-9, 1e12);
  const double cutoff = bounded(data, "cutoff", 100, 0, fs / 2);
  const double cutoff_high = bounded(data, "cutoff_high", 200, 0, 1e12);
  const auto window = data.text("window", "hamming");
  const auto coefficients = design_fir(kind, taps, fs, cutoff, cutoff_high, window);
  const Filter filter{coefficients, {1}, coefficients.size() - 1};
  const auto response = digital_response_samples(filter, fs, 0, fs / 2, 257);
  const double reference = kind == "highpass" ? fs / 2
                             : kind == "bandpass" ? (cutoff + cutoff_high) / 2 : 0;
  double error = std::abs(std::abs(polynomial_in_delay(
      coefficients, std::polar(1., -2 * pi * reference / fs))) - 1);
  for (std::size_t i = 0; i < coefficients.size(); ++i)
    error = std::max(error, std::abs(coefficients[i] - coefficients[coefficients.size() - 1 - i]));
  for (const auto &sample : response)
    error = std::max(error, sample.direct_error);
  SolutionBundle result;
  result.domain = "signals";
  result.topic = "fir_design";
  result.answer = std::to_string(taps) + "-tap " + window + "-window " + kind +
                  " FIR.\nGroup delay = " + std::to_string((taps - 1) / 2) +
                  " samples = " + format((taps - 1) / (2 * fs)) +
                  " s.\nCoefficients b[k]: " + preview(coefficients);
  result.steps = {
      {"FIR_IDEAL_KERNEL",
       "Start from the ideal low-pass impulse response. Use spectral "
       "inversion or the difference of two low-pass kernels for the selected band.",
       "h[m]=sin(2π(fc/fs)m)/(πm); h[0]=2fc/fs"},
      {"FIR_WINDOW_TRUNCATION",
       "Center the finite impulse response at (N−1)/2 and multiply by a "
       "symmetric window. Odd length gives type-I linear phase.",
       "window=" + window + "; taps=" + std::to_string(taps)},
      {"FIR_REFERENCE_NORMALIZATION",
       "Normalize the response magnitude to one at a selected passband "
       "reference frequency: DC, Nyquist, or band midpoint.",
       "reference=" + format(reference) + " Hz"},
      {"FIR_DESIGN_CHECK",
       "Check coefficient symmetry, unit reference gain, and direct "
       "trigonometric frequency evaluations. These checks do not certify an "
       "arbitrary stopband attenuation or transition-width specification.",
       "max symmetry/normalization/response residual=" + format(error)}};
  result.assumptions = {
      "Windowed-sinc design, not an optimal equiripple filter. More taps "
      "generally narrow the transition band; no custom attenuation guarantee "
      "is inferred from cutoff values alone.",
      "Cutoffs are ideal transition locations; the measured gain at a cutoff "
      "is not generally 0 dB. Inspect the supplied frequency response.",
      "The filter is causal after shifting by (N−1)/2. It has a finite impulse "
      "response and is BIBO stable, with the displayed group delay."};
  checked(result, error, "symmetry, passband normalization and direct response sums",
            "257 frequency points; every coefficient symmetry pair checked", 1e-7);
  auto visual = frequency_visual(response);
  visual.object()["coefficients"] = array_json(coefficients);
  visual.object()["group_delay_samples"] = (taps - 1) / 2;
  visual.object()["filter_input"] = Json::Object{
      {"operation", "filter"}, {"numerator", array_json(coefficients)},
      {"denominator", Json::Array{1}}, {"fs", fs}};
  result.visual_json = visual.dump();
  return result;
}

Complex finite_z_value(const std::vector<double> &samples, int origin,
                        Complex z) {
  if (z == Complex{}) {
    Complex result = 0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
      const int index = origin + static_cast<int>(i);
      if (samples[i] != 0 && index > 0)
        fail("The finite sequence has a pole at z=0; choose a nonzero evaluation point");
      if (index == 0)
        result += samples[i];
    }
    return result;
  }
  // Trim structural zero endpoints before evaluation to avoid manufacturing
  // overflow in a zero term at a far-away index.
  auto first = std::find_if(samples.begin(), samples.end(),
                             [](double value) { return value != 0; });
  if (first == samples.end())
    return 0;
  auto last = std::find_if(samples.rbegin(), samples.rend(),
                            [](double value) { return value != 0; }).base();
  Complex polynomial = 0;
  const Complex delay = 1. / z;
  for (auto it = last; it != first;) {
    --it;
    polynomial = polynomial * delay + *it;
  }
  const int first_index = origin + static_cast<int>(first - samples.begin());
  const Complex result = std::pow(z, -first_index) * polynomial;
  if (!std::isfinite(std::abs(result)))
    fail("Z-transform evaluation overflowed. Choose a point nearer the unit "
         "circle or a shorter/less extreme sequence.");
  return result;
}
SolutionBundle finite_z_transform(const Json &data) {
  data.only({"operation", "x", "origin", "z_real", "z_imag"});
  const auto samples = read_vector(data.at("x"), sample_limit);
  const int origin = integer(data, "origin", 0, -1024, 1024);
  const Complex point{bounded(data, "z_real", 1.2, -10, 10),
                       bounded(data, "z_imag", .1, -10, 10)};
  const auto value = finite_z_value(samples, origin, point);
  bool positive = false, negative = false;
  Json::Array coefficients;
  std::string expression;
  std::size_t terms = 0;
  for (std::size_t i = 0; i < samples.size(); ++i) {
    const int index = origin + static_cast<int>(i);
    coefficients.emplace_back(Json::Object{{"index", index}, {"value", samples[i]}});
    if (samples[i] == 0)
      continue;
    positive = positive || index > 0;
    negative = negative || index < 0;
    if (terms < 32) {
      if (!expression.empty())
        expression += " + ";
      expression += "(" + format(samples[i]) + ")*z^(" +
                    std::to_string(-index) + ")";
    }
    ++terms;
  }
  if (terms == 0)
    expression = "0";
  if (terms > 32)
    expression += " + … (all indexed coefficients in the result data)";
  const std::string roc = positive && negative
                              ? "0 < |z| < infinity"
                          : positive ? "0 < |z| (all finite nonzero z)"
                          : negative ? "all finite z (including zero)"
                                     : "entire z-plane; no finite poles";
  double worst = 0;
  Json::Array checks;
  for (int test = 0; test < 8; ++test) {
    const Complex z = std::polar(test % 2 ? 1.0005 : .9995, .13 * (test + 1));
    const Complex horner = finite_z_value(samples, origin, z);
    std::complex<long double> direct{};
    long double scale = 0;
    const std::complex<long double> extended(z);
    for (std::size_t i = 0; i < samples.size(); ++i) {
      if (samples[i] == 0)
        continue;
      const auto term = static_cast<long double>(samples[i]) *
                        std::pow(extended, -(origin + static_cast<int>(i)));
      direct += term;
      scale += std::abs(term);
    }
    const double error = static_cast<double>(
        std::abs(direct - std::complex<long double>(horner)) / std::max(1.L, scale));
    worst = std::max(worst, error);
    checks.emplace_back(Json::Object{{"z", complex_json(z)},
                                     {"value", complex_json(horner)},
                                     {"normalized_error", error}});
  }
  SolutionBundle result;
  result.domain = "signals";
  result.topic = "finite_z_transform";
  result.answer = "X(z) = " + expression + "\nROC: " + roc +
                  ".\nX(" + complex_text(point) + ") = " + complex_text(value);
  result.steps = {
      {"BILATERAL_Z_DEFINITION",
       "Associate each sample with its explicitly supplied integer index; "
       "the first sample is x[origin]. Treat all unspecified samples as zero.",
       "X(z) = sum_n x[n] z^(−n), n=" + std::to_string(origin) + "…" +
           std::to_string(origin + static_cast<int>(samples.size()) - 1)},
      {"FINITE_Z_POLYNOMIAL",
       "Collect the finite Laurent polynomial. Positive-time terms have "
       "negative powers of z; negative-time terms have positive powers.",
       expression},
      {"FINITE_Z_ROC",
       "A finite sequence has no nonzero finite poles. Exclude z=0 only if a "
       "nonzero positive-time sample introduces a negative power; negative-time "
       "samples prevent convergence at the point at infinity.",
       roc},
      {"Z_HORNER_REPLAY",
       "Compare shifted Horner evaluation against independent direct powers "
       "at eight complex points near, but not exactly on, the unit circle.",
       "maximum normalized difference=" + format(worst)}};
  result.assumptions = {
      "Finite, explicitly indexed sequence; no infinite continuation is "
      "inferred. This is a bilateral Z-transform, not a DTFT or Laplace transform.",
      "The displayed polynomial preview is capped at 32 nonzero terms. Every "
      "sample/index pair and all numerical cross-checks are retained in JSON."};
  checked(result, worst, "independent direct Laurent-polynomial sums",
            "Eight complex evaluation points checked", 1e-7);
  result.visual_json =
      Json(Json::Object{{"kind", "signal"},
                        {"label", "Finite sequence versus integer index"},
                        {"points", plot_points(samples, 1, origin)},
                        {"samples", array_json(samples)},
                        {"start_index", origin},
                        {"indexed_coefficients", coefficients},
                        {"evaluation_point", complex_json(point)},
                        {"evaluation", complex_json(value)},
                        {"cross_checks", checks},
                        {"roc", roc}}).dump();
  return result;
}
struct InverseZ {
  double h0{};
  Complex first_pole{}, second_pole{}, first_coefficient{}, second_coefficient{};
  std::string kind;
  std::string expression;
  double pole_radius{};
};
InverseZ inverse_z_terms(const Filter &filter) {
  InverseZ terms;
  terms.h0 = filter.numerator.front();
  const auto coefficient = [](const std::vector<double> &values, std::size_t index) {
    return index < values.size() ? values[index] : 0.;
  };
  const double a1 = coefficient(filter.denominator, 1);
  const double a2 = coefficient(filter.denominator, 2);
  const double h1 = coefficient(filter.numerator, 1) - a1 * terms.h0;
  const double h2 = coefficient(filter.numerator, 2) - a1 * h1 - a2 * terms.h0;
  if (filter.denominator.size() == 1) {
    terms.kind = "finite";
    terms.expression = "h[n] equals the supplied numerator coefficient b[n] "
                       "where defined and is zero elsewhere.";
    return terms;
  }
  if (filter.denominator.size() == 2) {
    terms.kind = "single";
    terms.first_pole = -a1;
    terms.first_coefficient = h1;
    terms.pole_radius = std::abs(terms.first_pole);
    terms.expression = "h[0]=" + format(terms.h0) +
                       "; h[n]=" + format(h1) + "*(" + format(-a1) +
                       ")^(n−1), n≥1";
    return terms;
  }
  const double discriminant = a1 * a1 - 4 * a2;
  const double scale = std::max({1e-300, a1 * a1, std::abs(4 * a2)});
  if (std::abs(discriminant) < 1e-12 * scale) {
    terms.kind = "repeated";
    terms.first_pole = -a1 / 2;
    if (terms.first_pole == Complex{})
      fail("Numerically degenerate repeated zero pole; remove structural "
           "zero denominator coefficients before inversion");
    terms.first_coefficient = h1;
    terms.second_coefficient = h2 / terms.first_pole - h1;
    terms.pole_radius = std::abs(terms.first_pole);
    terms.expression = "h[0]=" + format(terms.h0) + "; h[n]=(" +
                       complex_text(terms.first_coefficient) + "+(" +
                       complex_text(terms.second_coefficient) + ")*(n−1))*(" +
                       complex_text(terms.first_pole) + ")^(n−1), n≥1";
  } else {
    terms.kind = "distinct";
    if (discriminant > 0) {
      const double stable_root = -.5 * (a1 + std::copysign(std::sqrt(discriminant), a1));
      terms.first_pole = stable_root;
      terms.second_pole = stable_root == 0 ? 0 : a2 / stable_root;
    } else {
      terms.first_pole = Complex{-a1 / 2, std::sqrt(-discriminant) / 2};
      terms.second_pole = std::conj(terms.first_pole);
    }
    terms.first_coefficient = (h2 - terms.second_pole * h1) /
                               (terms.first_pole - terms.second_pole);
    terms.second_coefficient = h1 - terms.first_coefficient;
    terms.pole_radius = std::max(std::abs(terms.first_pole),
                                 std::abs(terms.second_pole));
    terms.expression = "h[0]=" + format(terms.h0) + "; h[n]=(" +
                       complex_text(terms.first_coefficient) + ")*(" +
                       complex_text(terms.first_pole) + ")^(n−1)+(" +
                       complex_text(terms.second_coefficient) + ")*(" +
                       complex_text(terms.second_pole) + ")^(n−1), n≥1";
  }
  return terms;
}
Complex inverse_z_sample(const Filter &filter, const InverseZ &terms, int index) {
  if (terms.kind == "finite")
    return static_cast<std::size_t>(index) < filter.numerator.size()
               ? filter.numerator[static_cast<std::size_t>(index)] : 0.;
  if (index == 0)
    return terms.h0;
  if (terms.kind == "single")
    return terms.first_coefficient * std::pow(terms.first_pole, index - 1);
  if (terms.kind == "repeated")
    return (terms.first_coefficient + terms.second_coefficient * (index - 1.)) *
           std::pow(terms.first_pole, index - 1);
  return terms.first_coefficient * std::pow(terms.first_pole, index - 1) +
         terms.second_coefficient * std::pow(terms.second_pole, index - 1);
}
Complex forward_inverse_z(const Filter &filter, const InverseZ &terms, Complex z) {
  if (terms.kind == "finite")
    return polynomial_in_delay(filter.numerator, 1. / z);
  if (terms.kind == "single")
    return terms.h0 + terms.first_coefficient / (z - terms.first_pole);
  if (terms.kind == "repeated") {
    const Complex difference = z - terms.first_pole;
    return terms.h0 + terms.first_coefficient / difference +
           terms.second_coefficient * terms.first_pole / (difference * difference);
  }
  return terms.h0 + terms.first_coefficient / (z - terms.first_pole) +
         terms.second_coefficient / (z - terms.second_pole);
}
SolutionBundle inverse_z_transform(const Json &data) {
  data.only({"operation", "numerator", "denominator", "count"});
  const auto filter = read_filter(data);
  if (filter.denominator.size() > 3 ||
      (filter.denominator.size() > 1 && filter.numerator.size() > filter.denominator.size()))
    fail("Causal rational inverse Z supports denominator degree 1–2 with "
         "numerator degree no larger, or a finite FIR numerator with denominator 1");
  const int count = integer(data, "count", 64, 2, 512);
  const auto terms = inverse_z_terms(filter);
  std::vector<double> impulse(static_cast<std::size_t>(count));
  impulse.front() = 1;
  const auto reference = filter_samples(filter, impulse,
      std::vector<double>(filter.numerator.size() - 1),
      std::vector<double>(filter.denominator.size() - 1));
  std::vector<double> output;
  double worst = reference.recurrence_error;
  for (int index = 0; index < count; ++index) {
    const Complex value = inverse_z_sample(filter, terms, index);
    if (!std::isfinite(std::abs(value)) || std::abs(value) > 1e100)
      fail("Inverse Z samples overflowed. The causal sequence may be unstable; "
           "reduce the sample count or inspect the poles.");
    const double expected = reference.output[static_cast<std::size_t>(index)];
    worst = std::max(worst, std::abs(value - expected) /
                               std::max({1., std::abs(value), std::abs(expected)}));
    output.push_back(value.real());
  }
  // A forward algebraic transform verifies the infinite causal formula. The
  // finite impulse sequence alone would not certify its unsampled tail.
  for (int test = 0; test < 12; ++test) {
    const Complex z = std::polar(terms.pole_radius + 1. + test * .11,
                                 .17 * (test + 1));
    const Complex expected = polynomial_in_delay(filter.numerator, 1. / z) /
                              polynomial_in_delay(filter.denominator, 1. / z);
    worst = std::max(worst, std::abs(forward_inverse_z(filter, terms, z) - expected) /
                               std::max(1., std::abs(expected)));
  }
  const auto stability = schur_stability(filter.denominator);
  SolutionBundle result;
  result.domain = "signals";
  result.topic = "inverse_z_transform";
  result.answer = terms.expression + "\nCausal: h[n]=0 for n<0.\n";
  if (terms.kind == "finite")
    result.answer += "Finite impulse response; no nonzero finite poles.\n";
  else
    result.answer += "Sufficient causal ROC: |z| > " + format(terms.pole_radius) +
                     " (may be larger than necessary after exact cancellation).\n";
  result.answer += "First samples: " + preview(output);
  result.steps = {
      {"INVERSE_Z_NORMALIZE",
       "Normalize the denominator and interpret coefficients in increasing "
       "powers of z⁻¹. Choose the causal (right-sided) inverse explicitly.",
       "H(z) = (b0+b1z⁻¹+b2z⁻²)/(1+a1z⁻¹+a2z⁻²)"},
      {"INVERSE_Z_INITIAL_SAMPLES",
       "Compute the first impulse samples directly from the recurrence to "
       "retain feedthrough and numerator terms before resolving the tail.",
       "h0=b0; h1=b1−a1h0; h2=b2−a1h1−a2h0"},
      {"INVERSE_Z_POLES",
       "Resolve the causal tail using one pole, two distinct poles or a "
       "repeated pole. Complex-conjugate terms combine to real samples.",
       terms.expression},
      {"INVERSE_Z_RECURRENCE_CHECK",
       "Compare every displayed closed-form sample with an independently "
       "filtered unit impulse, including explicit zero initial history.",
       std::to_string(count) + " time samples checked"},
      {"INVERSE_Z_FORWARD_CHECK",
       "Transform the closed-form infinite causal expression back to a "
       "rational function and compare at 12 complex points in the ROC.",
       "maximum combined normalized residual=" + format(worst)}};
  result.assumptions = {
      "The ROC/causality choice is essential: the same rational expression "
      "can have different noncausal inverse sequences. Only the causal "
      "interpretation is implemented here.",
      "Real coefficient degree 1–2 denominator, or a finite FIR. Repeated "
      "poles use a scale-relative 1e−12 discriminant tolerance.",
      "Pole-zero cancellation is not symbolically certified. The reported "
      "denominator pole radius gives a sufficient, possibly conservative ROC."};
  if (stability.status != "strictly_stable")
    result.warnings.push_back(stability.reason);
  checked(result, worst, "impulse-recurrence and forward Z-transform reconstruction",
            std::to_string(count) + " samples and 12 complex points checked", 1e-7);
  result.visual_json = Json(Json::Object{
      {"kind", "signal"}, {"label", "Causal impulse response versus sample index"},
      {"points", plot_points(output)}, {"samples", array_json(output)},
      {"pole_radius", terms.pole_radius}, {"stability", stability_json(stability)},
      {"pole_case", terms.kind}}).dump();
  return result;
}

// Additional digital operations follow the same bounded, data-only protocol.
} // namespace

SolutionBundle digital_signals(const ProblemSpec &request) {
  const auto data = Json::parse(request.input);
  const auto operation = data.at("operation").string();
  if (operation == "filter")
    return difference_equation(data);
  if (operation == "digital_response")
    return digital_frequency_response(data);
  if (operation == "fir_design")
    return fir_design(data);
  if (operation == "z_transform")
    return finite_z_transform(data);
  if (operation == "inverse_z")
    return inverse_z_transform(data);
  fail("Unknown signals operation");
}
} // namespace pocket_engineer::workbench
