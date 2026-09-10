// Download via actual website links, then verify bytes against the public manifest.
// Run only after publishing the release and deploying its download links.
const {chromium}=require('playwright');
const {expect}=require('@playwright/test');
const fs=require('node:fs/promises');
const path=require('node:path');
const {createHash}=require('node:crypto');
(async()=>{
  const site=process.env.PE_SITE_URL||'https://pocket-engineer.onrender.com/';
  const directory=process.argv[2];if(!directory)throw Error('Provide a new output directory');
  await fs.mkdir(directory);
  const browser=await chromium.launch({headless:true,...(process.env.PE_BROWSER_PATH?{executablePath:process.env.PE_BROWSER_PATH}:{})});
  try{
    const context=await browser.newContext({acceptDownloads:true});
    const page=await context.newPage();await page.goto(site);
    await expect(page.locator('#solve')).toBeEnabled();
    await page.locator('.nav[data-view="downloads"]').click();
    const manifestUrl=await page.locator('#download-checksums').getAttribute('href');
    const response=await context.request.get(manifestUrl);if(!response.ok())throw Error('Cannot download public checksum manifest');
    const manifest=await response.text();await fs.writeFile(path.join(directory,'SHA256SUMS.txt'),manifest);
    const rows=[];const links=page.locator('[data-download]');await expect(links).toHaveCount(5);
    for(const link of await links.all()){
      const name=await link.getAttribute('data-download');
      if(!/^PocketEngineer-0\.5\.0-[\w.-]+\.(apk|zip)$/.test(name))throw Error('Unexpected download name');
      const event=page.waitForEvent('download',{timeout:120000});await link.click();const download=await event;
      if(download.suggestedFilename()!==name)throw Error('Unexpected suggested download filename');
      const destination=path.join(directory,name);await download.saveAs(destination);
      if(await download.failure())throw Error('Browser download failed');
      const data=await fs.readFile(destination),sha256=createHash('sha256').update(data).digest('hex');
      if(!manifest.split('\n').includes(`${sha256}  ${name}`))throw Error('Downloaded bytes do not match manifest: '+name);
      rows.push({name,bytes:data.length,sha256,actual_browser_click:true,checksum_verified:true});
      console.log('Downloaded and verified: '+name);
    }
    await page.screenshot({path:path.join(directory,'download-page.png'),fullPage:true});
    await fs.writeFile(path.join(directory,'browser-downloads.json'),JSON.stringify({site,checked_at:new Date().toISOString(),downloads:rows,limitations:'Desktop Chromium download interactions and SHA-256 integrity; platform execution and Android installation are tested separately.'},null,2)+'\n');
  }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
