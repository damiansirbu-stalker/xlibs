Version: 1.9.1-snapshot
Changelog: https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/changelog
Health: https://damiansirbu-stalker.github.io/xlibs/health/
JitProfiler: https://damiansirbu-stalker.github.io/xlibs/jitprofiler/
Bugs: https://github.com/damiansirbu-stalker/xlibs/issues
Russian / На русском: https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/readme_ru.txt

---
Alife mods:
  AlifePlus: https://www.moddb.com/mods/stalker-anomaly/addons/alifeplus-v1-0-01
  AlifeTactics: https://www.moddb.com/mods/stalker-anomaly/addons/alifetactics
  AlifeBalance: https://www.moddb.com/mods/stalker-anomaly/addons/alifebalance
  AlifeGuard: https://www.moddb.com/mods/stalker-anomaly/addons/alifeguard-1001
Diegetic mods:
  DiegeticControl: https://www.moddb.com/mods/stalker-anomaly/addons/diegeticcontrol
  DiegeticAmbience
  DiegeticDread
Tools:
  JitProfiler: https://www.moddb.com/mods/stalker-anomaly/addons/jitprofiler
Libraries:
  xlibs: https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001
Engines:
  X-Ray Monolith: https://github.com/themrdemonized/xray-monolith/pulls?q=is%3Apr+author%3Adamiansirbu+is%3Amerged
  OpenXRay: https://github.com/OpenXRay/xray-16/pulls?q=is%3Apr+author%3Adamiansirbu+is%3Amerged
Integrations:
  Word of Mouth: https://github.com/joshcoppola/word_of_mouth
  Warfare (erepb): https://www.moddb.com/mods/stalker-anomaly/addons/warfare-alife-overhaul-new
  Stealth Overhaul: https://github.com/Alex-leon1594/Stealth_Overhaul_Reworked
  COMPASS: https://github.com/Crimento/COMPASS
Collaborations:
  xAGNA: https://www.moddb.com/mods/stalker-anomaly/addons/xagna

[ Hero image: xlibs-hero.gif - a mod runs with and without xlibs ]

Thanks for the support, but I don't need donations. Reviews, ratings, and proper reports help more.
An organized group copies my work, spreads daily lies, and mass-downvotes my mods across platforms.
My work is open source, present in most modpacks, and integrates with established projects.

xlibs is a modder's toolbox for what Anomaly mods typically need.
It covers entity queries, squad operations, smart terrain logic, stash manipulation, logging, profiling, event systems, and data structures.

The API design comes from reverse engineering the X-Ray engine and Anomaly internals, cross-referenced with patterns from the best modders in the European and Russian STALKER traditions.
Every function wraps engine quirks, guards against nil, and handles edge cases that would otherwise require each mod to solve independently.

Pure Lua where possible. No engine dependency unless necessary. No central loader. The engine auto-loads scripts on first access. Call xlog.get_logger() or xsquad.find_squads() and it works.

Features:

A-Life:
  xsquad       Squad search, scripted control, release, chase, iteration
  xsmart       Smart terrain queries, faction detection, capacity, arrival, conquest, job allocation
  xstash       Stash discovery, looting, filling, item filtering
  xlevel       Level/map queries, game time, location names, vertex validation
  xdata        Unscriptable NPC/squad tables (traders, mechanics, story characters)

Combat:
  xcombat      Combat-AI primitives: GOAP takeover, per-NPC aim/vision/fire engine binds, cover and LOS reads

Entity & Items:
  xcreature    Creature identification, type checks, translated names, money primitives
  xobject      Server object and online game object resolution from any input type
  xactor       Actor info portions, talking/trading partner, state queries
  xinventory   Item categorization (per-NPC ammo tier), slot semantics, ammo config, item lifecycle, LTX policy primitives (load + classify + iterate surplus)

Util:
  xtable       Count, shuffle, sort, set merge and subtract
  xttltable    TTL key-value store, sliding window counter, token bucket, FIFO cache
  xmath        Random sampling and partial shuffle
  xslice       Time-sliced array iteration across frames
  xstring      String interpolation with {key} placeholders
  xtime        Game-time seconds accumulator (wraps engine game_time())
  xconst       X-Ray engine sentinel constants (invalid entity ID, invalid level vertex ID)

Diagnostics:
  xlog         Buffered file logging with session management and rotation
  xprofiler    Microsecond code profiling via engine profile_timer
  xtrace       Trace ID generation for request tracking across modules
  xinspect     Deep table and userdata inspection, engine type identification

Effects:
  xpp          Post-process effector wrap (slot allocator, engine-smoothed factor, handle API, 35 verified-safe .ppe paths)
  xsound       Sound wrap (single sounds, looping handles, volume lerp) plus the engine ambient-bed and level-music trace/veto seams (engine PR #644)

Framework:
  xbus         Pub/sub event bus (direct delivery, errors stay visible)
  xevent       Runtime function hooking for synthetic callbacks
  xpda         PDA messages and map markers (squad and entity)
  xmcm         MCM config bundle (pre-seeded table, getter, loader)
  xchange      Liquibase-style save data migration registry, each changeset runs once per save

Requirements:
Anomaly 1.5.3
Modded exes: themrdemonized 20250908 or newer, or AOEngine v0.55 or newer. The full feature set needs the latest demonized build. A feature that needs a newer one stays inactive on older exes.

Configuration:
No configuration needed. xlibs is a passive library loaded on demand by other mods.

Compatibility:
Coexists with everything. A pure library with no gameplay of its own.
Only xlog is active at game start (save and level-change flush callbacks plus a periodic flush timer), and everything else stays dormant until a mod calls it.

How It's Built:

Although it started from work by Demonized, Alundaio, and Tronex, the current code and patterns are original, reverse-engineered from X-Ray and drawn from the best Lua and systems libraries.
The design is strict inversion of control. Every module is state plus handlers, and the caller wires them at its composition root.
It runs on proper data structures: a token-bucket rate limiter, a ring-buffer cache, a sliding-window counter, TTL maps, and a cooperative scheduler.
The raycasting and range math are hand-written and tested live, and the engine sentinels come straight from the X-Ray C++ headers.
The combat primitives run on my own engine changes: per-NPC aim, vision, fire, and selection gates written into xray-monolith and merged into the demonized exes.
It uses the engine and never reimplements it. A wrapper costs only the bridge call it wraps, and no code runs every frame.
It is the family's one rulebook. Every rule, policy, and check the mods share is implemented once, here, the same protection, distances, faction logic, and combat reads for all of them.
Profiled continuously with JitProfiler, an engine-native profiler. Manual tests run on unoptimized, single-threaded exes.
Every commit runs the full pipeline locally and in CI: luacheck, a Selene build built for STALKER with flags the public build lacks, and a load test running every script against engine stubs.
Rule layers then check crash safety, hotpath cost, engine correctness, complexity, architecture contracts, security, and the docs.
It sits directly on X-Ray and depends on no mod. Every other mod depends on it.

That pipeline runs on every commit and publishes what it finds. The header links a live health page and a JitProfiler capture of the mod's real CPU and allocation cost.

Credits:
Altogolik provided support, ideas, and source materials.

Usage and License:
  Calling xlibs functions from your mod: intended use, no restrictions.
  Modpacks: allowed and encouraged. Keep the readme and license files.
  Addons, patches, integrations: allowed. Credit "xlibs by Damian Sirbu" visibly on your mod page.
  Reproducing the implementation in other software: not allowed, even with credit.
  The full license is in the LICENSE file and on GitHub.

Diagnostics and reporting:
Every release goes through careful engineering and testing, but bugs can still slip through.
To report one, reproduce with debug logging on, and the world log where the mod has one.
First rule this mod out: reproduce with it off, then on. The cleanest test is this mod alone on vanilla and xlibs.
Send the traces on the Anomaly Discord, or file a defect on GitHub with the same information.
Attach xray.log, the mod log, the engine build, the modlist, and the load order.
For deep technical details and mechanisms, check the architecture docs on GitHub.

Tags: engine-native, performance, save-safe, reverse-engineering, lua-library, systems-library, xray-bridge, modder-api, data-structures, design-patterns, best-practices, logging, profiling, tracing, event-bus
