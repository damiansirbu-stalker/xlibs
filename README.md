# xlibs: Shared utility library for STALKER Anomaly modding

The library every Alife mod is built on: entity queries, squad operations, smart terrain logic, stash handling, logging, profiling, event systems and data structures.
Every function wraps an engine quirk, guards against nil, and handles the edge case each mod would otherwise solve again on its own.
Pure Lua where possible, with no central loader. Call the function and it works.

[![ModDB](https://img.shields.io/badge/ModDB-c8102e)](https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001) [![Nexus Mods](https://img.shields.io/badge/Nexus_Mods-da8e35)](https://www.nexusmods.com/stalkeranomaly/mods/102) [![Releases](https://img.shields.io/badge/Releases-24292f?logo=github&logoColor=white)](https://github.com/damiansirbu-stalker/xlibs/releases) [![Bugs, suggestions](https://img.shields.io/badge/Bugs%2C_suggestions-24292f?logo=github&logoColor=white)](https://github.com/damiansirbu-stalker/xlibs/issues) [![Ko-fi](https://img.shields.io/badge/Ko--fi-ff5e5b?logo=kofi&logoColor=white)](https://ko-fi.com/damiansirbu)

[![ci](https://github.com/damiansirbu-stalker/xlibs/actions/workflows/ci.yml/badge.svg)](https://github.com/damiansirbu-stalker/xlibs/actions/workflows/ci.yml) [![Project Health](https://img.shields.io/badge/project_health-dashboard-00ced1)](https://damiansirbu-stalker.github.io/xlibs/)

Requires: Anomaly 1.5.3, modded exes (themrdemonized or AOEngine). Exact versions in [readme.txt](doc/readme.txt).

## My work

- Alife mods: [AlifePlus](https://www.moddb.com/mods/stalker-anomaly/addons/alifeplus-v1-0-01) · [AlifeTactics](https://www.moddb.com/mods/stalker-anomaly/addons/alifetactics) · [AlifeBalance](https://www.moddb.com/mods/stalker-anomaly/addons/alifebalance) · [AlifeGuard](https://www.moddb.com/mods/stalker-anomaly/addons/alifeguard-1001)
- Diegetic mods: [DiegeticControl](https://www.moddb.com/mods/stalker-anomaly/addons/diegeticcontrol) · DiegeticAmbience · DiegeticDread
- Tools: [JitProfiler](https://www.moddb.com/mods/stalker-anomaly/addons/jitprofiler)
- Libraries: [xlibs](https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001)
- Engines: [X-Ray Monolith](https://github.com/themrdemonized/xray-monolith/pulls?q=is%3Apr+author%3Adamiansirbu+is%3Amerged) · [OpenXRay](https://github.com/OpenXRay/xray-16/pulls?q=is%3Apr+author%3Adamiansirbu+is%3Amerged) · [IX-Ray](https://github.com/ixray-team/ixray-1.6-stcop/commits/develop?author=damiansirbu)
- Dependents: [COMPASS](https://github.com/Crimento/COMPASS) · [Nemesis Spawn System](https://github.com/SirEndet/Nemesis-Spawn-System) · [Word of Mouth](https://www.moddb.com/mods/stalker-anomaly/addons/word-of-mouth)
- Integrations: [Warfare (erepb)](https://www.moddb.com/mods/stalker-anomaly/addons/warfare-alife-overhaul-new) · [Homestead](https://www.moddb.com/mods/stalker-anomaly/addons/homestead-stalker-settlement-builder) · [Stealth Overhaul](https://github.com/Alex-leon1594/Stealth_Overhaul_Reworked) · [TALKER](https://www.moddb.com/mods/stalker-anomaly/addons/talker-ai-powered-npc-dialogue-beta) · [Factions of the Zone](https://github.com/devinhorowitz/factionsofthezone) · [Anomaly Radios Extended](https://www.moddb.com/mods/stalker-anomaly/addons/anomaly-radios-extended-full-remaster-152)
- Collaborations: [xAGNA](https://www.moddb.com/mods/stalker-anomaly/addons/xagna)

## Documentation

- [readme.txt](doc/readme.txt) - full description, the API surface
- [changelog](doc/changelog) - version history
- [architecture.md](doc/architecture.md) - design and module layout

## License

PolyForm Perimeter License. See [LICENSE](LICENSE).
