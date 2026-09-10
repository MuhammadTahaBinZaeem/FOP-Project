const {test,expect}=require('@playwright/test');
const http=require('node:http');
const fs=require('node:fs/promises');
const path=require('node:path');
test('fresh installation bypasses stale CDN worker, manifest and demo cache keys',async({page,context})=>{
  test.setTimeout(90000);
  const seen={worker:0,manifest:0,banks:0};
  const server=http.createServer(async(req,res)=>{
    try{
      const url=new URL(req.url,'http://localhost'),name=url.pathname==='/'?'index.html':decodeURIComponent(url.pathname.slice(1));
      if(name.split('/').includes('..'))throw Error('Unsafe path');
      const sensitive=name==='service-worker.js'||name==='offline-manifest.json'||name.endsWith('.pebank');
      if(sensitive&&!url.search){res.writeHead(200,{'Content-Type':name.endsWith('.js')?'application/javascript':'application/json'});res.end(name.endsWith('.js')?'throw Error("stale CDN worker");':'{"stale":true}');return;}
      if(name==='service-worker.js')seen.worker++;if(name==='offline-manifest.json')seen.manifest++;if(name.endsWith('.pebank'))seen.banks++;
      const mime={'.js':'application/javascript','.json':'application/json','.wasm':'application/wasm','.html':'text/html','.css':'text/css','.png':'image/png','.webp':'image/webp','.webmanifest':'application/manifest+json'};
      const bytes=await fs.readFile(path.join('www',name));res.writeHead(200,{'Content-Type':mime[path.extname(name)]||'application/octet-stream'});res.end(bytes);
    }catch{res.writeHead(404);res.end();}
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const url=`http://127.0.0.1:${server.address().port}/`;
  try{
    await page.goto(url);await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
    expect(seen.worker).toBeGreaterThan(0);expect(seen.manifest).toBeGreaterThan(0);expect(seen.banks).toBe(165);
    await context.setOffline(true);await page.goto(url);await expect(page.locator('#solve')).toBeEnabled();
    await page.locator('.tool-launcher [data-view=demos]').click();await expect(page.locator('#demo-run')).toBeEnabled();await page.locator('#demo-run').click();await expect(page.locator('#demo-status')).toContainText('Completed: 25 cases; 0 differ');
  }finally{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
});
