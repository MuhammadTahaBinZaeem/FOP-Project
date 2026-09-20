const {ui}=require('./ui.cjs');
const {test,expect}=require('@playwright/test');
const fs=require('node:fs/promises');
test('Android strategy transfers bytes and batches comparisons when worker fetch is unavailable',async({page,context})=>{
  test.setTimeout(120000);
  await context.route('**/demo-worker.js',async route=>route.fulfill({contentType:'text/javascript',body:'self.fetch=async()=>{throw Error("Simulated WebView worker network restriction")};\n'+await fs.readFile('www/demo-worker.js','utf8')}));
  await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();
  // Exercise Android's delivery strategy using the real WASM C++ implementation.
  // This is explicitly a transport simulation, not an installed Android test.
  await page.evaluate(()=>{const original=PEApp.request;window.batchCalls=0;PEApp.request=(method,payload)=>{if(JSON.parse(payload).topic==='demo_batch')window.batchCalls++;const mode=PEApp.state.mode;PEApp.state.mode='wasm';const result=original(method,payload);PEApp.state.mode=mode;return result;};PEApp.state.mode='android';});
  await ui(page.locator('.tool-launcher [data-view=demos]')).click();await expect(page.locator('#demo-all')).toBeEnabled();await ui(page.locator('#demo-all')).click();await expect(page.locator('#demo-status')).toContainText('Completed: 5000 cases; 0 differ',{timeout:90000});expect(await page.evaluate(()=>batchCalls)).toBe(200);await expect(page.locator('#demo-rows tr')).toHaveCount(25);
});
