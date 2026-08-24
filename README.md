# xlibs: shared utility library for STALKER Anomaly modding

The library every Alife mod is built on: entity queries, squad operations, smart terrain logic, stash handling, logging, profiling, event systems and data structures.
Every function wraps an engine quirk, guards against nil, and handles the edge case each mod would otherwise solve again on its own.
Pure Lua where possible, with no central loader. Call the function and it works.

[ModDB](https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001) | [Releases](https://github.com/damiansirbu-stalker/xlibs/releases) | [Bugs, suggestions](https://github.com/damiansirbu-stalker/xlibs/issues)

Requires: Anomaly 1.5.3, modded exes (themrdemonized or AOEngine). Exact versions in [readme.txt](doc/readme.txt).

## Alife Collection

- [AlifeAmbience](https://github.com/damiansirbu-stalker/AlifeAmbience)
- [AlifeBalance](https://www.moddb.com/mods/stalker-anomaly/addons/alifebalance)
- [AlifeCompanions](https://github.com/damiansirbu-stalker/AlifeCompanions)
- [AlifeDiegetic](https://www.moddb.com/mods/stalker-anomaly/addons/diegetic-audio-control-100)
- [AlifeGuard](https://www.moddb.com/mods/stalker-anomaly/addons/alifeguard-1001)
- [AlifePlus](https://www.moddb.com/mods/stalker-anomaly/addons/alifeplus-v1-0-01)
- [AlifeSpooks](https://github.com/damiansirbu-stalker/AlifeSpooks)
- [AlifeTactics](https://www.moddb.com/mods/stalker-anomaly/addons/alifetactics)
- [FurnitureFuel](https://github.com/damiansirbu-stalker/FurnitureFuel)
- [JitProfiler](https://github.com/damiansirbu-stalker/JitProfiler)
- [TestZone](https://github.com/damiansirbu-stalker/TestZone)
- [xlibs](https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001)

## Documentation

- [readme.txt](doc/readme.txt) - full description, the API surface
- [changelog](doc/changelog) - version history
- [architecture.md](doc/architecture.md) - design and module layout

## License

PolyForm Perimeter License. See [LICENSE](LICENSE).
