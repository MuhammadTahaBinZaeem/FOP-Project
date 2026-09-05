const {test,expect}=require('@playwright/test');
const fs=require('node:fs/promises');
async function ready(page){await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();}
async function solve(page,domain,topic,input){
  await page.locator('#domain').selectOption(domain);await page.locator('#topic').selectOption(topic);
  await page.locator('#input').fill(input);await page.locator('#solve').click();
  await expect(page.locator('#solve-form')).toHaveAttribute('aria-busy','false');
  await expect(page.locator('#result')).toBeVisible();
}
test('every catalog topic through actual dropdowns, example and solve buttons',async({page})=>{
  test.setTimeout(180000);await ready(page);await page.emulateMedia({reducedMotion:'reduce'});
  const domains=await page.locator('#domain option').evaluateAll(options=>options.map(o=>o.value));let count=0;
  for(const domain of domains){await page.locator('#domain').selectOption(domain);
    const topics=await page.locator('#topic option').evaluateAll(options=>options.map(o=>({value:o.value,title:o.textContent})));
    for(const topic of topics){await page.locator('#topic').selectOption(topic.value);await page.locator('#example').click();await page.locator('#solve').click();
      await expect(page.locator('#solve-form')).toHaveAttribute('aria-busy','false');
      await expect(page.locator('#verification')).not.toContainText('Input could not be solved');
      await expect(page.locator('#result-topic')).toHaveText(topic.title);
      await expect(page.locator('#steps li').first()).toBeVisible();count++;
    }
  }
  expect(count).toBe(55);
});
test('spacing and touch targets across phone, landscape, tablet and desktop views',async({page},info)=>{
  test.setTimeout(150000);await ready(page);await page.emulateMedia({reducedMotion:'reduce'});
  for(const [width,height] of [[320,568],[360,640],[390,844],[667,375],[768,1024],[1024,768],[1440,1000]]){
    await page.setViewportSize({width,height});
    for(const view of ['workbench','subjects','history','downloads']){
      await page.locator(`.nav[data-view="${view}"]`).click();
      const issues=await page.evaluate(()=>{
        const issues=[];
        if(document.documentElement.scrollWidth>innerWidth+1)issues.push('horizontal page overflow');
        for(const group of ['.topbar','.welcome','.section-heading','.select-grid','.actions','.result-actions','.sidebar nav']){
          for(const parent of document.querySelectorAll(group)){
            const children=[...parent.children].filter(e=>e.getClientRects().length).map(e=>({id:e.id||e.className,r:e.getBoundingClientRect()}));
            for(let a=0;a<children.length;a++)for(let b=a+1;b<children.length;b++){
              const x=children[a],y=children[b];
              if(Math.min(x.r.right,y.r.right)-Math.max(x.r.left,y.r.left)>1&&Math.min(x.r.bottom,y.r.bottom)-Math.max(x.r.top,y.r.top)>1)issues.push(`${group}: ${x.id} overlaps ${y.id}`);
            }
          }
        }
        for(const e of document.querySelectorAll('button,select,input,textarea,summary')){
          if(!e.getClientRects().length)continue;const r=e.getBoundingClientRect();
          if(r.height<43)issues.push(`short touch target: ${e.id||e.className} ${r.height}`);
          if(r.left< -1||r.right>innerWidth+1)issues.push(`control outside viewport: ${e.id||e.className}`);
        }
        return issues;
      });
      expect(issues,`${width}×${height} / ${view}`).toEqual([]);
    }
    if(info.project.name==='desktop'&&[320,390,768,1440].includes(width)){
      await page.locator('.nav[data-view="workbench"]').click();await page.screenshot({path:`test-results/layout-${width}.png`,fullPage:true});
    }
  }
  // Chromium page zoom is not browser text-only zoom; explicitly enlarge text
  // separately to catch labels that assume one exact font size.
  await page.setViewportSize({width:320,height:568});await page.addStyleTag({content:'body{font-size:20px}button,select,label,p{font-size:18px!important}'});
  await page.locator('.nav[data-view="workbench"]').click();
  expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1)).toBeTruthy();
});
test('trajectory labels retain CSS size after phone resize and hidden-view navigation',async({page},info)=>{
  await ready(page);await page.emulateMedia({reducedMotion:'reduce'});
  await solve(page,'differential_equations','rk4','1 1 0.1 10');
  for(const [width,height] of [[320,568],[390,844],[667,375],[1440,1000]]){
    await page.setViewportSize({width,height});
    await expect.poll(()=>page.locator('canvas').evaluate(c=>{
      const r=c.getBoundingClientRect(),scale=Math.min(devicePixelRatio||1,2),ctx=c.getContext('2d');
      return c.width===Math.round(r.width*scale)&&c.height===Math.round(r.height*scale)&&ctx.font==='11px monospace'&&ctx.getTransform().a===scale;
    })).toBe(true);
    await page.locator('canvas').scrollIntoViewIfNeeded();
    if(width===390)await page.screenshot({path:`test-results/chart-${info.project.name}.png`});
  }
  await page.locator('.nav[data-view="subjects"]').click();await page.setViewportSize({width:360,height:640});
  await page.locator('.nav[data-view="workbench"]').click();
  await expect.poll(()=>page.locator('canvas').evaluate(c=>c.width===Math.round(c.getBoundingClientRect().width*Math.min(devicePixelRatio||1,2)))).toBe(true);
});
test('copy, save, print, identify, empty input, navigation and install feedback',async({page,context})=>{
  await context.grantPermissions(['clipboard-read','clipboard-write']);await ready(page);
  await page.locator('#input').fill('');await page.locator('#solve').click();await expect(page.locator('#notice')).toContainText('Enter a problem');
  await page.locator('#input').fill('2x+4=10');await page.locator('#identify').click();await expect(page.locator('#notice')).toContainText('Suggested:');
  await solve(page,'algebra','numeric_evaluation','7*8');await page.locator('#copy').click();
  expect(await page.evaluate(()=>navigator.clipboard.readText())).toContain('56');
  const download=page.waitForEvent('download');await page.locator('#export').click();
  const saved=JSON.parse(await fs.readFile(await (await download).path(),'utf8'));expect(saved.problem.input).toBe('7*8');expect(saved.result.answer.text).toBe('56');
  await page.evaluate(()=>{window.print=()=>window.printRequested=true;});await page.locator('#print').click();expect(await page.evaluate(()=>window.printRequested)).toBe(true);
  await solve(page,'algebra','numeric_evaluation','8*8');await expect(page.locator('#copy')).toHaveText('Copy solution');
  await page.locator('.nav[data-view="subjects"]').click();await page.locator('#search').fill('no-topic-matches-this');await expect(page.locator('#subject-list')).toContainText('No matching topics');
  await page.locator('.nav[data-view="downloads"]').click();await page.locator('#retry-cache').click();await expect(page.locator('#cache-status')).toContainText('Ready offline');
  // Real browser install dialogs cannot run in an incognito automated context.
  // Verify the application event lifecycle, separately from actual PWA install.
  await page.evaluate(()=>{const event=new Event('beforeinstallprompt');event.prompt=async()=>{window.installRequested=true;};event.userChoice=Promise.resolve({outcome:'dismissed'});window.dispatchEvent(event);});
  await page.locator('#install').click();expect(await page.evaluate(()=>window.installRequested)).toBe(true);await expect(page.locator('#install')).toBeHidden();
  for(const link of await page.locator('#downloads a').all()){expect(await link.getAttribute('href')).toMatch(/^https:\/\/github.com\/MuhammadTahaBinZaeem\/FOP-Project\/(releases|actions)$/);}
});
test('UI endurance with independent expected answers, failures, offline reload and bounded history',async({page,context},info)=>{
  const rounds=Number(process.env.PE_STRESS_ROUNDS||6);test.setTimeout(Math.max(180000,rounds*16000));
  await ready(page);await page.emulateMedia({reducedMotion:'reduce'});
  const errors=[],durations=[];page.on('pageerror',e=>errors.push(e.message));
  const session=await context.newCDPSession(page);await session.send('Performance.enable');
  const before=await session.send('Performance.getMetrics');
  for(let i=1;i<=rounds;i++){
    const start=Date.now();
    await solve(page,'algebra','numeric_evaluation',`${i}*(3+7)`);await expect(page.locator('#answer')).toHaveText(String(i*10));
    await solve(page,'linear_algebra','linear_system',`1,1,${i+2};1,-1,${i-2}`);await expect(page.locator('#answer')).toHaveText(`x1 = ${i}, x2 = 2`);
    await solve(page,'logic','kmap_minimization','vars=4; minterms=1,3,5,7,9,11,13,15');await expect(page.locator('#answer')).toHaveText('F = D');
    await solve(page,'algebra','numeric_evaluation',i%2?'sqrt(-1)':'('.repeat(150)+'1'+')'.repeat(150));await expect(page.locator('#verification')).toContainText('Input could not be solved');
    durations.push(Date.now()-start);
    if(i===Math.ceil(rounds/2)){await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true');await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled();await page.emulateMedia({reducedMotion:'reduce'});}
  }
  await page.locator('.nav[data-view="history"]').click();expect(await page.locator('.history-item').count()).toBeLessThanOrEqual(30);
  expect(errors).toEqual([]);const after=await session.send('Performance.getMetrics');
  const report={project:info.project.name,rounds,ui_solves:rounds*4,independent_answer_checks:rounds*3,rejected_inputs:rounds,page_errors:errors,before,after,round_ms:durations,limitations:'Desktop Chromium or mobile viewport, not physical Android. Metrics exclude the WASM worker heap. Print and install event callbacks simulated; JSON download and clipboard are real.'};
  await fs.writeFile(`test-results/endurance-${info.project.name}.json`,JSON.stringify(report,null,2));
});
