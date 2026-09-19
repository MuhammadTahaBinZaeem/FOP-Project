# Instructions

- Following Playwright test failed.
- Explain why, be concise, respect Playwright best practices.
- Provide a snippet of code with the fix, if possible.

# Test info

- Name: engine-version.spec.cjs >> visual labs bypass stale solver-worker.js and solve after offline restart
- Location: tests/browser/engine-version.spec.cjs:4:67

# Error details

```
Error: expect(locator).toBeVisible() failed

Locator: locator('#circuit-canvas')
Expected: visible
Timeout: 15000ms
Error: element(s) not found

Call log:
  - Expect "toBeVisible" locator('#circuit-canvas') with timeout 15000ms
  - waiting for locator('#circuit-canvas')

```

```yaml
- link "Skip to workspace":
  - /url: "#workspace"
- complementary:
  - link "Pocket Engineer home":
    - /url: "#workbench"
    - text: Pocket Engineer
  - paragraph: YOUR STUDY SPACE
  - navigation "Main navigation":
    - button "⌘ Workbench"
    - button "▦ All subjects"
    - button "↺ Recent work"
    - button "↓ Get the app"
  - text: Private by design
  - paragraph: Your questions stay on this device. No account. No cloud solver.
  - link "Open-source · C++ core ↗":
    - /url: https://github.com/MuhammadTahaBinZaeem/FOP-Project
- main:
  - text: Your pocket-sized engineering companion
  - status: C++ · on your device
  - button "← Back to workbench"
  - paragraph: OFFLINE ENGINEERING LABS · C++ POWERED
  - heading "Build it. Inspect it." [level=1]
  - paragraph: Connected circuits, sampled signals and state machines. The model and its limits stay visible.
  - group "Engineering lab":
    - button "Circuit drawing" [pressed]
    - button "Signals & transforms"
    - button "State machines"
  - status: Unsupported engine request
```

# Test source

```ts
  1  | const {test,expect}=require('@playwright/test');
  2  | const http=require('node:http'),fs=require('node:fs/promises'),path=require('node:path');
  3  | 
  4  | for(const stale of ['solver-worker.js','engine.js','engine.wasm'])test(`visual labs bypass stale ${stale} and solve after offline restart`,async({page,context})=>{
  5  |   test.setTimeout(90000);
  6  |   const seen=[];
  7  |   const server=http.createServer(async(req,res)=>{
  8  |     try{
  9  |       const url=new URL(req.url,'http://localhost'),name=url.pathname==='/'?'index.html':decodeURIComponent(url.pathname.slice(1));
  10 |       if(name.split('/').includes('..'))throw Error('Unsafe path');
  11 |       const mime={'.js':'application/javascript','.json':'application/json','.wasm':'application/wasm','.html':'text/html','.css':'text/css','.png':'image/png','.webp':'image/webp','.webmanifest':'application/manifest+json'};
  12 |       let bytes=await fs.readFile(path.join('www',name));
  13 |       if(['solver-worker.js','engine.js','engine.wasm'].includes(name))seen.push({name,key:url.searchParams.get('pe')});
  14 |       // The old worker can load the normal calculator, but rejects every lab.
  15 |       // Other stages model stale cached glue/binary delivery independently.
  16 |       if(name===stale&&!url.searchParams.has('pe'))bytes=name==='solver-worker.js'?bytes.toString().replace(",workbench:'pe_workbench_json'",''):name==='engine.js'?'throw Error("stale engine glue");':Buffer.from('stale WASM');
  17 |       res.writeHead(200,{'Content-Type':mime[path.extname(name)]||'application/octet-stream'});res.end(bytes);
  18 |     }catch{res.writeHead(404);res.end();}
  19 |   });
  20 |   await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const url=`http://127.0.0.1:${server.address().port}/`;
  21 |   async function solveLabs(){
  22 |     await page.locator('.tool-launcher [data-lab=circuit]').click();
> 23 |     await expect(page.locator('#circuit-canvas')).toBeVisible();
     |                                                   ^ Error: expect(locator).toBeVisible() failed
  24 |     await page.getByRole('button',{name:'RC low-pass',exact:true}).click();
  25 |     await page.locator('#circuit-analysis').selectOption('ac');
  26 |     await page.getByRole('button',{name:'Solve this circuit',exact:true}).click();
  27 |     await expect(page.locator('#answer')).toContainText('Node voltages');
  28 |     await expect(page.locator('#verification')).toContainText('KCL');
  29 |     await page.locator('.lab-tabs [data-lab=signals]').click();
  30 |     await page.locator('#signal-operation').selectOption('convolution');
  31 |     await page.locator('#signal-x').fill('1,2,3');await page.locator('#signal-h').fill('4,5');
  32 |     await page.getByRole('button',{name:'Calculate locally',exact:true}).click();
  33 |     await expect(page.locator('#answer')).toContainText('4, 13, 22, 15');
  34 |     await page.locator('.lab-tabs [data-lab=fsm]').click();
  35 |     await page.getByRole('button',{name:'Generate diagram & equations',exact:true}).click();
  36 |     await expect(page.locator('#answer')).toContainText('Simulation outputs: 0 0 1 0 1');
  37 |     await expect(page.locator('#visual canvas')).toHaveCount(1);
  38 |   }
  39 |   try{
  40 |     await page.goto(url);await expect(page.locator('#solve')).toBeEnabled();
  41 |     await expect(page.locator('body')).toHaveAttribute('data-engine','wasm');
  42 |     await solveLabs();
  43 |     for(const name of ['solver-worker.js','engine.js','engine.wasm'])expect(seen.find(r=>r.name===name)?.key).toMatch(/^[a-f0-9]{16}$/);
  44 |     await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  45 |     await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled();
  46 |     await page.locator('.nav[data-view=workbench]').click();await solveLabs();
  47 |   }finally{await test.info().attach('engine-requests',{body:JSON.stringify(seen),contentType:'application/json'});server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
  48 | });
  49 | 
```