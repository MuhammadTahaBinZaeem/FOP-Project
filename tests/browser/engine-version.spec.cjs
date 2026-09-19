const {test,expect}=require('@playwright/test');
const http=require('node:http'),fs=require('node:fs/promises'),path=require('node:path');

for(const stale of ['solver-worker.js','engine.js','engine.wasm'])test(`visual labs bypass stale ${stale} and solve after offline restart`,async({page,context})=>{
  test.setTimeout(90000);
  const seen=[];
  const server=http.createServer(async(req,res)=>{
    try{
      const url=new URL(req.url,'http://localhost'),name=url.pathname==='/'?'index.html':decodeURIComponent(url.pathname.slice(1));
      if(name.split('/').includes('..'))throw Error('Unsafe path');
      const mime={'.js':'application/javascript','.json':'application/json','.wasm':'application/wasm','.html':'text/html','.css':'text/css','.png':'image/png','.webp':'image/webp','.webmanifest':'application/manifest+json'};
      let bytes=await fs.readFile(path.join('www',name));
      if(['solver-worker.js','engine.js','engine.wasm'].includes(name))seen.push({name,key:url.searchParams.get('pe')});
      // The old worker can load the normal calculator, but rejects every lab.
      // Other stages model stale cached glue/binary delivery independently.
      if(name===stale&&!url.searchParams.has('pe'))bytes=name==='solver-worker.js'?bytes.toString().replace(",workbench:'pe_workbench_json'",''):name==='engine.js'?'throw Error("stale engine glue");':Buffer.from('stale WASM');
      res.writeHead(200,{'Content-Type':mime[path.extname(name)]||'application/octet-stream'});res.end(bytes);
    }catch{res.writeHead(404);res.end();}
  });
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const url=`http://127.0.0.1:${server.address().port}/`;
  async function solveLabs(){
    await page.locator('.tool-launcher [data-lab=circuit]').click();
    await expect(page.locator('#circuit-canvas')).toBeVisible();
    await page.getByRole('button',{name:'RC low-pass',exact:true}).click();
    await page.locator('#circuit-analysis').selectOption('ac');
    await page.getByRole('button',{name:'Solve this circuit',exact:true}).click();
    await expect(page.locator('#answer')).toContainText('Node voltages');
    await expect(page.locator('#verification')).toContainText('KCL');
    await page.locator('.lab-tabs [data-lab=signals]').click();
    await page.locator('#signal-operation').selectOption('convolution');
    await page.locator('#signal-x').fill('1,2,3');await page.locator('#signal-h').fill('4,5');
    await page.getByRole('button',{name:'Calculate locally',exact:true}).click();
    await expect(page.locator('#answer')).toContainText('4, 13, 22, 15');
    await page.locator('.lab-tabs [data-lab=fsm]').click();
    await page.getByRole('button',{name:'Generate diagram & equations',exact:true}).click();
    await expect(page.locator('#answer')).toContainText('Simulation outputs: 0 0 1 0 1');
    await expect(page.locator('#visual canvas')).toHaveCount(1);
  }
  try{
    await page.goto(url);await expect(page.locator('#solve')).toBeEnabled();
    await expect(page.locator('body')).toHaveAttribute('data-engine','wasm');
    await solveLabs();
    for(const name of ['solver-worker.js','engine.js','engine.wasm'])expect(seen.find(r=>r.name===name)?.key).toMatch(/^[a-f0-9]{16}$/);
    await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
    await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled();
    await page.locator('.nav[data-view=workbench]').click();await solveLabs();
  }finally{await test.info().attach('engine-requests',{body:JSON.stringify(seen),contentType:'application/json'});server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
});

test('legacy worker capability is rejected at startup and repair retains history',async({page,context})=>{
  test.setTimeout(90000);
  const worker=(await fs.readFile('www/solver-worker.js','utf8')).replace(",workbench:'pe_workbench_json'",'').replace("if(method==='catalog')result.workbench=!!m._pe_workbench_json;",'');
  await context.route('**/solver-worker.js*',route=>route.fulfill({contentType:'application/javascript',body:worker}));
  await context.route('**/api/catalog',route=>route.fulfill({status:404,body:'No native server in this static-site fixture'}));
  await page.addInitScript(()=>localStorage.setItem('pocket-engineer.history.v3',JSON.stringify([{domain:'algebra',topic:'arithmetic',input:'23+19',at:1}])));
  await page.goto('./');
  await expect(page.locator('#notice')).toContainText('Cached engine is outdated');
  await expect(page.locator('#solve')).toBeDisabled();
  await context.unroute('**/solver-worker.js*');
  await page.locator('.nav[data-view=downloads]').click();
  await expect(page.locator('#retry-cache')).toBeEnabled({timeout:45000});await page.locator('#retry-cache').click();
  await expect(page.locator('#solve')).toBeEnabled({timeout:45000});
  await expect(page.locator('body')).toHaveAttribute('data-engine','wasm');
  expect(await page.evaluate(()=>JSON.parse(localStorage.getItem('pocket-engineer.history.v3'))[0].input)).toBe('23+19');
  await page.locator('.nav[data-view=workbench]').click();await page.locator('.tool-launcher [data-lab=signals]').click();
  await page.locator('#signal-operation').selectOption('convolution');await page.locator('#signal-x').fill('1,2,3');await page.locator('#signal-h').fill('4,5');
  await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#answer')).toContainText('4, 13, 22, 15');
});
