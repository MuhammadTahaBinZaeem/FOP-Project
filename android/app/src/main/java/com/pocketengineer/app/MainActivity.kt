package com.pocketengineer.app

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Intent
import android.content.ClipData
import android.content.ClipboardManager
import android.graphics.Color
import android.net.Uri
import android.os.Bundle
import android.webkit.JavascriptInterface
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebView
import android.webkit.WebViewClient
import android.webkit.WebChromeClient
import androidx.webkit.WebViewAssetLoader
import org.json.JSONObject
import java.util.concurrent.Executors
import java.util.concurrent.RejectedExecutionException

/** All web assets are trusted, bundled files. No remote page receives the JNI bridge. */
class MainActivity : Activity() {
    companion object { init { System.loadLibrary("pocketengineer_jni") } }
    private lateinit var webView: WebView
    private val solver = Executors.newSingleThreadExecutor()
    @Volatile private var destroyed = false
    private var pendingExport: String? = null
    private external fun nativeDispatch(method: Int, input: ByteArray): ByteArray

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        pendingExport = savedInstanceState?.getString("pendingExport")
        val assets = WebViewAssetLoader.Builder()
            .addPathHandler("/assets/", WebViewAssetLoader.AssetsPathHandler(this)).build()
        webView = WebView(this)
        webView.tag = "pocket-engineer-web"
        // Debug APK only: enables local adb/CDP inspection. Release stays closed.
        WebView.setWebContentsDebuggingEnabled(
            applicationInfo.flags and android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE != 0)
        webView.setBackgroundColor(Color.rgb(245, 245, 238))
        webView.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true
            allowFileAccess = false
            allowContentAccess = false
            setSupportMultipleWindows(false)
            mixedContentMode = android.webkit.WebSettings.MIXED_CONTENT_NEVER_ALLOW
        }
        // Apply insets to a native container, not WebView padding: fixed-position
        // HTML controls must use the actual unobscured child viewport dimensions.
        val container = android.widget.FrameLayout(this)
        container.addView(webView, android.widget.FrameLayout.LayoutParams(
            android.view.ViewGroup.LayoutParams.MATCH_PARENT, android.view.ViewGroup.LayoutParams.MATCH_PARENT))
        container.setOnApplyWindowInsetsListener { view, insets ->
            @Suppress("DEPRECATION")
            view.setPadding(insets.systemWindowInsetLeft, insets.systemWindowInsetTop,
                insets.systemWindowInsetRight, insets.systemWindowInsetBottom)
            @Suppress("DEPRECATION")
            insets.consumeSystemWindowInsets()
        }
        webView.webViewClient = object : WebViewClient() {
            override fun shouldInterceptRequest(view: WebView, request: WebResourceRequest): WebResourceResponse? {
                val local = assets.shouldInterceptRequest(request.url)
                if (local != null) return local
                // No third-party resources or network fallback, even after navigation.
                return WebResourceResponse("text/plain", "UTF-8", 403, "Blocked", emptyMap(),
                    "Remote resources are disabled".byteInputStream())
            }
            override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                val uri = request.url
                if (uri.scheme == "https" && uri.host == "appassets.androidplatform.net"
                    && uri.path == "/assets/index.html") return false
                if (request.isForMainFrame && request.hasGesture() && uri.scheme == "https"
                    && uri.host == "github.com") {
                    try { startActivity(Intent(Intent.ACTION_VIEW, uri)) } catch (_: Exception) {}
                }
                return true
            }
        }
        webView.webChromeClient = WebChromeClient()
        webView.addJavascriptInterface(Bridge(), "PocketEngineerAndroid")
        setContentView(container)
        webView.loadUrl("https://appassets.androidplatform.net/assets/index.html")
    }

    private inner class Bridge {
        @JavascriptInterface
        fun copySolution(text: String) {
            if (text.length > 1000000) return
            runOnUiThread { if (!destroyed) (getSystemService(CLIPBOARD_SERVICE) as ClipboardManager)
                .setPrimaryClip(ClipData.newPlainText("Pocket Engineer solution", text)) }
        }

        @JavascriptInterface
        fun saveSolution(json: String) {
            if (json.length > 1000000) return
            runOnUiThread {
                if (!destroyed && pendingExport == null) {
                    pendingExport = json
                    @Suppress("DEPRECATION")
                    try { startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                        addCategory(Intent.CATEGORY_OPENABLE)
                        type = "application/json"
                        putExtra(Intent.EXTRA_TITLE, "pocket-engineer-solution.json")
                    }, 30) } catch (_: android.content.ActivityNotFoundException) {
                        pendingExport = null
                        android.widget.Toast.makeText(this@MainActivity, "No document picker is available", android.widget.Toast.LENGTH_LONG).show()
                    }
                }
            }
        }

        @JavascriptInterface
        fun printSolution() {
            runOnUiThread { if (!destroyed) (getSystemService(PRINT_SERVICE) as android.print.PrintManager)
                .print("Pocket Engineer", webView.createPrintDocumentAdapter("Pocket Engineer"), null) }
        }

        @JavascriptInterface
        fun request(id: Int, method: String, payload: String) {
            if (destroyed || id <= 0) return
            val operation = when (method) { "solve" -> 0; "identify" -> 1; "catalog" -> 2; else -> -1 }
            if (operation < 0 || payload.length > 32768) return
            try {
                solver.execute {
                    if (destroyed) return@execute
                    val result = try {
                        String(nativeDispatch(operation, payload.toByteArray(Charsets.UTF_8)), Charsets.UTF_8)
                    } catch (_: Exception) {
                        """{"status":"error","answer":{"text":"Native calculation failed"}}"""
                    }
                    webView.post {
                        if (!destroyed) webView.evaluateJavascript(
                            "window.peNativeResult($id," + JSONObject.quote(result) + ")", null)
                    }
                }
            } catch (_: RejectedExecutionException) { /* Activity has already closed. */ }
        }
    }

    @Deprecated("Platform back compatibility for API 24+")
    override fun onBackPressed() {
        if (!::webView.isInitialized) { super.onBackPressed(); return }
        webView.evaluateJavascript("Boolean(window.peHandleBack && window.peHandleBack())") { handled ->
            if (!destroyed && handled != "true") {
                if (webView.canGoBack()) webView.goBack() else finish()
            }
        }
    }
    override fun onSaveInstanceState(outState: Bundle) {
        pendingExport?.let { outState.putString("pendingExport", it) }
        super.onSaveInstanceState(outState)
    }
    override fun onPause() { if (::webView.isInitialized) webView.onPause(); super.onPause() }
    override fun onResume() { super.onResume(); if (::webView.isInitialized) webView.onResume() }
    @Deprecated("Compatibility with API 24 document picker")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != 30) return
        val text = pendingExport
        pendingExport = null
        if (resultCode == RESULT_OK && text != null && data?.data != null) {
            try { contentResolver.openOutputStream(data.data!!)?.use { it.write(text.toByteArray(Charsets.UTF_8)) } }
            catch (_: Exception) { android.widget.Toast.makeText(this, "Could not save the solution", android.widget.Toast.LENGTH_LONG).show() }
        }
    }
    override fun onDestroy() {
        destroyed = true
        solver.shutdownNow()
        webView.removeJavascriptInterface("PocketEngineerAndroid")
        webView.stopLoading()
        webView.destroy()
        super.onDestroy()
    }
}
