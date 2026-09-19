const {test,expect}=require('@playwright/test');
for(const valid of [true,false])test(`repair ${valid?'reuses verified':'rejects corrupt'} assets from a previous cache without network`,async({page,context})=>{
  await page.goto('./');await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  const current=await page.evaluate(async valid=>{
    const key=(await caches.keys()).find(k=>k.startsWith('pocket-engineer-')),cache=await caches.open(key),url=new URL('engine.wasm',location.href).href;
    const prior=await caches.open('pocket-engineer-prior-test');await prior.put(url,valid?await cache.match(url):new Response('invalid prior bytes'));await cache.delete(url);return key;
  },valid);
  await context.setOffline(true);await page.locator('.nav[data-view=downloads]').click();await expect(page.locator('#retry-cache')).toBeEnabled();await page.locator('#retry-cache').click();await expect(page.locator('#retry-cache')).toBeEnabled({timeout:35000});
  await expect(page.locator('body')).toHaveAttribute('data-offline-ready',String(valid));
  expect(await page.evaluate(async key=>Boolean(await(await caches.open(key)).match(new URL('engine.wasm',location.href).href)),current)).toBe(valid);
  if(valid){await page.reload();await expect(page.locator('#solve')).toBeEnabled();await page.locator('#input').fill('23+19');await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('42');}
});
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
test('offline rasters are reconstructed exactly even when the CDN image URL changes bytes',async({page,context})=>{
  let transformed=0;await context.route('**/assets/*.png',async route=>{transformed++;await route.fulfill({contentType:'image/png',body:Buffer.from('simulated CDN-transformed image bytes')});});
  await page.addInitScript(()=>{window.cdnImageProbe=fetch('assets/icon-192.png').then(r=>r.text());});
  await page.goto('./');await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  const checks=await page.evaluate(async()=>{const manifest=await(await fetch('offline-manifest.json')).json(),files=manifest.files.filter(f=>f.archive),keys=await caches.keys(),cache=await caches.open(keys.find(k=>k.startsWith('pocket-engineer-')));return Promise.all(files.map(async f=>{const r=await cache.match(new URL(f.path,location.href).href),hash=[...new Uint8Array(await crypto.subtle.digest('SHA-256',await r.arrayBuffer()))].map(b=>b.toString(16).padStart(2,'0')).join('');return hash===f.sha256;}));});
  expect(checks).toHaveLength(6);expect(checks.every(Boolean)).toBe(true);expect(transformed).toBeGreaterThan(0);await context.unroute('**/assets/*.png');await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled();expect(await page.locator('img[src="assets/logo.webp"]').first().evaluate(img=>img.complete&&img.naturalWidth>0)).toBe(true);
});
