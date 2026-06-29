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
|  +-----------+                                                     |
|  | xinventory|                                                     |
|  | Item Cats |                                                     |
|  +-----------+                                                     |
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
|  +-----------+                                                     |
|  |  xactor   |                                                     |
|  | Actor Ops |                                                     |
|  +-----------+                                                     |
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
- `is_trader(input)` - True if entity is Script_Trader CSE (clsid 37 / 36); catches Sidorovich-class
- `community(input)` - Get faction community
- `get_name(input)` - Get translated name
- `pos(input)` - Get position
- `give_money(obj, amount)` - Give rubles to game_object
- `get_mutant_species(input)` - Base species string from any entity ("bloodsucker", "dog", etc.). Handles modded exe clsid reuse via section prefix fallback. player_id fast reject for stalkers (0 luabind).
- `get_mutant_variant(input)` - Full NPC variant section ("bloodsucker_red_strong", "dog_weak_brown")
- `is_unscriptable(obj)` - Check against xdata.unscriptable_npcs
- `is_task_giver(obj)` - Check NPC/squad ID against active tasks
- `query()` - Fluent server objects iterator:
  - Filters: `stalkers()`, `traders()`, `mutants()`, `npcs()`, `alive()`, `online()`, `on_level(lvl_id)`, `filter(fn)`
  - Terminals: `each(callback)`, `collect()`, `count()`, `first()`, `get_npc_counts()`

### xsquad.script - Squad Queries & Operations

```lua
local squads = xsquad.find_squads(pos, { factions = outlaws, max_distance = 500, level_id = lvl })
xsquad.acquire_squad(squad, smart, true)
xsquad.release_squad(squad)
```

- `find_squad(pos, opts)`, `find_squads(pos, opts)` - Find matching squads
- `acquire_squad(squad, smart, rush)` - Take scripted control, direct to smart terrain
- `target_actor(squad, rush)` - Script squad to chase actor (engine-native pursuit)
- `release_squad(squad)` - Release from scripted control, return to simulation
- `release_squads(opts)` - Bulk release with filter
- `get_squad_smart(squad)`
- `is_stationed(squad, smart_id)` - True when engine considers squad stationed (current_action=1). With smart_id, also requires current_target_id match. Sticky until idle_time expires or new target assigned
- `get_squad_by_member(npc_id)` - Get squad containing NPC
- `get_community(squad)` - Raw community id (faction key), untranslated. Use for keying/comparison; the translated `get_community_name` is display-only
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

### xobject.script - Generic Object Lookup

```lua
local se = xobject.se(any_input)  -ID, game object, or server object
local npc = xobject.go(npc_id)  -online game object or nil
```

- `se(input)` - Get server object from any input type
- `go(id)` - Get online game object by ID (nil if offline)

Inventory-domain methods (item categorization, slot accessors, item lifecycle, ammo config) live in `xinventory.script` per the boundary: xobject = generic game_object lookup; xinventory = item-as-inventory-thing semantics.

### xinventory.script - Item Categorization, Slot Semantics, Item Lifecycle, Policy Primitives

```lua
local opts = xinventory.get_category_opts(npc)  -- canonical opts for get_category
local category = xinventory.get_category(item, opts)  -- "medkit" | "ammo_slot_3_t1" | ...
if xinventory.is_in_category(sec, "medkit") then ... end
local sections = xinventory.get_category_sections("medkit")  -- inverse: category -> list
xinventory.transfer_item(seller, item, buyer)
xinventory.release_item(item)

-- Policy primitives: load LTX policy, count NPC inventory, surface surplus
local policy = xinventory.load_policy("alifeplus\\ap_trade_policy.ltx",
    { "ap_trade_policy_rookie", "ap_trade_policy_veteran" }, { profit_max = true })
local counts = xinventory.classify(npc, opts)  -- { [category] = count } (ammo in rounds)
xinventory.iterate_surplus(npc, opts, {
    rules = policy.ap_trade_policy_rookie.rules,
    counts = counts,
    on_surplus = function(item, cat, sec, unit) xinventory.release_item(item) end,
})
local surplus = xinventory.build_surplus_map(counts, rules)  -- pure { [cat] = surplus }
```

Centralizes every engine inventory helper (`IsItem`, `IsWeapon`, `IsOutfit`, `IsHeadgear`, `IsArtefact`, `item:section/condition/ammo_get_count`, `npc:item_in_slot`, slot walks, `parse_list ammo_class`, `alife_create_item`, `alife_release_id`, `transfer_item`). Mods never call those directly — every call goes through xinventory.

**Category model** (symmetric forward + reverse maps):
- `get_section_category(sec)` - sec → category name. Untouchables (quest / anim / blacklisted) gate first. Medical 5 (medkit / bandage / antirad / stim / pill) and hand grenades via hand-maintained sets (vanilla bundles all medicals under `kind=i_medical` with no field to subdivide; no `_ITM["grenade"]` bucket exists). Other categories via Parse_ITM `_ITM[bucket]` lookups (outfit / helmet / artefact / device / money / grenade_ammo / ammo). Weapons by class prefix `WP_` (per-item fallback; vanilla `IsWeapon` is clsid-only).
- `get_category(item, opts)` - item → category name, adds per-NPC overrides on top of `get_section_category`: three runtime per-item untouchable checks (story_id via `get_object_story_id(item:id())`, companion-gifted via `axr_companions.is_assigned_item(opts.npc_id, item:id())`, player-strapped via `se_load_var(item:id(), "", "strapped_item")`); equipped check via `opts.equipped_ids`; ammo tier resolution to `ammo_slot_2_tN` / `ammo_slot_3_tN` per equipped pistol / rifle tier_map.
- `resolve_ammo_category(sec, opts)` - section-string variant of the ammo tier branch (no game_object required). Used by stash loot to match each stash ammo section against per-member opts.
- `is_in_category(sec, category)` - section-based predicate (wraps `get_section_category`). Per-NPC categories (`equipped`, `ammo_slot_*`) always return false on this path. Use `get_category` for those.
- `get_rank_chance(section, actor, ruleset)` - rank-gate kernel: a 0..1 appearance chance for a section under `ruleset = { floor = {[section|category]=tier}, bands = {[tier]={{cost,chance}, ... asc}, default} }`. Resolves the section category (`get_section_category`) + cost (`get_cost`) and the actor's rank tier (`ranks.get_obj_rank_name` / `character_rank`); a floor tier returns 0 below its rank, otherwise the first cost band whose `cost >= ` the section cost wins. Unconfigured opens (chance 1). Caller rolls and fail-closes. Used by the AlifePlus faction market.
- `get_category_sections(category)` - reverse: category → list of sections. Builders dispatch to hand-maintained sets (medical 5 + grenade), `_ITM[bucket]` reads (Parse_ITM-derived: outfit / helmet / artefact / device / money / grenade_ammo / ammo), kind filter over `_ITM["eatable"]` (food / drink), or `_ITM` unions (crafting = tool + part + upgrade). No `ini_sys:section_for_each` walks. Per-NPC categories (equipped, ammo_slot_*) and `weapon` return empty (weapon is per-item only). Lazy per-category build, weak-keyed cache, addon-aware for Parse_ITM bucket sources; grenade addons need an entry in `_grenade_set`.

**Item accessors** (`game_object` item in):
- `get_section(item)`, `get_condition(item)`, `get_cost(sec)` (vanilla `cost` field; nil if missing)

**NPC slot accessors**:
- `get_equipped_knife(npc)`, `get_equipped_pistol(npc)`, `get_equipped_rifle(npc)`, `get_equipped_grenade(npc)`, `get_equipped_outfit(npc)`, `get_equipped_helmet(npc)`
- `get_equipped_ids(npc)` - set of item ids across slots 1..LAST_MAIN_SLOT
- `get_category_opts(npc)` - canonical opts builder: `{ equipped_ids, equipped_pistol_sec, equipped_rifle_sec, npc_id }` for `get_category`

**Weapon config**:
- `get_ammo_sections(weapon_sec)` - ordered array of ammo sections accepted (cached)
- `get_ammo_tier_map(weapon_sec, n_tiers)` - map `{[ammo_sec]=tier_idx}`; sorts ammo_class by k_ap asc (cost tiebreaker, cost-only fallback if all k_ap=0), splits into N tiers via median. Cached per `(weapon_sec, n_tiers)`. Default `n_tiers=2`.
- `get_box_size(sec)` - rounds per ammo stack (cached)

**Category set returned by `get_category`** (24 values):
- Sentinels: `untouchable` (quest / anim / blacklisted, always filtered), `equipped` (per-NPC via `opts.equipped_ids`), `other` (fallthrough)
- Consumables: `medkit`, `bandage`, `antirad`, `stim`, `pill`, `food`, `drink`
- Throwables: `grenade`, `grenade_ammo`
- Ammo: `ammo_slot_2_t1` / `ammo_slot_2_t2` (pistol tiers), `ammo_slot_3_t1` / `ammo_slot_3_t2` (rifle tiers), `ammo_not_equipped` (ammo not in any equipped weapon's ammo_class)
- Gear: `weapon`, `outfit`, `helmet`, `artefact`, `device`, `money`, `crafting`
- Consumer dispatch should short-circuit on `untouchable` and `equipped` before any policy lookup.

**Item lifecycle**:
- `iterate_inventory(npc_id, callback)` - online-only inventory walk (engine has no offline iteration API). Replaces `xobject.iterate_online_inventory`.
- `create_item(section, npc_id, t)` - spawn item on any NPC (online / offline / cross-map via smart-terrain fallback). A requested `cond` is applied even on non-degradable sections (weapons, outfits) that itms_manager skips. Replaces `xobject.create_item`.
- `transfer_item(from_npc, item, to_npc)` - move item between owners.
- `release_item(item)` - alife_release_id wrapper (needs a live game_object).
- `release_item_id(id)` - id-based release sibling; works offline (no game_object required).

**Policy primitives** (uniform shape across consumers):
- `load_policy(path, sections, specials_set)` - generic LTX loader. Each named section yields `{ entries, rules, specials }`. `entries` preserves declaration order (BUY / LOOT priority); `rules` is the hash for O(1) per-category lookup; `specials` carries numeric keys named in `specials_set` (`profit_max`, `extras_max`, `fill_max`, ...).
- `classify(npc, opts)` - per-category count walker for an online NPC. Single iterate; ammo categories in rounds, others in items. Skips untouchable + equipped via `get_category`.
- `iterate_surplus(npc, opts, ctx)` - surfaces items while `count > max`. `ctx = { rules, counts, on_surplus }`. Mutates `ctx.counts` in place; `ctx.on_surplus(item, cat, sec, unit)` owns the action (transfer, release, queue). Returning `true` from `on_surplus` stops iteration. Lands at exactly `max` for `unit = 1` categories; for ammo (`unit > 1`) the final count can fall short of `max` by up to `unit - 1` rounds when the boundary stack overshoots.
- `build_surplus_map(counts, rules)` - pure. Derives `{ [category] = surplus_count }` from counts vs rules.

LTX policy file shape (consumer mods own the values):

```ini
[<section>]
<category_1> = <min>, <max>   ; two-value row: { min = M, max = N }
<category_2> = <max>          ; single-value row: { min = 0, max = N }  (cull-only consumers)
...
<special_key> = <number>      ; pulled out per specials_set
```

Consumers: `AlifePlus/configs/alifeplus/ap_trade_policy.ltx` (per-rank, two-value rows, `profit_max`), `AlifePlus/configs/alifeplus/ap_stash_policy.ltx` (uniform, two-value rows, `extras_max` + `fill_max`), `AlifeBalance/configs/alifebalance/ab_inventory_policy.ltx` (uniform, single-value rows, no specials).

**Slot constants** (from `xrServerEntities/inventory_space.h`):
- `SLOT_KNIFE=1`, `SLOT_PISTOL=2`, `SLOT_RIFLE=3`, `SLOT_GRENADE=4`, `SLOT_OUTFIT=7`, `SLOT_HELMET=12`, `BACKPACK_SLOT=13`, `LAST_MAIN_SLOT=14`. `m_slots` is sized from `system.ltx [inventory] slot_persistent_<N>` count at `Inventory.cpp:72-86`, not from the `MORE_INVENTORY_SLOTS` enum. Vanilla + GAMMA both ship 14 slots; raising `LAST_MAIN_SLOT` without a matching system.ltx is OOB UB on `ItemFromSlot` (`Inventory.cpp:658`, unbounded).

**Boundary rule**: xobject = generic game_object lookup; xinventory = anything that takes an item or talks about an NPC's items.

**Policy values live in consumer mods**; xinventory owns the shape (`load_policy`), the predicates (`get_category` family), and the walker primitives (`classify`, `iterate_surplus`, `build_surplus_map`). Each consumer ships its own LTX file with its own numbers; the mechanics are shared.

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
local hostile = xsmart.has_enemy_squad(smart.id, "stalker")
```

Smart property predicates (read from smart.props / engine fields):
- `is_base(smart)`, `is_lair(smart)`, `is_resource(smart)`, `is_territory(smart)`
- `has_surge_shelter(smart)` - Smart has surge shelter (emission-safe indoor)
- `has_campfire(smart)` - Has at least one online campfire (online scope)
- `has_anomaly(smart)` - Within 50m of any online anomaly zone
- `has_animated_stalker_jobs(smart)` - Has any non-stub stalker job (excludes generic_point / campfire_point stubs)
- `accepts_faction(smart, faction)` - Engine target_precondition Tier 1: props.all OR props.all_stalker/all_monster OR props[faction]
- `get_declared_factions(smart)` - Set of stalker factions explicitly listed in props (cached)

Smart finders:
- `get_actor_smart()` - Nearest smart to actor (engine-maintained, O(1))
- `find_smart(pos, opts)` - Generic nearest smart search (level_id, factions, min/max distance, exclude_id, filter, source). `factions` takes string or set
- `find_first_smart(opts)` - Distance-free variant; first matching smart in pool iteration order
- `find_smarts_spawning(level_id, faction)` - Array of smarts on level whose recipes produce given faction
- `smart_iter()` - Stateful iterator over all SIMBOARD smart terrains

SIMBOARD roster (sim-intent membership, NOT physical occupancy):
- `get_smart_squads(smart_id)` - Raw SIMBOARD.smarts[id].squads hash
- `assign_squad_to_smart(squad, smart_id)` - Wrap SIMBOARD:assign_squad_to_smart; nil to detach
- `iter_stationed_squads(smart_id, exclude_id, cap)` - Closure iterator yielding se for squads with current_action=1 AND current_target_id=smart_id (xsquad.is_stationed). Skips in-transit. Cap default 5
- `has_squad_of_faction(smart_id, faction, exclude_id)` - True if any stationed squad of given faction
- `has_enemy_squad(smart_id, community, exclude_id)` - True if any stationed squad is faction-enemy of community
- `is_smart_empty(smart_id)` - No squads assigned (raw roster check, includes in-transit)

Service NPC resolution (online + actor-level via npc_info walk):
- `get_npc_roles(npc)` - SET of roles `{trader=true, medic=true, mechanic=true}` (any subset, empty for non-service NPCs). Multi-role NPCs (Yar = medic+trader, mechanics with dm_init_trader = mechanic+trader) populate multiple keys. Cached per NPC. Signals: community="trader", clsid=script_trader/trader, section substring patterns, trade= field in active logic block (Demonized-style classifier per Trader Destockifier `trader_autoinject.script:85`). level_spot is NOT used (false-positives on quest NPCs)
- `is_trader_npc(npc)`, `is_medic_npc(npc)`, `is_mechanic_npc(npc)` - Boolean wrappers checking `get_npc_roles(npc)[role]`
- `get_trader_at_smart(smart)`, `get_medic_at_smart(smart)`, `get_mechanic_at_smart(smart)` - First online live NPC at smart with role in their set, or nil
- `has_trader_at_smart(smart)`, `has_medic_at_smart(smart)`, `has_mechanic_at_smart(smart)` - Boolean variants

Squad-smart interaction:
- `is_arrived(squad, smart)` - Delegates to engine's am_i_reached
- `get_proximity(squad, smart)` - Distance and arrival metadata (gvid match, same-level, threshold)
- `has_jobs_for(smart, squad)` - True if every squad member has engine-assigned job at smart (online + actor-level only)

Jobs (smart.stalker_jobs):
- `has_stalker_jobs(smart, type_id)` - Has any (type_id nil) or specific job_type_id (e.g. JOB_TYPE_TRADER = 15)
- `get_npc_for_job(smart, type_id)` - Online game_object of NPC assigned to job of given type_id. NOT the trader NPC (job_type_id=15 tags the visitor patrol slot, not the barman); use service-NPC accessors for traders

Section metadata (LTX squad_descr):
- `section_faction(section)` - Faction (player_id) a squad section produces (cached per section)
- `get_section_npc_max(section)` - Upper bound of npc_in_squad LTX field (cached)

Diagnostic:
- `dump_smarts(level_id)` - Per-smart faction + service role inventory (filtered by level when given)

Spawn helpers (set / clear shared / exclusive spawn, reset_spawns, repopulate) extracted to `xsmart_spawn.script`.

### xstash.script - Stash Operations

- `find_stashes(pos, opts)` - Find revealed stashes near position (opts: max_distance, min_distance, level_id, max_count)
- `is_stash_available(id)`, `is_stash_looted(id)`, `mark_stash_looted(id)`, `clear_stash(id)`
- `get_stash_items(stash_id)` - Read-only stash contents as parsed item list
- `loot_stash(id)` - Loot stash contents (marks looted, returns item list)
- `fill_stash(id, items, add_marker)` - Fill stash with items

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

