package com.pocketengineer.app

import android.webkit.WebView
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Test
import org.junit.Assert.assertTrue
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference

@RunWith(AndroidJUnit4::class)
class OfflineAppTest {
    private fun evaluate(scenario: ActivityScenario<MainActivity>, js: String): String {
        val latch=CountDownLatch(1)
        val output=AtomicReference("")
        scenario.onActivity { activity ->
            val root=activity.findViewById<android.view.ViewGroup>(android.R.id.content)
            val web=root.getChildAt(0) as WebView
            web.evaluateJavascript(js) { value -> output.set(value);latch.countDown() }
        }
        assertTrue("JavaScript callback timed out", latch.await(10,TimeUnit.SECONDS))
        return output.get()
    }
    private fun waitFor(scenario: ActivityScenario<MainActivity>, predicate: String) {
        val deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(30)
        while(System.nanoTime()<deadline) {
            if(evaluate(scenario,predicate)=="true")return
            Thread.sleep(100)
        }
        throw AssertionError("Offline app did not reach: $predicate")
    }
    @Test fun bundledAppSolvesWithNativeCpp() {
        // Manifest deliberately has no INTERNET permission. The entire test must
        // succeed using packaged HTML/CSS/JS and the JNI library, not a server.
        ActivityScenario.launch(MainActivity::class.java).use { scenario ->
            waitFor(scenario,"document.body.dataset.engine === 'android'")
            evaluate(scenario,"document.getElementById('input').value='-2^2+9'; document.getElementById('solve-form').requestSubmit();")
            waitFor(scenario,"document.getElementById('answer').textContent === '5'")
            assertTrue(evaluate(scenario,"document.querySelectorAll('#steps li').length > 0")=="true")
            evaluate(scenario,"document.getElementById('domain').value='linear_algebra'; document.getElementById('domain').dispatchEvent(new Event('change')); document.getElementById('topic').value='linear_system'; document.getElementById('input').value='1,1,2;2,2,5'; document.getElementById('solve-form').requestSubmit();")
            waitFor(scenario,"document.getElementById('answer').textContent.includes('No solution')")
            scenario.recreate()
            waitFor(scenario,"document.body.dataset.engine === 'android'")
            assertTrue(evaluate(scenario,"JSON.parse(localStorage.getItem('pocket-engineer.history.v3')).length > 0")=="true")
        }
    }
}
