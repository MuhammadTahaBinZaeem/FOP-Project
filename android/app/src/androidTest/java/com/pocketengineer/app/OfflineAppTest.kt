package com.pocketengineer.app

import android.webkit.WebView
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.UiDevice
import androidx.test.uiautomator.By
import androidx.test.uiautomator.Until
import org.json.JSONArray
import org.json.JSONTokener
import java.io.File
import org.junit.Test
import org.junit.Assert.assertTrue
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference

@RunWith(AndroidJUnit4::class)
class OfflineAppTest {
    private val device get() = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation())
    private fun evaluate(scenario: ActivityScenario<MainActivity>, js: String): String {
        val latch=CountDownLatch(1)
        val output=AtomicReference("")
        scenario.onActivity { activity ->
            val root=activity.findViewById<android.view.ViewGroup>(android.R.id.content)
            val web=root.findViewWithTag<WebView>("pocket-engineer-web")
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
        screenshot("failure")
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
    private fun tap(scenario: ActivityScenario<MainActivity>, selector: String) {
        // DOM only locates the control; UiDevice sends a real Android touch event.
        // This catches overlay/IME interception that element.click() cannot.
        evaluate(scenario,"document.querySelector('$selector').scrollIntoView({block:'center',behavior:'instant'})")
        val raw=evaluate(scenario,"JSON.stringify((()=>{const r=document.querySelector('$selector').getBoundingClientRect();return [(r.left+r.right)/2,(r.top+r.bottom)/2,innerWidth]})())")
        val point=JSONArray(JSONTokener(raw).nextValue() as String)
        val coordinates=IntArray(2)
        scenario.onActivity { activity ->
            val web=activity.findViewById<android.view.ViewGroup>(android.R.id.content).findViewWithTag<WebView>("pocket-engineer-web")
            web.getLocationOnScreen(coordinates)
            val scale=(web.width-web.paddingLeft-web.paddingRight)/point.getDouble(2)
            coordinates[0]+=(web.paddingLeft+point.getDouble(0)*scale).toInt()
            coordinates[1]+=(web.paddingTop+point.getDouble(1)*scale).toInt()
        }
        assertTrue("Touch injection failed: $selector",device.click(coordinates[0],coordinates[1]))
        device.waitForIdle()
    }
    private fun screenshot(name: String) {
        val directory=File(InstrumentationRegistry.getInstrumentation().targetContext.getExternalFilesDir(null),"evidence")
        directory.mkdirs()
        assertTrue("Screenshot failed",device.takeScreenshot(File(directory,"$name.png")))
    }
    @Test fun realTouchNavigationSolveClipboardAndRotation() {
        ActivityScenario.launch(MainActivity::class.java).use { scenario ->
            waitFor(scenario,"document.body.dataset.engine === 'android'")
            screenshot("android-home")
            tap(scenario,".nav[data-view=subjects]")
            waitFor(scenario,"document.body.dataset.view === 'subjects'")
            tap(scenario,".topic-link")
            waitFor(scenario,"document.body.dataset.view === 'workbench'")
            tap(scenario,"#input")
            // WebView's accessibility tree updates asynchronously after its DOM
            // and keyboard viewport. Wait for the real input, not a stale tree.
            val input=device.wait(Until.findObject(By.clazz("android.widget.EditText")),10000)
                ?: run { screenshot("missing-input");error("Native accessibility input was not found") }
            input.text="7*8"
            device.pressBack() // dismiss the actual Android keyboard
            tap(scenario,"#solve")
            waitFor(scenario,"document.getElementById('answer').textContent === '56'")
            screenshot("android-solution")
            tap(scenario,"#copy")
            waitFor(scenario,"document.getElementById('copy').textContent === 'Copied'")
            scenario.onActivity { activity ->
                val clipboard=activity.getSystemService(android.content.Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
                assertTrue("Actual native clipboard does not contain solution",clipboard.primaryClip?.getItemAt(0)?.text.toString().contains("56"))
            }
            device.setOrientationLeft()
            waitFor(scenario,"innerWidth > innerHeight")
            device.waitForIdle()
            // Allow the emulator's rotation compositor to finish before capture.
            // Correctness predicates above still determine pass/fail.
            Thread.sleep(500)
            assertTrue("Landscape content overflow",evaluate(scenario,"document.documentElement.scrollWidth <= innerWidth + 1")=="true")
            screenshot("android-landscape")
            device.setOrientationNatural()
            tap(scenario,".nav[data-view=history]")
            waitFor(scenario,"document.body.dataset.view === 'history'")
            device.pressBack()
            waitFor(scenario,"document.body.dataset.view === 'workbench'")
            device.unfreezeRotation()
        }
    }
}
