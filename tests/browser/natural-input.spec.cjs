const {test,expect}=require('@playwright/test');
test('human question corrects a wrong type and preserves the original text',async({page})=>{
  await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();
  await page.locator('#domain').selectOption('logic');await page.locator('#topic').selectOption('truth_table');
  const input='Could you please find the determinant of [[1,2],[3,4]]?';
  await page.locator('#input').fill(input);await page.locator('#solve').click();
  await expect(page.locator('#answer')).toHaveText('det(A) = -2');
  await expect(page.locator('#domain')).toHaveValue('linear_algebra');await expect(page.locator('#topic')).toHaveValue('determinant');
  await expect(page.locator('#input')).toHaveValue(input);await expect(page.locator('#interpretation')).toContainText('Problem type corrected');
  await expect(page.locator('#interpretation pre')).toHaveText('1,2;3,4');
  await page.locator('.nav[data-view="history"]').click();await page.locator('.history-item').first().click();
  await expect(page.locator('#input')).toHaveValue(input);await expect(page.locator('#topic')).toHaveValue('determinant');
});
test('manual mode stays explicit, then automatic interpretation can recover',async({page})=>{
  await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();
  await page.locator('#auto-type').uncheck();await page.locator('#input').fill('Please calculate twenty five plus seven');await page.locator('#solve').click();
  await expect(page.locator('#verification')).toContainText('could not be solved');await expect(page.locator('#interpretation')).toBeHidden();
  await page.locator('#auto-type').check();await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('32');
});
test('natural input works offline and ambiguity is visible without an invented answer',async({page,context})=>{
  await page.goto('./');await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:40000});
  await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled();
  await page.locator('#input').fill('Voltage divider Vin=12V, R1=1kOhm');await page.locator('#solve').click();
  await expect(page.locator('#interpretation')).toContainText('Please clarify');await expect(page.locator('#answer')).toContainText('r2');
  await page.locator('#input').fill('Voltage divider Vin=12V, R1=1kOhm, R2=2kOhm');await page.locator('#solve').click();
  await expect(page.locator('#answer')).toHaveText('Vout = 8 V; I = 0.004 A');
  await page.locator('#input').fill('Differentiate f(x) = 3x² - 2x + 7');await page.locator('#solve').click();
  await expect(page.locator('#answer')).toHaveText('6x - 2');await expect(page.locator('#topic')).toHaveValue('differentiation');
  expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth)).toBe(true);
});
