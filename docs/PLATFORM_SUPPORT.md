# Platforms, offline operation and downloads

## Website / PWA

The published site is https://muhammadtahabinzaeem.github.io/FOP-Project/.

A Web Worker loads an Emscripten build of the C++ engine. The service worker caches a versioned, bounded list including engine.js and engine.wasm. No solve request goes to a remote API. Assets use relative URLs for subpath deployment.

1. Open the site once online.
2. Open “Get the app” and wait for “Ready offline.”
3. Install with Chrome/Edge's install option, or use Android Chrome's Install app / Add to home screen.
4. Disconnect and reopen the same address. The browser test suite exercises this cold reload and a fresh solve.

A standalone desktop PWA can be installed using Chrome/Edge on Windows, macOS or Linux. Modern Firefox can run the website but does not offer the same desktop installation UI. Private mode, storage eviction, user-cleared site data, restrictive browser policies, or insufficient space can remove/prevent offline availability. First-ever use requires the solver download. Do not open index.html as a file:// page.

Requirements: JavaScript, WebAssembly, Web Workers, and HTTPS or localhost for service workers. The C++ browser heap has a 128 MiB maximum and grows on demand; that is a safety cap, not a measured total browser memory footprint. Main-thread UI work remains small, and graph data is sampled.

## Native Android

The Gradle application supports Android API 24+ with an updated Android System WebView. ABI builds use the NDK; Java/Kotlin source targets JDK 17. The app has no INTERNET permission and bundles the C++ solver and website, so it works on first launch without a prior website visit.

Security/lifecycle:

- WebViewAssetLoader serves trusted assets at an HTTPS-style local origin.
- File/content access and mixed content are disabled.
- Third-party subresources are blocked, and allowed GitHub links open outside the WebView.
- The JNI boundary uses standard UTF-8 byte arrays, not Modified UTF-8.
- Calculation runs on a single native-work executor and returns through a JSON-safe callback.
- Insets, WebView pause/resume/destruction and back navigation are handled.

Build with Android Studio, or the pinned SDK/NDK/CMake/Gradle versions in the workflow:

```sh
gradle -p android :app:assembleDebug :app:assembleRelease
gradle -p android :app:connectedDebugAndroidTest
```

The **debug APK is installable and development-signed**. The release APK is **unsigned** until an owner-controlled release signing key is configured; it must not be advertised as a signed production release. Never commit a private signing key.

Instrumentation launches the real WebView, waits for the JNI engine, solves arithmetic and an inconsistent linear system, then recreates the activity and checks local history. The manifest's absent INTERNET permission makes a remote solver unavailable during this test.

A build or emulator pass is not evidence that every old physical Android device performs well. No low-end physical phone benchmark is claimed.

## Native Windows / macOS / Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cpack --config build/CPackConfig.cmake -G ZIP -C Release
```

The package contains native CLI/server executables, the website, corpus tools and documentation. Launch the server executable and visit the printed 127.0.0.1 address. It serves only loopback, not a public production endpoint. The website uses native HTTP fallback if WASM assets are not included; keep the server running in that mode.

Windows and macOS binaries are compiled by their respective GitHub runners. GitHub Actions also uploads a self-contained website artifact after compiling the browser engine. Download links in the UI distinguish release assets from development build artifacts.

## Test interpretation

See [TEST_HISTORY.md](TEST_HISTORY.md) for completed runs. A successful earlier 0.2.0 build is not proof that the changed 0.3.0 application passed; consult the source commit and run attached to each result.

Technical basis: [Android local-content guidance](https://developer.android.com/develop/ui/views/layout/webapps/load-local-content), [Emscripten modular output](https://emscripten.org/docs/compiling/Modularized-Output.html), and [C++/JavaScript interoperation](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html).
