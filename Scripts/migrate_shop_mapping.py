#!/usr/bin/env python3
"""
One-time migration script: renames the 'npc_type' column to 'npc_config_id'
in the npc_shop_mapping table of gameconfigs.db.

Uses the safe table-rebuild approach since older SQLite versions may not
support ALTER TABLE RENAME COLUMN.
"""

import sqlite3
import os
import sys

DB_PATH = os.path.join(os.path.dirname(__file__), "..", "Binaries", "Server", "gameconfigs.db")


def main():
    db_path = os.path.abspath(DB_PATH)
    if not os.path.exists(db_path):
        print(f"ERROR: Database not found at {db_path}")
        sys.exit(1)

    print(f"Database: {db_path}")

    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row

    # Check that the old table exists and has the npc_type column
    columns = [row[1] for row in conn.execute("PRAGMA table_info(npc_shop_mapping)").fetchall()]
    if "npc_type" not in columns:
        if "npc_config_id" in columns:
            print("Migration already applied: npc_shop_mapping already has 'npc_config_id' column.")
            conn.close()
            return
        print(f"ERROR: npc_shop_mapping table does not have 'npc_type' column.")
        print(f"  Columns found: {columns}")
        conn.close()
        sys.exit(1)

    # Count existing rows
    row_count = conn.execute("SELECT COUNT(*) FROM npc_shop_mapping").fetchone()[0]
    print(f"Found {row_count} row(s) in npc_shop_mapping.")

    # Perform the migration inside a transaction
    print("Renaming 'npc_type' -> 'npc_config_id' in npc_shop_mapping...")
    conn.execute("BEGIN TRANSACTION")
    try:
        conn.execute(
            "CREATE TABLE npc_shop_mapping_new ("
            "  npc_config_id INTEGER PRIMARY KEY,"
            "  shop_id INTEGER NOT NULL,"
            "  description TEXT"
            ")"
        )
        conn.execute(
            "INSERT INTO npc_shop_mapping_new (npc_config_id, shop_id, description) "
            "SELECT npc_type, shop_id, description FROM npc_shop_mapping"
        )
        conn.execute("DROP TABLE npc_shop_mapping")
        conn.execute("ALTER TABLE npc_shop_mapping_new RENAME TO npc_shop_mapping")
        conn.execute("COMMIT")
    except Exception as e:
        conn.execute("ROLLBACK")
        print(f"ERROR: Migration failed, rolled back. {e}")
        conn.close()
        sys.exit(1)

    # Verify
    new_count = conn.execute("SELECT COUNT(*) FROM npc_shop_mapping").fetchone()[0]
    new_columns = [row[1] for row in conn.execute("PRAGMA table_info(npc_shop_mapping)").fetchall()]
    conn.close()

    print(f"Migration complete. {new_count} row(s) migrated.")
    print(f"New columns: {new_columns}")
    print()
    print("REMINDER: The npc_config_id values currently hold the old npc_type values.")
    print("You may need to update them to match actual npc_id values from the npc_configs table.")


if __name__ == "__main__":
    main()
