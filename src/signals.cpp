#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <numeric>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
using C = std::complex<double>;
constexpr double pi = std::numbers::pi;
[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(message);
}
std::vector<double> samples(const Json &data, std::size_t maximum = 4096) {
  std::vector<double> out;
  if (data.is_string())
    out = numbers(data.string(), maximum);
  else {
    if (data.array().size() > maximum)
      fail("Sample count exceeds this operation's budget");
    for (const auto &item : data.array())
      out.push_back(item.number());
  }
  if (out.empty())
    fail("Enter at least one sample or coefficient");
  for (const auto value : out)
    if (!std::isfinite(value) || std::abs(value) > 1e12)
      fail("Samples must be finite and at most 1e12 in magnitude");
  return out;
}
double bounded(const Json &data, std::string_view key, double fallback,
               double minimum, double maximum) {
  const auto value = data.numeric(key, fallback);
  if (value < minimum || value > maximum)
    fail(std::string(key) + " is outside its supported range");
  return value;
}
std::vector<C> complex_samples(const Json &data) {
  const auto real = samples(data.at("x"));
  std::vector<double> imag(real.size());
  if (const auto value = data.find("imag"))
    imag = samples(*value);
  if (real.size() != imag.size())
    fail("Real and imaginary arrays must have the same length");
  std::vector<C> out;
  for (std::size_t i = 0; i < real.size(); i++)
    out.emplace_back(real[i], imag[i]);
  return out;
}
void fft(std::vector<C> &values, bool inverse) {
  const auto n = values.size();
  if (n == 0 || n > 8192 || !std::has_single_bit(n))
    fail("FFT length must be a power of two, at most 8192 internally");
  for (std::size_t i = 1, j = 0; i < n; i++) {
    auto bit = n >> 1;
    while (j & bit) {
      j ^= bit;
      bit >>= 1;
    }
    j ^= bit;
    if (i < j)
      std::swap(values[i], values[j]);
  }
  for (std::size_t width = 2; width <= n; width <<= 1) {
    const auto root =
        std::polar(1., (inverse ? 2. : -2.) * pi / static_cast<double>(width));
    for (std::size_t start = 0; start < n; start += width) {
      C w = 1;
      for (std::size_t j = 0; j < width / 2; j++) {
        const auto even = values[start + j],
                   odd = w * values[start + j + width / 2];
        values[start + j] = even + odd;
        values[start + j + width / 2] = even - odd;
        w *= root;
      }
    }
  }
  if (inverse)
    for (auto &value : values)
      value /= static_cast<double>(n);
}
C polynomial(const std::vector<double> &coefficients, C variable) {
  C value = 0;
  for (const double coefficient : coefficients)
    value = value * variable + coefficient;
  return value;
}
Json::Array plot(const std::vector<double> &values, double xstep = 1,
                 double xstart = 0) {
  Json::Array points;
  const auto stride = std::max(std::size_t{1}, (values.size() + 511) / 512);
  for (std::size_t i = 0; i < values.size(); i += stride)
    points.emplace_back(
        Json::Array{xstart + static_cast<double>(i) * xstep, values[i]});
  if (values.size() > 1 && (values.size() - 1) % stride)
    points.emplace_back(
        Json::Array{xstart + static_cast<double>(values.size() - 1) * xstep,
                    values.back()});
  return points;
}
void verify(SolutionBundle &out, double error, std::string method,
            std::string evidence, double tolerance = 1e-8) {
  if (!std::isfinite(error))
    fail("The numerical result overflowed; reduce the input magnitude");
  out.verification = {
      error <= tolerance ? VerificationStatus::verified_numerical
                         : VerificationStatus::verification_failed,
      std::move(method),
      std::move(evidence) + "; normalized residual=" + format(error)};
  if (error > tolerance) {
    out.status = "verification_failed";
    out.warnings.push_back("The verification tolerance was not met. Do not "
                           "treat this result as checked.");
  }
}
SolutionBundle transform(const Json &data, bool inverse) {
  data.only({"operation", "x", "imag", "fs", "pad", "window"});
  auto values = complex_samples(data);
  const auto original_length = values.size();
  const auto window = data.text("window", "rectangular");
  if (window != "rectangular" && window != "hann" && window != "hamming")
    fail("Window must be rectangular, hann or hamming");
  if (inverse && window != "rectangular")
    fail("A time-domain window cannot be applied to inverse-FFT coefficients");
  double coherent_gain = 0;
  for (std::size_t i = 0; i < values.size(); i++) {
    const double angle = values.size() > 1
                             ? 2 * pi * static_cast<double>(i) /
                                   static_cast<double>(values.size() - 1)
                             : 0;
    const double w =
        window == "hann" && values.size() > 1      ? .5 - .5 * std::cos(angle)
        : window == "hamming" && values.size() > 1 ? .54 - .46 * std::cos(angle)
                                                   : 1.;
    values[i] *= w;
    coherent_gain += w;
  }
  coherent_gain /= static_cast<double>(original_length);
  if (!std::has_single_bit(values.size())) {
    if (!data.find("pad") || !data.at("pad").boolean() || inverse)
      fail("Use a power-of-two sample count or explicitly enable zero padding "
           "for a forward FFT");
    values.resize(std::bit_ceil(values.size()));
  }
  const auto fs = bounded(data, "fs", 1, 1e-12, 1e12);
  const auto before = values;
  fft(values, inverse);
  auto reconstructed = values;
  fft(reconstructed, !inverse);
  double error = 0, energy_in = 0, energy_out = 0, peak = 0;
  std::size_t peak_bin = 0;
  std::vector<double> magnitudes;
  Json::Array bins;
  for (std::size_t i = 0; i < values.size(); i++) {
    error = std::max(error, std::abs(before[i] - reconstructed[i]) /
                                std::max(1., std::abs(before[i])));
    energy_in += std::norm(before[i]);
    energy_out += std::norm(values[i]);
    if (std::abs(values[i]) > peak) {
      peak = std::abs(values[i]);
      peak_bin = i;
    }
    magnitudes.push_back(inverse ? values[i].real() : std::abs(values[i]));
    const double frequency =
        (i <= values.size() / 2
             ? static_cast<double>(i)
             : static_cast<double>(i) - static_cast<double>(values.size())) *
        fs / static_cast<double>(values.size());
    bins.emplace_back(Json::Object{{"index", i},
                                   {"frequency_hz", frequency},
                                   {"value", complex_json(values[i])}});
  }
  const auto parseval = inverse
                            ? energy_in / static_cast<double>(values.size())
                            : energy_in * static_cast<double>(values.size());
  const auto energy_error =
      std::abs(energy_out - parseval) / std::max(1., std::abs(parseval));
  error = std::max(error, energy_error);
  SolutionBundle out;
  out.domain = "signals";
  out.topic = inverse ? "ifft" : "fft";
  out.answer = (inverse ? "Inverse transform: " : "Forward transform: ") +
               std::to_string(values.size()) +
               " complex samples.\nLargest coefficient: index " +
               std::to_string(peak_bin) + ", " +
               complex_text(values[peak_bin]) +
               ".\nAll coefficients are included in the result data.";
  out.steps = {
      {"FFT_CONVENTION",
       "Use a negative complex-exponential sign for the forward transform and "
       "divide by N only in the inverse transform.",
       "X[k] = sum x[n] exp(-j 2 pi k n / N); x[n] = (1/N) sum X[k] exp(j 2 pi "
       "k n / N)"},
      {"FFT_WINDOW",
       "Apply the selected time-domain window before a forward transform. Zero "
       "padding changes bin spacing, not the information or true frequency "
       "resolution.",
       window + "; original samples=" + std::to_string(original_length) +
           "; coherent gain=" + format(coherent_gain)},
      {"FFT_BUTTERFLY",
       "Bit-reverse the sample indices, then combine even and odd transforms "
       "in radix-2 butterfly stages.",
       std::to_string(std::bit_width(values.size()) - 1) +
           " stages; O(N log N) work"},
      {"FFT_REPLAY",
       "Apply the opposite transform and compare every sample. Also check "
       "Parseval's energy identity with the stated normalization.",
       "input energy=" + format(energy_in) +
           "; transformed energy=" + format(energy_out)}};
  out.assumptions = {
      "Samples are uniformly spaced. Frequencies above Nyquist alias; the "
      "solver cannot recover missing sampling information.",
      "The forward output is a raw two-sided DFT, not an amplitude-normalized "
      "one-sided spectrum. The Nyquist bin is labeled +fs/2; bins above it are "
      "negative frequencies.",
      "Plots show at most 513 decimated points; exported coefficient data "
      "retains every bin."};
  if (coherent_gain == 0)
    out.warnings.push_back("This very short Hann window is entirely zero; use "
                           "more samples or a different window.");
  verify(out, error, "inverse reconstruction and Parseval identity",
         "all " + std::to_string(values.size()) + " samples checked");
  out.visual_json =
      Json(Json::Object{
               {"kind", "signal"},
               {"label",
                inverse ? "Reconstructed real component versus sample index"
                        : "Raw DFT magnitude versus bin index (negative "
                          "frequencies occupy upper bins)"},
               {"points", plot(magnitudes)},
               {"bins", bins},
               {"original_samples", original_length},
               {"coherent_gain", coherent_gain}})
          .dump();
  return out;
}
SolutionBundle convolution(const Json &data, bool correlation) {
  data.only({"operation", "x", "h"});
  const auto x = samples(data.at("x")), h = samples(data.at("h"));
  const auto length = x.size() + h.size() - 1;
  const auto size = std::bit_ceil(length);
  std::vector<C> a(size), b(size);
  for (std::size_t i = 0; i < x.size(); i++)
    a[i] = x[i];
  for (std::size_t i = 0; i < h.size(); i++)
    b[i] = correlation ? h[h.size() - 1 - i] : h[i];
  fft(a, false);
  fft(b, false);
  for (std::size_t i = 0; i < size; i++)
    a[i] *= b[i];
  fft(a, true);
  std::vector<double> y(length);
  double error = 0;
  for (std::size_t i = 0; i < length; i++) {
    y[i] = a[i].real();
    error =
        std::max(error, std::abs(a[i].imag()) / std::max(1., std::abs(y[i])));
  }
  // Direct finite sums use a separate O(NM) formulation. Check all short
  // outputs, or 66 deterministic boundary/interior samples for long signals.
  std::vector<std::size_t> indices;
  if (length <= 256) {
    for (std::size_t i = 0; i < length; i++)
      indices.push_back(i);
  } else {
    for (std::size_t i = 0; i <= 64; i++)
      indices.push_back(i * (length - 1) / 64);
    indices.push_back(h.size() - 1);
  }
  for (const auto n : indices) {
    long double expected = 0, scale = 0;
    for (std::size_t i = 0; i < x.size(); i++)
      if (n >= i && n - i < h.size()) {
        const auto product =
            static_cast<long double>(x[i]) *
            static_cast<long double>(correlation ? h[h.size() - 1 - (n - i)]
                                                 : h[n - i]);
        expected += product;
        scale += std::abs(product);
      }
    error = std::max(error,
                     static_cast<double>(
                         std::abs(static_cast<long double>(y[n]) - expected) /
                         std::max(1.L, scale)));
  }
  const double first_lag = correlation ? -static_cast<double>(h.size() - 1) : 0;
  SolutionBundle out;
  out.domain = "signals";
  out.topic = correlation ? "correlation" : "convolution";
  out.answer = std::to_string(length) + " output samples; indices " +
               format(first_lag) + " through " +
               format(first_lag + static_cast<double>(length - 1)) +
               ".\nFirst values: ";
  for (std::size_t i = 0; i < std::min(length, std::size_t{16}); i++) {
    if (i)
      out.answer += ", ";
    out.answer += format(y[i]);
  }
  if (length > 16)
    out.answer += " … (complete data in export)";
  out.steps = {
      {"LINEAR_NOT_CIRCULAR",
       "Assume finite sequences are zero outside the supplied samples. Pad to "
       "at least N+M−1 so circular wrap-around cannot contaminate linear "
       "output.",
       "FFT length=" + std::to_string(size) +
           "; output length=" + std::to_string(length)},
      {"CONVOLUTION_THEOREM",
       correlation ? "Reverse the second real sequence, then convolve. Lag l "
                     "means r[l] = sum_n x[n] h[n-l]; no biased/unbiased "
                     "normalization is applied."
                   : "Multiply the two forward transforms pointwise, then "
                     "apply the normalized inverse transform.",
       correlation ? "r_xh[l] = sum_n x[n] h[n-l]"
                   : "y[n] = sum_k x[k] h[n-k]"},
      {"DIRECT_SUM_REPLAY",
       "Recompute independent finite sums at the indicated output locations.",
       std::to_string(indices.size()) + " direct-sum comparisons"}};
  verify(out, error, "independent direct sums and imaginary leakage",
         std::to_string(indices.size()) + " of " + std::to_string(length) +
             " outputs directly checked",
         1e-7);
  if (length > 256)
    out.warnings.push_back(
        "Long-sequence verification samples deterministic locations; it is not "
        "an exhaustive independent check of every output.");
  out.visual_json =
      Json(Json::Object{
               {"kind", "signal"},
               {"label", correlation
                             ? "Unnormalized cross-correlation versus lag"
                             : "Linear convolution versus sample index"},
               {"points", plot(y, 1, first_lag)},
               {"samples", array_json(y)},
               {"start_index", first_lag}})
          .dump();
  return out;
}
SolutionBundle generator(const Json &data) {
  data.only({"operation", "waveform", "count", "fs", "frequency", "amplitude",
             "phase", "offset"});
  const auto count =
      data.find("count") ? data.at("count").integer(1, 4096) : 128;
  const double fs = bounded(data, "fs", 1000, 1e-9, 1e12),
               frequency = bounded(data, "frequency", 50, 0, 1e12),
               amplitude = bounded(data, "amplitude", 1, -1e9, 1e9),
               phase = bounded(data, "phase", 0, -360000, 360000) * pi / 180,
               offset = bounded(data, "offset", 0, -1e9, 1e9);
  const auto waveform = data.text("waveform", "sine");
  if (waveform != "sine" && waveform != "cosine" && waveform != "square" &&
      waveform != "triangle" && waveform != "impulse" && waveform != "step")
    fail("Choose sine, cosine, square, triangle, impulse or step");
  std::vector<double> y;
  for (int n = 0; n < count; n++) {
    const double angle =
        2 * pi * std::remainder(frequency / fs, 1.) * n + phase;
    const double unit =
        waveform == "sine"       ? std::sin(angle)
        : waveform == "cosine"   ? std::cos(angle)
        : waveform == "square"   ? (std::sin(angle) >= 0 ? 1. : -1.)
        : waveform == "triangle" ? 2 / pi * std::asin(std::sin(angle))
        : waveform == "impulse"  ? (n == 0 ? 1. : 0.)
                                 : 1.;
    y.push_back(offset + amplitude * unit);
  }
  double violation = 0;
  for (double value : y)
    violation = std::max(violation, std::max(0., std::abs(value - offset) -
                                                     std::abs(amplitude)) /
                                        std::max(1., std::abs(amplitude)));
  SolutionBundle out;
  out.domain = "signals";
  out.topic = "generator";
  out.answer = std::to_string(count) + " " + waveform + " samples at " +
               format(fs) +
               " samples/second. Duration between first and last sample: " +
               format((count - 1) / fs) + " s.";
  out.steps = {
      {"SAMPLE_CLOCK",
       "Evaluate the selected waveform on the uniform sample clock.",
       "t[n] = n / fs, n = 0.." + std::to_string(count - 1)},
      {"SIGNAL_SCALE",
       "Scale by amplitude and add the DC offset. Phase is in degrees; "
       "step/impulse ignore frequency and phase.",
       "amplitude=" + format(amplitude) + "; offset=" + format(offset)},
      {"NYQUIST",
       "The sampling theorem requires band-limited input. A sampled "
       "square/triangle wave contains aliased higher harmonics unless "
       "separately band-limited.",
       "Nyquist frequency=" + format(fs / 2) + " Hz"}};
  out.verification = {
      VerificationStatus::not_verified, "range check only",
      "Generated samples satisfy the requested amplitude bounds; no "
      "independent waveform oracle is claimed. Residual=" +
          format(violation)};
  if (frequency >= fs / 2 && waveform != "step" && waveform != "impulse")
    out.warnings.push_back("The requested fundamental is at or above Nyquist "
                           "and aliases or degenerates at this sample rate.");
  out.visual_json =
      Json(Json::Object{{"kind", "signal"},
                        {"label", waveform + " amplitude versus seconds"},
                        {"points", plot(y, 1 / fs)},
                        {"samples", array_json(y)},
                        {"fs", fs}})
          .dump();
  return out;
}
SolutionBundle response(const Json &data) {
  data.only(
      {"operation", "numerator", "denominator", "f_min", "f_max", "count"});
  auto numerator = samples(data.at("numerator"), 17),
       denominator = samples(data.at("denominator"), 17);
  if (denominator.front() == 0)
    fail("The highest-order denominator coefficient must be nonzero");
  const auto low = bounded(data, "f_min", 1, 1e-9, 1e12),
             high = bounded(data, "f_max", 100000, 1e-9, 1e12);
  if (high <= low)
    fail("Maximum frequency must exceed minimum frequency");
  const int count = data.find("count") ? data.at("count").integer(2, 512) : 128;
  Json::Array rows, points, phases;
  double maximum = 0, last_phase = 0;
  for (int i = 0; i < count; i++) {
    const double f = std::exp(std::log(low) + (std::log(high) - std::log(low)) *
                                                  i / (count - 1));
    const C s{0, 2 * pi * f};
    const auto a = polynomial(denominator, s), b = polynomial(numerator, s);
    if (std::abs(a) < 1e-280)
      fail("The frequency sweep hits a pole; choose a range that excludes it");
    const auto h = b / a;
    if (!std::isfinite(std::abs(h)))
      fail("Frequency response overflowed");
    maximum =
        std::max(maximum, std::abs(a * h - b) / std::max(1., std::abs(b)));
    double phase = std::arg(h) * 180 / pi;
    if (i) {
      while (phase - last_phase > 180)
        phase -= 360;
      while (phase - last_phase < -180)
        phase += 360;
    }
    last_phase = phase;
    const bool zero = std::abs(h) == 0;
    const double db = zero ? -600 : 20 * std::log10(std::abs(h));
    rows.emplace_back(Json::Object{{"frequency_hz", f},
                                   {"value", complex_json(h)},
                                   {"gain_db", zero ? Json() : Json(db)},
                                   {"unwrapped_phase_deg", phase}});
    points.emplace_back(Json::Array{std::log10(f), db});
    phases.emplace_back(Json::Array{std::log10(f), phase});
  }
  SolutionBundle out;
  out.domain = "signals";
  out.topic = "frequency_response";
  out.answer = "Evaluated H(j2πf) at " + std::to_string(count) +
               " logarithmically spaced frequencies from " + format(low) +
               " to " + format(high) + " Hz.";
  out.steps = {
      {"TRANSFER_POLYNOMIALS",
       "Interpret coefficient lists in descending powers of s, including "
       "internal zeros.",
       "H(s) = numerator(s) / denominator(s)"},
      {"HORNER_EVALUATION",
       "Substitute s=j2πf and evaluate both polynomials by Horner's method.",
       "H(jω) = B(jω)/A(jω), ω=2πf"},
      {"BODE_CONVENTION",
       "Convert magnitude to decibels and unwrap adjacent phase samples by "
       "multiples of 360 degrees.",
       "gain=20 log10 |H|; phase=arg(H)"}};
  out.assumptions = {
      "Continuous-time rational transfer function, not a digital z-domain "
      "filter. Coefficients are real and in descending powers of s.",
      "Frequency response does not certify stability, causality or pole "
      "cancellation. A coarse sweep may miss a narrow resonance.",
      "Exactly zero magnitude has −infinite dB; its data entry is null and the "
      "plot uses a −600 dB floor."};
  verify(out, maximum, "rational identity substitution",
         "A(jω)H(jω)−B(jω) checked at each requested frequency");
  out.visual_json =
      Json(Json::Object{
               {"kind", "signal"},
               {"label", "Gain in dB versus log10(frequency in Hz)"},
               {"points", points},
               {"secondary_label",
                "Unwrapped phase in degrees versus log10(frequency in Hz)"},
               {"secondary_points", phases},
               {"response", rows}})
          .dump();
  return out;
}
struct LaplaceTerm {
  std::string kind;
  double amplitude{}, rate{}, omega{}, delay{};
  int power{};
};
LaplaceTerm term(const Json &data) {
  data.only({"kind", "amplitude", "rate", "omega", "power", "delay"});
  LaplaceTerm t{data.text("kind", "power"),
                bounded(data, "amplitude", 1, -1e6, 1e6),
                bounded(data, "rate", 0, -100, 100),
                bounded(data, "omega", 1, 0, 1000),
                bounded(data, "delay", 0, 0, 100),
                data.find("power") ? data.at("power").integer(0, 12) : 0};
  if (t.kind != "power" && t.kind != "sine" && t.kind != "cosine")
    fail("Laplace terms support exponential-weighted powers, sine and cosine, "
         "with causal delay");
  return t;
}
C laplace_value(const LaplaceTerm &t, C s) {
  const C shifted = s - t.rate;
  double factorial = 1;
  for (int i = 2; i <= t.power; i++)
    factorial *= i;
  const auto base =
      t.kind == "power"  ? factorial / std::pow(shifted, t.power + 1)
      : t.kind == "sine" ? t.omega / (shifted * shifted + t.omega * t.omega)
                         : shifted / (shifted * shifted + t.omega * t.omega);
  return t.amplitude * std::exp(-s * t.delay) * base;
}
std::string laplace_expression(const LaplaceTerm &t) {
  const auto shifted = "(s-(" + format(t.rate) + "))";
  double factorial = 1;
  for (int i = 2; i <= t.power; i++)
    factorial *= i;
  const auto kernel =
      t.kind == "power" ? format(factorial) + "/" + shifted + "^" +
                              std::to_string(t.power + 1)
      : t.kind == "sine"
          ? format(t.omega) + "/(" + shifted + "^2+" +
                format(t.omega * t.omega) + ")"
          : shifted + "/(" + shifted + "^2+" + format(t.omega * t.omega) + ")";
  return "(" + format(t.amplitude) + ")*exp(-s*" + format(t.delay) + ")*(" +
         kernel + ")";
}
SolutionBundle laplace(const Json &data) {
  data.only({"operation", "terms"});
  const auto &entries = data.at("terms").array();
  if (entries.empty() || entries.size() > 16)
    fail("Use 1–16 elementary Laplace terms");
  std::vector<LaplaceTerm> terms;
  double roc = -std::numeric_limits<double>::infinity();
  SolutionBundle out;
  out.domain = "signals";
  out.topic = "laplace";
  for (const auto &entry : entries) {
    const auto t = term(entry);
    terms.push_back(t);
    if (t.amplitude != 0)
      roc = std::max(roc, t.rate);
    if (!out.answer.empty())
      out.answer += " + ";
    out.answer += laplace_expression(t);
    out.steps.push_back(
        {"LAPLACE_TABLE",
         "Transform the causal elementary term, shift s by the exponential "
         "rate, then multiply by exp(−s·delay) for a delayed copy.",
         laplace_expression(t)});
  }
  if (!std::isfinite(roc))
    roc = 0;
  out.answer +=
      "\nSufficient common region of convergence: Re(s) > " + format(roc) + ".";
  // Independent numerical quadrature of the defining integral for each term.
  // Select s with ample positive damping so a bounded tail is negligible.
  double worst = 0;
  for (const auto &t : terms) {
    const double damping = std::max(4., t.omega / 2), s = t.rate + damping,
                 T = (t.power + 50.) / damping;
    const int intervals = 4096;
    const double h = T / intervals;
    long double sum = 0;
    for (int i = 0; i <= intervals; i++) {
      const double u = h * i, kernel = t.kind == "power" ? std::pow(u, t.power)
                                       : t.kind == "sine"
                                           ? std::sin(t.omega * u)
                                           : std::cos(t.omega * u);
      const double integrand = std::exp(-damping * u) * kernel;
      sum += (i == 0 || i == intervals ? 1
              : i % 2                  ? 4
                                       : 2) *
             static_cast<long double>(integrand);
    }
    // Factor amplitude and delay out of both sides to avoid underflow
    // concealing an incorrect kernel for a very long delay.
    auto unit = t;
    unit.amplitude = 1;
    unit.delay = 0;
    const double expected = laplace_value(unit, s).real(),
                 numerical = static_cast<double>(sum * h / 3);
    worst = std::max(worst, std::abs(numerical - expected) /
                                std::max(1e-9, std::abs(expected)));
  }
  out.assumptions = {
      "Unilateral causal transform. Each delayed term is "
      "A·exp(rate·(t−delay))·g(t−delay)·u(t−delay). Delay shifts the entire "
      "term.",
      "The stated common ROC is sufficient, not necessarily maximal after "
      "exact cancellation. No distributions, arbitrary symbolic functions or "
      "noncausal signals are inferred."};
  out.steps.push_back(
      {"LAPLACE_INTEGRAL_CHECK",
       "Numerically integrate each unscaled, undelayed kernel at a real point "
       "in its ROC using composite Simpson quadrature; exponential damping "
       "bounds the neglected tail.",
       "4096 subintervals per term; maximum kernel relative difference=" +
           format(worst)});
  verify(out, worst, "defining-integral numerical cross-check",
         "elementary kernels individually checked", 1e-6);
  return out;
}
SolutionBundle inverse_laplace(const Json &data) {
  data.only({"operation", "numerator", "denominator", "duration", "count"});
  auto b = samples(data.at("numerator"), 2),
       a = samples(data.at("denominator"), 3);
  if (a.size() < 2 || a.front() == 0 || b.size() >= a.size())
    fail("Inverse Laplace supports strictly proper real rational functions "
         "with denominator degree 1 or 2; omit leading zeros");
  const double lead = a.front();
  for (auto &value : a)
    value /= lead;
  for (auto &value : b)
    value /= lead;
  std::function<double(double)> time;
  std::function<C(C)> forward;
  std::string expression;
  double roc{};
  if (a.size() == 2) {
    const double amplitude = b[0], rate = -a[1];
    time = [=](double t) { return amplitude * std::exp(rate * t); };
    forward = [=](C s) { return amplitude / (s - rate); };
    expression = format(amplitude) + "*exp(" + format(rate) + "*t)";
    roc = rate;
  } else {
    const double linear = b.size() == 2 ? b[0] : 0, constant = b.back(),
                 center = -a[1] / 2, disc = a[1] * a[1] - 4 * a[2],
                 scale = std::max({1., a[1] * a[1], std::abs(4 * a[2])});
    if (std::abs(disc) <= 1e-12 * scale) {
      const double q = constant + linear * center;
      time = [=](double t) { return (linear + q * t) * std::exp(center * t); };
      forward = [=](C s) {
        return linear / (s - center) + q / ((s - center) * (s - center));
      };
      expression = "(" + format(linear) + "+" + format(q) + "*t)*exp(" +
                   format(center) + "*t)";
      roc = center;
    } else if (disc > 0) {
      const double r1 = center + std::sqrt(disc) / 2,
                   r2 = center - std::sqrt(disc) / 2,
                   c1 = (linear * r1 + constant) / (r1 - r2),
                   c2 = (linear * r2 + constant) / (r2 - r1);
      time = [=](double t) {
        return c1 * std::exp(r1 * t) + c2 * std::exp(r2 * t);
      };
      forward = [=](C s) { return c1 / (s - r1) + c2 / (s - r2); };
      expression = format(c1) + "*exp(" + format(r1) + "*t)+" + format(c2) +
                   "*exp(" + format(r2) + "*t)";
      roc = r1;
    } else {
      const double omega = std::sqrt(-disc) / 2,
                   q = (constant + linear * center) / omega;
      time = [=](double t) {
        return std::exp(center * t) *
               (linear * std::cos(omega * t) + q * std::sin(omega * t));
      };
      forward = [=](C s) {
        const C u = s - center;
        return (linear * u + q * omega) / (u * u + omega * omega);
      };
      expression = "exp(" + format(center) + "*t)*(" + format(linear) +
                   "*cos(" + format(omega) + "*t)+" + format(q) + "*sin(" +
                   format(omega) + "*t))";
      roc = center;
    }
  }
  const double duration = bounded(data, "duration", 5, 1e-9, 1e6);
  const int count = data.find("count") ? data.at("count").integer(2, 512) : 128;
  std::vector<double> y;
  for (int i = 0; i < count; i++) {
    const auto value = time(duration * i / (count - 1));
    if (!std::isfinite(value) || std::abs(value) > 1e100)
      fail("Time-domain result overflows; reduce the plotted duration");
    y.push_back(value);
  }
  double error = 0;
  for (int i = 0; i < 12; i++) {
    const C s{roc + 1 + i * .73, .31 * i};
    const auto expected = polynomial(b, s) / polynomial(a, s);
    error = std::max(error, std::abs(forward(s) - expected) /
                                std::max(1., std::abs(expected)));
  }
  SolutionBundle out;
  out.domain = "signals";
  out.topic = "inverse_laplace";
  out.answer = "f(t) = " + expression +
               ", t ≥ 0; zero for t < 0.\nCausal ROC: Re(s) > " + format(roc) +
               " (sufficient if poles cancel).";
  out.steps = {{"NORMALIZE_RATIONAL",
                "Divide numerator and denominator by the leading denominator "
                "coefficient. Require a strictly proper rational function.",
                "denominator degree=" + std::to_string(a.size() - 1)},
               {"PARTIAL_FRACTIONS",
                "Resolve a single pole, two distinct real poles, a repeated "
                "real pole, or a complex-conjugate pair.",
                expression},
               {"FORWARD_RECONSTRUCTION",
                "Apply the forward transform to the returned time-domain terms "
                "and compare against the original rational function at 12 "
                "complex points in the ROC.",
                "maximum normalized difference=" + format(error)}};
  out.assumptions = {
      "Real coefficients in descending powers of s; internal zeros must be "
      "retained. Only strictly proper denominator degree 1–2 is supported.",
      "Near-repeated poles use a scale-relative 1e−12 discriminant tolerance; "
      "verification is numerical, not an exact symbolic proof."};
  verify(out, error, "forward-transform reconstruction",
         "12 complex evaluation points checked", 1e-7);
  out.visual_json =
      Json(Json::Object{
               {"kind", "signal"},
               {"label", "Causal inverse Laplace amplitude versus seconds"},
               {"points", plot(y, duration / (count - 1))},
               {"samples", array_json(y)}})
          .dump();
  return out;
}
} // namespace
SolutionBundle signals(const ProblemSpec &request) {
  const auto data = Json::parse(request.input);
  const auto operation = data.at("operation").string();
  if (operation == "fft")
    return transform(data, false);
  if (operation == "ifft")
    return transform(data, true);
  if (operation == "convolution")
    return convolution(data, false);
  if (operation == "correlation")
    return convolution(data, true);
  if (operation == "generator")
    return generator(data);
  if (operation == "frequency_response")
    return response(data);
  if (operation == "laplace")
    return laplace(data);
  if (operation == "inverse_laplace")
    return inverse_laplace(data);
  return digital_signals(request);
}
} // namespace pocket_engineer::workbench
