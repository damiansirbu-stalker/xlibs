# xlibs Architecture

Shared utility library for STALKER Anomaly Lua modding. Pure Lua, game globals only.

---

## Module Overview

```
+-------------------------------------------------------------------+
|                           xlibs                                    |
+-------------------------------------------------------------------+
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  |   xlog    |  |   xbus    |  | xcreature |  |  xsquad   |       |
|  | Logging   |  | Event Bus |  | Entity    |  | Squad Ops |       |
|  | File I/O  |  | Pub/Sub   |  | Identity  |  | & Query   |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  |  xlevel   |  |  xsmart   |  |  xstash   |  |  xobject  |       |
|  | Level/Map |  | SmartTrn  |  | Stash Ops |  | SrvEntity |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  |  xtable   |  | xttltable |  |  xslice   |  |   xmath   |       |
|  | Table Ops |  | TTL Table |  | TimeSlice |  | RNG       |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  |   xmcm    |  | xprofiler |  |  xtrace   |  | xinspect  |       |
|  | MCM Cfg   |  | Profiling |  | Trace IDs |  | Deep Dbg  |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  |  xevent   |  |   xpda    |  | xstring   |  |  xtime    |       |
|  | Fn Hooks  |  | PDA/Map   |  | Interp.   |  | Game Time |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  |  xconst   |  |  xdata    |  |  xlibs    |  | xlibs_mcm |       |
|  | Sentinels |  | Static    |  | Metadata  |  | MCM Page  |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+                                                    |
|  |  xactor   |                                                    |
|  | Actor Ops |                                                    |
|  +-----------+                                                    |
+-------------------------------------------------------------------+
```

---

## Modules

### Note: Derived Events via xevent Hooks

The derived event pattern uses `xevent.hook` to intercept game functions and emit synthetic callbacks. Currently used inline in `ap_cause_wounded.script` (hooks `xr_eat_medkit.consume_medkit` -> `x_npc_medkit_use` callback). The pattern is reusable: any game function that lacks a callback can be hooked the same way. See AlifePlus `architecture.md` Synthetic Callbacks section for details.

### xlog.script - Logging

Buffered file logging with session management.

```lua
local log = xlog.get_logger("MY.MODULE", { outfile = "mymod.log" })
log.info("Message: %s", value)
log.error("Error")  -includes stack trace
```

- `get_logger(name, cfg)` - Create logger (levels: DEBUG, INFO, WARN, ERROR)
- `flush_all()` - Force write all buffers
- `get_session_id()` - Current session identifier
- `get_levels()` - Available log level names
- `init(bindings)` - Initialize logging subsystem
- Logger methods: `write(lvl, fmt, ...)`, `set_level(lvl)`, `get_level()`, `get_level_name()`, `get_outfile()`, `check_rotation()`, `flush_buffer()`

### xbus.script - Event Bus

```lua
xbus.subscribe("event:name", function(data) ... end, "my_handler")
xbus.publish("event:name", { key = value })
```

- `subscribe(event, callback, name)` - Register handler (pcall-wrapped delivery)
- `unsubscribe(event, name)` - Remove handler by name
- `publish(event, data)` - Dispatch to all handlers
- `get_subscribers(event)` - List handlers for event
- `count(event)` - Number of subscribers
- `has(event, name)` - Check if handler registered
- `stats()` - { published, delivered, errors }
- `reset_stats()` - Zero counters
- `debug_state()` - Full diagnostic dump

### xcreature.script - Creature/Entity Identification

```lua
local is_mutant = xcreature.is_mutant(entity_id)
local community = xcreature.community(entity_id)
local name = xcreature.get_name(entity_id)
xcreature.query():stalkers():alive():on_level(lvl):each(fn)
```

- `get_clsid(input)` - Engine class ID from any input
- `entity_type(input)` - ENTITY.STALKER, ENTITY.MUTANT, or nil
- `is_stalker(input)`, `is_mutant(input)`, `is_npc(input)` - Entity type checks
- `community(input)` - Get faction community
- `get_name(input)` - Get translated name
- `pos(input)` - Get position
- `give_money(obj, amount)` - Give rubles to game_object
- `get_mutant_species(input)` - Base species string from any entity ("bloodsucker", "dog", etc.). Handles modded exe clsid reuse via section prefix fallback. player_id fast reject for stalkers (0 luabind).
- `get_mutant_variant(input)` - Full NPC variant section ("bloodsucker_red_strong", "dog_weak_brown")
- `is_unscriptable(obj)` - Check against xdata.unscriptable_npcs
- `is_task_giver(obj)` - Check NPC/squad ID against active tasks
- `query()` - Fluent server objects iterator:
  - Filters: `stalkers()`, `mutants()`, `npcs()`, `alive()`, `online()`, `on_level(lvl_id)`, `filter(fn)`
  - Terminals: `each(callback)`, `collect()`, `count()`, `first()`, `get_npc_counts()`

### xsquad.script - Squad Queries & Operations

```lua
local squads = xsquad.find_squads(pos, { factions = outlaws, max_distance = 500, level_id = lvl })
xsquad.control_squad(squad, smart, true)
xsquad.release_squad(squad)
```

- `find_squad(pos, opts)`, `find_squads(pos, opts)` - Find matching squads
- `control_squad(squad, smart, rush)` - Take scripted control, direct to smart terrain
- `target_actor(squad, rush)` - Script squad to chase actor (engine-native pursuit)
- `release_squad(squad)` - Release from scripted control, return to simulation
- `release_squads(opts)` - Bulk release with filter
- `is_squad_at_base(squad)`, `get_squad_smart(squad)`
- `get_squad_by_member(npc_id)` - Get squad containing NPC
- `get_community_name(squad)` - Translated community name (safe, never nil)
- `is_permanent_squad(squad)` - Static identity check (story, trader, named_npc, empty), cached
- `has_active_role(squad)` - Dynamic role check (task_giver, companion)
- `is_task_target(squad)` - Task target check (task_squads hash + bounty/hostage member fallback)
- `is_scripted(squad)` - Check engine/vanilla scripting fields (scripted_target, condlist, random_targets)
- `is_protected(squad, opts)` - Unified protection check. Runs guards in order: exclude_filter, is_scripted, is_permanent, has_active_role, is_task_target. Shares commander resolution across checks. Each guard toggled via opts flags.
- `reassert_target(squad, target)` - Restore scripted_target if cleared by another mod between scans
- `iter_squads()` - Iterator over all SIMBOARD squads
- `iter_member_ids(squad)` - Iterator yielding member entity IDs
- `dump_squads()` - Diagnostic string of all SIMBOARD squads

### xobject.script - Generic Object Helpers

```lua
local se = xobject.se(any_input)  -ID, game object, or server object
local npc = xobject.go(npc_id)  -online game object or nil
local item = xobject.create_item("wpn_ak74", npc_id)  -works offline
local item = xobject.create_item("ammo_5.45x39_ap", npc_id, { ammo = 60 })  -60 rounds
local box = xobject.get_box_size("ammo_5.45x39_ap")  -cached INI read
```

- `se(input)` - Get server object from any input type
- `go(id)` - Get online game object by ID (nil if offline)
- `get_box_size(sec)` - Get ammo box size from system INI (cached)
- `create_item(section, npc_id, t)` - Create item for any NPC (online/offline, falls back to smart terrain position for invalid lvid). Optional `t` forwarded to alife_create_item ({ammo, cond, uses})

### xlevel.script - Level/Map and Time

```lua
local level_id = xlevel.get_level_id(se_obj)
local name = xlevel.get_location_name(pos, level_id)
local valid = xlevel.is_valid_lvid(se_obj)
```

- `get_level_id(se_obj)` - Level ID from server entity (pcall-guarded)
- `get_actor_level_id()` - Actor's current level ID
- `get_game_hours()` - Total game hours since start
- `get_smart_display_name(smart)` - Translated smart terrain name
- `get_location_name(pos, lvl_id)` - Location name from nearest smart
- `is_valid_lvid(se_obj)` - Check level vertex validity (0xFFFFFFFF = invalid)

### xsmart.script - Smart Terrain Queries

```lua
local smart = xsmart.find_smart(pos, { factions = faction, level_id = level_id, filter = xsmart.is_base })
local arrived = xsmart.is_arrived(squad, smart)
local has_room = xsmart.has_capacity(smart, faction)
```

- `get_actor_smart()` - Nearest smart to actor (engine-maintained, O(1))
- `is_base(smart)`, `is_lair(smart)`, `is_resource(smart)`, `is_territory(smart)`
- `is_smart_important(smart)` - Base, resource, or territory
- `has_faction(smart, faction)`, `has_factions(smart, factions)` - Props-based faction check
- `accepts_mutant(smart, player_id)` - Engine target_precondition gate 1 (props.all OR props.all_monster OR props[player_id])
- `get_smart_factions(smart)` - All accepted factions from props (cached)
- `get_smart_faction(smart)` - Owning faction (warfare -> runtime -> props -> "none")
- `is_smart_empty(smart)` - No squads assigned
- `find_smart(pos, opts)` - Generic nearest smart search (level_id, factions, min/max distance, exclude_id, filter). Optimized filter order: cheapest checks first (exclude_id, factions hash, level_id) before the luabind distance_to_sqr call. Uses squared distance throughout (no sqrt). max_distance seeds the best_dist threshold so distant smarts are rejected after one comparison.
- `is_arrived(squad, smart)` - Delegates to engine's am_i_reached
- `get_proximity(squad, smart)` - Distance and arrival metadata
- `has_capacity(smart, faction, incoming)` - SIMBOARD squads + incoming vs max_population
- `set_shared_spawn(smart, key, faction, spawn_num)` - Additive spawn injection (adds entry alongside originals, no faction_controlled)
- `clear_shared_spawn(smart, key)` - Remove shared spawn entry, restore original-only spawning
- `set_exclusive_spawn(smart, key, faction, spawn_num)` - Exclusive spawn injection (sets faction_controlled, suppresses originals via faction gate)
- `clear_exclusive_spawn(smart, key)` - Remove exclusive spawn, revert faction_controlled and faction to defaults
- `get_smart_squads(smart_id)` - Raw SIMBOARD squads table for a smart terrain
- `smart_iter()` - Stateful iterator over all SIMBOARD smart terrains (for smart in xsmart.smart_iter())
- `dump_smarts(factions, level_id)` - Diagnostic dump
- `reset_spawns()`, `repopulate()` - Smart terrain population management

### xstash.script - Stash Operations

- `find_stashes(pos, opts)` - Find revealed stashes near position (opts: max_distance, min_distance, level_id, max_count)
- `is_stash_available(id)`, `is_stash_looted(id)`, `mark_stash_looted(id)`, `clear_stash(id)`
- `get_stash_items(stash_id)` - Read-only stash contents as parsed item list
- `loot_stash(id)` - Loot stash contents (marks looted, returns item list)
- `fill_stash(id, items, add_marker)` - Fill stash with items
- `filter_notable_stash_items(items, max)` - Filter to weapons/armor/artefacts

### xtable.script - Table Utilities

```lua
local filtered = xtable.filter(tbl, function(v) return v > 0 end)
local found = xtable.find(tbl, function(v) return v.id == target end)
```

- `is_array(x)` - Check if table is array-like
- `clone(t)` - Shallow copy
- `count(t, fn)` - Count elements (optional predicate)
- `filter(t, fn, opts)`, `find(t, fn)`, `reduce(t, fn, init)`
- `shuffle(t)`, `sort(t, comparator)`
- `bisect_left(arr, value, compare_fn)` - Binary search insertion point
- `binary_insert(arr, value, compare_fn)` - Sorted insert
- `merge(...)` - Set union of hash tables. Variadic, nil-safe, returns new table.
- `subtract(base, ...)` - Set difference. Returns base keys minus all keys in remaining args.
- `memoize(fn)` - Function memoization
- `acquire_lock(key, sec)` - Time-based lock (returns false if in window)
- `clear_locks()` - Wipe all locks

### xttltable.script - TTL Data Structures

All three time-based structures support clock injection via `opts.clock`. Default is `os.clock` (zero luabind, wall time). Pass `xtime.game_sec` for game-time expiry (2 luabind per clock call). Use wall time for internal rate limiting and dedup guards. Use game time for gameplay-facing timers (decay windows, cooldowns) so they respect time acceleration, sleep, and realism mods.

- `create_ttl_table(opts)` - TTL table with auto-expiry
  - `default_ttl` -> expiry seconds, `clock` -> clock function (default os.clock)
  - `:set(key, value, ttl)`, `:get(key)`, `:has(key)`, `:remove(key)`
  - `:remaining_ttl(key)` - calls clock function for accurate expiry
  - `:size()`, `:all()`, `:clear()`
  - `:export()`/`:import()` for save/load (imported entries get fresh TTL)
- `create_ttl_counter(opts)` - Sliding window counter
  - `clock` -> clock function (default os.clock)
  - `:add(key, metadata)`, `:count(key)`, `:reset(key)`, `:clear()`, `:all()`
- `create_token_bucket(opts)` - Per-key O(1) rate limiter with fractional accumulation
  - `capacity` -> max tokens, `rate` -> tokens per second, `clock` -> clock function (default os.clock)
  - `:acquire(key)` - Consume a token, returns true if available
  - `:peek(key)` - Check availability without consuming
  - `:reset(key)`, `:clear()`
- `create_fifo_cache(opts)` - Bounded FIFO cache with ring buffer eviction
  - `capacity` -> max entries (default 128)
  - `:get(key)`, `:set(key, value)`, `:has(key)` - O(1) hash lookup
  - `:clear()`, `:size()`
  - Oldest entry evicted when capacity reached. Use false for negative cache entries.

### xslice.script - Time-Sliced Iteration

Spread array processing across frames. Process `step` items per frame.
Items drained (`func` returns true) are removed between passes. Remaining items
are revisited in the next pass (circular). Queue finishes when all items drained,
`func` returns false (early stop), or queue is cancelled.

Pattern: time slicing -- amortize O(n) work across n/step frames.
One shared `AddUniqueCall` drives all active queues.

```lua
-- One-pass drain-all (process every item once)
xslice.start("campfire_scan", ids, {
    step = 100,
    func = function(id)
        scan_entity(id)
        return true  -- drain
    end,
    on_done = function() end,
})

-- Circular (items stay until ready)
xslice.start("spread_releases", entities, {
    step = 3,
    func = function(ent)
        if done_enough then return false end  -- early stop
        if not ready(ent) then return nil end  -- keep, revisit
        release(ent)
        return true                            -- drain
    end,
    on_done = function() end,
})
```

- `start(name, items, opts)` - Start queue. opts: `func(item, index)`, `on_done()`, `step`
  - func returns: `true` = drain, `false` = early stop, `nil` = keep for next pass
  - Returns false if name already active or invalid args
  - Items array referenced, not copied. Do not modify while active.
- `cancel(name, silent)` - Cancel queue. Fires `on_done` unless `silent` = true
- `is_active(name)` - Check if queue is running

Internals: deferred compaction. Survivors collected during each pass, become
next pass's input. O(step) per frame, O(n) per pass. No per-item `table.remove`.

### xmath.script - RNG

- `chance(percent)` - Probability roll
- `prd_chance(key, percent)` - Pseudo-random distribution (Dota 2 PRD algorithm, reduces streak variance)
- `sample(tbl)` - Random element
- `sample_n(tbl, n)` - N random elements
- `partial_shuffle(tbl, count)` - Shuffle first N elements
- `weighted_choice(weights)` - Weighted random selection
- `vary(value, percent)` - Random variation within percent

### xmcm.script - MCM Config

- `create_getter(mod_id, defaults, path_builder)` - Live config getter
- `create_loader(mod_id, defaults, path_builder)` - One-shot config loader
- `extract_defaults(op, recursive)` - Extract defaults from MCM options tree
- `format_defaults(mod_id, defaults)` - Debug-friendly defaults dump
- `format_config(mod_id, defaults, get_config)` - Debug-friendly config dump

### xprofiler.script - Code Profiling

Source: xray-monolith/src/xrServerEntities/script_engine_script.cpp:127-196

- `new()`, `new_if(condition)` -> `:start()`, `:stop()`, `:get_us()`, `:get_ms()`, `:reset()`
- `new_if(false)` returns NOOP singleton (zero overhead)
- `wrap(fn)` - Profile a function call, returns result + elapsed
- Uses `profile_timer` (CPU clock, microsecond resolution)

### xtrace.script - Tracing

- `new()`, `new_if(condition)` -> `.id`, `.path`
- `new_if(false)` returns NOOP singleton (zero overhead)
- `wrap(trace, op_name, fn)` - Execute fn within a trace segment
- `reset()` - Reset trace ID counter

### xinspect.script - Debug

- `inspect(value, opts)` - Deep table/userdata inspection
- `format_table(t, max_depth)` - Table to string
- `get_type(obj, cls)` - Engine type identification
- `userdata(obj)` - Userdata field enumeration

### xevent.script - Synthetic Callbacks

Runtime function hooking. Intercept any Lua function, emit callbacks from systems that don't have them.

```lua
xevent.hook("xr_eat_medkit", "consume_medkit", function(orig, npc, medkit, kind)
    xevent.emit("x_npc_medkit_use", npc, medkit, kind)
    return orig(npc, medkit, kind)
end)
```

- `hook(module, func, wrapper)` - Wrap function, returns success
- `unhook(module, func)` - Restore original
- `emit(name, ...)` - Emit synthetic callback (SendScriptCallback)
- `is_hooked(module, func)` - Check if hooked
- `list_hooks()` - Active hooks

**Naming convention:** `x_` prefix for synthetic events (e.g., `x_npc_medkit_use`)

**How it works:** Lua functions are table entries. We save the original, replace with wrapper that calls original + emits callback. Zero engine modification.

### xpda.script - PDA/Map

- `send(caption, msg, icon)` - PDA message
- `mark_squad(id, opts)`, `unmark_squad(id)`, `clear_squad_markers()`
- `mark_entity(id, opts)`, `unmark_entity(id, type)`, `hydrate_markers(markers)`

### xstring.script - Interpolation

- `interpolate(template, vars)` - `"Hello ${name}!"` -> `"Hello World!"`

### xtime.script - Game Time

- `game_sec()` - Game-seconds since epoch (cached start_time, invalidated on on_game_start)

### xconst.script - Engine Sentinel Constants

X-Ray engine sentinel values extracted from C++ source headers.

- `INVALID_ENTITY_ID` - u16 MAX (65535), from alife_space.h:39
- `INVALID_LEVEL_VERTEX_ID` - u32 MAX (4294967295)

### xdata.script - Static Data

Unscriptable NPC/squad tables used by `xcreature.is_unscriptable` and `xsquad.is_permanent_squad`.

- `unscriptable_npcs` - Lookup table of trader, mechanic, leader, medic, barmen, guide, and story character squad IDs that should not be moved or despawned by mods

### xactor.script - Actor Helpers

- `give_info(info_id)` - Give info portion to actor (nil-guarded)

### xlibs.script - Package Metadata

- `get_version()` - Version string (e.g. "1.2.3")
- `is_compatible(required)` - Semver check: same major, actual >= required

### xlibs_mcm.script - MCM Info Page

MCM registration for xlibs. Banner slide, description, dynamic version text via `ui_hook_functor`.

---

## Patterns

- **NOOP singleton**: `new_if(condition)` returns real or NOOP object. Zero overhead when disabled.
- **Lazy init**: first access creates resource, subsequent access returns cached.
- **TTL cleanup**: expired entries pruned on access or periodic sweep.
- **Local caching**: `local time_global = time_global` for hot paths.
- **Guard clauses**: early return on nil/invalid, max 2 nesting levels.
- **Time slicing**: spread O(n) iteration across frames via `AddUniqueCall` + step index. Deferred compaction between passes.

---

## Engine Protection Reality

X-Ray's `alife():release()` and `SIMBOARD:remove_squad()` have zero built-in protection. Any script call will unconditionally destroy the entity and all children (cascading through squad member cleanup, smart terrain unregistration, and server_entity_on_unregister). All NPC/squad protection is script-side -- the engine tracks story_id objects in a registry but does not block their removal.

This means every mod that releases or scripts squads must implement its own guard chain. xsquad provides four guards (`is_permanent_squad`, `has_active_role`, `is_task_target`, `is_scripted`) that match Anomaly's own protection layers in sim_offline_combat.

---

