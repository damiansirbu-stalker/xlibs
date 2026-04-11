xlibs: Shared utility library for STALKER Anomaly modding, by Damian
GitHub: https://github.com/damiansirbu-stalker/xlibs
Changelog: https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/changelog

xlibs is a modder's toolbox covering the full surface of what Anomaly mods typically need - entity queries, squad operations, smart terrain logic, stash manipulation, logging, profiling, event systems, and data structures.

The API design comes from reverse engineering the X-Ray engine and Anomaly internals, cross-referenced with patterns from the best modders in both the European and Russian STALKER modding traditions. Every function wraps engine quirks, guards against nil, and handles edge cases that would otherwise require each mod to solve independently.

Pure Lua where possible. No engine dependency unless necessary. No central loader - the engine auto-loads scripts on first access. Just call xlog.get_logger() or xsquad.find_squads() and it works.

Features:

Entity and World:
  xcreature    Creature identification, type checks, fluent server object query iterator
  xsquad       Squad search, scripted control, release, chase, iteration
  xsmart       Smart terrain queries, faction detection, capacity, arrival, conquest
  xlevel       Level/map queries, game time, location names, vertex validation
  xobject      Server object resolution from any input type, offline item creation
  xstash       Stash discovery, looting, filling, item filtering
  xdata        Unscriptable NPC/squad tables (traders, mechanics, story characters)
  xconst       X-Ray engine sentinel constants (invalid entity ID, invalid level vertex ID)

Data Structures:
  xtable       Filter, find, reduce, clone, shuffle, sort, binary insert, memoize, locks
  xttltable    TTL key-value store and sliding window counter with auto-expiry
  xmath        Probability rolls, weighted choice, random sampling, variation
  xslice       Time-sliced array iteration across frames
  xstring      String interpolation with ${key} placeholders

Diagnostics:
  xlog         Buffered file logging with session management and rotation
  xprofiler    Microsecond code profiling via engine profile_timer
  xtrace       Trace ID generation for request tracking across modules
  xinspect     Deep table and userdata inspection, engine type identification

Integration:
  xbus         Pub/sub event bus with pcall-wrapped delivery and diagnostics
  xevent       Runtime function hooking for synthetic callbacks
  xpda         PDA messages and map markers (squad and entity)
  xmcm         MCM config getters and loaders with defaults extraction

Requirements:
Anomaly 1.5.3
Modded exes

Install (MO2):
1. Install xlibs
2. Load order does not matter (scripts are auto-loaded by the engine on demand)

Uninstall (MO2):
Disable or remove in MO2. Any mod depending on xlibs will stop working.

Configuration:
No configuration needed. xlibs is a passive library loaded on demand by other mods.

Compatibility:
Compatible with all modded exe variants (Demonized, AOE, MT).
Pure library. Does not modify any base scripts, does not register callbacks, does not run any background logic. Compatible with everything including GAMMA.

Development:
Written against X-Ray Monolith engine source, Demonized exes source code, and Anomaly 1.5.3 unpacked gamedata.
Code patterns and engine usage validated against established work by reputable GAMMA modders (Demonized, Vintar0, RavenAscendant, xcvb).
The code is validated in real time by a multi-stage pipeline: luacheck, selene, tree-sitter AST analysis, contract rules, cross-file dependency resolution, cyclomatic complexity analysis, crash and vulnerability pattern detection, lua54 integration testing with X-Ray engine stubs, gitleaks secret scanning.
Full report in doc/test-report.log.

Credits:
Stalker_Boss - Russian translation
Altogolik - support, ideas, source materials

Usage and License:
  Calling xlibs functions from your mod: intended use, no restrictions.
  Modpacks: allowed and encouraged. Keep the readme and license files.
  Addons, patches, integrations: allowed. Credit "xlibs by Damian Sirbu" visibly on your mod page.
  Reproducing the implementation in other software: not allowed, even with credit.
  Full license in LICENSE file and on GitHub.
