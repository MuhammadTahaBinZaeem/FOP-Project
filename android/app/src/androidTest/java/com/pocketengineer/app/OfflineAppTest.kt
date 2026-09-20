package com.pocketengineer.app

import android.webkit.WebView
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.UiDevice
import android.view.KeyEvent
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
        evaluate(scenario,"window.__peTouchFrameReady=false; requestAnimationFrame(()=>requestAnimationFrame(()=>window.__peTouchFrameReady=true));")
        waitFor(scenario,"window.__peTouchFrameReady === true")
        device.waitForIdle()
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
        device.dumpWindowHierarchy(File(directory,"$name.xml"))
    }
    private fun typeQuestion(scenario: ActivityScenario<MainActivity>, text: String) {
        require(text.all { it.code in 32..126 && it!='\'' && it!='\\' && it!='%' })
        tap(scenario,"#input")
        waitFor(scenario,"document.activeElement === document.getElementById('input')")
        // Use Android key events, not UiObject.setText or DOM assignment. A
        // just-created WebView's accessibility EditText can lag its real focus.
        device.pressKeyCode(KeyEvent.KEYCODE_A,KeyEvent.META_CTRL_ON)
        device.pressKeyCode(KeyEvent.KEYCODE_DEL)
        // UiAutomation splits arguments directly; it does NOT interpret shell
        // quotes. Quoting here injects literal apostrophes into the question.
        device.executeShellCommand("input text ${text.replace(" ","%s")}")
        waitFor(scenario,"document.getElementById('input').value === '$text'")
    }
    @Test fun realTouchNavigationSolveClipboardAndRotation() {
        ActivityScenario.launch(MainActivity::class.java).use { scenario ->
            waitFor(scenario,"document.body.dataset.engine === 'android'")
            screenshot("android-home")
            tap(scenario,".nav[data-view=subjects]")
            waitFor(scenario,"document.body.dataset.view === 'subjects'")
            tap(scenario,".topic-link")
            waitFor(scenario,"document.body.dataset.view === 'workbench'")
            typeQuestion(scenario,"7*8")
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
    @Test fun naturalQuestionCorrectsWrongTypeAndProfilesRealScrolling() {
        ActivityScenario.launch(MainActivity::class.java).use { scenario ->
            waitFor(scenario,"document.body.dataset.engine === 'android'")
            evaluate(scenario,"document.getElementById('domain').value='logic'; document.getElementById('domain').dispatchEvent(new Event('change')); document.getElementById('topic').value='truth_table';")
            val question="Please find the determinant of [[1,2],[3,4]]"
            typeQuestion(scenario,question)
            device.pressBack()
            tap(scenario,"#solve")
            waitFor(scenario,"document.getElementById('answer').textContent === 'det(A) = -2'")
            assertTrue(evaluate(scenario,"document.getElementById('topic').value === 'determinant'")=="true")
            assertTrue(evaluate(scenario,"document.getElementById('input').value === '$question'")=="true")
            assertTrue(evaluate(scenario,"document.getElementById('interpretation').textContent.includes('Problem type corrected')")=="true")
            screenshot("android-natural-question")
            tap(scenario,".nav[data-view=subjects]")
            device.executeShellCommand("dumpsys gfxinfo com.pocketengineer.app reset")
            val x=device.displayWidth/2
            for(index in 0 until 10){
                val top=device.displayHeight/3
                val bottom=device.displayHeight*3/4
                device.swipe(x,if(index%2==0)bottom else top,x,if(index%2==0)top else bottom,20)
            }
            device.waitForIdle()
            val directory=File(InstrumentationRegistry.getInstrumentation().targetContext.getExternalFilesDir(null),"evidence")
            directory.mkdirs()
            File(directory,"scroll-frames.txt").writeText(device.executeShellCommand("dumpsys gfxinfo com.pocketengineer.app framestats"))
            assertTrue(evaluate(scenario,"document.documentElement.scrollWidth <= innerWidth + 1")=="true")
        }
    }
    @Test fun visualLabsAndStoredDemosUseOfflineNativeEngine() {
        ActivityScenario.launch(MainActivity::class.java).use { scenario ->
            waitFor(scenario,"document.body.dataset.engine === 'android'")
            tap(scenario,".nav[data-view=downloads]")
            tap(scenario,"#retry-cache")
            waitFor(scenario,"document.body.dataset.offlineReady === 'true'")
            tap(scenario,".nav[data-view=workbench]")
            evaluate(scenario,"document.getElementById('domain').value='logic';document.getElementById('domain').dispatchEvent(new Event('change'));document.getElementById('topic').value='kmap_minimization';document.getElementById('topic').dispatchEvent(new Event('change'));")
            tap(scenario,"#guided-toggle")
            waitFor(scenario,"Boolean(document.getElementById('kmap-variables'))")
            evaluate(scenario,"document.getElementById('kmap-variables').value='6';document.getElementById('kmap-variables').dispatchEvent(new Event('change'));")
            waitFor(scenario,"document.querySelectorAll('.kmap-cell').length === 64 && !PEApp.state.busy")
            tap(scenario,".kmap-cell")
            waitFor(scenario,"document.querySelector('.kmap-cell[data-cell=\"0\"]').dataset.value === '1'")
            screenshot("android-kmap-six")
            tap(scenario,".tool-launcher [data-lab=circuit]")
            waitFor(scenario,"Boolean(document.getElementById('circuit-canvas'))")
            evaluate(scenario,"[...document.querySelectorAll('#lab-content button')].find(b=>b.textContent==='RC low-pass').id='test-rc'")
            tap(scenario,"#test-rc")
            waitFor(scenario,"!PEApp.state.busy && document.getElementById('circuit-netlist').value.includes('C C1')")
            tap(scenario,"#circuit-stage-0 .flow-next")
            evaluate(scenario,"document.getElementById('circuit-analysis').value='ac';document.getElementById('circuit-analysis').dispatchEvent(new Event('change'));[...document.querySelectorAll('#lab-content button')].find(b=>b.textContent==='Solve this circuit').id='test-solve-network'")
            tap(scenario,"#test-solve-network")
            waitFor(scenario,"document.getElementById('verification').textContent.includes('KCL')")
            screenshot("android-ac-circuit")
            tap(scenario,".lab-tabs [data-lab=signals]")
            waitFor(scenario,"Boolean(document.getElementById('signal-x'))")
            tap(scenario,"#signals-stage-0 .flow-next")
            evaluate(scenario,"document.getElementById('signal-x').value='1,0,0,0';[...document.querySelectorAll('#lab-content button')].find(b=>b.textContent==='Calculate locally').id='test-fft'")
            tap(scenario,"#test-fft")
            waitFor(scenario,"document.getElementById('verification').textContent.includes('Parseval')")
            screenshot("android-fft")
            tap(scenario,".nav[data-view=workbench]")
            tap(scenario,".tool-launcher [data-view=demos]")
            waitFor(scenario,"document.querySelectorAll('#demo-rows tr').length === 25")
            tap(scenario,"#demo-run")
            waitFor(scenario,"document.getElementById('demo-status').textContent.includes('Completed: 25 cases; 0 differ')")
            screenshot("android-offline-demos")
            evaluate(scenario,"document.getElementById('demo-domain').value='differential_equations';document.getElementById('demo-domain').dispatchEvent(new Event('change'))")
            waitFor(scenario,"!document.getElementById('demo-all').disabled")
            evaluate(scenario,"document.getElementById('demo-topic').value='rk4';document.getElementById('demo-topic').dispatchEvent(new Event('change'))")
            waitFor(scenario,"!document.getElementById('demo-all').disabled")
            evaluate(scenario,"document.getElementById('demo-difficulty').value='hard';document.getElementById('demo-difficulty').dispatchEvent(new Event('change'))")
            waitFor(scenario,"!document.getElementById('demo-all').disabled")
            tap(scenario,"#demo-run")
            waitFor(scenario,"document.getElementById('demo-status').textContent.includes('Completed: 25 cases; 0 differ')")
            screenshot("android-rk4-demos")
            assertTrue("Editor or demo page overflow",evaluate(scenario,"document.documentElement.scrollWidth <= innerWidth + 1")=="true")
        }
    }
}
