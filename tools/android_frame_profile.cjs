// Isolated journey profiling on an explicitly selected disposable Android AVD.
// gfxinfo describes the native window; rAF describes WebView JS cadence, not
// presentation. Keep both measurements instead of treating them as identical.
const {_android}=require('playwright');
const {expect}=require('@playwright/test');
const fs=require('node:fs/promises');
const serial=process.env.PE_ANDROID_SERIAL;
if(!serial?.startsWith('emulator-'))throw new Error('Select a disposable emulator with PE_ANDROID_SERIAL');
(async()=>{
  const device=(await _android.devices({omitDriverInstall:true})).find(d=>d.serial()===serial);
  if(!device)throw new Error('Selected emulator unavailable');
  const output=process.env.PE_PROFILE_OUTPUT||'build-evidence/frame-profile';await fs.mkdir(output,{recursive:true});
  try{
    const page=await (await device.webView({pkg:'com.pocketengineer.app'})).page();
    await expect(page.locator('body')).toHaveAttribute('data-engine','android');
    if(process.env.PE_PROFILE_LOCAL_ASSETS==='1'){
      await page.route('**/assets/app.js',route=>route.fulfill({path:'www/app.js',contentType:'text/javascript'}));
      await page.route('**/assets/styles.css',route=>route.fulfill({path:'www/styles.css',contentType:'text/css'}));
      await page.reload();await expect(page.locator('body')).toHaveAttribute('data-engine','android');
      console.log('Profiling local web assets intercepted in the unchanged native APK (not a rebuilt APK).');
    }
    const cdp=await page.context().newCDPSession(page);await cdp.send('Performance.enable');
    const shell=async cmd=>(await device.shell(cmd)).toString();
    const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
    const results=[];
    async function phase(name,action){
      await delay(600);await shell('dumpsys gfxinfo com.pocketengineer.app reset');
      const before=await cdp.send('Performance.getMetrics');
      await page.evaluate(()=>{
        window.peProfile={times:[],long:[],active:true};const p=window.peProfile;
        p.observer=new PerformanceObserver(list=>p.long.push(...list.getEntries().map(e=>e.duration)));p.observer.observe({entryTypes:['longtask']});
        function frame(t){if(!p.active)return;p.times.push(t);requestAnimationFrame(frame);}requestAnimationFrame(frame);
      });
      const start=Date.now();await action();await delay(250);
      const raf=await page.evaluate(()=>{const p=window.peProfile;p.active=false;p.observer.disconnect();return {intervals:p.times.slice(1).map((t,i)=>t-p.times[i]),long_tasks:p.long};});
      const after=await cdp.send('Performance.getMetrics'),elapsed=Date.now()-start;
      const frames=await shell('dumpsys gfxinfo com.pocketengineer.app framestats');await fs.writeFile(`${output}/${name}-gfx.txt`,frames);
      const sorted=raf.intervals.slice().sort((a,b)=>a-b),metrics={};
      for(const key of ['TaskDuration','ScriptDuration','LayoutDuration','RecalcStyleDuration','LayoutCount','RecalcStyleCount'])metrics[key]=(after.metrics.find(m=>m.name===key)?.value||0)-(before.metrics.find(m=>m.name===key)?.value||0);
      const count=Number(frames.match(/Total frames rendered: (\d+)/)?.[1]||0),janky=Number(frames.match(/Janky frames: (\d+)/)?.[1]||0);
      const result={name,elapsed_ms:elapsed,native_frames:count,native_janky:janky,native_jank_percent:count?100*janky/count:null,raf_median_ms:sorted[Math.floor(sorted.length/2)],raf_p95_ms:sorted[Math.ceil(sorted.length*.95)-1],raf_over_25ms:sorted.filter(x=>x>25).length,raf_frames:sorted.length,long_tasks:raf.long_tasks,metrics};results.push(result);console.log(JSON.stringify(result));
    }
    await page.locator('.nav[data-view=workbench]').click();await page.locator('#domain').selectOption('algebra');await page.locator('#topic').selectOption('numeric_evaluation');
    await phase('idle',()=>delay(3000));
    await page.locator('.nav[data-view=subjects]').click();
    const scrollPositions=[];
    await phase('scroll',async()=>{for(let i=0;i<10;i++){await shell(`input swipe 550 ${i%2?500:1450} 550 ${i%2?1450:500} 350`);scrollPositions.push(await page.evaluate(()=>scrollY));}});
    results.at(-1).scroll_positions=scrollPositions;
    expect(Math.max(...scrollPositions)-Math.min(...scrollPositions),'Android swipes must actually move the page; a system overlay invalidates this profile').toBeGreaterThan(20);
    await phase('navigation',async()=>{for(let i=0;i<12;i++)await page.locator(`.nav[data-view=${i%2?'workbench':'subjects'}]`).click();});
    await page.locator('.nav[data-view=workbench]').click();
    await phase('solve',async()=>{for(let i=1;i<=12;i++){await page.locator('#input').fill(`${i}*(3+7)`);await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText(String(i*10));}});
    await phase('keyboard',async()=>{for(let i=0;i<8;i++){await page.locator('#input').click();await delay(300);await shell('input keyevent 4');await delay(300);}});
    if(process.env.PE_PROFILE_CONTROL==='1'){
      await page.evaluate(()=>{document.head.querySelectorAll('style,link[rel=stylesheet]').forEach(e=>e.remove());document.body.replaceChildren(...Array.from({length:120},(_,i)=>{const p=document.createElement('p');p.textContent=`Control paragraph ${i}: plain text without the application UI.`;p.style.padding='20px';return p;}));});
      const controlPositions=[];
      await phase('plain-control-scroll',async()=>{for(let i=0;i<10;i++){await shell(`input swipe 550 ${i%2?500:1450} 550 ${i%2?1450:500} 350`);controlPositions.push(await page.evaluate(()=>scrollY));}});
      results.at(-1).scroll_positions=controlPositions;
      expect(Math.max(...controlPositions)-Math.min(...controlPositions),'Plain control must actually scroll').toBeGreaterThan(20);
      await page.reload();await expect(page.locator('body')).toHaveAttribute('data-engine','android');
    }
    const report={serial,source:process.env.PE_PROFILE_SOURCE||'unspecified',local_web_assets:process.env.PE_PROFILE_LOCAL_ASSETS==='1',results,limitations:'Debug APK; isolated journeys on emulator, no physical-device claim. rAF is JS callback cadence, not displayed-frame timing. Plain control is temporary DOM replacement in the same native WebView, restored by reload. Local-assets mode intercepts JS/CSS for an A/B experiment; final APK validation is separate.'};
    await fs.writeFile(`${output}/report.json`,JSON.stringify(report,null,2));
  }finally{await device.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
