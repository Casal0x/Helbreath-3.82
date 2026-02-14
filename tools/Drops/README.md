# Drop Manager Tools

Tools to manage the NPC drop system in Helbreath 3.82.

## 📁 Files

| File | Description |
|------|-------------|
| `drop_manager_server.py` | Web server with UI to edit drops |
| `export_drops_snapshot.py` | Exports current state as a restore script |
| `restore_all_drops.py` | Auto-generated script to restore drops |
| `changelog.txt` | Change history (auto-generated) |

---

## 🎮 Drop Manager (Web UI)

Visual editor for managing NPC drops.

### Usage

```bash
cd tools/Drops
python drop_manager_server.py
```

Then open: **http://localhost:8888**

### Features

- **NPC List** - Left sidebar with search
- **Item Search** - Navbar: search "chain mail" to see which NPCs drop it
- **Drop Editor** - Add/remove items from each tier
- **Gate Rates** - Global multipliers and stats
- **Changelog** - Automatically logs all changes

### Multiplier Architecture (C++ Server)
The server uses fixed base rates multiplied by configurable values (defaults 1.0).

```
NPC Dies
    │
    ├─ Primary Gate (Base Check)
    │   ├─ Base: 10% * `primary-drop-rate` (default 1.0)
    │   └─ If pass: Roll Tier 1 (Common Items)
    │
    ├─ Gold Check (Independent)
    │   ├─ Base: 30% * `gold-drop-rate` (default 1.0)
    │   └─ If pass: Drop Gold
    │
    └─ Secondary Gate (Independent)
        ├─ Base: 5% * `secondary-drop-rate` (default 1.0)
        └─ If pass: Roll Tier 2 (Rare Items)
```

> **Note:** Multipliers are floats (e.g., 1.5 = +50% drops, 2.0 = double drops).
> Base rates (10%, 30%, 5%) are hardcoded in `EntityManager.cpp`.

---

## 📤 Export Snapshot

Generates a Python script with the CURRENT state of all drops.

### Usage

```bash
cd tools/Drops
python export_drops_snapshot.py
```

### Output
- Creates/Overwrites `restore_all_drops.py`
- Includes: drop_tables, drop_entries, npc_configs, settings

### When to use
- Before making experimental changes
- To create "checkpoints" of the drop state
- To share configuration between servers

---

## 🔄 Restore All Drops

Restores the previously saved drop state.

### Usage

```bash
# From tools/Drops
python restore_all_drops.py

# Or from project root
python tools/Drops/restore_all_drops.py
```

### ⚠️ Warning
This script **DELETES** all current drops before restoring.

---

## 📊 Database

Drops are stored in `Binaries/Server/GameConfigs.db`:

| Table | Content |
|-------|---------|
| `drop_tables` | Drop table definitions per NPC |
| `drop_entries` | Items and weights per table/tier |
| `npc_configs` | Link NPC → drop_table_id |
| `settings` | `primary-drop-rate`, `secondary-drop-rate` |

### Backup
```bash
# Manually create backup
copy Binaries\Server\GameConfigs.db Binaries\Server\GameConfigs.db.bak
```

---

## 🔧 Troubleshooting

### Port 8888 occupied
Edit `PORT = 8888` in `drop_manager_server.py`

### "No NPCs drop this item"
The item is not currently in any drop table.

### Changelog not updating
Reload page with Ctrl+F5 (clear cache).
