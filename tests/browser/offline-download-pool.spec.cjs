const {test,expect}=require('@playwright/test');
const http=require('node:http'),fs=require('node:fs/promises'),path=require('node:path');

test('a slow offline asset does not block later banks or exceed three simultaneous downloads',async({page})=>{
  let release,bank=false,active=0,maximum=0;
  const gate=new Promise(resolve=>{release=resolve;});
  const server=http.createServer(async(req,res)=>{
    try{
      const url=new URL(req.url,'http://localhost'),name=url.pathname==='/'?'index.html':decodeURIComponent(url.pathname.slice(1));
      if(name.split('/').includes('..'))throw Error('Unsafe path');
      // UI assets use 16-character keys; only the worker's integrity-checked
      // downloads use a full SHA-256. Hold an early file, not the page itself.
      if(url.searchParams.get('pe')?.length===64){
        active++;maximum=Math.max(maximum,active);res.once('close',()=>active--);
        if(name==='styles.css')await gate;
        if(name.endsWith('.pebank'))bank=true;
      }
      const types={'.js':'application/javascript','.json':'application/json','.html':'text/html','.css':'text/css','.wasm':'application/wasm'};
      const body=await fs.readFile(path.join('www',name));res.writeHead(200,{'Content-Type':types[path.extname(name)]||'application/octet-stream'});res.end(body);
    }catch{res.writeHead(404);res.end();}
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  try{
    await page.goto(`http://127.0.0.1:${server.address().port}/`);await expect(page.locator('#solve')).toBeEnabled();
    await expect.poll(()=>bank,{timeout:5000}).toBe(true);expect(maximum).toBeLessThanOrEqual(3);
    await expect(page.locator('body')).toHaveAttribute('data-offline-ready','false');release();
    await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:30000});expect(maximum).toBeLessThanOrEqual(3);
  }finally{release();server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
});
