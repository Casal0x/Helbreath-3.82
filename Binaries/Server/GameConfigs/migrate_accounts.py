#!/usr/bin/env python3
"""
Offline migration script for Helbreath account databases.

Migrations:
  1. Drop quiz/answer columns (if present)
  2. Hash plaintext passwords with SHA-256 + random salt (if still plaintext)

Matching C++ implementation: SHA256(salt_hex + password) where salt is 16 random bytes.

Usage:
  python migrate_accounts.py

Processes all .db files in Accounts/ directory. Creates .bak backups before modifying.
Uses only Python stdlib (sqlite3, hashlib, secrets).
"""

import os
import sys
import glob
import shutil
import sqlite3
import hashlib
import secrets


def generate_salt_hex():
    """Generate 16 random bytes, return as 32-char hex string."""
    return secrets.token_hex(16)


def hash_password(password, salt_hex):
    """SHA-256(salt_hex + password), returns 64-char hex string."""
    data = (salt_hex + password).encode("utf-8")
    return hashlib.sha256(data).hexdigest()


def column_exists(cursor, table, column):
    """Check if a column exists in a table."""
    cursor.execute(f"PRAGMA table_info({table});")
    return any(row[1] == column for row in cursor.fetchall())


def migrate_drop_quiz_answer(cursor):
    """Drop quiz and answer columns from accounts table if present."""
    if not column_exists(cursor, "accounts", "quiz"):
        return False  # Already migrated

    print("  Dropping quiz/answer columns...")

    cursor.execute(
        "CREATE TABLE accounts_new ("
        " account_name TEXT PRIMARY KEY,"
        " password TEXT NOT NULL,"
        " email TEXT NOT NULL,"
        " created_at TEXT NOT NULL,"
        " password_changed_at TEXT NOT NULL,"
        " last_ip TEXT NOT NULL"
        ");"
    )

    cursor.execute(
        "INSERT INTO accounts_new(account_name, password, email, created_at, password_changed_at, last_ip) "
        "SELECT account_name, password, email, created_at, password_changed_at, last_ip FROM accounts;"
    )

    cursor.execute("DROP TABLE accounts;")
    cursor.execute("ALTER TABLE accounts_new RENAME TO accounts;")
    return True


def migrate_password_to_hash(cursor):
    """Hash plaintext passwords with SHA-256 + random salt."""
    if column_exists(cursor, "accounts", "password_hash"):
        return False  # Already migrated

    if not column_exists(cursor, "accounts", "password"):
        return False  # Nothing to migrate

    print("  Hashing plaintext passwords...")

    # Read all accounts
    cursor.execute(
        "SELECT account_name, password, email, created_at, password_changed_at, last_ip FROM accounts;"
    )
    rows = cursor.fetchall()

    # Prepare hashed data
    hashed_rows = []
    for name, password, email, created_at, pw_changed, last_ip in rows:
        salt_hex = generate_salt_hex()
        pw_hash = hash_password(password or "", salt_hex)
        hashed_rows.append((name, pw_hash, salt_hex, email, created_at, pw_changed, last_ip))

    # Create new table
    cursor.execute(
        "CREATE TABLE accounts_new ("
        " account_name TEXT PRIMARY KEY,"
        " password_hash TEXT NOT NULL,"
        " password_salt TEXT NOT NULL,"
        " email TEXT NOT NULL,"
        " created_at TEXT NOT NULL,"
        " password_changed_at TEXT NOT NULL,"
        " last_ip TEXT NOT NULL"
        ");"
    )

    cursor.executemany(
        "INSERT INTO accounts_new VALUES(?,?,?,?,?,?,?);",
        hashed_rows,
    )

    cursor.execute("DROP TABLE accounts;")
    cursor.execute("ALTER TABLE accounts_new RENAME TO accounts;")

    print(f"  Hashed {len(hashed_rows)} account(s)")
    return True


def update_schema_version(cursor, version):
    """Update the schema_version in meta table."""
    cursor.execute(
        "CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);"
    )
    cursor.execute(
        "INSERT OR REPLACE INTO meta(key, value) VALUES('schema_version', ?);",
        (str(version),),
    )


def migrate_db(db_path):
    """Run all migrations on a single account database."""
    print(f"Processing: {db_path}")

    # Create backup
    bak_path = db_path + ".bak"
    if not os.path.exists(bak_path):
        shutil.copy2(db_path, bak_path)
        print(f"  Backup created: {bak_path}")
    else:
        print(f"  Backup already exists: {bak_path}")

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        # Keep foreign_keys OFF during migration to prevent ON DELETE CASCADE
        # from wiping child tables when we DROP TABLE accounts
        cursor.execute("PRAGMA foreign_keys = OFF;")

        # Check if accounts table exists
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='accounts';")
        if not cursor.fetchone():
            print("  No accounts table found, skipping.")
            return

        changed = False

        # Migration 1: Drop quiz/answer
        if migrate_drop_quiz_answer(cursor):
            changed = True

        # Migration 2: Hash passwords
        if migrate_password_to_hash(cursor):
            changed = True

        if changed:
            update_schema_version(cursor, 6)
            conn.commit()
            print("  Migration complete.")
        else:
            print("  Already up to date.")

        # Re-enable foreign keys after migration
        cursor.execute("PRAGMA foreign_keys = ON;")

    except Exception as e:
        conn.rollback()
        print(f"  ERROR: {e}")
        raise
    finally:
        conn.close()


def main():
    accounts_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Accounts")

    if not os.path.isdir(accounts_dir):
        print(f"Accounts directory not found: {accounts_dir}")
        sys.exit(1)

    db_files = glob.glob(os.path.join(accounts_dir, "*.db"))
    if not db_files:
        print("No .db files found in Accounts/")
        sys.exit(0)

    print(f"Found {len(db_files)} database(s) to process.\n")

    success = 0
    failed = 0
    for db_path in sorted(db_files):
        try:
            migrate_db(db_path)
            success += 1
        except Exception:
            failed += 1
        print()

    print(f"Done. {success} succeeded, {failed} failed.")


if __name__ == "__main__":
    main()
