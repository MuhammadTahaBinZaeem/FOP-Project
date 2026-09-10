const {test,expect}=require('@playwright/test');
test('readiness button repairs a missing cached solver before an offline revisit',async({page,context})=>{
  await page.goto('./');await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  await page.evaluate(async()=>{for(const key of await caches.keys())if(key.startsWith('pocket-engineer-'))await (await caches.open(key)).delete(new URL('engine.wasm',location.href).href);});
  await page.locator('.nav[data-view="downloads"]').click();await page.locator('#retry-cache').click();
  await expect.poll(()=>page.evaluate(async()=>{for(const key of await caches.keys())if(key.startsWith('pocket-engineer-')&&await (await caches.open(key)).match(new URL('engine.wasm',location.href)))return true;return false;}),{timeout:35000}).toBe(true);
  await expect(page.locator('#cache-status')).toContainText('Ready offline');
  await context.setOffline(true);await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();
  await page.locator('#input').fill('twenty five plus seven');await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('32');
});
test('readiness retry registers again after an offline registration was removed',async({page})=>{
  await page.goto('./');await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  await page.evaluate(async()=>{for(const r of await navigator.serviceWorker.getRegistrations())await r.unregister();});
  await page.locator('.nav[data-view="downloads"]').click();await page.locator('#retry-cache').click();
  await expect.poll(()=>page.evaluate(async()=>Boolean((await navigator.serviceWorker.getRegistration())?.active)),{timeout:35000}).toBe(true);
  await expect(page.locator('#cache-status')).toContainText('Ready offline');
});
test('explicit repair replaces corrupt cached content, not just missing files',async({page})=>{
  await page.goto('./');await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  await page.evaluate(async()=>{for(const key of await caches.keys())if(key.startsWith('pocket-engineer-'))await(await caches.open(key)).put(new URL('engine.wasm',location.href).href,new Response('corrupted-test-content'));});
  await page.locator('.nav[data-view="downloads"]').click();await page.locator('#retry-cache').click();await expect(page.locator('#retry-cache')).toBeEnabled({timeout:40000});await expect(page.locator('#cache-status')).toContainText('Ready offline');
  const length=await page.evaluate(async()=>{for(const key of await caches.keys())if(key.startsWith('pocket-engineer-')){const r=await(await caches.open(key)).match(new URL('engine.wasm',location.href).href);if(r)return(await r.arrayBuffer()).byteLength;}return 0;});expect(length).toBeGreaterThan(100000);
});
