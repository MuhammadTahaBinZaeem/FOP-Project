const {ui}=require('./ui.cjs');
const {test,expect}=require('@playwright/test');
const http=require('node:http'),fs=require('node:fs/promises'),path=require('node:path');
test('prepared service worker bypasses installer navigation and never caches installers',async({page,context})=>{
  test.setTimeout(90000);const payload=Buffer.from('504b0304506f636b6574456e67696e656572','hex');
  const server=http.createServer(async(req,res)=>{
    const url=new URL(req.url,'http://localhost');
    if(url.pathname==='/downloads/fixture.apk'){res.writeHead(200,{'Content-Type':'application/octet-stream'});res.end(payload);return;}
    if(url.pathname==='/downloads/help.txt'){res.writeHead(200,{'Content-Type':'text/plain'});res.end('Installer help, not the app shell');return;}
    try{
      const name=url.pathname==='/'?'index.html':decodeURIComponent(url.pathname.slice(1));if(name.split('/').includes('..'))throw Error('Unsafe path');
      const mime={'.js':'application/javascript','.json':'application/json','.wasm':'application/wasm','.html':'text/html','.css':'text/css','.png':'image/png','.webp':'image/webp','.webmanifest':'application/manifest+json'};
      const bytes=await fs.readFile(path.join('www',name));res.writeHead(200,{'Content-Type':mime[path.extname(name)]||'application/octet-stream'});res.end(bytes);
    }catch{res.writeHead(404);res.end();}
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const root=`http://127.0.0.1:${server.address().port}`;
  try{
    await page.goto(root);await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});await page.reload();
    expect(await page.evaluate(()=>!!navigator.serviceWorker.controller)).toBe(true);
    await ui(page.locator('.nav[data-view=downloads]')).click();
    await page.locator('[data-download]').first().evaluate((a,href)=>a.href=href,root+'/downloads/fixture.apk');
    const event=page.waitForEvent('download');await ui(page.locator('[data-download]').first()).click();const download=await event;
    expect(download.suggestedFilename()).toBe('fixture.apk');const chunks=[];for await(const chunk of await download.createReadStream())chunks.push(chunk);expect(Buffer.concat(chunks)).toEqual(payload);
    expect(await page.evaluate(async()=>{for(const key of await caches.keys())if(await(await caches.open(key)).match('/downloads/fixture.apk'))return true;return false;})).toBe(false);
    await page.goto(root+'/downloads/help.txt');await expect(page.locator('body')).toHaveText('Installer help, not the app shell');
    await context.setOffline(true);await page.goto(root);await expect(page.locator('#solve')).toBeEnabled();
  }finally{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
});
