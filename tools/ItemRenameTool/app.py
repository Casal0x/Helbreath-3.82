"""
Item Rename Tool for Helbreath GameConfigs.db
Hosts a web interface to rename items with update/revert support.
"""

import sqlite3
import os
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs

DB_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "Binaries", "Server", "GameConfigs.db")
BACKUP_FILE = os.path.join(os.path.dirname(__file__), "original_names.json")

def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def save_originals():
    """Save a snapshot of all item names for revert support."""
    conn = get_db()
    rows = conn.execute("SELECT item_id, name FROM items ORDER BY item_id").fetchall()
    conn.close()
    data = {str(r["item_id"]): r["name"] for r in rows}
    with open(BACKUP_FILE, "w") as f:
        json.dump(data, f, indent=2)
    return data

def load_originals():
    """Load the saved original names, creating the snapshot if it doesn't exist."""
    if not os.path.exists(BACKUP_FILE):
        return save_originals()
    with open(BACKUP_FILE, "r") as f:
        return json.load(f)

HTML_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Item Rename Tool</title>
<style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
        font-family: 'Segoe UI', Tahoma, sans-serif;
        background: #1a1a2e;
        color: #e0e0e0;
        min-height: 100vh;
    }
    .header {
        background: #16213e;
        padding: 16px 24px;
        border-bottom: 2px solid #0f3460;
        display: flex;
        justify-content: space-between;
        align-items: center;
        position: sticky;
        top: 0;
        z-index: 100;
    }
    .header h1 {
        font-size: 20px;
        color: #e94560;
    }
    .header-actions {
        display: flex;
        gap: 8px;
        align-items: center;
    }
    .search-box {
        padding: 8px 12px;
        background: #1a1a2e;
        border: 1px solid #0f3460;
        color: #e0e0e0;
        border-radius: 4px;
        font-size: 14px;
        width: 250px;
    }
    .search-box:focus { outline: none; border-color: #e94560; }
    .btn {
        padding: 8px 16px;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        font-size: 13px;
        font-weight: 600;
        transition: background 0.2s;
    }
    .btn-save { background: #28a745; color: #fff; }
    .btn-save:hover { background: #218838; }
    .btn-revert { background: #dc3545; color: #fff; }
    .btn-revert:hover { background: #c82333; }
    .btn-revert-one { background: #6c757d; color: #fff; font-size: 11px; padding: 4px 8px; }
    .btn-revert-one:hover { background: #5a6268; }
    .btn-snapshot { background: #17a2b8; color: #fff; }
    .btn-snapshot:hover { background: #138496; }
    .status {
        padding: 8px 16px;
        margin: 0;
        font-size: 13px;
        text-align: center;
        display: none;
    }
    .status.success { display: block; background: #155724; color: #d4edda; }
    .status.error { display: block; background: #721c24; color: #f8d7da; }
    .container { max-width: 900px; margin: 0 auto; padding: 16px; }
    .gender-badge {
        display: inline-block;
        padding: 2px 8px;
        border-radius: 3px;
        font-size: 11px;
        font-weight: 600;
        text-align: center;
        min-width: 50px;
    }
    .gender-any { background: #333; color: #888; }
    .gender-male { background: #1a3a5c; color: #5b9bd5; }
    .gender-female { background: #5c1a3a; color: #d55b9b; }
    .item-table {
        width: 100%;
        border-collapse: collapse;
    }
    .item-table thead th {
        background: #16213e;
        padding: 10px 12px;
        text-align: left;
        font-size: 12px;
        text-transform: uppercase;
        color: #888;
        border-bottom: 2px solid #0f3460;
        position: sticky;
        top: 57px;
        z-index: 50;
    }
    .item-table tbody tr {
        border-bottom: 1px solid #222244;
    }
    .item-table tbody tr:hover { background: #1e1e3a; }
    .item-table tbody tr.modified { background: #2a2a1a; }
    .item-table td { padding: 6px 12px; }
    .item-table td:first-child {
        width: 60px;
        color: #888;
        font-size: 13px;
    }
    .item-table td:nth-child(2) {
        width: 200px;
        color: #666;
        font-size: 12px;
    }
    .name-input {
        width: 100%;
        padding: 6px 8px;
        background: #12122a;
        border: 1px solid #333;
        color: #e0e0e0;
        border-radius: 3px;
        font-size: 14px;
    }
    .name-input:focus { outline: none; border-color: #e94560; }
    .name-input.changed { border-color: #ffc107; }
    .changes-count {
        background: #e94560;
        color: #fff;
        border-radius: 12px;
        padding: 2px 8px;
        font-size: 12px;
        display: none;
    }
    .hidden { display: none; }
</style>
</head>
<body>
<div class="header">
    <h1>Item Rename Tool</h1>
    <div class="header-actions">
        <span class="changes-count" id="changesCount">0 changes</span>
        <input type="text" class="search-box" id="searchBox" placeholder="Search items...">
        <button class="btn btn-snapshot" onclick="takeSnapshot()">New Snapshot</button>
        <button class="btn btn-revert" onclick="revertAll()">Revert All</button>
        <button class="btn btn-save" onclick="saveAll()">Save Changes</button>
    </div>
</div>
<div class="status" id="statusBar"></div>
<div class="container">
    <table class="item-table">
        <thead>
            <tr>
                <th>ID</th>
                <th>Original Name</th>
                <th>Current Name</th>
                <th></th>
                <th>Gender</th>
            </tr>
        </thead>
        <tbody id="itemsBody"></tbody>
    </table>
</div>

<script>
let items = [];
let originals = {};

async function loadItems() {
    const resp = await fetch('/api/items');
    const data = await resp.json();
    items = data.items;
    originals = data.originals;
    renderItems();
}

function renderItems() {
    const filter = document.getElementById('searchBox').value.toLowerCase();
    const tbody = document.getElementById('itemsBody');
    tbody.innerHTML = '';
    let count = 0;
    for (const item of items) {
        const orig = originals[String(item.item_id)] || item.name;
        const isModified = item.name !== orig;
        const genderLabel = item.gender_limit === 1 ? 'Male' : item.gender_limit === 2 ? 'Female' : 'Any';
        const genderClass = item.gender_limit === 1 ? 'gender-male' : item.gender_limit === 2 ? 'gender-female' : 'gender-any';
        if (filter && !item.name.toLowerCase().includes(filter)
            && !orig.toLowerCase().includes(filter)
            && !String(item.item_id).includes(filter)
            && !genderLabel.toLowerCase().includes(filter)) continue;
        const tr = document.createElement('tr');
        if (isModified) { tr.className = 'modified'; count++; }
        tr.innerHTML = `
            <td>${item.item_id}</td>
            <td>${escapeHtml(orig)}</td>
            <td><input type="text" class="name-input ${isModified ? 'changed' : ''}"
                value="${escapeAttr(item.name)}"
                data-id="${item.item_id}" data-orig="${escapeAttr(orig)}"
                oninput="onNameChange(this)"></td>
            <td>${isModified ? '<button class=\\'btn btn-revert-one\\' onclick=\\'revertOne(this, ' + item.item_id + ')\\'>Revert</button>' : ''}</td>
            <td><span class="gender-badge ${genderClass}">${genderLabel}</span></td>
        `;
        tbody.appendChild(tr);
    }
    const badge = document.getElementById('changesCount');
    if (count > 0) {
        badge.textContent = count + ' change' + (count > 1 ? 's' : '');
        badge.style.display = 'inline';
    } else {
        badge.style.display = 'none';
    }
}

function onNameChange(input) {
    const id = parseInt(input.dataset.id);
    const orig = input.dataset.orig;
    const item = items.find(i => i.item_id === id);
    if (item) item.name = input.value;
    input.className = 'name-input' + (input.value !== orig ? ' changed' : '');
    const tr = input.closest('tr');
    tr.className = input.value !== orig ? 'modified' : '';
    updateChangesCount();
}

function updateChangesCount() {
    let count = 0;
    for (const item of items) {
        const orig = originals[String(item.item_id)] || item.name;
        if (item.name !== orig) count++;
    }
    const badge = document.getElementById('changesCount');
    if (count > 0) {
        badge.textContent = count + ' change' + (count > 1 ? 's' : '');
        badge.style.display = 'inline';
    } else {
        badge.style.display = 'none';
    }
}

async function saveAll() {
    const changes = [];
    for (const item of items) {
        const orig = originals[String(item.item_id)] || item.name;
        if (item.name !== orig) {
            changes.push({ item_id: item.item_id, name: item.name });
        }
    }
    if (changes.length === 0) {
        showStatus('No changes to save.', 'error');
        return;
    }
    const resp = await fetch('/api/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ changes })
    });
    const data = await resp.json();
    if (data.ok) {
        showStatus('Saved ' + changes.length + ' item(s).', 'success');
        await loadItems();
    } else {
        showStatus('Error: ' + data.error, 'error');
    }
}

async function revertAll() {
    if (!confirm('Revert ALL item names to their original snapshot values?')) return;
    const resp = await fetch('/api/revert', { method: 'POST' });
    const data = await resp.json();
    if (data.ok) {
        showStatus('Reverted ' + data.count + ' item(s) to original names.', 'success');
        await loadItems();
    } else {
        showStatus('Error: ' + data.error, 'error');
    }
}

function revertOne(btn, id) {
    const orig = originals[String(id)];
    if (!orig) return;
    const item = items.find(i => i.item_id === id);
    if (item) item.name = orig;
    const tr = btn.closest('tr');
    const input = tr.querySelector('.name-input');
    input.value = orig;
    input.className = 'name-input';
    tr.className = '';
    btn.remove();
    updateChangesCount();
}

async function takeSnapshot() {
    if (!confirm('Save current database names as the new revert snapshot? This replaces the previous snapshot.')) return;
    const resp = await fetch('/api/snapshot', { method: 'POST' });
    const data = await resp.json();
    if (data.ok) {
        showStatus('New snapshot saved with ' + data.count + ' items.', 'success');
        await loadItems();
    } else {
        showStatus('Error: ' + data.error, 'error');
    }
}

function showStatus(msg, type) {
    const bar = document.getElementById('statusBar');
    bar.textContent = msg;
    bar.className = 'status ' + type;
    setTimeout(() => { bar.className = 'status'; }, 4000);
}

function escapeHtml(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
function escapeAttr(s) {
    return s.replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

document.getElementById('searchBox').addEventListener('input', renderItems);
loadItems();
</script>
</body>
</html>
"""

class RequestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[{self.log_date_time_string()}] {format % args}")

    def do_GET(self):
        if self.path == "/":
            self._send_html(HTML_PAGE)
        elif self.path == "/api/items":
            self._send_items()
        else:
            self._send_error(404, "Not found")

    def do_POST(self):
        if self.path == "/api/update":
            self._handle_update()
        elif self.path == "/api/revert":
            self._handle_revert()
        elif self.path == "/api/snapshot":
            self._handle_snapshot()
        else:
            self._send_error(404, "Not found")

    def _send_html(self, html):
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(html.encode())

    def _send_json(self, data, status=200):
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def _send_error(self, code, msg):
        self._send_json({"ok": False, "error": msg}, code)

    def _read_body(self):
        length = int(self.headers.get("Content-Length", 0))
        return self.rfile.read(length)

    def _send_items(self):
        conn = get_db()
        rows = conn.execute("SELECT item_id, name, gender_limit FROM items ORDER BY item_id").fetchall()
        conn.close()
        originals = load_originals()
        items = [{"item_id": r["item_id"], "name": r["name"], "gender_limit": r["gender_limit"]} for r in rows]
        self._send_json({"items": items, "originals": originals})

    def _handle_update(self):
        try:
            body = json.loads(self._read_body())
            changes = body.get("changes", [])
            if not changes:
                self._send_json({"ok": False, "error": "No changes provided"})
                return
            conn = get_db()
            for c in changes:
                item_id = int(c["item_id"])
                name = str(c["name"]).strip()
                if not name:
                    conn.close()
                    self._send_json({"ok": False, "error": f"Item {item_id}: name cannot be empty"})
                    return
                conn.execute("UPDATE items SET name = ? WHERE item_id = ?", (name, item_id))
            conn.commit()
            conn.close()
            self._send_json({"ok": True, "count": len(changes)})
        except Exception as e:
            self._send_json({"ok": False, "error": str(e)})

    def _handle_revert(self):
        try:
            originals = load_originals()
            conn = get_db()
            count = 0
            for item_id_str, name in originals.items():
                item_id = int(item_id_str)
                cur = conn.execute("SELECT name FROM items WHERE item_id = ?", (item_id,)).fetchone()
                if cur and cur["name"] != name:
                    conn.execute("UPDATE items SET name = ? WHERE item_id = ?", (name, item_id))
                    count += 1
            conn.commit()
            conn.close()
            self._send_json({"ok": True, "count": count})
        except Exception as e:
            self._send_json({"ok": False, "error": str(e)})

    def _handle_snapshot(self):
        try:
            data = save_originals()
            self._send_json({"ok": True, "count": len(data)})
        except Exception as e:
            self._send_json({"ok": False, "error": str(e)})

def main():
    port = 8080
    print(f"Item Rename Tool")
    print(f"Database: {os.path.abspath(DB_PATH)}")
    print(f"Serving on http://localhost:{port}")
    server = HTTPServer(("127.0.0.1", port), RequestHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")
        server.server_close()

if __name__ == "__main__":
    main()
