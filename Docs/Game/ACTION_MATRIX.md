# MasterRepo Reset — Action Matrix

## Keep
- standalone game/engine/editor repo boundary
- dev world as the early test target
- voxel-first direction
- existing world, voxel, selection, outliner, inspector, and command scaffolds where they map to the reset plan

## Rewrite
- roadmap and task docs to reflect actual status
- viewport render ownership and presentation path
- shell-to-editor command binding
- selection synchronization path
- editor boot flow toward editor-first dev world boot

## Archive
- the current repo state before refactor
- deprecated viewport paths
- shell-only or duplicate panel implementations that are replaced by real behavior
- outdated docs that conflict with the reset order

## Next
1. archive current repo
2. make the editor window trustworthy
3. make the viewport real
4. wire selection and property editing
5. harden the dev world and voxel edit loop
