const {defineConfig}=require('@playwright/test');
module.exports=defineConfig({
  testDir:'tests/browser',timeout:45000,expect:{timeout:15000},fullyParallel:false,workers:1,
  reporter:[['list'],['json',{outputFile:'test-results/browser-results.json'}]],
  use:{baseURL:process.env.PE_SITE_URL||'http://127.0.0.1:8091',headless:true,trace:'retain-on-failure',
    launchOptions:process.env.PE_BROWSER_PATH?{executablePath:process.env.PE_BROWSER_PATH}:{}},
  webServer:process.env.PE_SITE_URL?undefined:{command:'./build/pocket-engineer-server 8091 www',url:'http://127.0.0.1:8091',reuseExistingServer:!process.env.CI},
  projects:[{name:'desktop',use:{viewport:{width:1440,height:1000}}},{name:'android-layout',use:{viewport:{width:390,height:844},isMobile:true,hasTouch:true,deviceScaleFactor:2}}]
});
