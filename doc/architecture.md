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
|  |  xtable   |  | xttltable |  |   xmath   |  |   xmcm    |       |
|  | Table Ops |  | TTL Table |  | RNG       |  | MCM Cfg   |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  | xprofiler |  |  xtrace   |  | xinspect  |  |  xevent   |       |
|  | Profiling |  | Trace IDs |  | Deep Dbg  |  | Fn Hooks  |       |
|  +-----------+  +-----------+  +-----------+  +-----------+       |
|  +-----------+  +-----------+  +-----------+                       |
|  |   xpda    |  | xstring   |  |  xdata    |                       |
|  | PDA/Map   |  | Interp.   |  | Static    |                       |
|  +-----------+  +-----------+  +-----------+                       |
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
log.error("Error")  -- includes stack trace
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
- `is_protected(obj)` - Check against xdata.protected_npcs
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
- `find_online_squads(pos, opts)` - Find squads with online (near-player) members
- `control_squad(squad, smart, rush)` - Take scripted control, direct to smart terrain
- `target_actor(squad, rush)` - Script squad to chase actor (engine-native pursuit)
- `release_squad(squad)` - Release from scripted control, return to simulation
- `release_squads(opts)` - Bulk release with filter
- `is_squad_at_base(squad)`, `get_squad_smart(squad)`
- `get_squad_by_member(npc_id)` - Get squad containing NPC
- `get_community_name(squad)` - Translated community name (safe, never nil)
- `is_protected_squad(squad)` - Check against xdata.protected_npcs
- `iter_squads()` - Iterator over all SIMBOARD squads
- `iter_member_ids(squad)` - Iterator yielding member entity IDs
- `dump_squads()` - Diagnostic string of all SIMBOARD squads

### xobject.script - Generic Object Helpers

```lua
local se = xobject.se(any_input)  -- ID, game object, or server object
local npc = xobject.go(npc_id)  -- online game object or nil
local item = xobject.create_item("wpn_ak74", npc_id)  -- works offline
local item = xobject.create_item("ammo_5.45x39_ap", npc_id, { ammo = 60 })  -- 60 rounds
local box = xobject.get_box_size("ammo_5.45x39_ap")  -- cached INI read
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
- `get_smart_factions(smart)` - All accepted factions from props (cached)
- `get_smart_faction(smart)` - Owning faction (warfare -> runtime -> props -> "none")
- `is_smart_empty(smart)` - No squads assigned
- `find_smart(pos, opts)` - Generic nearest smart search (level_id, factions, min/max distance, exclude_id, filter)
- `is_arrived(squad, smart)` - Delegates to engine's am_i_reached
- `get_proximity(squad, smart)` - Distance and arrival metadata
- `has_capacity(smart, faction)` - SIMBOARD squads vs max_population
- `set_faction_controlled(smart, faction, spawn_num)` - Runtime respawn mutation (mirrors engine read_params)
- `clear_faction_controlled(smart)` - Revert to default faction
- `dump_smarts(factions, level_id)` - Diagnostic dump
- `reset_spawns()`, `repopulate()` - Smart terrain population management

### xstash.script - Stash Operations

- `find_stashes(pos, opts)` - Find revealed stashes near position (opts: max_distance, min_distance, level_id, max_count)
- `is_stash_available(id)`, `is_stash_looted(id)`, `mark_stash_looted(id)`, `clear_stash(id)`
- `loot_stash_to_npc(id, npc_id)` - Transfer stash contents to NPC
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
- `memoize(fn)` - Function memoization
- `acquire_lock(key, sec)` - Time-based lock (returns false if in window)
- `clear_locks()` - Wipe all locks

### xttltable.script - TTL Data Structures

- `create_ttl_table(opts)` - TTL table with auto-expiry
  - `default_ttl` -> expiry seconds
  - `:set(key, value, ttl)`, `:get(key)`, `:has(key)`, `:remove(key)`
  - `:remaining_ttl(key)` - os.clock for accurate expiry
  - `:size()`, `:all()`, `:clear()`
  - `:export()`/`:import()` for save/load (imported entries get fresh TTL)
- `create_ttl_counter(opts)` - Sliding window counter
  - `:add(key, metadata)`, `:count(key)`, `:reset(key)`, `:clear()`, `:all()`

### xmath.script - RNG

- `chance(percent)` - Probability roll
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

### xdata.script - Static Data

Protected NPC/squad tables used by `xcreature.is_protected` and `xsquad.is_protected_squad`.

- `protected_npcs` - Lookup table of trader, mechanic, leader, medic, barmen, guide, and story character squad IDs that should not be moved or despawned by mods

---

## Patterns

- **NOOP singleton**: `new_if(condition)` returns real or NOOP object. Zero overhead when disabled.
- **Lazy init**: first access creates resource, subsequent access returns cached.
- **TTL cleanup**: expired entries pruned on access or periodic sweep.
- **Local caching**: `local time_global = time_global` for hot paths.
- **Guard clauses**: early return on nil/invalid, max 2 nesting levels.

---

**Version:** 1.0.0
