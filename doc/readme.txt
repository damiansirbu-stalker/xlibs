xlibs: Shared utility library for STALKER Anomaly modding, by Damian
Latest: 1.2.0

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

Data Structures:
  xtable       Filter, find, reduce, clone, shuffle, sort, binary insert, memoize, locks
  xttltable    TTL key-value store and sliding window counter with auto-expiry
  xmath        Probability rolls, weighted choice, random sampling, variation
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
Pure library. Does not modify any base scripts, does not register callbacks, does not run any background logic. Compatible with everything including GAMMA.

Credits:
DrakoMT and SaloEater for their support.
Demonized, Catspaw, Vintar0, RavenAscendant, xcvb, lizzardman, Aoldri, and Feel_Fried. Their work on the engine, modded exes, scripts, and tools shaped how Anomaly modding is done.

Development:
Source: https://github.com/damiansirbu-stalker/xlibs
Releases: https://github.com/damiansirbu-stalker/xlibs/releases
Written against X-Ray Monolith engine source, Demonized exes source code, and Anomaly 1.5.3 unpacked gamedata.
Code patterns and engine usage validated against established work by reputable GAMMA modders (Demonized, Vintar0, RavenAscendant, xcvb).
The code is validated in real time by a multi-stage pipeline: luacheck, selene, tree-sitter AST analysis, contract rules, cross-file dependency resolution, cyclomatic complexity analysis, crash and vulnerability pattern detection, lua54 integration testing with X-Ray engine stubs, gitleaks secret scanning.
Full report in doc/test-report.log.

License:
MIT License. See LICENSE file.

Versions:

1.2.0
  Added: xttltable.create_token_bucket -- per-key O(1) rate limiter with fractional accumulation
  Added: xsmart.get_smart_squads -- returns raw SIMBOARD squads table for a smart terrain
  Fixed: xobject.get_box_size crash when ammo section doesn't exist in system_ini

1.0.5
  xlevel pcall removal, nil guard improvements.
  Fixed: get_game_hours unnecessary pcall, replaced with level.present() guard
  Fixed: get_current_level_name missing level.present() guard

1.0.4
  Renamed: is_protected -> is_unscriptable (xcreature), is_protected_squad -> is_unscriptable_squad (xsquad)
  Renamed: xdata.protected_npcs -> xdata.unscriptable_npcs
  Renamed: exclude_protected -> exclude_unscriptable in find_squad/find_squads/find_online_squads opts
  Added: xcreature.is_task_giver(obj) - checks NPC ID and squad ID against active tasks
  Fixed: xsquad task giver check now covers dialog tasks (commander_id)

1.0.3
  Added: xlibs.version() API for dependency checking
  Added: MCM info page showing version and description

1.0.2
  Fixed: xstash.find_stashes excludes player-created stashes (inv_backpack)

1.0.1
  Fixed: xsquad.is_protected_squad excludes companion squads

1.0.0
  First release. 19 utility modules for Anomaly Lua modding.
  Added: xcreature - entity identification and fluent query iterator
  Added: xsquad - squad search, scripted control, release, iteration
  Added: xsmart - smart terrain queries, faction, capacity, arrival, conquest
  Added: xlevel - level/map, game time, location names
  Added: xobject - server object resolution, offline item creation
  Added: xstash - stash discovery, looting, filling
  Added: xdata - protected NPC/squad lookup tables
  Added: xtable - table utilities, locks, memoize
  Added: xttltable - TTL tables and sliding window counters
  Added: xmath - probability, sampling, weighted choice
  Added: xstring - string interpolation
  Added: xlog - buffered file logging
  Added: xprofiler - microsecond code profiling
  Added: xtrace - trace ID generation
  Added: xinspect - deep inspection
  Added: xbus - pub/sub event bus
  Added: xevent - runtime function hooking
  Added: xpda - PDA messages and map markers
  Added: xmcm - MCM config getters and loaders
