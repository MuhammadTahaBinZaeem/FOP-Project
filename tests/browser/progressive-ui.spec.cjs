const {test,expect}=require('@playwright/test');
async function ready(page){await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();}
async function next(page,name){await page.getByRole('button',{name:'Next: '+name,exact:true}).click();}
test('one workspace, neutral demos, correct selection and browser Back retain the question',async({page})=>{
  await ready(page);await expect(page.locator('.tool-launcher .primary')).toHaveCount(0);await expect(page.locator('.tool-launcher [data-view=demos]')).toHaveAttribute('aria-pressed','false');
  await page.locator('#input').fill('23+19');await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('42');
  await page.locator('[data-lab=fsm]').click();await expect(page.locator('#workbench')).toBeVisible();await expect(page.locator('#question-panel')).toBeHidden();await expect(page.locator('.nav[data-view=workbench]')).toHaveAttribute('aria-current','page');await expect(page.locator('[data-lab=fsm]')).toHaveAttribute('aria-pressed','true');
  await page.goBack();await expect(page.locator('#input')).toBeVisible();await expect(page.locator('#input')).toHaveValue('23+19');await expect(page.locator('#answer')).toHaveText('42');
  await page.goForward();await expect(page.locator('#fsm-count')).toBeVisible();await expect(page.locator('.tool-launcher [aria-pressed=true]')).toHaveCount(1);
});
test('natural input needs no type selection and advanced controls are disclosed explicitly',async({page})=>{
  await ready(page);await expect(page.locator('#domain')).toBeHidden();await expect(page.locator('#auto-type')).toBeHidden();await expect(page.locator('#input')).toHaveValue('');
  await page.locator('#input').fill('Please find the determinant of [[2,3],[4,5]]');await page.locator('#solve').click();await expect(page.locator('#answer')).toContainText('-2');
  await page.locator('#question-advanced>summary').click();await expect(page.locator('#auto-type')).toBeVisible();await page.locator('#auto-type').uncheck();await expect(page.locator('#input')).toHaveValue('Please find the determinant of [[2,3],[4,5]]');
});
test('guided matrix solves directly without exposing serialized text',async({page})=>{
  await ready(page);await page.locator('#guided-toggle').click();await page.locator('#domain').selectOption('linear_algebra');await page.locator('#topic').selectOption('determinant');
  await expect(page.locator('#input')).toBeHidden();await page.getByRole('textbox',{name:'Matrix A row 1 column 1',exact:true}).fill('5');await page.getByRole('button',{name:'Solve with these values',exact:true}).click();await expect(page.locator('#answer')).toContainText('14');
  await page.locator('#guided-toggle').click();await expect(page.locator('#input')).toHaveValue('5,2;3,4');await page.locator('#guided-toggle').click();await expect(page.getByRole('textbox',{name:'Matrix A row 1 column 1',exact:true})).toHaveValue('5');
});
test('circuit flow exposes only the analysis fields that apply',async({page})=>{
  await ready(page);await page.locator('[data-lab=circuit]').click();await expect(page.locator('#circuit-canvas')).toBeVisible();await expect(page.locator('#circuit-value')).toBeHidden();await expect(page.locator('#circuit-netlist')).toBeHidden();
  await next(page,'choose analysis');await expect(page.locator('#circuit-analysis')).toBeVisible();await expect(page.locator('#circuit-frequency')).toBeHidden();await expect(page.locator('#circuit-study')).toBeHidden();
  await page.getByRole('button',{name:'Solve this circuit',exact:true}).click();await expect(page.locator('#verification')).toContainText('KCL');
  await page.locator('#circuit-analysis').selectOption('ac');await expect(page.locator('#circuit-frequency')).toBeVisible();await expect(page.locator('#circuit-step')).toBeHidden();
  await page.getByRole('button',{name:'Back',exact:true}).click();await expect(page.locator('#circuit-canvas')).toBeVisible();await next(page,'choose analysis');await expect(page.locator('#circuit-analysis')).toHaveValue('ac');
});
test('signals retain edited operations and advanced sample rates are explicit',async({page})=>{
  await ready(page);await page.locator('[data-lab=signals]').click();await expect(page.locator('#signal-operation')).toBeVisible();await expect(page.locator('#signal-x')).toBeHidden();await next(page,'enter values');
  await page.locator('#signal-x').fill('1,0,0,0');await expect(page.locator('#signal-fs')).toBeHidden();await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#verification')).toContainText('Parseval');
  await page.locator('#signals-stage-1 details>summary').click();await page.locator('#signal-fs').fill('2000');await expect(page.locator('#signals-stage-1')).toContainText('Sample rate: 2000 Hz');
  await page.getByRole('button',{name:'Back',exact:true}).click();await page.locator('#signal-operation').selectOption('convolution');await next(page,'enter values');await page.locator('#signal-x').fill('2,-1,3');await page.locator('#signal-h').fill('4,2');await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#answer')).toContainText('8, 0, 10, 6');
  await page.getByRole('button',{name:'Back',exact:true}).click();await page.locator('#signal-operation').selectOption('fft');await next(page,'enter values');await expect(page.locator('#signal-x')).toHaveValue('1,0,0,0');await page.locator('#signals-stage-1 details>summary').click();await expect(page.locator('#signal-fs')).toHaveValue('2000');
});
test('state setup, transitions and optional simulation remain in the same workspace',async({page})=>{
  await ready(page);await page.locator('[data-lab=fsm]').click();await expect(page.locator('#fsm-count')).toBeVisible();await expect(page.locator('#fsm-input-bits')).toBeHidden();await expect(page.locator('#fsm-next-0-0')).toBeHidden();
  await next(page,'edit transitions');await page.locator('#fsm-next-0-0').selectOption('S1');await expect(page.locator('#fsm-sequence')).toBeHidden();await page.getByRole('button',{name:'Back',exact:true}).click();await page.locator('#fsm-count').selectOption('4');await next(page,'edit transitions');await expect(page.locator('#fsm-next-0-0')).toHaveValue('S1');
  await page.getByRole('button',{name:'Generate diagram & equations',exact:true}).click();await expect(page.locator('#visual canvas')).toBeVisible();await expect(page.locator('#verification')).not.toContainText('Input could not be solved');
});
test('conditional transform fields do not ask for irrelevant values',async({page})=>{
  await ready(page);await page.locator('[data-lab=signals]').click();await page.locator('#signal-operation').selectOption('laplace');await next(page,'enter values');await expect(page.locator('#signal-power')).toBeVisible();await expect(page.locator('#signal-omega')).toBeHidden();await page.locator('#signal-kind').selectOption('sine');await expect(page.locator('#signal-power')).toBeHidden();await expect(page.locator('#signal-omega')).toBeVisible();
});
test('update drafts keep the active input stage and do not jump from questions to a previous tool',async({page})=>{
  await ready(page);await page.locator('[data-lab=signals]').click();await next(page,'enter values');await page.locator('#signal-x').fill('9,8,7,6');
  await page.evaluate(()=>dispatchEvent(new Event('pe-save-draft')));await page.reload();await expect(page.locator('#signal-x')).toBeVisible();await expect(page.locator('#signal-x')).toHaveValue('9,8,7,6');
  await page.locator('.tool-launcher [data-view=workbench]').click();await page.locator('#input').fill('12+34');await page.evaluate(()=>dispatchEvent(new Event('pe-save-draft')));await page.reload();await expect(page.locator('#input')).toBeVisible();await expect(page.locator('#input')).toHaveValue('12+34');await expect(page.locator('#labs')).toBeHidden();
  await page.locator('[data-lab=signals]').click();await expect(page.locator('#signal-x')).toBeVisible();await expect(page.locator('#signal-x')).toHaveValue('9,8,7,6');
});
test('offline deep links, tool results and guided history preserve context',async({page,context})=>{
  await ready(page);await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:60000});await context.setOffline(true);await page.goto('./#signals');await next(page,'enter values');await page.locator('#signal-x').fill('1,0,0,0');await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#answer')).toContainText('4 complex samples');
  await page.locator('[data-lab=fsm]').click();await expect(page.locator('#result')).toBeHidden();await next(page,'edit transitions');await page.getByRole('button',{name:'Generate diagram & equations',exact:true}).click();await expect(page.locator('#answer')).toContainText('Simulation outputs: 0 0 1 0 1');
  await page.locator('[data-lab=signals]').click();await expect(page.locator('#answer')).toContainText('4 complex samples');await expect(page.locator('#signal-x')).toHaveValue('1,0,0,0');
  await page.locator('.tool-launcher [data-view=workbench]').click();await expect(page.locator('#result')).toBeHidden();await page.locator('#guided-toggle').click();await page.locator('#domain').selectOption('linear_algebra');await page.locator('#topic').selectOption('determinant');await page.getByRole('button',{name:'Solve with these values',exact:true}).click();await expect(page.locator('#answer')).toContainText('-2');
  await page.locator('.nav[data-view=history]').click();await expect(page.locator('.history-item')).toHaveCount(1);await page.locator('.history-item').click();await expect(page.locator('#input')).toHaveValue('1,2;3,4');
});
test('both stages fit small screens and keyboard navigation opens only the requested stage',async({page})=>{
  await ready(page);for(const width of [320,390,768,1440]){await page.setViewportSize({width,height:844});for(const lab of ['circuit','signals','fsm']){await page.locator(`[data-lab=${lab}]`).click();for(const stage of [0,1]){const control=page.locator(`[aria-controls=${lab}-stage-${stage}]`);await control.focus();await page.keyboard.press('Enter');await expect(page.locator(`#${lab}-stage-${stage}`)).toBeVisible();await expect(page.locator('#lab-content .flow-step:visible')).toHaveCount(1);expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1)).toBe(true);if(lab==='fsm'&&stage===1&&width>=390)expect(await page.locator('#fsm-stage-1 .table-scroll').evaluate(e=>e.scrollWidth<=e.clientWidth+1)).toBe(true);}}}
});
