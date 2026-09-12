const {test,expect}=require('@playwright/test');

test('every manifest bank loads in the real worker, including digit-bearing RK4',async({page})=>{
  test.setTimeout(120000);
  await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();
  const checked=await page.evaluate(async()=>{
    const manifest=await(await fetch('demo-data/manifest.json')).json(),worker=new Worker('demo-worker.js');
    let id=0;
    const call=data=>new Promise((resolve,reject)=>{
      const request=++id,timer=setTimeout(()=>reject(Error('Worker timeout')),10000);
      worker.onmessage=({data})=>{if(data.id!==request)return;clearTimeout(timer);data.error?reject(Error(data.error)):resolve(data.result);};
      worker.postMessage({id:request,...data});
    });
    try{
      let cases=0;for(const bank of manifest.sets){
        const loaded=await call({method:'load',bank});if(loaded.count!==5000)throw Error(bank.file);
        const last=await call({method:'page',start:4999});if(last.length!==1||last[0].index!==4999)throw Error('Missing final case');cases+=loaded.count;
      }
      // Digit support must not allow traversal, absolute paths or query injection.
      for(const file of ['../demo-data/a.b.easy.pebank','/demo-data/a.b.easy.pebank','demo-data/a.b.easy.pebank?x=1']){
        let rejected=false;try{await call({method:'load',bank:{...manifest.sets[0],file}});}catch(e){rejected=e.message==='Invalid demo manifest entry';}
        if(!rejected)throw Error('Unsafe filename was accepted');
      }
      return {banks:manifest.sets.length,cases};
    }finally{worker.terminate();}
  });
  expect(checked).toEqual({banks:165,cases:825000});
  await page.locator('.tool-launcher [data-view=demos]').click();await expect(page.locator('#demo-all')).toBeEnabled();
  await page.locator('#demo-domain').selectOption('differential_equations');await expect(page.locator('#demo-all')).toBeEnabled();
  await page.locator('#demo-topic').selectOption('rk4');await expect(page.locator('#demo-all')).toBeEnabled();
  await page.locator('#demo-difficulty').selectOption('hard');await expect(page.locator('#demo-all')).toBeEnabled();
  await page.locator('#demo-run').click();await expect(page.locator('#demo-status')).toContainText('Completed: 25 cases; 0 differ');
});
