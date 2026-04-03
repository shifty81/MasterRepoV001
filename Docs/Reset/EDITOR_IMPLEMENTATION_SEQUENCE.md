# Editor Implementation Sequence

## Step 0 — Checkpoint the repo
Create a clean archive branch and zip snapshot of the current repo before destructive refactors.

Output:
- tag or branch for the last pre-reset state
- archived zip under a local ignored archive path
- docs note stating what was frozen

## Step 1 — Make the window trustworthy
Target:
- input is clickable
- resizing/maximize works
- dark mode and fonts are sane
- shell panels no longer behave like a static image

## Step 2 — Make the center viewport real
Target:
- viewport owns panel bounds and render size
- scene renders into a viewport target
- target is presented into the viewport rect
- camera input affects real rendered output

## Step 3 — Bind editor state to the shell
Target:
- toolbar/menu actions call real commands
- status bar reflects live mode/state
- selection changes are visible outside the viewport

## Step 4 — Selection loop
Target:
- viewport click selects
- outliner selection syncs
- inspector selection syncs
- active selection highlight is visible in viewport

## Step 5 — Voxel editing loop
Target:
- voxel inspect mode works
- add/remove voxel works from viewport-local hit data
- touched chunk results drive overlay refresh and selection updates

## Step 6 — Undo/redo and edit batching
Target:
- voxel edits enter a command stack
- drag edits are batched
- chunk rebuild invalidation is not wasteful
- collision rebuild can be deferred separately from mesh rebuild

## Step 7 — Editor-first boot path
Target:
- editor boots directly to the dev world
- play-in-editor launches the real runtime path
- standalone client can be launched from the editor

## Step 8 — Only then polish panels
Target:
- content browser
- material/asset tools
- preference surfaces
- additional workflow panels
