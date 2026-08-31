# Test history and result comparison

This file records reproducible evidence for the current Pocket Engineer release candidate. A green unit test alone is not treated as evidence that all generated answers are correct: the full corpus comparison re-runs every stored input through the C++ solver and compares both the answer text and verification status.

## 2026-08-31 — release candidate 0.2.0

### Commands

~~~
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DPE_ENABLE_IPO=OFF
cmake --build build-release -j2
ctest --test-dir build-release --output-on-failure
./build-release/pe-verify-corpus test-data 0 docs/generated/CORPUS_COMPARISON.json
./build-release/pe-generate-explanations explanation-data 10000
~~~

### Results

| Check | Result |
| --- | --- |
| C++20 release build | passed |
| CTest engine suite | 1 / 1 passed, 0 failed |
| Golden inputs compared | 825,000 |
| Answer text matches | 825,000 correct; 0 incorrect |
| Verification-status matches | 825,000 correct; 0 incorrect |
| Solver errors during comparison | 0 |
| Overall answer-and-verification matches | 825,000 / 825,000 |
| Release comparison elapsed time | 12.221 seconds |
| Throughput | about 67,500 cases/second |
| Generated explanation rows | 550,000 |
| Explanation-data size | about 180 MB |
| SVG assets in www | 0 |
| Linux offline ZIP package | generated and contents inspected |

The machine-readable per-topic comparison is [CORPUS_COMPARISON.json](generated/CORPUS_COMPARISON.json). Every one of the 55 declared topics has 15,000 cases and reports 15,000 answer matches, 15,000 verification matches, and zero solver errors.

The generated Linux package was PocketEngineer-0.2.0-Linux.zip with SHA-256 635328b7bea3f091b92f1bb369d7273643813adfaedf4d86b1bc1d8927bd0e5b. Its ZIP contents include the native CLI, local server, corpus tools, website, PNG assets, and platform documentation.

Expected verifier distribution in the comparison:

| Verification kind | Cases |
| --- | ---: |
| verified_exact | 495,000 |
| verified_exhaustive | 75,000 |
| verified_numerical | 255,000 |

### Platform checks

| Platform path | Current evidence |
| --- | --- |
| Linux desktop | release build, CTest, full corpus comparison, local server/API, and CPack package tested in this workspace |
| Windows desktop | portable WinSock server and CMake/CPack path are in source; GitHub Actions builds/tests/packages on windows-latest after push |
| macOS desktop | POSIX server and CMake/CPack path are in source; GitHub Actions builds/tests/packages on macos-latest after push |
| Android API 24+ | offline WebView/JNI project is included; local SDK/NDK/Gradle are absent in this workspace, so no APK is falsely recorded as built |

### Comparison semantics

A case counts as correct only if:

1. the native solver returns success;
2. its answer text exactly matches the golden expected answer; and
3. its verification status exactly matches the expected status.

The comparison tool retains up to ten mismatch examples in its JSON report if any result fails. The current report has an empty mismatch list.

## Ongoing regression procedure

After a solver, parser, or explanation change:

1. Run CTest.
2. Regenerate the corpus only if the public solver contract intentionally changed.
3. Run the full comparison and commit the new small JSON report.
4. Inspect any mismatch examples before accepting an answer change.
5. Let GitHub Actions repeat a smoke generation/comparison and produce Windows, macOS, and Linux packages.
