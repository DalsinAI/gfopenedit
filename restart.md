# Restart: GFOpenEdit

_Written 6 October 2026 at about 23:55 UTC, while all work is paused on @SacredTrees's word (23:28 UTC). Read this first when work resumes; the newest capsule and the live PR list win if they disagree._

## What this repo is

GFOpenEdit, the plain text editor for GFOpenUp in place of TextEdit and Ed: tabs, find and replace, syntax colours, Shell and ARexx use, safe saving. GadTools only.

## Where it stands

Phase 1 is merged (#1) and ships in GFOpenUp 0.6.13; Main Discourse built it on the home PC (104,336 bytes, no warnings). The repo's default branch is main.

## Merged lately

- #1 (672f0ef, 2026-10-06): GFOpenEdit phase 1: tabs, find and replace, colours, Shell and ARexx, safe save
- #2 (c2df873, 2026-10-06): Credit who made GFOpenEdit: CONTRIBUTORS.md

## Open pull requests

- None.

## Next step

1. Route text files to GFOpenEdit in GFOpenTypes and set ENV:EDITOR to "C:GFOpenEdit WAIT" (Main Discourse packaging).
2. Phase 2: split view, bookmarks, compare, macros.

## Waiting on @SacredTrees

- The go for phase 2.

## Who owns it

GFOpenEdit thread; Main Discourse for packaging.

## Capsules

Restart capsules for this repo's workstreams, in amicradle's `capjumps/` shelf:

- [`20261006_AmigaChrome_OpenEdit_Restart_Capsule.zip`](https://github.com/DalsinAI/amicradle/tree/main/capjumps)

Team rules that still hold: commits as SacredTrees with no co-author lines; third-party code only on "yes with review" (licence checked, commit and sha256 pinned, fetched at build, never committed); deploys with deploy_dev.py only, on a typed line.
