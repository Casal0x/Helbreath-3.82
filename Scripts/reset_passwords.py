"""Reset all account passwords to 'admin1' using the new portable SHA-256 hash format."""
import hashlib
import os
import sqlite3
import secrets

ACCOUNTS_DIR = r"Z:\Helbreath-3.82\Binaries\Server\Accounts"
PASSWORD = "admin1"

def generate_salt_hex():
    """Generate 16 random bytes as hex string (32 chars)."""
    return secrets.token_hex(16)

def hash_password(password, salt_hex):
    """Hash password with salt using SHA-256 (matches C++ PasswordHash::HashPassword)."""
    # Input is: salt_hex + password (concatenated as strings)
    input_str = salt_hex + password
    h = hashlib.sha256(input_str.encode('ascii')).hexdigest()
    return h

def main():
    if not os.path.isdir(ACCOUNTS_DIR):
        print(f"Accounts directory not found: {ACCOUNTS_DIR}")
        return

    db_files = [f for f in os.listdir(ACCOUNTS_DIR) if f.endswith('.db')]
    if not db_files:
        print("No .db files found in Accounts directory.")
        return

    total_updated = 0
    for db_file in sorted(db_files):
        db_path = os.path.join(ACCOUNTS_DIR, db_file)
        try:
            conn = sqlite3.connect(db_path)
            cursor = conn.cursor()

            # Check if accounts table exists with password columns
            cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='accounts'")
            if not cursor.fetchone():
                conn.close()
                continue

            # Check column names
            cursor.execute("PRAGMA table_info(accounts)")
            columns = {row[1] for row in cursor.fetchall()}
            if 'password_hash' not in columns or 'password_salt' not in columns:
                conn.close()
                continue

            # Generate new salt and hash for admin1
            salt_hex = generate_salt_hex()
            hash_hex = hash_password(PASSWORD, salt_hex)

            # Update all accounts in this database
            cursor.execute("UPDATE accounts SET password_salt = ?, password_hash = ?", (salt_hex, hash_hex))
            updated = cursor.rowcount
            conn.commit()
            conn.close()

            if updated > 0:
                total_updated += updated
                print(f"  {db_file}: {updated} account(s) updated")
        except Exception as e:
            print(f"  {db_file}: ERROR - {e}")

    print(f"\nTotal: {total_updated} account(s) reset to password '{PASSWORD}'")

if __name__ == "__main__":
    main()
