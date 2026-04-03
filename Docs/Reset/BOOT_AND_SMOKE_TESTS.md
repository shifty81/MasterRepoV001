# Boot and Smoke Tests

## Repo Reset Smoke Test
- configure from a clean checkout
- build editor and game targets
- launch editor directly into the dev world
- verify window is interactive
- verify viewport shows real output
- verify resize/maximize keeps viewport and panels stable
- verify toolbar/menu buttons execute real commands
- verify viewport click changes selection
- verify outliner selection round-trips to viewport
- verify inspector reflects selected state
- verify one voxel add/remove action works
- verify save/reload preserves the edited result
- verify standalone client launches from editor context

## Failure Classification

### Class 1 — Boot failure
The app does not start or cannot open the project/dev world.

### Class 2 — Shell failure
The app opens but panels are not clickable, readable, or resize-safe.

### Class 3 — Viewport failure
The center panel does not present authoritative render output.

### Class 4 — Interaction failure
Selection, tool actions, or command execution do not round-trip.

### Class 5 — Persistence failure
Edits are not saved, restored, or reflected across relaunch.
