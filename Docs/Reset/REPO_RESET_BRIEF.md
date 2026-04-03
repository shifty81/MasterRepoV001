# Repo Reset Brief

## Purpose
This reset pack is the implementation baseline for a hard editor-system correction.
It is designed for the current `MasterRepo` state and assumes the repo will be cleaned, archived, then rebuilt forward from the real viewport/editor path rather than continuing to stack shell-only UI over an unrealized center panel.

## What This Pack Changes
- rewrites the roadmap around **editor reality first**
- reclassifies existing work into **keep / rewrite / archive / defer**
- locks the repo as a **standalone NovaForge game + engine + game-specific editor**
- makes the viewport the highest-priority implementation target
- treats panel polish as secondary until selection, picking, tool state, and render output are real
- defines a checkpoint/archive step before refactors
- adds implementation order, smoke tests, and GitHub Copilot directions

## Reset Outcome
The intended result after this reset is:
1. the repo boots cleanly
2. the editor window is interactive
3. the center viewport is real
4. selection/tool state round-trips between viewport, outliner, and inspector
5. the dev world is the single early test target
6. voxel editing is visible and authoritative
7. the project roadmap is aligned to the actual critical path

## What This Pack Does Not Pretend
This zip does **not** claim the full code rewrite is finished.
It provides a repo-reset baseline, updated docs, implementation order, and archive strategy so the repo can be rewritten against one coherent plan.
