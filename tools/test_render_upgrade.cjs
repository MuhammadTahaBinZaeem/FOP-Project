// Exercise the update controls using ONLY a profile created by our own tests.
const {chromium}=require('playwright'),{expect}=require('@playwright/test');
const fs=require('node:fs/promises'),path=require('node:path');
(async()=>{
 const profile=path.resolve(process.argv[2]||''),output=process.argv[3];
 if(!profile.startsWith(path.resolve('build-evidence')+path.sep)||!path.basename(profile).startsWith('browser-profile-')||!output)throw Error('Select an owned test profile and new output directory');
 await fs.mkdir(output);const site='https://pocket-engineer.onrender.com/';
 const context=await chromium.launchPersistentContext(profile,{headless:true,viewport:{width:390,height:844},executablePath:process.env.PE_BROWSER_PATH||undefined});
 const report={site,profile,checked_at:new Date().toISOString(),checks:[]};
 try{
  const page=context.pages()[0]||await context.newPage();await page.goto(site);await expect(page.locator('#solve')).toBeEnabled({timeout:60000});
  report.before=await page.locator('[data-download]').first().getAttribute('href');
  expect(report.before).toMatch(/^https:\/\/github\.com\//);
  const draft='Please find the determinant of [[2,3],[4,5]]';await page.locator('#input').fill(draft);await page.locator('#auto-type').check();
  await page.locator('.nav[data-view=downloads]').click();await expect(page.locator('#retry-cache')).toBeEnabled({timeout:180000});await page.locator('#retry-cache').click();
  await expect(page.locator('#update-offline')).toBeVisible({timeout:180000});await page.locator('#update-offline').click();
  await expect(page.locator('[data-download]').first()).toHaveAttribute('href',/^https:\/\/pocket-engineer\.onrender\.com\/downloads\/v0\.5\.0-rc2\//,{timeout:60000});
  await expect(page.locator('#input')).toHaveValue(draft);report.checks.push('actual repair/update buttons','new public Render links','unsolved draft preserved across the update');
  report.after=await page.locator('[data-download]').first().getAttribute('href');
  await page.locator('.nav[data-view=workbench]').click();await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('det(A) = -2');
  // Update-draft preservation is a one-shot contract, not unsolved autosave
  // across every later ordinary refresh. Enter the offline question explicitly.
  await context.setOffline(true);await page.reload();await expect(page.locator('#solve')).toBeEnabled({timeout:60000});await page.locator('.nav[data-view=workbench]').click();await page.locator('#input').fill(draft);await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('det(A) = -2');report.checks.push('updated install accepts and solves a question after offline reload');
  await page.screenshot({path:output+'/updated-offline.png'});report.success=true;
 }catch(error){report.success=false;report.error=error.message;throw error;}finally{await fs.writeFile(output+'/report.json',JSON.stringify(report,null,2));await context.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
