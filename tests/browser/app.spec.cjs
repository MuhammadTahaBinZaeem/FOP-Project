const {test,expect}=require('@playwright/test');
async function ready(page){await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();}
async function select(page,domain,topic,input){await page.locator('#domain').selectOption(domain);await page.locator('#topic').selectOption(topic);await page.locator('#input').fill(input);await page.locator('#solve').click();await expect(page.locator('#result')).toBeVisible();}
test('local C++ calculation and accessible responsive layout',async({page},info)=>{
  const errors=[];page.on('pageerror',e=>errors.push(e.message));await ready(page);
  await expect(page.locator('body')).toHaveAttribute('data-engine','wasm');
  await select(page,'algebra','numeric_evaluation','-2^2 + 2^-3');
  await expect(page.locator('#answer')).toHaveText('-3.875');
  await expect(page.locator('#steps li')).not.toHaveCount(0);
  expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth)).toBeTruthy();
  await page.screenshot({path:'test-results/'+info.project.name+'-solution.png',fullPage:true});
  expect(errors).toEqual([]);
});
test('selected topic is not silently overridden; matrix rank cases',async({page})=>{
  await ready(page);await select(page,'linear_algebra','linear_system','1,1,2;2,2,5');
  await expect(page.locator('#answer')).toContainText('No solution');
  await expect(page.locator('#topic')).toHaveValue('linear_system');
  await select(page,'linear_algebra','linear_system','1,1,2;2,2,4');
  await expect(page.locator('#answer')).toContainText('Infinitely many');
});
test('K-map renders a raster-free native table, RK4 renders canvas',async({page})=>{
  await ready(page);await select(page,'logic','kmap_minimization','vars=3; minterms=1,3,5,7');
  await expect(page.locator('#answer')).toHaveText('F = C');await expect(page.locator('.kmap td')).toHaveCount(8);
  await select(page,'differential_equations','rk4','1 1 0.1 10');
  await expect(page.locator('#visual canvas')).toBeVisible();await expect(page.locator('#verification')).toContainText('analytic');
  await expect(page.locator('svg')).toHaveCount(0);
});
test('offline cold reload still solves without API or network',async({page,context})=>{
  await ready(page);await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled();
  await expect(page.locator('body')).toHaveAttribute('data-engine','wasm');
  await select(page,'circuit','voltage_divider','12 1000 2000');
  await expect(page.locator('#answer')).toContainText('Vout = 8 V');
});
test('history survives reload and can be cleared; no stored markup injection',async({page})=>{
  await ready(page);await select(page,'algebra','numeric_evaluation','7*8');
  await page.reload();await expect(page.locator('#solve')).toBeEnabled();await page.locator('.nav[data-view="history"]').click();
  await expect(page.locator('.history-item')).toHaveCount(1);await page.locator('.history-item').click();
  await expect(page.locator('#input')).toHaveValue('7*8');
  await page.locator('.nav[data-view="history"]').click();await page.locator('#clear-history').click();await expect(page.locator('.history-item')).toHaveCount(0);
  await page.locator('.nav[data-view="workbench"]').click();await select(page,'algebra','numeric_evaluation','<img src=x onerror="window.injected=1">');
  expect(await page.evaluate(()=>window.injected)).toBeUndefined();await expect(page.locator('#result img')).toHaveCount(0);
});
test('catalog search, keyboard submit, JSON export',async({page})=>{
  await ready(page);await page.locator('.nav[data-view="subjects"]').click();await page.locator('#search').fill('Karnaugh');
  await expect(page.locator('.topic-link')).toHaveCount(1);await page.locator('.topic-link').click();
  await page.locator('#input').press('Control+Enter');await expect(page.locator('#answer')).toHaveText('F = C');
  const download=page.waitForEvent('download');await page.locator('#export').click();expect((await download).suggestedFilename()).toBe('pocket-engineer-solution.json');
});
test('bounded input failure leaves engine available for next solve',async({page})=>{
  await ready(page);await select(page,'differential_equations','rk4','1 1 0.1 2147483647');await expect(page.locator('#answer')).toContainText('10000');
  await select(page,'algebra','numeric_evaluation','sqrt(81)');await expect(page.locator('#answer')).toHaveText('9');
});
