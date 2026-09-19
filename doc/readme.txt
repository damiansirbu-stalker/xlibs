xlibs: Shared utility library for STALKER Anomaly modding, by Damian
Version: 1.9.0-snapshot
Changelog: https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/changelog
Russian / На русском: https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/readme_ru.txt

My work:
GitHub: https://github.com/orgs/damiansirbu-stalker/repositories
ModDB: https://www.moddb.com/members/damian-sirbu/addons
Nexus: https://www.nexusmods.com/profile/damiansirbu/mods

My contributions:
X-Ray Monolith: https://github.com/themrdemonized/xray-monolith

[ HERO IMAGE: readme_pic_1.jpg — xlibs module tree ]

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

How It's Built:

Although it started from work by Demonized, Alundaio, and Tronex, the current code and patterns are original, reverse-engineered from X-Ray and drawn from the best-known Lua and systems libraries.
The design is strict inversion of control. Every module is state plus handlers, and the caller wires them at its composition root.
It runs on proper data structures: a token-bucket rate limiter, a ring-buffer cache, a sliding-window counter, TTL maps, and a cooperative scheduler.
The raycasting and range math are hand-written and tested live, and the engine sentinels come straight from the X-Ray C++ headers.
The combat primitives run on my own engine changes: per-NPC aim, vision, fire, and selection gates written into xray-monolith and merged into the demonized exes.
It uses the engine and never reimplements it. A wrapper costs only the bridge call it wraps, and no code runs every frame.
It is the family's one rulebook. Every rule, policy, and check the mods share is implemented once, here, the same protection, distances, faction logic, and combat reads for all of them.
Profiled continuously with JitProfiler, an engine-native scientific tool. Manual tests run on unoptimized, single-threaded exes.
Every commit runs the full pipeline locally and in CI: luacheck, a Selene build compiled for STALKER with flags the public build lacks, and a load test that runs every script against engine stubs.
Rule layers then check crash safety, hotpath cost, engine correctness, complexity, architecture contracts, security, and the docs.
It sits directly on X-Ray and depends on no mod. Every other mod depends on it.

[Screenshot: xlibs under JitProfiler, a live CPU and allocation capture]
Project Health: https://damiansirbu-stalker.github.io/xlibs/

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
