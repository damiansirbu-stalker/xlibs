xlibs: Shared utility library for STALKER Anomaly modding, by Damian
Version: next
GitHub: https://github.com/damiansirbu-stalker/xlibs
Changelog: https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/changelog
Russian / На русском: https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/readme_ru.txt
Bugs, suggestions: https://github.com/damiansirbu-stalker/xlibs/issues

Alife Collection:
AlifeAmbience: https://github.com/damiansirbu-stalker/AlifeAmbience
AlifeBalance: https://www.moddb.com/mods/stalker-anomaly/addons/alifebalance
AlifeCompanions: https://github.com/damiansirbu-stalker/AlifeCompanions
AlifeDiegetic: https://www.moddb.com/mods/stalker-anomaly/addons/diegetic-audio-control-100
AlifeGuard: https://www.moddb.com/mods/stalker-anomaly/addons/alifeguard-1001
AlifePlus: https://www.moddb.com/mods/stalker-anomaly/addons/alifeplus-v1-0-01
AlifeSpooks: https://github.com/damiansirbu-stalker/AlifeSpooks
AlifeTactics: https://www.moddb.com/mods/stalker-anomaly/addons/alifetactics
FurnitureFuel: https://github.com/damiansirbu-stalker/FurnitureFuel
JitProfiler: https://github.com/damiansirbu-stalker/JitProfiler
TestZone: https://github.com/damiansirbu-stalker/TestZone
xlibs: https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001

xlibs is a modder's toolbox for what Anomaly mods typically need.
It covers entity queries, squad operations, smart terrain logic, stash manipulation, logging, profiling, event systems, and data structures.

The API design comes from reverse engineering the X-Ray engine and Anomaly internals, cross-referenced with patterns from the best modders in both the European and Russian STALKER modding traditions.
Every function wraps engine quirks, guards against nil, and handles edge cases that would otherwise require each mod to solve independently.

Pure Lua where possible. No engine dependency unless necessary. No central loader. The engine auto-loads scripts on first access. Call xlog.get_logger() or xsquad.find_squads() and it works.

Features:

Entity and World:
  xactor       Actor info portions, talking/trading partner, state queries
  xcreature    Creature identification, type checks, translated names, money primitives
  xsquad       Squad search, scripted control, release, chase, iteration
  xsmart       Smart terrain queries, faction detection, capacity, arrival, conquest, job allocation
  xlevel       Level/map queries, game time, location names, vertex validation
  xobject      Server object and online game object resolution from any input type
  xinventory   Item categorization (per-NPC ammo tier), slot semantics, ammo config, item lifecycle, LTX policy primitives (load + classify + iterate surplus)
  xstash       Stash discovery, looting, filling, item filtering
  xdata        Unscriptable NPC/squad tables (traders, mechanics, story characters)
  xconst       X-Ray engine sentinel constants (invalid entity ID, invalid level vertex ID)

Data Structures:
  xtable       Count, shuffle, sort, set merge and subtract
  xttltable    TTL key-value store, sliding window counter, token bucket, FIFO cache
  xmath        Random sampling and partial shuffle
  xslice       Time-sliced array iteration across frames
  xstring      String interpolation with {key} placeholders
  xtime        Game-time seconds accumulator (wraps engine game_time())

Diagnostics:
  xlog         Buffered file logging with session management and rotation
  xprofiler    Microsecond code profiling via engine profile_timer
  xtrace       Trace ID generation for request tracking across modules
  xinspect     Deep table and userdata inspection, engine type identification

Effects:
  xpp          Post-process effector wrap (slot allocator, engine-smoothed factor, handle API, 35 verified-safe .ppe paths)
  xsound       Sound wrap (single sounds, looping handles, volume lerp) plus the engine ambient-bed and level-music trace/veto seams (engine PR #644)

Integration:
  xbus         Pub/sub event bus (direct delivery, errors stay visible)
  xevent       Runtime function hooking for synthetic callbacks
  xpda         PDA messages and map markers (squad and entity)
  xmcm         MCM config bundle (pre-seeded table, getter, loader)
  xchange      Liquibase-style save data migration registry, each changeset runs once per save

Requirements:
Anomaly 1.5.3
Modded exes: themrdemonized 20250908 or newer, or AOEngine v0.55 or newer. The full feature set needs the latest demonized build. A feature that needs a newer one stays inactive on older exes.

Install (MO2):
1. Install xlibs
2. Load order does not matter (scripts are auto-loaded by the engine on demand)

Uninstall (MO2):
Disable or remove in MO2. Any mod depending on xlibs will stop working.

Configuration:
No configuration needed. xlibs is a passive library loaded on demand by other mods.

Compatibility:
Coexists with everything. A pure library with no gameplay of its own; only xlog is active at game start (save and level-change flush callbacks plus a periodic flush timer), and everything else stays dormant until a mod calls it.

Performance and Infrastructure:
Performance comes first, ahead of any feature.
A wrapper costs only the bridge call it wraps and adds no work of its own.
When something cannot fit the budget it is reworked or moved into an X-Ray engine modification, never left to slow the game.
The frame budget is fixed.
Built from the X-Ray engine source by reverse engineering, with targeted engine changes of my own for performance, precision, and accuracy.
Heavy work spreads across frames, paced by rate limiters and staggered, deferred queues, with the math to keep cost bounded at any entity count.
A layered validator runs on every change, locally and in CI, and blocks the build on any crash, unsafe engine call, performance regression, style break, failed smoke load, or leaked secret.
Profiled with JitProfiler, an engine-native, scientific profiler.
Timings are worst-case, from a build with no multithreading or optimizations, so yours runs faster.
Project Health: https://damiansirbu-stalker.github.io/xlibs/
[JitProfiler: xlibs under CPU and allocation capture]

Credits:
Altogolik: support, ideas, source materials

Usage and License:
  Calling xlibs functions from your mod: intended use, no restrictions.
  Modpacks: allowed and encouraged. Keep the readme and license files.
  Addons, patches, integrations: allowed. Credit "xlibs by Damian Sirbu" visibly on your mod page.
  Reproducing the implementation in other software: not allowed, even with credit.
  Full license in LICENSE file and on GitHub.

Diagnostics and reporting:
Report at https://github.com/damiansirbu-stalker/xlibs/issues/new/choose or the EFP, Anomaly, and Zona Discord. Include repro steps, engine build, modlist, load order, and xray.log.
