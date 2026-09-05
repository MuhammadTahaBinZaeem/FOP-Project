package com.pocketengineer.app

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Intent
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
    private external fun nativeDispatch(method: Int, input: ByteArray): ByteArray

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val assets = WebViewAssetLoader.Builder()
            .addPathHandler("/assets/", WebViewAssetLoader.AssetsPathHandler(this)).build()
        webView = WebView(this)
        webView.setBackgroundColor(Color.rgb(245, 245, 238))
        webView.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true
            allowFileAccess = false
            allowContentAccess = false
            setSupportMultipleWindows(false)
            mixedContentMode = android.webkit.WebSettings.MIXED_CONTENT_NEVER_ALLOW
        }
        // Edge-to-edge target SDK 35: keep the content clear of system bars and IME.
        webView.setOnApplyWindowInsetsListener { view, insets ->
            @Suppress("DEPRECATION")
            view.setPadding(insets.systemWindowInsetLeft, insets.systemWindowInsetTop,
                insets.systemWindowInsetRight, insets.systemWindowInsetBottom)
            insets
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
        setContentView(webView)
        webView.loadUrl("https://appassets.androidplatform.net/assets/index.html")
    }

    private inner class Bridge {
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
        if (::webView.isInitialized && webView.canGoBack()) webView.goBack() else super.onBackPressed()
    }
    override fun onPause() { if (::webView.isInitialized) webView.onPause(); super.onPause() }
    override fun onResume() { super.onResume(); if (::webView.isInitialized) webView.onResume() }
    override fun onDestroy() {
        destroyed = true
        solver.shutdownNow()
        webView.removeJavascriptInterface("PocketEngineerAndroid")
        webView.stopLoading()
        webView.destroy()
        super.onDestroy()
    }
}
