package com.pocketengineer.app

import android.annotation.SuppressLint
import android.app.Activity
import android.os.Bundle
import android.webkit.JavascriptInterface
import android.webkit.WebView

class MainActivity : Activity() {
    companion object {
        init {
            System.loadLibrary("pocketengineer_jni")
        }
    }

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val webView = WebView(this)
        webView.settings.javaScriptEnabled = true
        webView.settings.domStorageEnabled = true
        webView.settings.allowFileAccess = true
        webView.addJavascriptInterface(PocketEngineerBridge(), "PocketEngineerAndroid")
        webView.loadUrl("file:///android_asset/index.html")
        setContentView(webView)
    }

    private class PocketEngineerBridge {
        @JavascriptInterface
        external fun identify(input: String): String

        @JavascriptInterface
        external fun solve(request: String): String
    }
}
