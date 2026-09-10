const {test,expect}=require('@playwright/test');
test('demo action labels do not collapse into vertical strips near phone breakpoints',async({page},info)=>{
  await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();await page.locator('.tool-launcher [data-view=demos]').click();await expect(page.locator('#demo-run')).toBeEnabled();
  for(const width of [320,381,390,412,480,600,700,768,1440]){
    await page.setViewportSize({width,height:844});
    const buttons=await page.locator('#demos .actions button').evaluateAll(nodes=>nodes.map(n=>({label:n.textContent,width:n.getBoundingClientRect().width,height:n.getBoundingClientRect().height})));
    for(const b of buttons){expect(b.height,b.label+' at '+width).toBeLessThanOrEqual(76);if(b.label.includes('Test this page'))expect(b.width).toBeGreaterThan(140);}
    expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1)).toBe(true);
  }
  await page.setViewportSize({width:412,height:844});await page.locator('#demo-run').click();await expect(page.locator('#demo-status')).toContainText('Completed: 25 cases; 0 differ');await page.locator('#demo-status').scrollIntoViewIfNeeded();await page.screenshot({path:`test-results/demo-buttons-${info.project.name}.png`});
});
