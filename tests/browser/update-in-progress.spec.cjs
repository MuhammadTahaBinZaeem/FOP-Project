const {ui}=require('./ui.cjs');
const {test,expect}=require('@playwright/test');
const http=require('node:http'),fs=require('node:fs/promises'),path=require('node:path');

test('an update already installing at page startup exposes the real update button',async({page,context})=>{
  test.setTimeout(90000);
  let hold=false;
  const server=http.createServer(async(req,res)=>{
    try{
      const url=new URL(req.url,'http://localhost'),name=url.pathname==='/'?'index.html':decodeURIComponent(url.pathname.slice(1));
      if(name.split('/').includes('..'))throw Error('Unsafe path');
      let body=await fs.readFile(path.join('www',name));
      if(name==='service-worker.js'&&hold&&url.searchParams.get('refresh')==='install-race')body=body.toString()+`\nself.addEventListener('install',e=>e.waitUntil(new Promise(resolve=>self.addEventListener('message',e=>{if(e.data.type==='TEST_RELEASE_INSTALL')resolve();}))));`;
      const types={'.js':'application/javascript','.json':'application/json','.html':'text/html','.css':'text/css','.wasm':'application/wasm'};
      res.writeHead(200,{'Content-Type':types[path.extname(name)]||'application/octet-stream','Cache-Control':'no-store'});res.end(body);
    }catch{res.writeHead(404);res.end();}
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const url=`http://127.0.0.1:${server.address().port}/`;
  try{
    await page.goto(url);await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
    await ui(page.locator('#input')).fill('23+19');await ui(page.locator('#solve')).click();await expect(page.locator('#answer')).toHaveText('42');
    hold=true;
    await page.evaluate(async()=>{await navigator.serviceWorker.register('service-worker.js?refresh=install-race',{scope:'./',updateViaCache:'none'});});
    await expect.poll(()=>page.evaluate(async()=>Boolean((await navigator.serviceWorker.getRegistration())?.installing))).toBe(true);
    // The updatefound event happened in the previous document. The new page
    // must subscribe to the existing installing worker, not just future ones.
    await page.reload();await expect(page.locator('#solve')).toBeEnabled();await ui(page.locator('.nav[data-view=downloads]')).click();
    await expect(page.locator('#retry-cache')).toBeEnabled();
    await page.evaluate(async()=>(await navigator.serviceWorker.getRegistration()).installing.postMessage({type:'TEST_RELEASE_INSTALL'}));
    await expect.poll(()=>page.evaluate(async()=>Boolean((await navigator.serviceWorker.getRegistration())?.waiting))).toBe(true);
    await expect(page.locator('#update-offline')).toBeVisible();
    await ui(page.locator('.nav[data-view=workbench]')).click();await ui(page.locator('#input')).fill('17+29');
    await expect(page.locator('#update-offline')).toBeVisible();
    await ui(page.locator('.tool-launcher [data-lab=signals]')).click();await ui(page.locator('#signal-x')).fill('9,8,7,6');
    await expect(page.locator('#update-offline')).toBeVisible();
    expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1)).toBe(true);
    await Promise.all([page.waitForEvent('load'),page.locator('#update-offline').click()]);await expect(page.locator('#solve')).toBeEnabled();
    await expect(page.locator('#input')).toHaveValue('17+29');
    await expect(page.locator('#signal-x')).toHaveValue('9,8,7,6');
    expect(await page.evaluate(()=>JSON.parse(localStorage.getItem('pocket-engineer.history.v3')).some(r=>r.input==='23+19'))).toBe(true);
    await context.setOffline(true);await ui(page.locator('.nav[data-view=workbench]')).click();await ui(page.locator('#solve')).click();await expect(page.locator('#answer')).toHaveText('46');
  }finally{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
});
