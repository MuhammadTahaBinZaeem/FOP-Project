// Local APK inspection. Requires an explicitly selected, disposable AVD with
// the debug APK already installed. Never chooses or clears a physical device.
const {_android}=require('playwright');
const {expect}=require('@playwright/test');
const fs=require('node:fs/promises');
const serial=process.env.PE_ANDROID_SERIAL;
if(!serial?.startsWith('emulator-'))throw new Error('Set PE_ANDROID_SERIAL to the disposable emulator serial');
(async()=>{
  const devices=await _android.devices({omitDriverInstall:true});const device=devices.find(d=>d.serial()===serial);
  if(!device)throw new Error('Selected emulator is not attached');
  const output=process.env.PE_ANDROID_OUTPUT||'build-evidence/android-local';await fs.mkdir(output,{recursive:true});
  device.setDefaultTimeout(30000);
  try{
    const webview=await device.webView({pkg:'com.pocketengineer.app'});const page=await webview.page();
    page.setDefaultTimeout(20000);await expect(page.locator('body')).toHaveAttribute('data-engine','android');
    await page.emulateMedia({reducedMotion:'reduce'});
    const errors=[];page.on('pageerror',e=>errors.push(e.message));
    async function capture(name){await device.shell('uiautomator dump /sdcard/pocket-engineer-ui.xml');await device.screenshot({path:`${output}/${name}.png`});}
    async function shell(command){return (await device.shell(command)).toString();}
    async function nativeTap(selector){
      await page.locator(selector).evaluate(e=>e.scrollIntoView({block:'center',behavior:'instant'}));
      await shell('uiautomator dump /sdcard/pocket-engineer-ui.xml');
      const xml=await shell('cat /sdcard/pocket-engineer-ui.xml');
      const tag=xml.match(/<node\b[^>]*class="android.webkit.WebView"[^>]*>/)?.[0];
      const bounds=tag?.match(/bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"/);
      if(!bounds)throw new Error('Native WebView bounds unavailable');
      const box=await page.locator(selector).boundingBox(),width=await page.evaluate(()=>innerWidth);
      const ratio=(Number(bounds[3])-Number(bounds[1]))/width;
      const x=Math.round(Number(bounds[1])+(box.x+box.width/2)*ratio),y=Math.round(Number(bounds[2])+(box.y+box.height/2)*ratio);
      await shell(`input tap ${x} ${y}`);
    }
    await capture('home');
    await nativeTap('.nav[data-view=subjects]');await expect(page.locator('#subjects')).toBeVisible();await capture('subjects');
    await page.locator('#search').fill('Scientific calculator');await nativeTap('.topic-link');
    await page.locator('#input').fill('');await nativeTap('#input');
    await shell("input text '7*8'");await expect(page.locator('#input')).toHaveValue('7*8');await capture('keyboard');
    await shell('input keyevent 4');await nativeTap('#solve');await expect(page.locator('#answer')).toHaveText('56');await capture('solution');
    await nativeTap('#copy');await expect(page.locator('#copy')).toHaveText('Copied');
    await nativeTap('#export');
    await expect.poll(()=>shell('dumpsys activity activities')).toMatch(/topResumedActivity[^\n]*documentsui/i);await capture('document-picker');
    await shell('input keyevent 4');
    await nativeTap('#print');
    await expect.poll(()=>shell('dumpsys activity activities')).toMatch(/topResumedActivity[^\n]*printspooler/i);await capture('print-dialog');await shell('input keyevent 4');
    // Endurance drives real WebView controls through CDP. Initial interactions
    // above additionally inject Android OS touch and keyboard events through adb.
    const latencies=[];const rounds=Number(process.env.PE_ANDROID_ROUNDS||100);
    for(let i=1;i<=rounds;i++){
      await page.locator('#domain').selectOption('algebra');await page.locator('#topic').selectOption('numeric_evaluation');
      await page.locator('#input').fill(`${i}*(3+7)`);const start=Date.now();await page.locator('#solve').click();
      await expect(page.locator('#answer')).toHaveText(String(i*10));latencies.push(Date.now()-start);
      if(i%10===0)console.log(`Android native WebView: ${i}/${rounds} expected answers matched`);
    }
    await nativeTap('.nav[data-view=history]');expect(await page.locator('.history-item').count()).toBeLessThanOrEqual(30);await capture('history');
    await shell('input keyevent 4');await expect(page.locator('#workbench')).toBeVisible();
    expect(errors).toEqual([]);
    const memory=await shell('dumpsys meminfo com.pocketengineer.app');await fs.writeFile(`${output}/memory-after.txt`,memory);
    const frames=await shell('dumpsys gfxinfo com.pocketengineer.app');await fs.writeFile(`${output}/frames.txt`,frames);
    const report={serial,model:device.model(),engine:'android-jni',rounds,independent_expected_answers:rounds,errors,solve_ui_ms:latencies,android_release:(await shell('getprop ro.build.version.release')).trim(),webview:await shell('dumpsys webviewupdate'),meminfo_scope:'App process only; shared renderer and system overhead may be additional',limitations:'Debug APK on x86_64 Android emulator with software rendering; not physical ARM or release performance. Document picker and print dialog opened; no OS file write/print job claimed.'};
    await fs.writeFile(`${output}/report.json`,JSON.stringify(report,null,2));console.log(JSON.stringify(report));
  }catch(error){try{await device.screenshot({path:`${output}/failure.png`});}catch{}throw error;}
  finally{await device.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
