# Keep / Rewrite / Archive / Defer Matrix

## Keep
- standalone repo boundary for game + engine + game-specific editor
- dev world as the single early test world
- voxel-first world authority
- starter R.I.G. interaction loop direction
- `ProjectPathService`
- `WorldFileService`
- `DevWorldSerializer`
- `SelectionService`
- `PropertyInspectorSystem`
- `WorldOutlinerPanel`
- `EditorCommandRegistry`
- `EditorHotkeyMap`
- `VoxelPickService`

## Rewrite
- editor viewport ownership and presentation path
- toolbar/menu command bindings if currently cosmetic
- input routing from shell to viewport/editor command layer
- inspector widgets that are static or not dirty-tracked
- outliner data source if not backed by real world state
- startup path that still behaves like a temporary bootstrap chooser
- roadmap/task/gap docs that overstate completion

## Archive
- current repo snapshot before reset
- deprecated editor panel paths kept only for reference
- duplicate or superseded viewport implementations
- obsolete shell experiments once real replacements exist
- old doc packs that conflict with the reset order

## Defer
- low-poly wrapper pass
- fleet command systems
- broad galaxy simulation
- advanced multiplayer expansion
- broad content tools
- non-critical editor luxuries
