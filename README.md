# Lists Integrations — Geode Mod

A Geometry Dash mod that displays **demon list rankings** (AREDL, NARLL, Demonlist, and more) directly on levels as you browse them in-game.

## Features

- 🏷️ **Badges on levels** — see which lists a level appears on at a glance
- 🔢 **Rank display** — shows the level's position (e.g. `AREDL #42`)
- 🎨 **Color-coded badges** — each list has its own color
- ⚙️ **Fully configurable** — enable/disable each list, change URLs, set badge position
- 🔗 **Custom list support** — add any list with a JSON API via the settings
- 💾 **Smart caching** — list data is cached to reduce API requests

## Supported Lists (default)

| List | Color | API |
|------|-------|-----|
| AREDL | 🟠 Orange | `api.aredl.net/api/aredl/levels` |
| NARLL | 🔵 Teal | `narll.net/api/levels` |
| Pointercrate Demonlist | 🔴 Red | `pointercrate.com/api/v2/demons/listed/` |
| Custom (user-defined) | 🩵 Light Blue | Your URL |

## Adding a Custom List

1. Open **Mod Settings** in-game
2. Enable **"Enable Custom List"**
3. Set **"Custom List Name"** to your list's display name
4. Set **"Custom List API URL"** to the JSON endpoint

### Required JSON Format

Your API must return a JSON array where each entry has at minimum:

```json
[
  { "id": 12345678, "position": 1, "name": "Level Name" },
  { "id": 87654321, "position": 2, "name": "Another Level" }
]
```

**Supported field aliases:**
- Level ID: `id`, `level_id`, `levelId`, `gd_id`
- Position: `position`, `rank`, `placement`
- Name: `name`, `title`, `level_name`

A root object with a `"data"` array is also supported:
```json
{ "data": [ ... ] }
```

## Building

### Prerequisites

- [Geode SDK](https://geode-sdk.org) installed and `GEODE_SDK` env variable set
- CMake 3.21+
- C++20-compatible compiler

### Steps

```bash
cmake -B build
cmake --build build
```

The built `.geode` file will appear in `build/`.

## Settings Reference

| Setting | Default | Description |
|---------|---------|-------------|
| Show List Badges | ✅ | Toggle badge visibility |
| Show Rank Number | ✅ | Show `#N` after list name |
| AREDL Enabled | ✅ | Fetch from AREDL |
| AREDL API URL | `api.aredl.net/...` | Customizable endpoint |
| NARLL Enabled | ✅ | Fetch from NARLL |
| NARLL API URL | `narll.net/...` | Customizable endpoint |
| Demonlist Enabled | ✅ | Fetch from Pointercrate |
| Demonlist API URL | `pointercrate.com/...` | Customizable endpoint |
| Custom List Enabled | ❌ | Add your own list |
| Custom List Name | `My List` | Badge label |
| Custom List API URL | *(empty)* | Your JSON endpoint |
| Badge Position | Top Right | Corner for badge placement |
| Cache Duration | 15 min | How long to keep data |

## Architecture

```
src/
├── main.cpp          — Mod entry point, loads ListManager
├── ListManager.hpp   — Core data types & manager interface
├── ListManager.cpp   — HTTP fetching, JSON parsing, caching
├── ListBadgeNode.hpp — CCNode badge UI (single badge + container)
└── hooks.cpp         — LevelCell & LevelBrowserLayer hooks
```

## License

MIT — feel free to fork and add more lists!
