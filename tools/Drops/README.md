# Drop Manager Tools

Herramientas para gestionar el sistema de drops de NPCs en Helbreath 3.82.

## 📁 Archivos

| Archivo | Descripción |
|---------|-------------|
| `drop_manager_server.py` | Servidor web con UI para editar drops |
| `export_drops_snapshot.py` | Exporta el estado actual como script de restauración |
| `restore_all_drops.py` | Script auto-generado para restaurar drops |
| `changelog.txt` | Historial de cambios (auto-generado) |

---

## 🎮 Drop Manager (UI Web)

Editor visual para gestionar drops de NPCs.

### Uso

```bash
cd tools/Drops
python drop_manager_server.py
```

Luego abre: **http://localhost:8888**

### Funciones

- **Lista de NPCs** - Sidebar izquierdo con búsqueda
- **Búsqueda de items** - Navbar: busca "chain mail" para ver qué NPCs lo dropean
- **Editor de drops** - Agrega/quita items de cada tier
- **Gate Rates** - Muestra probabilidades globales (Tier 2 editable)
- **Changelog** - Registra automáticamente todos los cambios

### Arquitectura de Gates (Servidor C++)

```
NPC Muere
    │
    ├─ Primary Gate (99.99% pasa, configurable)
    │
    ├─ Gate 1: Gold vs Items T1
    │   ├─ 60% → Drop Gold (hardcoded)
    │   └─ 40% → Roll Tier 1 table
    │
    └─ Gate 2: Tier 2 (independiente)
        └─ 4% → Roll Tier 2 table (configurable)
```

---

## 📤 Export Snapshot

Genera un script Python con el estado ACTUAL de todos los drops.

### Uso

```bash
cd tools/Drops
python export_drops_snapshot.py
```

### Output
- Crea/sobrescribe `restore_all_drops.py`
- Incluye: drop_tables, drop_entries, npc_configs, settings

### Cuándo usar
- Antes de hacer cambios experimentales
- Para crear "checkpoints" del estado de drops
- Para compartir configuración entre servidores

---

## 🔄 Restore All Drops

Restaura el estado de drops guardado previamente.

### Uso

```bash
# Desde tools/Drops
python restore_all_drops.py

# O desde la raíz del proyecto
python tools/Drops/restore_all_drops.py
```

### ⚠️ Advertencia
Este script **BORRA** todos los drops actuales antes de restaurar.

---

## 📊 Base de Datos

Los drops se guardan en `Binaries/Server/GameConfigs.db`:

| Tabla | Contenido |
|-------|-----------|
| `drop_tables` | Definición de tablas de drop por NPC |
| `drop_entries` | Items y pesos por tabla/tier |
| `npc_configs` | Link NPC → drop_table_id |
| `settings` | `primary-drop-rate`, `secondary-drop-rate` |

### Backup
```bash
# Crear backup manualmente
copy Binaries\Server\GameConfigs.db Binaries\Server\GameConfigs.db.bak
```

---

## 🔧 Troubleshooting

### Puerto 8888 ocupado
Edita `PORT = 8888` en `drop_manager_server.py`

### "No NPCs drop this item"
El item no está en ninguna tabla de drops actualmente.

### Changelog no se actualiza
Recarga la página con Ctrl+F5 (limpiar cache).
