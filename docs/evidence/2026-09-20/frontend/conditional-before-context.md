# Instructions

- Following Playwright test failed.
- Explain why, be concise, respect Playwright best practices.
- Provide a snippet of code with the fix, if possible.

# Test info

- Name: progressive-ui.spec.cjs >> conditional transform fields do not ask for irrelevant values
- Location: tests/browser/progressive-ui.spec.cjs:40:1

# Error details

```
Error: expect(locator).toContainText(expected) failed

Locator: locator('#answer')
Expected substring: "s^2"
Received string:    ""
Timeout: 15000ms

Call log:
  - Expect "toContainText" locator('#answer') with timeout 15000ms
  - waiting for locator('#answer')
    34 × locator resolved to <pre id="answer"></pre>
       - unexpected value ""

```

```yaml
- link "Skip to workspace":
  - /url: "#workspace"
- complementary:
  - link "Pocket Engineer home":
    - /url: "#workbench"
    - text: Pocket Engineer
  - paragraph: YOUR STUDY SPACE
  - navigation "Main navigation":
    - button "⌘ Workbench"
    - button "▦ All subjects"
    - button "↺ Recent work"
    - button "↓ Get the app"
  - text: Private by design
  - paragraph: Your questions stay on this device. No account. No cloud solver.
  - link "Open-source · C++ core ↗":
    - /url: https://github.com/MuhammadTahaBinZaeem/FOP-Project
- main:
  - text: Your pocket-sized engineering companion
  - status: C++ · on your device
  - heading "Your engineering workspace." [level=1]
  - paragraph: Choose a tool. Add your values. Work through the answer.
  - group "Choose a tool":
    - button "Question"
    - button "Circuits"
    - button "Signals" [pressed]
    - button "State diagrams"
    - button "Test demos"
  - heading "Signals & transforms" [level=2]
  - paragraph: Choose a calculation, then enter the samples or coefficients.
  - list:
    - listitem:
      - button "1. Choose calculation"
    - listitem:
      - button "2. Enter values"
  - button "Back"
  - text: "Causal kernel: power = tⁿ"
  - 'combobox "Causal kernel: power = tⁿ"':
    - option "power"
    - option "sine" [selected]
    - option "cosine"
  - text: Amplitude A
  - textbox "Amplitude A": "1"
  - text: Angular frequency ω (rad/s)
  - textbox "Angular frequency ω (rad/s)": "2"
  - group: Advanced · optional settings
  - button "Calculate locally"
  - status: Enter a value for Power n (0–12)
  - text: Ready offline · solver and all 825,000 demos saved (184 files; build 47f59c09). Revisit http://127.0.0.1:8091/ in this browser without internet.
  - button "Offline access & downloads"
```

# Test source

```ts
  1  | const {test,expect}=require('@playwright/test');
  2  | async function ready(page){await page.goto('./');await expect(page.locator('#solve')).toBeEnabled();}
  3  | async function next(page,name){await page.getByRole('button',{name:'Next: '+name,exact:true}).click();}
  4  | test('one workspace, neutral demos, correct selection and browser Back retain the question',async({page})=>{
  5  |   await ready(page);await expect(page.locator('.tool-launcher .primary')).toHaveCount(0);await expect(page.locator('.tool-launcher [data-view=demos]')).toHaveAttribute('aria-pressed','false');
  6  |   await page.locator('#input').fill('23+19');await page.locator('#solve').click();await expect(page.locator('#answer')).toHaveText('42');
  7  |   await page.locator('[data-lab=fsm]').click();await expect(page.locator('#workbench')).toBeVisible();await expect(page.locator('#question-panel')).toBeHidden();await expect(page.locator('.nav[data-view=workbench]')).toHaveAttribute('aria-current','page');await expect(page.locator('[data-lab=fsm]')).toHaveAttribute('aria-pressed','true');
  8  |   await page.goBack();await expect(page.locator('#input')).toBeVisible();await expect(page.locator('#input')).toHaveValue('23+19');await expect(page.locator('#answer')).toHaveText('42');
  9  |   await page.goForward();await expect(page.locator('#fsm-count')).toBeVisible();await expect(page.locator('.tool-launcher [aria-pressed=true]')).toHaveCount(1);
  10 | });
  11 | test('natural input needs no type selection and advanced controls are disclosed explicitly',async({page})=>{
  12 |   await ready(page);await expect(page.locator('#domain')).toBeHidden();await expect(page.locator('#auto-type')).toBeHidden();await expect(page.locator('#input')).toHaveValue('');
  13 |   await page.locator('#input').fill('Please find the determinant of [[2,3],[4,5]]');await page.locator('#solve').click();await expect(page.locator('#answer')).toContainText('-2');
  14 |   await page.locator('#question-advanced>summary').click();await expect(page.locator('#auto-type')).toBeVisible();await page.locator('#auto-type').uncheck();await expect(page.locator('#input')).toHaveValue('Please find the determinant of [[2,3],[4,5]]');
  15 | });
  16 | test('guided matrix solves directly without exposing serialized text',async({page})=>{
  17 |   await ready(page);await page.locator('#guided-toggle').click();await page.locator('#domain').selectOption('linear_algebra');await page.locator('#topic').selectOption('determinant');
  18 |   await expect(page.locator('#input')).toBeHidden();await page.getByRole('textbox',{name:'Matrix A row 1 column 1',exact:true}).fill('5');await page.getByRole('button',{name:'Solve with these values',exact:true}).click();await expect(page.locator('#answer')).toContainText('14');
  19 |   await page.locator('#guided-toggle').click();await expect(page.locator('#input')).toHaveValue('5,2;3,4');await page.locator('#guided-toggle').click();await expect(page.getByRole('textbox',{name:'Matrix A row 1 column 1',exact:true})).toHaveValue('5');
  20 | });
  21 | test('circuit flow exposes only the analysis fields that apply',async({page})=>{
  22 |   await ready(page);await page.locator('[data-lab=circuit]').click();await expect(page.locator('#circuit-canvas')).toBeVisible();await expect(page.locator('#circuit-value')).toBeHidden();await expect(page.locator('#circuit-netlist')).toBeHidden();
  23 |   await next(page,'choose analysis');await expect(page.locator('#circuit-analysis')).toBeVisible();await expect(page.locator('#circuit-frequency')).toBeHidden();await expect(page.locator('#circuit-study')).toBeHidden();
  24 |   await page.getByRole('button',{name:'Solve this circuit',exact:true}).click();await expect(page.locator('#verification')).toContainText('KCL');
  25 |   await page.locator('#circuit-analysis').selectOption('ac');await expect(page.locator('#circuit-frequency')).toBeVisible();await expect(page.locator('#circuit-step')).toBeHidden();
  26 |   await page.getByRole('button',{name:'Back',exact:true}).click();await expect(page.locator('#circuit-canvas')).toBeVisible();await next(page,'choose analysis');await expect(page.locator('#circuit-analysis')).toHaveValue('ac');
  27 | });
  28 | test('signals retain edited operations and advanced sample rates are explicit',async({page})=>{
  29 |   await ready(page);await page.locator('[data-lab=signals]').click();await expect(page.locator('#signal-operation')).toBeVisible();await expect(page.locator('#signal-x')).toBeHidden();await next(page,'enter values');
  30 |   await page.locator('#signal-x').fill('1,0,0,0');await expect(page.locator('#signal-fs')).toBeHidden();await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#verification')).toContainText('Parseval');
  31 |   await page.locator('#signals-stage-1 details>summary').click();await page.locator('#signal-fs').fill('2000');await expect(page.locator('#signals-stage-1')).toContainText('Sample rate: 2000 Hz');
  32 |   await page.getByRole('button',{name:'Back',exact:true}).click();await page.locator('#signal-operation').selectOption('convolution');await next(page,'enter values');await page.locator('#signal-x').fill('2,-1,3');await page.locator('#signal-h').fill('4,2');await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#answer')).toContainText('8, 0, 10, 6');
  33 |   await page.getByRole('button',{name:'Back',exact:true}).click();await page.locator('#signal-operation').selectOption('fft');await next(page,'enter values');await expect(page.locator('#signal-x')).toHaveValue('1,0,0,0');await page.locator('#signals-stage-1 details>summary').click();await expect(page.locator('#signal-fs')).toHaveValue('2000');
  34 | });
  35 | test('state setup, transitions and optional simulation remain in the same workspace',async({page})=>{
  36 |   await ready(page);await page.locator('[data-lab=fsm]').click();await expect(page.locator('#fsm-count')).toBeVisible();await expect(page.locator('#fsm-input-bits')).toBeHidden();await expect(page.locator('#fsm-next-0-0')).toBeHidden();
  37 |   await next(page,'edit transitions');await page.locator('#fsm-next-0-0').selectOption('S1');await expect(page.locator('#fsm-sequence')).toBeHidden();await page.getByRole('button',{name:'Back',exact:true}).click();await page.locator('#fsm-count').selectOption('4');await next(page,'edit transitions');await expect(page.locator('#fsm-next-0-0')).toHaveValue('S1');
  38 |   await page.getByRole('button',{name:'Generate diagram & equations',exact:true}).click();await expect(page.locator('#visual canvas')).toBeVisible();await expect(page.locator('#verification')).not.toContainText('Input could not be solved');
  39 | });
  40 | test('conditional transform fields do not ask for irrelevant values',async({page})=>{
  41 |   await ready(page);await page.locator('[data-lab=signals]').click();await page.locator('#signal-operation').selectOption('laplace');await next(page,'enter values');await expect(page.locator('#signal-power')).toBeVisible();await expect(page.locator('#signal-omega')).toBeHidden();await page.locator('#signal-kind').selectOption('sine');await expect(page.locator('#signal-power')).toBeHidden();await expect(page.locator('#signal-omega')).toBeVisible();
> 42 |   await page.locator('#signal-kind').selectOption('power');await page.locator('#signal-power').fill('');await page.locator('#signal-kind').selectOption('sine');await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#answer')).toContainText('s^2');await expect(page.locator('#lab-status')).toBeEmpty();
     |                                                                                                                                                                                                                                                                                    ^ Error: expect(locator).toContainText(expected) failed
  43 |   await page.locator('#signal-kind').selectOption('power');await expect(page.locator('#signal-power')).toHaveValue('');await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#lab-status')).toContainText('Enter a value for Power');
  44 | });
  45 | test('update drafts keep the active input stage and do not jump from questions to a previous tool',async({page})=>{
  46 |   await ready(page);await page.locator('[data-lab=signals]').click();await next(page,'enter values');await page.locator('#signal-x').fill('9,8,7,6');
  47 |   await page.evaluate(()=>dispatchEvent(new Event('pe-save-draft')));await page.reload();await expect(page.locator('#signal-x')).toBeVisible();await expect(page.locator('#signal-x')).toHaveValue('9,8,7,6');
  48 |   await page.locator('.tool-launcher [data-view=workbench]').click();await page.locator('#input').fill('12+34');await page.evaluate(()=>dispatchEvent(new Event('pe-save-draft')));await page.reload();await expect(page.locator('#input')).toBeVisible();await expect(page.locator('#input')).toHaveValue('12+34');await expect(page.locator('#labs')).toBeHidden();
  49 |   await page.locator('[data-lab=signals]').click();await expect(page.locator('#signal-x')).toBeVisible();await expect(page.locator('#signal-x')).toHaveValue('9,8,7,6');
  50 | });
  51 | test('offline deep links, tool results and guided history preserve context',async({page,context})=>{
  52 |   await ready(page);await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:60000});await context.setOffline(true);await page.goto('./#signals');await next(page,'enter values');await page.locator('#signal-x').fill('1,0,0,0');await page.getByRole('button',{name:'Calculate locally',exact:true}).click();await expect(page.locator('#answer')).toContainText('4 complex samples');
  53 |   await page.locator('[data-lab=fsm]').click();await expect(page.locator('#result')).toBeHidden();await next(page,'edit transitions');await page.getByRole('button',{name:'Generate diagram & equations',exact:true}).click();await expect(page.locator('#answer')).toContainText('Simulation outputs: 0 0 1 0 1');
  54 |   await page.locator('[data-lab=signals]').click();await expect(page.locator('#answer')).toContainText('4 complex samples');await expect(page.locator('#signal-x')).toHaveValue('1,0,0,0');
  55 |   await page.locator('.tool-launcher [data-view=workbench]').click();await expect(page.locator('#result')).toBeHidden();await page.locator('#guided-toggle').click();await page.locator('#domain').selectOption('linear_algebra');await page.locator('#topic').selectOption('determinant');await page.getByRole('button',{name:'Solve with these values',exact:true}).click();await expect(page.locator('#answer')).toContainText('-2');
  56 |   await page.locator('.nav[data-view=history]').click();await expect(page.locator('.history-item')).toHaveCount(1);await page.locator('.history-item').click();await expect(page.locator('#input')).toHaveValue('1,2;3,4');
  57 | });
  58 | test('both stages fit small screens and keyboard navigation opens only the requested stage',async({page})=>{
  59 |   await ready(page);for(const width of [320,390,768,1440]){await page.setViewportSize({width,height:844});for(const lab of ['circuit','signals','fsm']){await page.locator(`[data-lab=${lab}]`).click();for(const stage of [0,1]){const control=page.locator(`[aria-controls=${lab}-stage-${stage}]`);await control.focus();await page.keyboard.press('Enter');await expect(page.locator(`#${lab}-stage-${stage}`)).toBeVisible();await expect(page.locator('#lab-content .flow-step:visible')).toHaveCount(1);expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1)).toBe(true);if(lab==='fsm'&&stage===1&&width>=390)expect(await page.locator('#fsm-stage-1 .table-scroll').evaluate(e=>e.scrollWidth<=e.clientWidth+1)).toBe(true);}}}
  60 | });
  61 | 
```