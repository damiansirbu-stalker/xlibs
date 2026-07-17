# xlibs Architecture

Shared utility library for STALKER Anomaly Lua modding. Pure Lua, game globals only.

---

## Invariants

- **No steady-state per-frame work.** Ongoing work runs on a throttled tick (a fixed interval) or on a discrete engine event (hit, shot, spawn, option change); it never runs continuously every frame. A per-frame engine callback (`npc_on_update`) is used only as a carrier that throttles before doing anything, and we never place our code on a path the engine runs every frame (a visibility or fire functor). Frame-spreading a bounded one-off batch (xslice, 1 item per frame) to avoid a single-frame spike is the one allowed use of the frame; it completes and stops. Full rule and rationale: `doc/standards/code-standards.md` "No Per-Frame Work".
- **Steady-state silence.** xlibs modules do not log during normal operation; instrumentation runs only through a consumer-created xlog logger in the consumer's own code. The one permitted logging class is a load-time caller-misuse warning (a boundary diagnostic surfacing a caller bug before it becomes silent data loss): xchange warns on a bad or duplicate changeset registration because a silently skipped migration is a data hazard nobody would hear.

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

The derived event pattern uses `xevent.hook` to intercept game functions and emit synthetic callbacks: any game function that lacks a callback can be hooked, the wrapper emitting via `SendScriptCallback`. xlibs ships the `xevent` primitive only; consumers declare (`AddScriptCallback`) and fire their own events with it.

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

- `subscribe(event, callback, name)` - Register handler; rejects duplicates by name
- `publish(event, data)` - Dispatch to all handlers, direct calls (no pcall: subscribers are internal modules, errors must be visible)

### xcreature.script - Creature/Entity Identification

```lua
local is_mutant = xcreature.is_mutant(entity_id)
local community = xcreature.community(entity_id)
local name = xcreature.get_name(entity_id)
```

- `is_stalker(input)`, `is_mutant(input)`, `is_npc(input)` - Entity type checks
- `is_trader(input)` - True if entity is Script_Trader CSE (clsid 37 / 36); catches Sidorovich-class
- `community(input)` - Get faction community
- `get_name(input)` - Translated name; translate-first for mutants (inv_name, then section key, then the English hash), memoized per section. Returns nil for nil entities and nameless stalkers - callers pick the fallback
- `get_money(obj)`, `give_money(obj, amount)`, `transfer_money(from, amount, to)` - Ruble reads and moves, u32-underflow guarded
- `online_iter_with_id()` - Iterator over online game objects yielding (id, obj) pairs
- `get_mutant_species(input)` - Base species string from any entity ("bloodsucker", "dog", etc.). Handles modded exe clsid reuse via section prefix fallback. player_id fast reject for stalkers (0 luabind).
- `get_mutant_variant(input)` - Full NPC variant section ("bloodsucker_red_strong", "dog_weak_brown")
- `is_unscriptable(obj)` - Check against xdata.unscriptable_npcs
- `is_task_giver(obj)` - Check NPC/squad ID against active tasks

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
- `create_squad(smart, section)` - Spawn a squad at a smart through the vanilla board path (roster + squad_on_npc_creation native)
- `get_squad_smart(squad)`
- `is_stationed(squad, smart_id)` - True when engine considers squad stationed (current_action=1). With smart_id, also requires current_target_id match. Sticky until idle_time expires or new target assigned
- `get_squad_by_member(npc_id)` - Get squad containing NPC
- `get_community(squad)` - Raw community id (faction key), untranslated. Use for keying/comparison; the translated `get_community_name` is display-only
- `get_community_name(squad)` - Translated community name (safe, never nil)
- `is_permanent_squad(squad)` - Static identity check (story, trader, named_npc, empty), cached
- `has_active_role(squad)` - Dynamic role check (task_giver, companion)
- `is_task_target(squad)` - Task target check (task_squads hash + bounty/hostage member fallback)
- `is_scripted(squad)` - Check engine/vanilla scripting fields (scripted_target, condlist, random_targets)
- `is_squad(obj)` - clsid guard: true only for sim_squad_scripted instances (id-recycling defense)
- `has_squad(pos, opts)` - Short-circuiting boolean: any squad within max_distance matches?
- `collect_squad_ids(out)` - Fill a set with all SIMBOARD squad ids
- `get_commander_rank(squad)`, `get_commander_name(squad)` - Commander metadata reads
- `is_protected(squad, opts)` - Unified protection check. Runs guards in order: exclude_filter, is_scripted, is_permanent, has_active_role, is_task_target. Shares commander resolution across checks. Each guard toggled via opts flags.
- `reassert_target(squad, target)` - Restore scripted_target if cleared by another mod between scans
- `iter_squads()` - Iterator over all SIMBOARD squads
- `iter_member_ids(squad)` - Iterator yielding member entity IDs
- `dump_squads()` - Diagnostic string of all SIMBOARD squads

### xcombat.script - Combat AI Primitives

```lua
xcombat.set_combat(npc, { fire = xcombat.FIRE, posture = xcombat.CROUCH, movement = xcombat.STILL, aim = enemy })
xcombat.send_to(npc, xcombat.find_cover(npc, enemy_pos))
if xcombat.fire_make_sense(npc, enemy) then ... end
```

Combat-AI primitives for a script-driven NPC combat takeover, plus wrappers over the per-NPC combat-AI engine binds. Registers no callbacks and holds no game state; caches are transient (TTL, level-key) or session-stable exe probes.

- `get_weapon_kind(npc)`, `get_weapon_range(kind)` - Active weapon kind (TTL-cached) and its engagement range band
- `sees(npc, obj)`, `get_unseen_ms(npc, enemy, tg)`, `get_track_pos(npc, enemy, sees)` - Enemy-memory reads: visible right now (the engine's accumulated visible_now), any-sense recency, live-or-last-known position
- `disclose_enemy(npc, enemy)` - Inject a known enemy into NPC memory and register in combat, relation-clean
- `install_takeover(npc, spec)`, `release_takeover(npc)`, `release_takeover_id(id)` - Graft the GOAP gate evaluator + action per stalker; the consumer owns policy via spec { gate, on_begin }. Single-consumer: a second differing spec asserts. release_takeover_id is the id-keyed release for server-side unregister
- `register_in_combat(npc)`, `unregister_in_combat(npc)` - Squad memory-sharing bookkeeping the blocked combat planner no longer performs
- `get_blocked_planners()`, `get_operator(npc)`, `get_facing_offset(npc, pos)` - Blocked planner-id list, current brain operator, body-facing offset
- `get_cover_state(npc)` - The combat planner's stored cover bookkeeping as one guarded read: (in_cover, looked_out, position_held). Engine beliefs, not concealment geometry; nil when there is no planner to ask
- `set_combat(npc, opts)` - One command for weapon mode + posture + movement, resolved through the combat-state matrix so state and explicit posture/movement never contradict
- `has_obstacle_between(a, b)`, `has_obstacle_to_target(a, b)`, `has_friendly_in_line(npc, a, b, thresh)` - Chest-height object-aware rays (movement lane, shot line capped short of the target body) and the squad firing-lane check
- `find_cover(npc, enemy_pos, search_pos)`, `find_shot(npc, enemy_pos)`, `find_flee_lane(npc, dir, m, arc, spread)` - Maneuver vertex finders (ring sweeps, clear-lane fans)
- `send_to(npc, vid)`, `is_arrived(npc)` - Engine-routed movement (nearest-accessible fallback) and arrival truth
- `is_indoor(pos)` - Indoor-level table plus surge-shelter proximity
- `claim_cover(lvid, owner_id)`, `release_cover(lvid, owner_id)` - Ownership-checked cover-vertex reservation over db.used_level_vertex_ids
- `get_squad_ordinal(squad_id, npc_id)` - Stable per-member spread bucket

Engine fire-gate wrappers (themrdemonized/xray-monolith PRs #594 aim+vision, #595 switch veto, #596 fire gates). Each probes its bind once on first call (`type(npc.<bind>) == "function"`, cached module-local, never a version compare) and carries a defined floor fallback, so consumers call unconditionally and the engine geometry switches on by itself when the exe has the merged PRs:

- `set_aim_params(npc, max_angle, min_angle, min_speed, predict_time)` - Per-NPC sight swing/lead; negative component reverts to the global ai_aim_* cvar. Floor: no-op, vanilla aim is the fallback
- `set_vision_speed(npc, factor)` - Per-NPC vision acquisition factor composing with g_ai_vision_speed_boost. Floor: no-op by design; the only script substitute is a per-frame functor wrap, banned by code-standards "No Per-Frame Work"
- `set_fire_queue_scale(npc, size_k, interval_k)` - Per-NPC combat-planner burst-size and inter-burst-interval scale (PR #603), applied in select_queue_params; vanilla-planner fire only (state_mgr fire states bypass it). Floor: no-op, vanilla queues are the fallback
- `can_kill_enemy(npc, enemy)` - Engine shot clearance (5-ray safety fan, frame-cached). Floor: one capped rqtBoth ray from the weapon-hand bone, stopped short of the enemy body
- `can_kill_member(npc, enemy_pos)` - Non-enemy in the fire lane; a repositioning hint, never a hold-fire gate (the engine's CObjectActionFire hold survives a combat-planner block). Floor: living squadmates only via has_friendly_in_line
- `is_under_fire(npc, enemy, tg, window_ms)` - Danger-memory read: has the enemy hit or shot at the NPC within the window
- `fire_make_sense(npc, enemy)` - The engine fire-discipline gate. Floor: the engine gate order from floor primitives (height gap, capped clearance ray, see-now, 10s unseen window with an automatic weapon), constants tracking the ai_fire_* cvars when the exe has them
- `on_action_switch(fn)` - Register fn on the npc_on_combat_action_switch veto; registers only when the seam exists and returns whether it did. Enhancement-only, no floor imitation possible

### xobject.script - Generic Object Lookup

```lua
local se = xobject.se(any_input)  -ID, game object, or server object
local npc = xobject.go(npc_id)  -online game object or nil
```

- `se(input)` - Get server object from any input type
- `go(id)` - Get online game object by ID (nil if offline)
- `is_story(se_obj)` - True when the entity carries a story id (pure Lua STORY_PAIRS lookup, 0 luabind)

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
local valid = xlevel.is_valid_lvid(se_obj)
```

- `get_level_id(se_obj)` - Level ID from server entity (pcall-guarded)
- `get_actor_level_id()` - Actor's current level ID
- `get_level_name(level_id)`, `get_level_id_by_name(name)` - Level id <-> name resolution (session-cached)
- `get_smart_display_name(smart)` - Translated smart terrain name
- `get_neighbor_levels(source_id, hops)` - Level-id set within N graph hops. Returns the cached set BY REFERENCE: read-only, never mutate or hold across sessions
- `is_valid_lvid(se_obj)` - Check level vertex validity (0xFFFFFFFF = invalid)
- `is_surge()` - True while a surge or psi-storm is underway (pure Lua module reads via xr_conditions.surge_started)

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
- `is_near_campfire(smart, position, radius)` - Position within radius of one of the smart's campfires

Smart finders:
- `get_actor_smart()` - Nearest smart to actor (engine-maintained, O(1))
- `find_smart(pos, opts)` - Generic nearest smart search (level_id, factions, min/max distance, exclude_id, filter, source). `factions` takes string or set
- `find_first_smart(opts)` - Distance-free variant; first matching smart in pool iteration order
- `find_smarts_spawning(level_id, faction)` - Array of smarts on level whose recipes produce given faction
- `find_friendly_base(community, pos, opts)` - Nearest base-class smart friendly to the community (min_distance filter)
- `smarts_by_names()` - Name-keyed smart lookup table (SIMBOARD.smarts_by_names read)
- `smart_iter()` - Stateful iterator over all SIMBOARD smart terrains

SIMBOARD roster (sim-intent membership, NOT physical occupancy):
- `get_smart_squads(smart_id)` - Raw SIMBOARD.smarts[id].squads hash
- `assign_squad_to_smart(squad, smart_id)` - Wrap SIMBOARD:assign_squad_to_smart; nil to detach
- `reconcile_squad_roster(squad, from_smart_id)` - Sync SIMBOARD.smarts rosters after a squad changed smart via the roster-blind vanilla `sim_squad_scripted:assign_smart` (called from specific_update / generic_update, which never update the squads table or population). Drops the stale `from` entry, adds the current `smart_id` entry, recomputes both populations via `smart_terrain_squad_count`, fires leave/enter callbacks. Idempotent (table-state gated), so it is a no-op on a base that already syncs. Consumed by AlifePlus `ap_core_anomaly_fixes`
- `iter_stationed_squads(smart_id, exclude_id, cap)` - Closure iterator yielding se for squads with current_action=1 AND current_target_id=smart_id (xsquad.is_stationed). Skips in-transit. Cap default 5
- `has_squad_of_faction(smart_id, faction, exclude_id)` - True if any stationed squad of given faction
- `has_enemy_squad(smart_id, community, exclude_id)` - True if any stationed squad is faction-enemy of community
- `get_faction_smart_count(factions, level_id)` - Count of smarts on the level with a stationed squad of the community (string) or of any community in the set (table). Per-level snapshot of stationed communities, 60s TTL (xttltable). Consumed by AlifePlus per-faction expansion caps
- `is_smart_empty(smart_id)` - No squads assigned (raw roster check, includes in-transit)

Service NPC resolution (online + actor-level via npc_info walk):
- `get_npc_roles(npc)` - SET of roles `{trader=true, medic=true, mechanic=true}` (any subset, empty for non-service NPCs). Multi-role NPCs (Yar = medic+trader, mechanics with dm_init_trader = mechanic+trader) populate multiple keys. Cached per NPC id with a section verify on hit (id-recycling defense). Signals: community="trader", clsid=script_trader/trader, section substring patterns, trade= field in active logic block (Demonized-style classifier per Trader Destockifier `trader_autoinject.script:85`). level_spot is NOT used (false-positives on quest NPCs)
- `is_trader_npc(npc)`, `is_medic_npc(npc)`, `is_mechanic_npc(npc)` - Boolean wrappers checking `get_npc_roles(npc)[role]`
- `get_trader_at_smart(smart)`, `get_medic_at_smart(smart)`, `get_mechanic_at_smart(smart)` - First online live NPC at smart with role in their set, or nil
- `has_trader_at_smart(smart)`, `has_medic_at_smart(smart)`, `has_mechanic_at_smart(smart)` - Boolean variants

Squad-smart interaction:
- `is_arrived(squad, smart)` - Delegates to engine's am_i_reached
- `get_proximity(squad, smart)` - Distance and arrival metadata (gvid match, same-level, threshold)
- `has_jobs_for(smart, squad)` - True if every squad member has engine-assigned job at smart (online + actor-level only)

Jobs (smart.stalker_jobs):
- `has_stalker_jobs(smart, type_id)` - Has any (type_id nil) or specific job_type_id (e.g. JOB_TYPE_TRADER = 15)
- `get_job_for_npc(smart, npc_id)` - Job descriptor the NPC currently holds at the smart (npc_info read)

Section metadata (LTX squad_descr):
- `section_faction(section)` - Faction (player_id) a squad section produces (cached per section)
- `get_section_npc_max(section)` - Upper bound of npc_in_squad LTX field (cached)

Diagnostic:
- `dump_smarts(level_id)` - Per-smart faction + service role inventory (filtered by level when given)

Spawn helpers (set / clear shared / exclusive spawn, reset_spawns, repopulate) extracted to `xsmart_spawn.script`.

### xstash.script - Stash Operations

- `find_stashes(pos, opts)` - Find revealed stashes near position (opts: max_distance, min_distance, level_id, max_count; exact-count take)
- `is_stash_looted(id)` - caches[id] == true read
- `get_stash_items(stash_id)` - Read-only stash contents as parsed item list
- `loot_stash(id)` - Loot stash contents (marks looted, returns item list)
- `fill_stash(id, items, add_marker)` - Fill stash with items

### xtable.script - Table Utilities

- `count(t, fn)` - Count elements (optional predicate)
- `shuffle(t)`, `sort(t, comparator)`
- `merge(...)` - Set union of hash tables. Variadic, nil-safe, returns new table.
- `subtract(base, ...)` - Set difference. Returns base keys minus all keys in remaining args.

### xttltable.script - TTL Data Structures

All three time-based structures support clock injection via `opts.clock`. Default is `os.clock` (zero luabind, wall time). Pass `xtime.game_sec` for game-time expiry (2 luabind per clock call). Use wall time for internal rate limiting and dedup guards. Use game time for gameplay-facing timers (decay windows, cooldowns) so they respect time acceleration, sleep, and realism mods.

- `create_ttl_table(opts)` - TTL table with auto-expiry
  - `default_ttl` -> expiry seconds, `clock` -> clock function (default os.clock)
  - `:set(key, value, ttl)`, `:get(key)`, `:has(key)`, `:remove(key)` - false round-trips through :get (negative-cache convention, same as fifo_cache)
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

- `sample(tbl)` - Random element
- `sample_n(tbl, n)` - Random-COUNT sample: k = random(1, min(n, #t)) elements (deliberate dispatch variability; slice partial_shuffle for an exact take)
- `partial_shuffle(tbl, count)` - Shuffle first N elements

### xmcm.script - MCM Config

- `create_config(mod_id, defaults, path_builder)` - Pre-seeded cfg table + per-key getter + load() refresher (every mod's mcm file uses this)

### xprofiler.script - Code Profiling

Source: xray-monolith/src/xrServerEntities/script_engine_script.cpp:127-196

- `new()`, `new_if(condition)` -> `:start()`, `:stop()`, `:get_us()`, `:get_ms()`, `:reset()`
- `new_if(false)` returns NOOP singleton (zero overhead)
- Uses `profile_timer` (CPU clock, microsecond resolution)

### xtrace.script - Tracing

- `new()`, `new_if(condition)` -> `.id`, `.path`, `:push(op)`, `:pop(segment)`
- `new_if(false)` returns NOOP singleton (zero overhead)

### xinspect.script - Debug

- `format_table(t, max_depth)` - Table to string (cycle detection, depth cap)
- `userdata(obj)` - Userdata field enumeration

### xevent.script - Synthetic Callbacks

Runtime function hooking. Intercept any Lua function, emit callbacks from systems that don't have them.

```lua
-- consumer declares its event once (AddScriptCallback), then hooks a game function to fire it
AddScriptCallback("my_synthetic_event")
xevent.hook("xr_eat_medkit", "consume_medkit", function(orig, npc, medkit, kind)
    SendScriptCallback("my_synthetic_event", npc, medkit, kind)
    return orig(npc, medkit, kind)
end)
```

- `hook(module, func, wrapper)` - Wrap function, returns success
- `is_hooked(module, func)` - Check if hooked

**Naming convention:** prefix synthetic event names by owner to avoid collision with engine callbacks (AlifePlus uses `ap_`, e.g. `ap_npc_medkit_use`).

**How it works:** Lua functions are table entries. We save the original, replace with wrapper that calls original + emits callback. Zero engine modification.

### xpda.script - PDA/Map

- `send(caption, msg, icon)` - PDA message
- `npc_tip(npc, msg, opts)` - NPC-attributed tip
- `mark_squad(id, opts)`, `unmark_squad(id)`, `clear_squad_markers()`
- `mark_entity(id, opts)`, `unmark_entity(id, type)`

### xstring.script - Interpolation

- `interpolate(template, vars)` - `"Hello {name}!"` -> `"Hello World!"`

### xtime.script - Game Time

- `game_sec()` - Game-seconds since epoch (cached start_time, invalidated on on_game_start)
- `game_time()` - Engine CTime for the current in-game moment (nil when no level)
- `hms()` - Current in-game hour and minute via CTime:get() (nil, nil when no level)

### xconst.script - Engine Sentinel Constants

X-Ray engine sentinel values extracted from C++ source headers.

- `INVALID_ENTITY_ID` - u16 MAX (65535), from alife_space.h:39
- `INVALID_LEVEL_VERTEX_ID` - u32 MAX (4294967295)
- `INVALID_GAME_VERTEX_ID` - u16 MAX (65535), GameGraph::_GRAPH_ID(-1) at xrServer_Objects_ALife.cpp:369
- `INVALID_BONE_ID` - u16 MAX (65535); `game_object:get_bone_id` returns it for a bone name the skeleton does not have
- `ACTOR_ENTITY_ID` - 0 (engine convention: actor allocated as the first server slot, `db.actor:id() == 0`)

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

