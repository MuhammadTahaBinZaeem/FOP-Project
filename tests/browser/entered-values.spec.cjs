const {ui}=require('./ui.cjs');
const {test,expect}=require('@playwright/test');
const fs=require('node:fs/promises');

test('entered circuit, signal and transition-table values match independent answers online and offline',async({page,context},info)=>{
  test.setTimeout(120000);
  const checks=[],errors=[];page.on('pageerror',error=>errors.push(error.message));
  await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();
  const site=page.url();
  async function check(label,expected,actual){
    checks.push({label,input:await page.evaluate(()=>PEApp.state.problem),expected,actual});expect(actual).toEqual(expected);
    await expect(page.locator('#verification')).not.toContainText('Input could not be solved');
    const screenshot=info.outputPath(label+'.png');await page.locator('#lab-result').screenshot({path:screenshot});
    await info.attach(label+'.png',{path:screenshot,contentType:'image/png'});
  }
  for(const mode of ['online','offline']){
    if(mode==='offline'){
      await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:60000});
      await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled();
    }
    await ui(page.locator('.nav[data-view=workbench]')).click();await ui(page.locator('.tool-launcher [data-lab=circuit]')).click();
    await ui(page.locator('#circuit-netlist')).fill('V V1 in 0 12; R R1 in out 1000; R R2 out 0 2000');
    await ui(page.locator('#circuit-analysis')).selectOption('dc');await ui(page.locator('#circuit-study')).selectOption('port');await ui(page.locator('#circuit-port-positive')).fill('out');
    await ui(page.getByRole('button',{includeHidden:true,name:'Solve this circuit',exact:true})).click();await expect(page.locator('#answer')).toContainText('Thévenin voltage = 8');
    await check(mode+'-12V-divider',8,Number((await page.locator('#answer').innerText()).match(/Thévenin voltage = ([0-9.e+-]+)/)[1]));
    await ui(page.locator('.lab-tabs [data-lab=signals]')).click();await ui(page.locator('#signal-operation')).selectOption('convolution');
    await ui(page.locator('#signal-x')).fill('2,-1,3');await ui(page.locator('#signal-h')).fill('4,2');
    await ui(page.getByRole('button',{includeHidden:true,name:'Calculate locally',exact:true})).click();await expect(page.locator('#answer')).toContainText('8, 0, 10, 6');
    await check(mode+'-convolution',[8,0,10,6],await page.evaluate(()=>JSON.parse(PEApp.state.result.visual).samples));
    await ui(page.locator('.lab-tabs [data-lab=fsm]')).click();await ui(page.locator('#fsm-count')).selectOption('2');
    for(const [state,input,next,output] of [[0,0,0,0],[0,1,1,1],[1,0,1,1],[1,1,0,0]]){
      await ui(page.locator(`#fsm-next-${state}-${input}`)).selectOption('S'+next);await ui(page.locator(`#fsm-out-${state}-${input}`)).selectOption(String(output));
    }
    await ui(page.locator('#fsm-sequence')).fill('1,0,1,1,0');await ui(page.getByRole('button',{includeHidden:true,name:'Generate diagram & equations',exact:true})).click();
    await expect(page.locator('#answer')).toContainText('Simulation outputs: 1 1 0 1 1');await expect(page.locator('#visual canvas')).toHaveCount(1);
    await check(mode+'-edited-toggle-machine',['1','1','0','1','1'],await page.evaluate(()=>JSON.parse(PEApp.state.result.visual).trace.map(r=>r.output)));
  }
  expect(errors).toEqual([]);
  const report=info.outputPath('entered-values.json');
  await fs.writeFile(report,JSON.stringify({site,checked_at:new Date().toISOString(),layout:info.project.name,checks,page_errors:errors},null,2)+'\n');
  await info.attach('entered-values.json',{path:report,contentType:'application/json'});
});
