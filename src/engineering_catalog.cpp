#include "pocket_engineer/workbench.hpp"
#include <algorithm>
#include <stdexcept>

namespace pocket_engineer::workbench {
namespace {
struct Field {
  std::string id;
  std::string label;
  std::string initial;
  std::string type;
  std::vector<std::string> choices;
  bool optional{};
};
struct Operation {
  std::string id;
  std::string title;
  std::string scope;
  std::vector<Field> fields;
};
const std::vector<Operation> &operations() {
  static const Field x{"x", "Samples x (comma-separated)", "1,0,-1,0,1,0,-1,0", "textarea", {}};
  static const Field rate{"fs", "Sample rate (Hz)", "1000", "number", {}};
  static const Field numerator{"numerator", "Numerator b[0], b[1], … (delay coefficients)", "0.25,0.5,0.25", "text", {}};
  static const Field denominator{"denominator", "Denominator a[0], a[1], … (delay coefficients)", "1", "text", {}};
  static const Field plot_count{"count", "Plot samples (2–512)", "128", "number", {}};
  static const std::vector<Operation> values{
      {"fft", "FFT · time to frequency",
       "Raw two-sided radix-2 DFT. Non-power-of-two length requires explicit "
       "zero padding. Forward sign is negative; no 1/N forward normalization.",
       {x, {"imag", "Imaginary samples (blank = zero)", "", "textarea", {}, true},
        rate, {"window", "Time-domain window", "rectangular", "select",
               {"rectangular", "hann", "hamming"}},
        {"pad", "Zero pad non-power-of-two input?", "false", "boolean", {"false", "true"}}}},
      {"ifft", "Inverse FFT",
       "Inverse radix-2 transform with 1/N normalization. Supply full two-sided "
       "complex coefficients; arbitrary frequency spacing is not inferred.",
       {{"x", "Real frequency coefficients", "0,0,4,0,0,0,4,0", "textarea", {}},
        {"imag", "Imaginary frequency coefficients (blank = zero)", "", "textarea", {}, true}, rate}},
      {"convolution", "Linear convolution",
       "Finite real sequences, zero outside the supplied range. Output length "
       "N+M−1; zero-padded FFT prevents circular wrap-around.",
       {x, {"h", "Second sequence h", "0.25,0.5,0.25", "textarea", {}}}},
      {"correlation", "Cross-correlation",
       "Unnormalized real cross-correlation r[l]=sum x[n]h[n−l]. Lags start "
       "at −(length(h)−1); this is not a circular or normalized correlation.",
       {x, {"h", "Second sequence h", "0.25,0.5,0.25", "textarea", {}}}},
      {"generator", "Generate a signal",
       "Uniformly sampled sine, cosine, square, triangle, impulse or step. "
       "Square and triangle harmonics are not automatically band-limited.",
       {{"waveform", "Waveform", "sine", "select", {"sine", "cosine", "square", "triangle", "impulse", "step"}},
        {"count", "Sample count (1–4096)", "128", "number", {}}, rate,
        {"frequency", "Frequency (Hz)", "50", "number", {}},
        {"amplitude", "Amplitude", "1", "number", {}},
        {"phase", "Phase (degrees)", "0", "number", {}},
        {"offset", "DC offset", "0", "number", {}}}},
      {"frequency_response", "Analog transfer function / Bode",
       "Continuous-time real rational H(s), evaluated at s=j2πf. Coefficients "
       "are in descending powers of s, not increasing delay powers.",
       {{"numerator", "Numerator coefficients · descending powers of s", "1", "text", {}},
        {"denominator", "Denominator coefficients · descending powers of s", "0.001,1", "text", {}},
        {"f_min", "Minimum frequency (Hz)", "1", "number", {}},
        {"f_max", "Maximum frequency (Hz)", "100000", "number", {}}, plot_count}},
      {"laplace", "Laplace · elementary causal terms",
       "Transforms A·exp(a(t−d))·g(t−d)·u(t−d). The advanced JSON editor accepts "
       "sums of up to 16 exponential-weighted power, sine or cosine terms.",
       {{"kind", "Causal kernel: power = tⁿ", "power", "select", {"power", "sine", "cosine"}},
        {"amplitude", "Amplitude A", "1", "number", {}},
        {"rate", "Exponential rate a (per second)", "0", "number", {}},
        {"omega", "Angular frequency ω (rad/s)", "2", "number", {}},
        {"power", "Power n (0–12)", "2", "number", {}},
        {"delay", "Delay d (s)", "0", "number", {}}}},
      {"inverse_laplace", "Inverse Laplace · degree 1–2",
       "Causal, strictly proper real rational functions with denominator "
       "degree 1 or 2. Coefficients are in descending powers of s.",
       {{"numerator", "Numerator coefficients · descending powers of s", "1", "text", {}},
        {"denominator", "Denominator coefficients · descending powers of s", "1,3,2", "text", {}},
        {"duration", "Plot duration (s)", "5", "number", {}}, plot_count}},
      {"filter", "FIR / IIR difference equations",
       "Evaluate a causal difference equation with explicit past input/output "
       "histories. O(samples × order) work; every output is independently replayed.",
       {x, numerator, denominator, rate,
        {"past_input", "Past x[−1], x[−2], … (blank = zero history)", "", "text", {}, true},
        {"past_output", "Past y[−1], y[−2], … (blank = zero history)", "", "text", {}, true}}},
      {"digital_response", "Digital filter frequency response",
       "Evaluate H(z) on z=exp(jω), with a Schur denominator-stability check. "
       "Delay coefficients are b0,b1,… and a0,a1,… .",
       {numerator, denominator, rate,
        {"f_min", "Minimum frequency (Hz)", "0", "number", {}},
        {"f_max", "Maximum frequency (≤ fs/2)", "500", "number", {}}, plot_count}},
      {"fir_design", "Design a windowed FIR filter",
       "Odd-length, linear-phase windowed-sinc design. Inspect the response; "
       "arbitrary ripple or stopband attenuation specifications are not guaranteed.",
       {{"kind", "Passband type", "lowpass", "select", {"lowpass", "highpass", "bandpass", "bandstop"}},
        {"taps", "Odd tap count (3–255)", "31", "number", {}}, rate,
        {"cutoff", "Cutoff / lower band edge (Hz)", "100", "number", {}},
        {"cutoff_high", "Upper band edge (bandpass / bandstop only)", "200", "number", {}},
        {"window", "Window", "hamming", "select", {"rectangular", "hann", "hamming", "blackman"}}}},
      {"z_transform", "Finite bilateral Z-transform",
       "An explicitly indexed finite sequence; samples outside its range are "
       "zero. Reports the Laurent polynomial, ROC and a complex evaluation.",
       {x, {"origin", "Index of first sample (−1024…1024)", "0", "number", {}},
        {"z_real", "Evaluation point: real part of z", "1.2", "number", {}},
        {"z_imag", "Evaluation point: imaginary part of z", "0.1", "number", {}}}},
      {"inverse_z", "Causal inverse Z-transform",
       "Real rational denominator degree 1–2, or a finite FIR numerator. "
       "The causal ROC and closed-form tail are checked separately from the plot.",
       {{"numerator", "Numerator delay coefficients", "1", "text", {}},
        {"denominator", "Denominator delay coefficients", "1,-0.5", "text", {}},
        {"count", "Impulse-response samples (2–512)", "64", "number", {}}}}
  };
  return values;
}
const Operation &operation(const std::string &id) {
  const auto &all = operations();
  const auto found = std::find_if(all.begin(), all.end(),
                                 [&](const auto &item) { return item.id == id; });
  if (found == all.end())
    throw std::runtime_error("Unknown engineering form operation");
  return *found;
}
Json describe(const Operation &operation) {
  Json::Array fields;
  for (const auto &field : operation.fields) {
    Json::Array choices;
    for (const auto &choice : field.choices)
      choices.emplace_back(choice);
    // Presentation metadata only: hidden settings retain the same validated
    // defaults and remain editable. These flags never alter solver inputs.
    const auto &id = field.id;
    const bool advanced = field.optional || id == "window" || id == "pad" ||
        id == "phase" || id == "offset" || id == "rate" || id == "delay" ||
        id == "origin" || id == "z_real" || id == "z_imag" ||
        (id == "count" && operation.id != "generator") ||
        (id == "fs" && (operation.id == "fft" || operation.id == "ifft" ||
                        operation.id == "filter"));
    Json when = Json::Object{};
    if (operation.id == "laplace" && (id == "power" || id == "omega"))
      when = Json::Object{{"field", "kind"}, {"values", id == "power"
          ? Json::Array{"power"} : Json::Array{"sine", "cosine"}}};
    if (operation.id == "fir_design" && id == "cutoff_high")
      when = Json::Object{{"field", "kind"},
                          {"values", Json::Array{"bandpass", "bandstop"}}};
    if (operation.id == "generator" && id == "frequency")
      when = Json::Object{{"field", "waveform"},
          {"values", Json::Array{"sine", "cosine", "square", "triangle"}}};
    fields.emplace_back(Json::Object{{"id", field.id}, {"label", field.label},
                                     {"value", field.initial}, {"type", field.type},
                                     {"advanced", advanced}, {"when", when},
                                     {"options", choices}, {"optional", field.optional}});
  }
  return Json::Object{{"status", "success"}, {"operation", operation.id},
                      {"title", operation.title}, {"scope", operation.scope},
                      {"fields", fields}};
}
} // namespace
Json engineering_schema(const ProblemSpec &request) {
  if (request.input.empty()) {
    Json::Array catalog;
    for (const auto &operation : operations())
      catalog.push_back(describe(operation));
    return Json::Object{{"status", "success"}, {"operations", catalog}};
  }
  return describe(operation(request.input));
}
Json engineering_form_input(const ProblemSpec &request) {
  const auto data = Json::parse(request.input);
  data.only({"operation", "values"});
  const auto &specification = operation(data.at("operation").string());
  const auto &values = data.at("values").object();
  for (const auto &[key, value] : values) {
    (void)value;
    if (std::none_of(specification.fields.begin(), specification.fields.end(),
                     [&](const auto &field) { return field.id == key; }))
      throw std::runtime_error("Unknown form field: " + key);
  }
  Json::Object input{{"operation", specification.id}};
  for (const auto &field : specification.fields) {
    const auto found = values.find(field.id);
    const auto text = found == values.end() ? "" : trim(found->second.string());
    if (text.empty()) {
      if (field.optional)
        continue;
      throw std::runtime_error("Enter a value for " + field.label);
    }
    if (field.type == "number")
      input[field.id] = finite_number(text);
    else if (field.type == "boolean") {
      if (text != "true" && text != "false")
        throw std::runtime_error("Boolean form choices must be true or false");
      input[field.id] = text == "true";
    } else if (field.type == "select") {
      if (std::find(field.choices.begin(), field.choices.end(), text) == field.choices.end())
        throw std::runtime_error("Invalid selection for " + field.label);
      input[field.id] = text;
    } else {
      if (text.size() > 24000)
        throw std::runtime_error("Form text exceeds the interactive input budget");
      input[field.id] = text;
    }
  }
  if (specification.id == "laplace") {
    input.erase("operation");
    input = Json::Object{{"operation", "laplace"}, {"terms", Json::Array{input}}};
  }
  return Json::Object{{"status", "success"}, {"input", input}};
}
} // namespace pocket_engineer::workbench
