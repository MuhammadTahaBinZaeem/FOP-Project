// Verify a real deployed PWA across a complete browser restart, not only reload.
const {chromium}=require('playwright');
const {expect}=require('@playwright/test');
const fs=require('node:fs/promises');
const path=require('node:path');
(async()=>{
  const output=process.argv[2];if(!output)throw Error('Provide a new evidence directory');await fs.mkdir(output);
  const profile=await fs.mkdtemp(path.join(output,'browser-profile-'));
  const site=process.env.PE_SITE_URL||'https://pocket-engineer.onrender.com/';
  const options={headless:true,viewport:{width:390,height:844},...(process.env.PE_BROWSER_PATH?{executablePath:process.env.PE_BROWSER_PATH}:{})};
  let context=await chromium.launchPersistentContext(profile,options);
  const report={site,recorded_at:new Date().toISOString(),checks:[]};
  try{
    let page=context.pages()[0]||await context.newPage();await page.goto(site);await expect(page.locator('body')).toHaveAttribute('data-engine','wasm',{timeout:60000});await expect(page.locator('.tool-launcher [data-lab=signals]')).toBeVisible();
    await page.locator('.nav[data-view=downloads]').click();await expect(page.locator('#retry-cache')).toBeEnabled({timeout:180000});await page.locator('#retry-cache').click();await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:180000});await expect(page.locator('#retry-cache')).toBeEnabled({timeout:180000});
    report.readiness=await page.locator('#cache-status').textContent();report.checks.push('live site uses WASM','explicit full cache repair');
    report.manifest=await page.evaluate(async()=>{const r=await fetch('offline-manifest.json'),m=await r.json();return{core_bytes:m.core_bytes,total_bytes:m.total_bytes,files:m.files.length};});
    await context.close();context=await chromium.launchPersistentContext(profile,{...options,offline:true});page=context.pages()[0]||await context.newPage();await page.goto(site,{waitUntil:'domcontentloaded'});await expect(page.locator('body')).toHaveAttribute('data-engine','wasm',{timeout:60000});await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:60000});report.checks.push('same URL after full browser restart with networking disabled');
    await page.locator('#input').fill('Please find the determinant of [[1,2],[3,4]]');await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('det(A) = -2');report.checks.push('offline natural question and type correction');
    await page.locator('.tool-launcher [data-lab=signals]').click();await page.locator('#signal-operation').selectOption('convolution');await page.locator('#signal-x').fill('1,2,3');await page.locator('#signal-h').fill('4,5');await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#answer')).toContainText('4, 13, 22, 15');report.checks.push('offline signals through actual controls');
    await page.locator('.nav[data-view=workbench]').click();await page.locator('.tool-launcher [data-view=demos]').click();await expect(page.locator('#demo-run')).toBeEnabled();await page.locator('#demo-difficulty').selectOption('hard');await expect(page.locator('#demo-all')).toBeEnabled();await page.locator('#demo-all').click();await expect(page.locator('#demo-status')).toContainText('Completed: 5000 cases; 0 differ',{timeout:180000});report.checks.push('offline decompression and 5000 stored hard-case comparisons');
    await page.screenshot({path:path.join(output,'offline-restarted-phone.png'),fullPage:true});report.success=true;
  }finally{await context.close();await fs.writeFile(path.join(output,'live-offline-report.json'),JSON.stringify(report,null,2)+'\n');}
  console.log(JSON.stringify(report,null,2));
})().catch(error=>{console.error(error);process.exitCode=1;});
