# xlibs: Shared utility library for STALKER Anomaly modding

The library every Alife mod is built on: entity queries, squad operations, smart terrain logic, stash handling, logging, profiling, event systems and data structures.
Every function wraps an engine quirk, guards against nil, and handles the edge case each mod would otherwise solve again on its own.
Pure Lua where possible, with no central loader. Call the function and it works.

[ModDB](https://www.moddb.com/mods/stalker-anomaly/addons/xlibs-1001) | [Releases](https://github.com/damiansirbu-stalker/xlibs/releases) | [Bugs, suggestions](https://github.com/damiansirbu-stalker/xlibs/issues)

[![validate](https://github.com/damiansirbu-stalker/xlibs/actions/workflows/validate.yml/badge.svg)](https://github.com/damiansirbu-stalker/xlibs/actions/workflows/validate.yml) [![Project Health](https://img.shields.io/badge/project_health-dashboard-00ced1)](https://damiansirbu-stalker.github.io/xlibs/)

Requires: Anomaly 1.5.3, modded exes (themrdemonized or AOEngine). Exact versions in [readme.txt](doc/readme.txt).

## My work

- [GitHub](https://github.com/orgs/damiansirbu-stalker/repositories)
- [ModDB](https://www.moddb.com/members/damian-sirbu/addons)
- [Nexus](https://www.nexusmods.com/profile/damiansirbu/mods)

My contributions to the engine: [X-Ray Monolith](https://github.com/themrdemonized/xray-monolith)

## Documentation

- [readme.txt](doc/readme.txt) - full description, the API surface
- [changelog](doc/changelog) - version history
- [architecture.md](doc/architecture.md) - design and module layout

## License

PolyForm Perimeter License. See [LICENSE](LICENSE).
