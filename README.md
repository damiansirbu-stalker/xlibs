# xlibs: Shared Utility Library for STALKER Anomaly Modding

A modder's toolbox covering entity queries, squad operations, smart terrain logic, stash manipulation, logging, profiling, event systems, and data structures. API design comes from reverse engineering the X-Ray engine and Anomaly internals, cross-referenced with patterns from established European and Russian STALKER modders. Every function wraps engine quirks, guards against nil, and handles edge cases that each mod would otherwise solve independently.

Pure Lua where possible. No central loader. Just call the function and it works.

[ModDB](https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001) | [Releases](https://github.com/damiansirbu-stalker/xlibs/releases) | [Report a bug](https://github.com/damiansirbu-stalker/xlibs/issues)

Requires: Anomaly 1.5.3, Modded exes

## Documentation

- [readme.txt](doc/readme.txt) -- full description, module list
- [changelog](https://github.com/damiansirbu-stalker/xlibs/blob/main/doc/changelog) -- version history
- [architecture.md](doc/architecture.md) -- technical reference for modders

## License

PolyForm Perimeter License. Other mods can depend on, call, and ship with xlibs in modpacks, with visible credit. What is not allowed: cloning the architecture, reverse-engineering internal systems, or reproducing the implementation in a competing mod. See [LICENSE](LICENSE) and [usage guide](https://damiansirbu-stalker.github.io/siski-report/guide.html).

A report documenting unauthorized reproduction of this codebase is available at https://damiansirbu-stalker.github.io/siski-report/
