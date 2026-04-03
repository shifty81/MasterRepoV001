# GitHub Copilot Reset Directions

Use these rules while implementing the repo reset.

## Priority Order
1. fix boot and interaction defects
2. make viewport output real
3. wire selection/tool/inspector/outliner loop
4. only then improve panel breadth

## Coding Rules
- prefer replacing fake behavior with thin real behavior over adding more stubs
- remove or archive duplicate paths once the real path works
- keep engine/editor/game boundaries clean
- do not add Atlas Suite or generic workspace systems into this repo
- do not mark work complete until it is visible in editor behavior

## Required Validation Per Change
For every editor-facing change, verify:
- can the user click it
- does it change real state
- is the result visible in viewport, outliner, inspector, or status bar
- does it survive resize/relaunch/save-load when applicable

## High-Value Files To Review First
- `Source/Editor/Viewport/*`
- `Source/Editor/Application/*`
- `Source/Editor/Commands/*`
- `Source/Editor/Selection/*`
- `Source/Editor/Inspector/*`
- `Source/Editor/Panels/WorldOutlinerPanel.*`
- `Source/Game/World/*`
- `Source/Game/Voxel/*`
- `Config/novaforge.project.json`
- `Content/Definitions/DevWorld.json`

## Initial Implementation Questions To Answer In Code
- where does the viewport own size and presentation
- what code path presents the rendered frame into the center panel
- what path computes the pick ray from viewport-local coordinates
- what panel class is authoritative for world outliner data
- what inspector path writes changes back to the selected object/chunk/voxel
- what startup path should be removed so editor-first boot is default
