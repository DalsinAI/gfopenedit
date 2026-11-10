# GFOpenEdit

The plain text editor for GFOpenUp. It replaces TextEdit and Ed for everyday
editing: tabs, find and replace, syntax colours for scripts, Shell and ARexx
use, and safe saving. GadTools only, so it needs nothing beyond AmigaOS 3.2.

Runs on a 68040 with FPU under AmigaOS 3.2.3 with GFOpenUp, without GFOpenGPU.

Status: phase 1 written, waiting on its first m68k build. See [DESIGN.md](DESIGN.md).

New in 0.1.1 (10 October 2026): the window first opens at 800 x 600, in the
middle of the space beside GFOpenDock (or all of that space on a smaller
screen), instead of filling the screen. A size you give it is kept.

## Building

With the os32 stove and GFOpenGadTools checked out beside this repo:

    ./build.sh                    # build/os3/GFOpenEdit and build/os3/Kinds/
    OGT=path/to/gfopengadtools ./build.sh

The text engine (gap buffer, undo, find, patterns, colour kinds) is plain C and
has host tests: `tests/host/run.sh`.

## Installing

- `C:GFOpenEdit`
- the kind files into `ENVARC:GFOpenEdit/Kinds/` (GFOpenEdit also looks in
  `PROGDIR:Kinds/`)
- `SetEnv SAVE EDITOR "C:GFOpenEdit WAIT"` so tools that call `$EDITOR` use it

## Using it from the Shell

    GFOpenEdit [FILES ...] [LINE n] [WAIT] [READONLY] [NEW] [PUBSCREEN name]

GFOpenEdit runs once. A second start hands its files to the running copy and
returns; with `WAIT` it returns when those tabs close.

## ARexx

Port `GFOPENEDIT`. With `OPTIONS RESULTS`, GET commands answer in `RESULT`.

| Command | Does |
|---|---|
| `OPEN file [LINE n]` | opens a file in a tab |
| `NEW` | a new Untitled tab |
| `FRONT` | brings the window to the front |
| `SAVE`, `SAVEAS file` | saves the current tab |
| `CLOSE [FORCE]` | closes the current tab (RC 5, kept, when it has unsaved changes and no FORCE) |
| `TAB n` | goes to tab n |
| `GOTO line [col]` | moves the caret |
| `FIND text` | finds the next match (RC 5 when none) |
| `REPLACEALL text with` | replaces all, answers the count |
| `INSERT text` | types text at the caret |
| `GETTEXT`, `GETLINE [n]`, `GETFILE`, `GETPOS` | answers the text, a line, the path, or "line col" |
| `QUIT [FORCE]` | quits (RC 5, stays, when there are unsaved changes and no FORCE) |
| `ACTIVATE` | the same as `FRONT` |
| `DEACTIVATE` | RC 5: GFOpenEdit has no icon to go to |
| `VERSION` | answers "GFOpenEdit 0.1.2" |
| `HELP [command]` | answers the commands, or one command and its arguments |
| `FAULT code` | answers the text of a DOS error code |
| `LASTERROR` | answers why the last command that failed failed |

`ACTIVATE`, `DEACTIVATE`, `VERSION`, `HELP`, `FAULT` and `LASTERROR` are the commands every Open app's port has.

Return codes (as every Open app's port): 0 done; 5 not done, nothing wrong (FIND
finds nothing, unsaved changes kept); 10 an error (a missing argument, a file that
can't be opened or saved); 20 an unknown command. No command shows a requester:
what one would have said is in `LASTERROR`. `Rexx/LineCount.rexx` is an example,
and `tests/arexx.rexx` checks the port.

## Settings

`ENV:GFOpenEdit/Settings` (Use) and `ENVARC:GFOpenEdit/Settings` (Save): font, tab
width, spaces for tabs, auto-indent, line numbers, current line, colours, and a
colour per class for light and dark themes.

MIT licence, © 2026 AmiCradle Team.

## Contributors

GFOpenEdit is created and maintained by [SacredTrees](https://github.com/SacredTrees) with The AmiCradle Team, copyright AmiCradle Team. Everyone whose work it includes is credited in [`CONTRIBUTORS.md`](CONTRIBUTORS.md).
