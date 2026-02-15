"""Update quest_configs.target_config_id values from NPC types to NPC config IDs.

Cross-references npc_configs to find the npc_id for each npc_type.
Unambiguous mappings (1 config per type) are applied automatically.
Ambiguous mappings (multiple configs per type) are flagged for manual review.

Usage:
    python Scripts/migrate_quest_target_values.py [path_to_db]

Default path: Binaries/Server/gameconfigs.db
"""

import sqlite3
import sys
import os

def main():
    if len(sys.argv) > 1:
        db_path = sys.argv[1]
    else:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        db_path = os.path.join(script_dir, "..", "Binaries", "Server", "gameconfigs.db")

    db_path = os.path.normpath(db_path)

    if not os.path.exists(db_path):
        print(f"ERROR: Database not found: {db_path}")
        sys.exit(1)

    print(f"Database: {db_path}")

    db = sqlite3.connect(db_path)

    # Verify column exists
    columns = [row[1] for row in db.execute("PRAGMA table_info(quest_configs)")]
    if "target_config_id" not in columns:
        print("ERROR: Column target_config_id not found. Run migrate_quest_target_column.py first.")
        db.close()
        sys.exit(1)

    # Build type -> [npc_id, ...] mapping
    type_to_configs = {}
    for npc_id, npc_type, name in db.execute("SELECT npc_id, npc_type, name FROM npc_configs"):
        type_to_configs.setdefault(npc_type, []).append((npc_id, name))

    # Get all quests with non-zero targets
    quests = db.execute(
        "SELECT quest_index, target_config_id, min_level, max_level "
        "FROM quest_configs WHERE target_config_id > 0 "
        "ORDER BY quest_index"
    ).fetchall()

    updated = 0
    ambiguous = []

    for quest_index, old_type, min_level, max_level in quests:
        configs = type_to_configs.get(old_type, [])

        if len(configs) == 0:
            print(f"  WARNING: quest {quest_index}: type {old_type} has no matching NPC config")

        elif len(configs) == 1:
            new_id = configs[0][0]
            npc_name = configs[0][1]
            db.execute(
                "UPDATE quest_configs SET target_config_id = ? WHERE quest_index = ?",
                (new_id, quest_index)
            )
            print(f"  quest {quest_index:3d}: type {old_type:3d} -> config_id {new_id:3d} ({npc_name})")
            updated += 1

        else:
            # Ambiguous - pick lowest npc_id (base variant) as default
            default = configs[0]
            db.execute(
                "UPDATE quest_configs SET target_config_id = ? WHERE quest_index = ?",
                (default[0], quest_index)
            )
            others = ", ".join(f"{c[0]}={c[1]}" for c in configs[1:])
            print(f"  quest {quest_index:3d}: type {old_type:3d} -> config_id {default[0]:3d} ({default[1]}) "
                  f"[AMBIGUOUS - also: {others}] levels={min_level}-{max_level}")
            ambiguous.append((quest_index, old_type, configs, min_level, max_level))
            updated += 1

    # Skip quests with target 0 (non-monster-hunt quests)
    zero_count = db.execute("SELECT COUNT(*) FROM quest_configs WHERE target_config_id = 0").fetchone()[0]

    db.commit()
    db.close()

    print(f"\nUpdated {updated} quest(s). Skipped {zero_count} quest(s) with target=0.")

    if ambiguous:
        print(f"\n{len(ambiguous)} AMBIGUOUS quest(s) defaulted to lowest npc_id.")
        print("Review and manually fix with:")
        print("  UPDATE quest_configs SET target_config_id = <correct_id> WHERE quest_index = <index>;")

if __name__ == "__main__":
    main()
