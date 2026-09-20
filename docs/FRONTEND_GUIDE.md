# A simpler workspace

The September 2026 frontend revision keeps questions, circuits, signals, state
diagrams and test demos under one persistent workspace toolbar. Only the chosen
tool is highlighted. Browser Back/Forward and direct links such as `/#fsm` work
within this workspace. Switching tools retains their entered values and results.

## Start with the minimum

- **Question:** type a supported natural question and choose **Show me the steps**.
  Automatic interpretation starts enabled. Selecting a subject/type is optional;
  **Advanced** exposes manual interpretation and exact syntax.
- **Use a form:** choose a subject/type, fill its values, then solve directly.
  Matrices have cells; K-maps have clickable 0/1/X cells. The text representation
  stays hidden until **Write a question** is selected. Ordinary guided solutions
  also appear in Recent work.
- **Circuits:** build a drawing (or start from an example), then choose analysis.
  DC does not request AC frequency or transient settings. Exact coordinates,
  editable netlists, port studies and initial conditions are in Advanced.
  A visible note distinguishes the drawing from an overridden netlist.
- **Signals:** choose a calculation, then enter the required samples or
  coefficients. Optional histories, complex parts and specialist settings are in
  Advanced. Applicable sample-rate defaults are stated outside it. Conditional
  fields follow the chosen operation; changing operations retains edited values.
- **State diagrams:** set up states, then edit their transitions. Bit widths,
  initial state and simulation sequence are optional Advanced controls. The
  initial 101-detector example is explicitly identified, not presented as a
  blank machine. Changing state count retains compatible transition cells.
- **Test demos:** choose category, problem type and difficulty. Test the visible
  25 cases normally; open Advanced for calibration, all 5,000 cases or export.

Numbered step buttons and Next/Back show one input stage at a time. Advanced
disclosures use native keyboard-accessible controls. Inputs remain in the DOM
when a stage is hidden; they are not reset by navigating backwards. Pending
app updates preserve the active tool/stage and edited lab values. Reloading
without an app-update draft is not a promise of permanent unsolved-work storage.

## Scope and delivery

This is a presentation revision, not an expansion of supported mathematics.
Validation, default values, serialization and solving remain in C++. Hidden
settings retain their documented defaults; open Advanced to inspect/change them.
There are no new SVGs, framework dependencies or animations. The Android app
packages the same frontend, but an already-installed APK does not update itself
when the website changes. Existing versioned prerelease downloads are immutable.

See [test history](TEST_HISTORY.md) for actual runs, failures and platform limits.
