"""Rename quest_configs.target_type → target_config_id in gameconfigs.db.

Only renames the column — does NOT change data values.
The user must separately update the values from NPC types to NPC config IDs.

Usage:
    python Scripts/migrate_quest_target_column.py [path_to_db]

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

    # Check current schema
    columns = [row[1] for row in db.execute("PRAGMA table_info(quest_configs)")]

    if "target_config_id" in columns:
        print("Column already renamed to target_config_id. Nothing to do.")
        db.close()
        return

    if "target_type" not in columns:
        print("ERROR: Column target_type not found in quest_configs table.")
        db.close()
        sys.exit(1)

    # Show current data for reference
    rows = db.execute("SELECT quest_index, target_type FROM quest_configs ORDER BY quest_index").fetchall()
    print(f"\nCurrent quest_configs ({len(rows)} rows):")
    for row in rows:
        print(f"  quest_index={row[0]:3d}  target_type={row[1]}")

    # Rename column
    db.execute("ALTER TABLE quest_configs RENAME COLUMN target_type TO target_config_id")
    db.commit()

    # Verify
    columns_after = [row[1] for row in db.execute("PRAGMA table_info(quest_configs)")]
    if "target_config_id" in columns_after and "target_type" not in columns_after:
        print("\nColumn renamed: target_type -> target_config_id")
        print("NOTE: Data values are unchanged. Update them from NPC types to NPC config IDs manually.")
    else:
        print("\nERROR: Rename verification failed!")
        sys.exit(1)

    db.close()

if __name__ == "__main__":
    main()
