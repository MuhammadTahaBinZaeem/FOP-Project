const {test,expect}=require('@playwright/test');
test('maximum-size supported matrix, ODE, polynomial and expression through the UI',async({page})=>{
  test.setTimeout(90000);await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();await page.emulateMedia({reducedMotion:'reduce'});
  async function solve(domain,topic,input){await page.locator('#domain').selectOption(domain);await page.locator('#topic').selectOption(topic);await page.locator('#input').fill(input);await page.locator('#solve').click();await expect(page.locator('#solve-form')).toHaveAttribute('aria-busy','false');}
  const system=Array.from({length:16},(_,r)=>[...Array.from({length:16},(_,c)=>r===c?1:0),r-8].join(',')).join(';');
  await solve('linear_algebra','linear_system',system);
  await expect(page.locator('#answer')).toHaveText(Array.from({length:16},(_,i)=>`x${i+1} = ${i-8}`).join(', '));
  const identity=Array.from({length:16},(_,r)=>Array.from({length:16},(_,c)=>r===c?1:0).join(',')).join(';');
  await solve('linear_algebra','inverse',identity);await expect(page.locator('#verification')).not.toContainText('Input could not be solved');
  const values=(await page.locator('#answer').innerText()).match(/-?\d+(?:\.\d+)?/g).map(Number);
  expect(values).toEqual(Array.from({length:256},(_,i)=>Math.floor(i/16)===i%16?1:0));
  await solve('differential_equations','rk4','0.01 1 0.001 10000');
  const answer=await page.locator('#answer').innerText();expect(answer).toMatch(/^y\(10\) ≈ /);const value=Number(answer.split('≈')[1]);
  expect(value).toBeCloseTo(Math.exp(0.1),8);await expect(page.locator('canvas')).toBeVisible();
  expect(await page.locator('#steps li').count()).toBeLessThanOrEqual(512);
  await solve('calculus','differentiation','x^32');await expect(page.locator('#answer')).toHaveText('32x^31');
  await solve('algebra','numeric_evaluation','+'+Array(256).fill('1').join('+'));await expect(page.locator('#answer')).toHaveText('256');
  await solve('algebra','numeric_evaluation',Array(2048).fill('1').join('+'));await expect(page.locator('#answer')).toContainText('at most 512 bytes');
  await solve('algebra','numeric_evaluation','2+2');await expect(page.locator('#answer')).toHaveText('4');
  expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1)).toBeTruthy();
});
