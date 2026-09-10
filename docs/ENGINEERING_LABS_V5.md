# Offline visual engineering labs · v0.5

Implementation and validation record, 2026-09-10. This extends the original 55-topic engine; it does not claim to solve every engineering exercise or eliminate lag on every device.

## What changed

- Guided, C++-defined forms for the original catalog, including editable matrix cells and dimension controls. The text path remains available; natural interpretation of the original topics is unchanged.
- Clickable 2–6 variable Karnaugh maps, Gray-code labels, four panels for six variables, variable-order controls, 0→1→X cycling, automatic minimization and validated manual groups. Keyboard focus survives cycling. A minimum cover is claimed only when the bounded search proves it; otherwise the function is still exhaustively checked on every input assignment and the limitation is explicit.
- A 24×18 schematic grid with R/C/L/V/I textbook symbols, orthogonal wires, junction dots, shared ground, move/edit/delete, undo/redo and saved drawing JSON. C++ derives the netlist from connectivity. Crossings without a junction do not connect. An edited netlist is preserved separately until **Replace netlist with current drawing** is pressed.
- DC, AC RMS phasors and fixed-step backward-Euler RLC analysis. Advanced netlists also support VCCS/VCVS/CCCS/CCVS. Results include voltages, currents, powers, actual MNA equations and normalized residual checks.
- Computed Thévenin/Norton equivalents for an unloaded port, a separately checked probe load, maximum-power conditions, source-by-source superposition, AC sweeps and local component sensitivity.
- Moore/Mealy transition tables, reachable-state reduction, a raster state diagram, state encodings, D-flip-flop input/output SOP equations and input-sequence simulation. The default is an overlapping 101 detector.
- Thirteen signals/transform forms: FFT, IFFT, convolution, correlation, generator, analog frequency response, Laplace, inverse Laplace, difference-equation filter, digital frequency response, FIR design, finite Z-transform and inverse Z-transform.
- PNG diagrams and JSON data exports. Android prepares exports on an I/O executor and stores only a temporary filename in activity state, not a multi-megabyte payload.
- Draft preservation across an explicitly requested website update for original text, circuit model/netlist, K-map state and lab fields. Switching lab tabs or closing guided input preserves editable values.

All solving, topology, validation, form definitions, minimization, state reduction and diagram geometry are implemented in C++20. JavaScript/Kotlin provide browser/device presentation, storage and transport. No SVG, remote model, account or online calculation service is used.

## Using the offline demo bank

Press **Test demos · 825k cases**, then select subject → topic → difficulty. The 165 checked-in gzip-compressed `.pebank` files contain the actual original stored corpus: 55 topics × 3 difficulties × 5,000 rows. Source and compressed/expanded SHA-256 digests are recorded in `www/demo-data/manifest.json`. `tools/build_demo_bank.mjs` reproduces the files from a generated `test-data` directory.

Only one set is decompressed at a time, off the UI thread. The visible table has 25 rows, not 825,000 DOM entries. **Calibrate** measures 25 calls on this device, discards the first warm-up call for percentiles and estimates a range for 5,000 calls. The estimate includes bridge round trips, not just numerical arithmetic. It is not a worst-case promise. **Run all 5,000** supports cancellation after the current bounded solve. Export includes each tested input, stored answer, current answer, verification labels, match decision and elapsed time.

Snapshot equality is not independent mathematical verification. Some original bounded families have repeated inputs. New circuit/signal operations have independent deterministic tests and presets, but are not represented as 5,000 stored snapshots for every new operation.

Actual API35 APK testing found two Android-only failures: worker fetch could not read bundled files, and Android's asset merger transparently expanded `.gz` assets while removing their suffix. `.pebank` preserves the original compressed bytes; Android asynchronously reads those bytes in the WebView and transfers the buffer to the verification/decompression worker. `tools/test_android_assets.mjs` checks all 825,000 packaged records and both hashes inside the assembled APK. Browser transport simulation is labelled separately from installed-APK evidence.

Android comparisons use C++ batches of at most 25 records, reduced further when needed to stay below the 32 KB bridge budget. Cancellation is checked between batches. Results retain expected and actual answers per record; `elapsed_ms` is explicitly amortized batch round-trip time, not a claim of individually timed calls. `engine_duration_ms` measures each C++ solve. Calibration still uses serial calls and labels that distinction.

## Offline readiness and performance

The service worker uses a content-hashed manifest. **Prepare / repair offline access** handles missing registration, missing files and corrupt cached bytes. All core files and all demo files must be present before readiness is reported. Downloads use three bounded parallel requests and retain partial work for retry. An update activates only after the user chooses it; input drafts are saved before reloading. A previously installed v0.4 page has no new update control: close all its tabs and reopen after the new worker finishes installing.

Live deployment exposed a CDN-specific defect that localhost did not: Cloudflare Polish recompressed PNG icons (`cf-polished: ok`), so bytes no longer matched the original hash. For example, the 192-pixel icon changed from 2,961 to 2,712 bytes. Hash checking was not weakened. The build now packs original rasters into a separately hashed `offline-images.json` transport; the worker verifies the archive and every reconstructed image, then caches each image at its normal URL. This raises the critical bundle to approximately 1.25 MB and the complete cached set to 9.2 MB, still below both gates. The regression simulates transformed CDN images and checks exact original cached hashes and offline display.

A second live deployment check caught mixed CDN generations: `service-worker.js` still had the previous deployment's `Last-Modified` while newer files were served. Initial/explicit repair registration now refreshes the worker URL, and verified downloads include the expected hash in their CDN cache key. Canonical cache URLs remain unchanged for offline navigation. A local HTTP regression deliberately serves stale unversioned worker/manifest/bank responses and requires installation plus offline demo solving to succeed. Old valid installations are retained until the user activates a completed update.

Visit `https://pocket-engineer.onrender.com/` online once, wait for **Ready offline**, then use that same URL in the same browser offline. Browser installation is optional for URL revisits, but recommended. A fresh device, private session, cleared site data or evicted cache cannot already possess the files. **Protect offline storage** requests persistent storage; only the browser decides whether to grant it. Native desktop mode needs its local launcher running. Android bundles the website and solver and has no INTERNET permission.

The WASM build uses `-Oz`, while native/JNI retains optimized Release settings. The first expanded `-O3` browser build exceeded its 1.5 MB gate (1,714,361 bytes); the current size-optimized core is about 1.25 MB. The full installed set is about 9.2 MB, including 825,000 demos. Exact numbers are emitted by `node tools/verify_web_assets.mjs` for each build. Matrix LU factors are reused for transient steps, superposition and sensitivity solves. Solver/data workers, bounded searches, 25-row pagination and bounded raster sizes keep intensive calculation away from the UI thread.

“Smoothness established” requires measured frame presentation on specified devices and journeys, not merely passing a phone-sized browser test. Previous isolated release-emulator scrolling measured 1.83–2.50% native jank; the old combined ~32% measurement was not a comparable isolated run. The emulator and its host can stall even a plain control page. Neither these numbers nor JavaScript requestAnimationFrame timing proves physical-phone 60 FPS. Physical-device validation, production Android signing, Windows signing and macOS notarization remain separate requirements. See the immutable [v4 measurements](JANK_INPUT_DOWNLOADS_V4.md).

## Structured interface

The same operations are exposed through `pe_workbench_json`, Android JNI method `workbench`, the WASM worker, and the desktop `POST /api/workbench` route. The outer request is bounded to 32,768 UTF-8 bytes. `input` and `payload` are strings containing the operation's JSON or netlist; unknown keys, duplicate keys, excessive depth and nonfinite values are rejected. Result JSON contains status, steps, answer, verification, warnings and optional visual data. Preserve the complete request when exporting for reproducibility.

Example outer request for convolution:

```json
{"domain":"workbench","topic":"signals","input":"{\"operation\":\"convolution\",\"x\":\"1,2,3\",\"h\":\"4,5\"}","mode":"manual"}
```

Expected samples: `[4,13,22,15]`. `engineering_schema` returns C++ form definitions; `engineering_input` validates string fields and returns structured operation input. `schema`/`guided_input` do the same for the original catalog. `circuit_editor` and `kmap_editor` return validated models, not HTML.

### Circuit netlists and conventions

Separate parts with semicolons/newlines:

```text
V V1 in 0 12
R R1 in out 1k
C C1 out 0 1u
```

For `topic: network`, use `payload: {"analysis":"ac","frequency":159.154943091895}` encoded as a string. This RC example gives `V(out)=6-j6 V` for a 12 V RMS source. Currents are positive from the first node to the second; absorbed complex power is `V × conjugate(I)`. Independent AC sources accept `magnitude@degrees`. Suffixes include p/n/u/m/k/K/M/meg/G; `m` means milli, not mega.

Supported syntax:

```text
R/C/L/V/I name n+ n- value
G/E name n+ n- control+ control- gain
F/H name n+ n- controlling-voltage-source-name gain
```

Ground is `0`. At DC, capacitors open and inductors impose zero voltage. AC uses `jωC` and `1/(jωL)`. No diode/transistor, nonlinear iteration, arbitrary time-dependent source, mutual inductance or general SPICE import is implemented. The drawing palette is R/C/L/V/I; controlled sources are entered in the netlist.

Transient settings are `analysis: transient`, `step`, `steps`, optional `observe`, and `initial` mapping capacitor names to initial volts / inductor names to initial amps. This is first-order backward Euler with an h/2 endpoint refinement comparison, not an adaptive solver or a universal error bound. Maximum 48 components, 32 nonground nodes, 80 circuit unknowns, 2,000 steps, plus aggregate work limits.

For `topic: network_study`, payload additionally specifies:

| operation | Additional fields and meaning |
| --- | --- |
| `port` | `positive`, `negative` (default 0), `load_real`, `load_imag`; provide the circuit without its external load. Suppression retains dependent sources; a 1 A test source obtains impedance. A full loaded-network solve checks the equivalent. |
| `superposition` | optional `observe`; at most 16 independent sources. Sum all unknowns and compare with a full all-sources solve. Power is explicitly not superposed. |
| `sweep` | AC only, `frequency`, `frequency_high`, `count` (2–256), optional `observe`. Log-spaced magnitudes/phases; the largest sampled value is not a mathematically exact resonance. |
| `sensitivity` | optional `observe`, `tolerance` (fraction in (0,0.5]). Analytic MNA derivative checked against a four-point finite-difference solve. The linearized tolerance estimate is local, not a rigorous global bound. |

### Signals and transform limits

Use `topic: signals` and `operation` in the structured input. The UI renders the canonical fields from C++; use **Copy form into JSON** to obtain exact field names and a runnable request.

- FFT/IFFT: radix-2, up to 4,096 samples; forward explicit padding and rectangular/Hann/Hamming windows. Forward sign is negative with no 1/N scaling; inverse sign positive with 1/N. Full complex bins, Parseval and reconstruction checks are supplied. They are raw two-sided coefficients, not silently rescaled single-sided amplitude spectra.
- Convolution/correlation: linear, real sequences; FFT padding avoids circular aliasing. Output up to 8,191 samples. All direct positions are independently checked for short inputs; long outputs use 66 stated direct checkpoints. Correlation is not normalized.
- Generator: uniformly sampled supported waveforms. Range validation is labelled `not_verified`, not an independent mathematical oracle. Sampling above Nyquist can alias.
- Analog response: real polynomial coefficients in descending powers of s; logarithmic frequency grid, up to 512 points. Rational evaluation is checked; no general pole/stability proof is implied.
- FIR/IIR filter: real coefficients in delay order, normalized by a0; up to 4,096 samples, numerator order 256 and denominator order 64. Direct-form-II transposed output is cross-checked against a long-double direct-form-I recurrence, including explicit past input/output histories. Unstable/diverging output is bounded. Schur recursion reports denominator stability or a boundary/conditioning limitation, not pole-zero cancellation analysis.
- Digital response: delay-order coefficients on the unit circle, linear Hz grid, up to 512 frequencies; Horner versus direct trigonometric-sum checks.
- FIR design: odd 3–255 taps, low/high/bandpass/bandstop, rectangular/Hann/Hamming/Blackman window, explicitly normalized passband reference. Reports group delay, symmetry, gain, coefficients and sampled response. It does not promise arbitrary ripple/attenuation specifications.
- Finite bilateral Z: real samples with an explicit starting index, a finite Laurent polynomial and a conservative ROC; direct complex-power cross-checks. A finite sequence and a causal infinite response are distinct operations.
- Inverse Z: causal rational models with denominator degree 1–2, or FIR, up to 512 samples. Handles distinct/repeated/complex poles and feedthrough, checks against an impulse-filter recurrence and forward complex evaluation. No arbitrary-degree symbolic inverse is claimed.
- Laplace: 1–16 causal delayed exponential-power/sine/cosine terms, with the stated ROC. Each kernel is cross-checked against a numerical defining integral. Inverse Laplace accepts strictly proper real rational functions of denominator degree 1–2 and checks forward reconstruction. These are explicit transform families, not unrestricted symbolic integration.

### State machines

1–16 states, 1–2 input bits and 1–4 output bits. Every state/input combination must have exactly one transition. Moore output belongs to a state; Mealy output belongs to an edge. The reduced reachable machine is checked for output-equivalence, transition consistency and simulated trace agreement. Unreachable states are listed. Equations use binary state encoding and D flip-flops; JK/T synthesis, asynchronous timing and hardware hazard freedom are not implied. Diagrams are C++ geometry rendered to raster canvas; dense graphs may require zoom/pan. Tables and full JSON remain the authoritative exact mappings.

## Validation record

Local Release: all five CTest suites pass. Independent original-engine stress: **1,606,929 / 1,606,929** checks. Stored-corpus replay: **825,000 / 825,000** answers and verification labels match. New workbench suite: **23,374 / 23,374** checks, including 5,000 independently expected batched arithmetic cases. New engineering-studies suite: **31,941 / 31,941** checks, including 5,000 randomized AC RC cases against analytic real/imaginary formulas. AddressSanitizer + UndefinedBehaviorSanitizer: all five suites pass after the batching changes.

One real failure was caught in superposition verification: a nearly-zero residual row was normalized against its own nearly-zero magnitude. Two checks failed at approximately 8.67e-7 despite a correct linear solve. The corrected branch-law replay uses a scale-aware absolute floor for near-zero rows, while still checking all node constraints. The original failing and repaired logs are retained with test history; the verification was corrected, not disabled.

The first full browser run passed 56/58: the two failures measured only the checkbox glyph, ignoring its clickable label. The test now measures the actual label target. Expanded tests cover keyboard focus, update drafts, corrupted-cache repair, 320/390/768/1440 layouts and actual controls for all 13 signals operations. Final CI/Android/download results belong in [test history](TEST_HISTORY.md); local passes do not stand in for unrun platform tests.

`node tools/source_audit.mjs` gates ≥80% C++ of maintained runtime code and ≥77% including HTML/CSS. The current result is approximately 82.13% / 77.12%. Tests, generated corpora, documentation, third-party code and Emscripten-generated JavaScript are excluded from both denominators; their bytes are not disguised as C++ implementation. `node tools/verify_web_assets.mjs` independently gates actual browser download size.

## Primary algorithm references

Implementation follows standard methods, with independent tests; none of these sources is a claim of exhaustive curriculum coverage.

- Erik Cheever, Swarthmore: [Modified Nodal Analysis](https://lpsa.swarthmore.edu/Systems/Electrical/mna/MNAAll.html).
- Julius O. Smith III: [DFT theorems](https://www.dsprelated.com/freebooks/mdft/Fourier_Theorems_DFT.html), [convolution theorem](https://www.dsprelated.com/freebooks/mdft/Convolution_Theorem.html), [Schur-Cohn pole stability](https://www.dsprelated.com/freebooks/filters/Pole_Zero_Analysis_I.html), [window-method FIR design](https://www.dsprelated.com/freebooks/SASP/Window_Method_FIR_Filter.html).
- MIT Computation Structures: [finite-state-machine notes](https://computationstructures.org/notes/fsms/notes.html).
- Browser delivery model: [PWA serving](https://web.dev/learn/pwa/serving), [offline data and eviction](https://web.dev/learn/pwa/offline-data).
