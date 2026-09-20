const {ui}=require('../tests/browser/ui.cjs');
// Fault injection only in a disposable profile owned by this test. Never use a
// personal browser profile. Run "seed" before deployment, "repair" afterward.
const {chromium}=require('playwright'),{expect}=require('@playwright/test');
const fs=require('node:fs/promises'),path=require('node:path'),{execFileSync}=require('node:child_process');
(async()=>{
 const [phase,directory]=process.argv.slice(2),output=path.resolve(directory||'');
 if(!['seed','repair'].includes(phase)||!output.startsWith(path.resolve('build-evidence')+path.sep))throw Error('Choose seed/repair and a test-owned build-evidence directory');
 const site='https://pocket-engineer.onrender.com/',profile=path.join(output,'browser-profile-engine-upgrade');
 if(phase==='seed')await fs.mkdir(output);else await fs.access(path.join(output,'seed.json'));
 const options={headless:true,viewport:{width:390,height:844},executablePath:process.env.PE_BROWSER_PATH||undefined};
 let context=await chromium.launchPersistentContext(profile,options);
 const report={site,phase,checked_at:new Date().toISOString(),checks:[],fault_injection:'Legacy worker deliberately placed in this test-owned cache; this is not inspection of the user browser.'};
 async function labs(page){
  await ui(page.locator('.nav[data-view=workbench]')).click();await ui(page.locator('.tool-launcher [data-lab=circuit]')).click();
  await ui(page.getByRole('button',{includeHidden:true,name:'RC low-pass',exact:true})).click();await ui(page.locator('#circuit-analysis')).selectOption('ac');
  await ui(page.getByRole('button',{includeHidden:true,name:'Solve this circuit',exact:true})).click();await expect(page.locator('#answer')).toContainText('Node voltages');await expect(page.locator('#verification')).toContainText('KCL');
  await ui(page.locator('.lab-tabs [data-lab=signals]')).click();await ui(page.locator('#signal-operation')).selectOption('convolution');await ui(page.locator('#signal-x')).fill('1,2,3');await ui(page.locator('#signal-h')).fill('4,5');
  await ui(page.getByRole('button',{includeHidden:true,name:'Calculate locally',exact:true})).click();await expect(page.locator('#answer')).toContainText('4, 13, 22, 15');
  await ui(page.locator('.lab-tabs [data-lab=fsm]')).click();await ui(page.getByRole('button',{includeHidden:true,name:'Generate diagram & equations',exact:true})).click();await expect(page.locator('#answer')).toContainText('Simulation outputs: 0 0 1 0 1');await expect(page.locator('#visual canvas')).toHaveCount(1);
 }
 try{
  let page=context.pages()[0]||await context.newPage();await page.goto(site);await expect(page.locator('#solve')).toBeEnabled({timeout:60000});
  if(phase==='seed'){
   await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:180000});
   await ui(page.locator('#input')).fill('23+19');await ui(page.locator('#solve')).click();await expect(page.locator('#answer')).toHaveText('42');
   const legacy=execFileSync('git',['show','de7fa4e:www/solver-worker.js'],{encoding:'utf8'});
   await page.evaluate(async legacy=>{const registration=await navigator.serviceWorker.getRegistration();if(!registration?.active)throw Error('No active test cache');const keys=(await caches.keys()).filter(k=>k.startsWith('pocket-engineer-'));if(keys.length!==1)throw Error('Expected one owned cache');await(await caches.open(keys[0])).put(new URL('solver-worker.js',location.href).href,new Response(legacy,{headers:{'Content-Type':'application/javascript'}}));},legacy);
   await page.reload();await expect(page.locator('#solve')).toBeEnabled();await ui(page.locator('.tool-launcher [data-lab=circuit]')).click();await expect(page.locator('#lab-status')).toContainText('Unsupported engine request');
   report.checks.push('current live installation seeded','arithmetic result 42 saved','exact unsupported-engine-request reproduced with old cached worker');await page.screenshot({path:path.join(output,'before.png'),fullPage:true});
  }else{
   const draft='Please find the determinant of [[2,3],[4,5]]';await ui(page.locator('#input')).fill(draft);
   await ui(page.locator('.nav[data-view=downloads]')).click();await expect(page.locator('#retry-cache')).toBeEnabled({timeout:180000});await ui(page.locator('#retry-cache')).click();await expect(page.locator('#update-offline')).toBeVisible({timeout:180000});await ui(page.locator('#update-offline')).click();
   await expect.poll(()=>page.evaluate(async()=>!!(await PEApp.request('catalog')).workbench).catch(()=>false),{timeout:60000}).toBe(true);
   await expect(page.locator('#input')).toHaveValue(draft);expect(await page.evaluate(()=>JSON.parse(localStorage.getItem('pocket-engineer.history.v3')).some(row=>row.input==='23+19'))).toBe(true);
   await labs(page);report.checks.push('actual repair and update buttons','draft and history retained','AC circuit, convolution and state diagram solved');
   await context.close();context=await chromium.launchPersistentContext(profile,{...options,offline:true});page=context.pages()[0]||await context.newPage();await page.goto(site);await expect(page.locator('#solve')).toBeEnabled({timeout:60000});await labs(page);
   report.checks.push('all three labs solved after full browser shutdown and offline restart');await page.screenshot({path:path.join(output,'after-offline.png'),fullPage:true});
  }
  report.success=true;
 }catch(error){report.success=false;report.error=error.message;throw error;}finally{await context.close();await fs.writeFile(path.join(output,phase+'.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));}
})().catch(error=>{console.error(error);process.exitCode=1;});
