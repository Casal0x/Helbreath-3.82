"""
Migration script: Remove dynamic appearance columns from account databases.

Removes weapon_glare, shield_glare, effect_type, hide_armor, and is_walking
columns from the characters table in each account .db file under Accounts/.

These fields are now computed at runtime from equipped items rather than
persisted, preventing stale visual effects (e.g. stuck Merien aura).

Usage: python migrate_remove_dynamic_appearance.py
"""

import sqlite3
import os
import sys
import shutil

ACCOUNTS_DIR = "Accounts"
COLUMNS_TO_REMOVE = [
    # Dynamic state (computed at runtime)
    "weapon_glare", "shield_glare", "effect_type", "hide_armor", "is_walking",
    # Equipment appearance (computed from equipped items in character_items table)
    "shield_type", "weapon_type", "arm_armor_type", "helm_type", "pants_type",
    "armor_type", "mantle_type", "boots_type", "weapon_color", "shield_color",
    "armor_color", "mantle_color", "arm_color", "pants_color", "boots_color", "helm_color",
]


def get_table_columns(cursor, table_name):
    cursor.execute(f"PRAGMA table_info({table_name});")
    return [row[1] for row in cursor.fetchall()]


def migrate_database(db_path):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        columns = get_table_columns(cursor, "characters")
    except sqlite3.OperationalError:
        conn.close()
        return "skipped (no characters table)"

    cols_present = [c for c in COLUMNS_TO_REMOVE if c in columns]
    if not cols_present:
        conn.close()
        return "already clean"

    # Create backup before modifying
    backup_path = db_path + ".bak"
    conn.close()
    shutil.copy2(db_path, backup_path)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # SQLite 3.35.0+ supports ALTER TABLE DROP COLUMN
    sqlite_version = sqlite3.sqlite_version_info
    if sqlite_version >= (3, 35, 0):
        for col in cols_present:
            cursor.execute(f"ALTER TABLE characters DROP COLUMN {col};")
        conn.commit()
        conn.close()
        return f"dropped {cols_present} (backup: {os.path.basename(backup_path)})"

    # Fallback for older SQLite: recreate table without the columns
    remaining_cols = [c for c in columns if c not in COLUMNS_TO_REMOVE]
    col_list = ", ".join(remaining_cols)

    cursor.execute("BEGIN TRANSACTION;")
    cursor.execute(f"CREATE TABLE characters_new AS SELECT {col_list} FROM characters;")
    cursor.execute("DROP TABLE characters;")
    cursor.execute("ALTER TABLE characters_new RENAME TO characters;")
    cursor.execute("COMMIT;")

    conn.close()
    return f"rebuilt table without {cols_present} (backup: {os.path.basename(backup_path)})"


def main():
    if not os.path.isdir(ACCOUNTS_DIR):
        print(f"Error: '{ACCOUNTS_DIR}' directory not found.")
        print("Run this script from the Server binary directory.")
        sys.exit(1)

    db_files = [f for f in os.listdir(ACCOUNTS_DIR) if f.endswith(".db")]
    if not db_files:
        print("No .db files found in Accounts/.")
        return

    print(f"Found {len(db_files)} account database(s).\n")

    for db_file in sorted(db_files):
        db_path = os.path.join(ACCOUNTS_DIR, db_file)
        try:
            result = migrate_database(db_path)
            print(f"  {db_file}: {result}")
        except Exception as e:
            print(f"  {db_file}: ERROR - {e}")

    print("\nDone. Backups saved as .db.bak files.")


if __name__ == "__main__":
    main()
