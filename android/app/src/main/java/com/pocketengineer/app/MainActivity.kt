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
import java.util.concurrent.atomic.AtomicBoolean
import java.io.File
import android.util.Base64

/** All web assets are trusted, bundled files. No remote page receives the JNI bridge. */
class MainActivity : Activity() {
    companion object { init { System.loadLibrary("pocketengineer_jni") } }
    private lateinit var webView: WebView
    private val solver = Executors.newSingleThreadExecutor()
    private val exports = Executors.newSingleThreadExecutor()
    private val preparingExport = AtomicBoolean(false)
    @Volatile private var destroyed = false
    private var pendingExportPath: String? = null
    private external fun nativeDispatch(method: Int, input: ByteArray): ByteArray

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        pendingExportPath = savedInstanceState?.getString("pendingExportPath")?.takeIf {
            it.startsWith("pe-export-") && !it.contains('/') && !it.contains('\\') && File(cacheDir, it).isFile
        }
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
        container.setBackgroundColor(Color.rgb(245, 245, 238))
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
        // The app is always light-themed; system-bar icons must stay readable.
        @Suppress("DEPRECATION")
        window.decorView.systemUiVisibility = window.decorView.systemUiVisibility or
            android.view.View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR or
            (if (android.os.Build.VERSION.SDK_INT >= 26) android.view.View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR else 0)
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
            if (json.length > 12000000) { exportNotice("Export exceeds 12 MB; save a smaller test run"); return }
            prepareExport("application/json", "pocket-engineer-data.json") { json.toByteArray(Charsets.UTF_8) }
        }

        @JavascriptInterface
        fun saveImage(dataUrl: String) {
            if (!dataUrl.startsWith("data:image/png;base64,") || dataUrl.length > 8000000) {
                exportNotice("PNG export is invalid or too large"); return
            }
            prepareExport("image/png", "pocket-engineer-diagram.png") {
                val bytes = Base64.decode(dataUrl.substringAfter(','), Base64.DEFAULT)
                require(bytes.size >= 24 && bytes.take(8).toByteArray().contentEquals(
                    byteArrayOf(137.toByte(), 80, 78, 71, 13, 10, 26, 10)))
                val dimensions = java.nio.ByteBuffer.wrap(bytes, 16, 8)
                require(dimensions.int in 1..4096 && dimensions.int in 1..4096)
                bytes
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
            val operation = when (method) { "solve" -> 0; "identify" -> 1; "catalog" -> 2; "workbench" -> 3; else -> -1 }
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

    private fun exportNotice(message: String) {
        runOnUiThread { if (!destroyed) android.widget.Toast.makeText(this, message, android.widget.Toast.LENGTH_LONG).show() }
    }

    private fun prepareExport(mime: String, title: String, content: () -> ByteArray) {
        if (destroyed || !preparingExport.compareAndSet(false, true)) return
        runOnUiThread {
            if (destroyed || pendingExportPath != null) { preparingExport.set(false); return@runOnUiThread }
            try {
                exports.execute {
                    var temporary: File? = null
                    try {
                        val bytes = content()
                        require(bytes.size <= 12000000)
                        val ready = File.createTempFile("pe-export-", ".tmp", cacheDir)
                        temporary = ready
                        ready.writeBytes(bytes)
                        runOnUiThread exportReady@ {
                            preparingExport.set(false)
                            if (destroyed) { ready.delete(); return@exportReady }
                            pendingExportPath = ready.name
                            @Suppress("DEPRECATION")
                            try { startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                                addCategory(Intent.CATEGORY_OPENABLE)
                                type = mime
                                putExtra(Intent.EXTRA_TITLE, title)
                            }, 30) } catch (_: Exception) {
                                pendingExportPath = null
                                ready.delete()
                                exportNotice("No document picker is available")
                            }
                        }
                    } catch (_: Exception) {
                        temporary?.delete()
                        preparingExport.set(false)
                        exportNotice("Could not prepare this export")
                    }
                }
            } catch (_: RejectedExecutionException) { preparingExport.set(false) }
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
        // Store only a tiny cache filename, never megabytes in an Android Bundle.
        pendingExportPath?.let { outState.putString("pendingExportPath", it) }
        super.onSaveInstanceState(outState)
    }
    override fun onPause() { if (::webView.isInitialized) webView.onPause(); super.onPause() }
    override fun onResume() { super.onResume(); if (::webView.isInitialized) webView.onResume() }
    @Deprecated("Compatibility with API 24 document picker")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != 30) return
        val path = pendingExportPath
        pendingExportPath = null
        if (path == null) return
        val temporary = File(cacheDir, path)
        if (resultCode != RESULT_OK || data?.data == null) { temporary.delete(); return }
        val destination = data.data!!
        exports.execute {
            try {
                val output = contentResolver.openOutputStream(destination) ?: error("Output stream unavailable")
                output.use { target -> temporary.inputStream().use { source -> source.copyTo(target, 32768) } }
                exportNotice("Export saved")
            } catch (_: Exception) { exportNotice("Could not save the export") }
            finally { temporary.delete() }
        }
    }
    override fun onDestroy() {
        destroyed = true
        solver.shutdownNow()
        exports.shutdown()
        webView.removeJavascriptInterface("PocketEngineerAndroid")
        webView.stopLoading()
        webView.destroy()
        super.onDestroy()
    }
}
