#!/usr/bin/env python3
"""Simple HTTP file server for the Helbreath auto-updater.

Serves a directory over HTTP with CORS headers.
Settings are saved to update_server.json — delete it to reconfigure.
"""

import json
import os
import sys
import time
from http.server import HTTPServer, SimpleHTTPRequestHandler
from functools import partial


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(SCRIPT_DIR, "update_server.json")

DEFAULTS = {
    "directory": "Binaries/Game",
    "host": "0.0.0.0",
    "port": 8080,
    "rate_limit_mbps": 10,
}


class UpdateRequestHandler(SimpleHTTPRequestHandler):
    """HTTP handler with CORS support, rate limiting, and cleaner logging."""

    rate_limit_bps = 0

    def copyfile(self, source, outputfile):
        """Rate-limited file copy."""
        if self.rate_limit_bps <= 0:
            return super().copyfile(source, outputfile)

        chunk_size = max(4096, self.rate_limit_bps // 10)  # ~100ms chunks
        interval = chunk_size / self.rate_limit_bps

        while True:
            start = time.monotonic()
            buf = source.read(chunk_size)
            if not buf:
                break
            outputfile.write(buf)
            elapsed = time.monotonic() - start
            sleep_time = interval - elapsed
            if sleep_time > 0:
                time.sleep(sleep_time)

    def end_headers(self):
        # Permissive CORS for dev use
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, HEAD, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Range")
        super().end_headers()

    def list_directory(self, path):
        self.send_error(403, "Directory listing disabled")
        return None

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def log_message(self, format, *args):
        # Cleaner log format
        print(f"[{self.log_date_time_string()}] {args[0]}" if args else "")


def resolve_path(path: str) -> str:
    """Resolve a path relative to the script's directory, not CWD."""
    path = path.strip("\"'")
    if os.path.isabs(path):
        return os.path.normpath(path)
    return os.path.normpath(os.path.join(SCRIPT_DIR, path))


def prompt(text: str, default: str = "") -> str:
    if default:
        result = input(f"{text} [{default}]: ").strip()
        return result if result else default
    return input(f"{text}: ").strip()


def load_config() -> dict | None:
    if not os.path.isfile(CONFIG_PATH):
        return None
    try:
        with open(CONFIG_PATH) as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError):
        return None


def save_config(cfg: dict):
    with open(CONFIG_PATH, "w", newline="\n") as f:
        json.dump(cfg, f, indent=4)
    print(f"  Config saved to {CONFIG_PATH}\n")


def run_setup() -> dict:
    """Interactive first-time setup. Returns config dict."""
    print("  No config found — running first-time setup.")
    print(f"  (Delete {CONFIG_PATH} to reconfigure)\n")

    # Directory
    directory = prompt("Directory to serve", DEFAULTS["directory"])
    while True:
        resolved = resolve_path(directory)
        if not os.path.isdir(resolved):
            print(f"  '{resolved}' is not a valid directory.")
            directory = prompt("Directory to serve")
            continue

        print(f"  Resolved: {resolved}")
        ok = prompt("  Correct? (y/n)", "y")
        if ok.lower() in ("y", "yes"):
            directory = resolved
            break
        directory = prompt("Directory to serve")

    # Rate limit
    rate_str = prompt("Rate limit MB/s (0 = unlimited)", str(DEFAULTS["rate_limit_mbps"]))
    while True:
        try:
            rate_mbps = float(rate_str)
            if rate_mbps >= 0:
                break
        except ValueError:
            pass
        print("  Invalid number.")
        rate_str = prompt("Rate limit MB/s (0 = unlimited)", str(DEFAULTS["rate_limit_mbps"]))

    # Host / Port
    host = prompt("Host", DEFAULTS["host"])
    port_str = prompt("Port", str(DEFAULTS["port"]))
    while not port_str.isdigit() or not (1 <= int(port_str) <= 65535):
        print("  Invalid port number.")
        port_str = prompt("Port", str(DEFAULTS["port"]))

    cfg = {
        "directory": directory,
        "host": host,
        "port": int(port_str),
        "rate_limit_mbps": rate_mbps,
    }
    save_config(cfg)
    return cfg


def main():
    print("=== Helbreath Update Server ===")
    print(f"  Working from: {SCRIPT_DIR}\n")

    cfg = load_config()
    if cfg is None:
        cfg = run_setup()
    else:
        print(f"  Loaded config from {CONFIG_PATH}")

    directory = cfg.get("directory", DEFAULTS["directory"])
    host = cfg.get("host", DEFAULTS["host"])
    port = cfg.get("port", DEFAULTS["port"])
    rate_mbps = cfg.get("rate_limit_mbps", DEFAULTS["rate_limit_mbps"])

    # Resolve directory
    directory = resolve_path(directory)
    if not os.path.isdir(directory):
        print(f"  Error: '{directory}' is not a valid directory.")
        print(f"  Delete {CONFIG_PATH} to reconfigure.")
        return 1

    # Check for manifest
    manifest_path = os.path.join(directory, "update.manifest.json")
    if not os.path.isfile(manifest_path):
        print(f"\n  Warning: No update.manifest.json found in {directory}")
        print("  Run gen_update_manifest.py first to generate one.")

    # Apply rate limit
    if rate_mbps > 0:
        UpdateRequestHandler.rate_limit_bps = int(rate_mbps * 1024 * 1024)
    else:
        UpdateRequestHandler.rate_limit_bps = 0

    # Start
    abs_dir = os.path.abspath(directory)
    rate_display = f"{rate_mbps} MB/s" if rate_mbps > 0 else "unlimited"
    print(f"\n  Directory:  {abs_dir}")
    print(f"  Rate limit: {rate_display}")
    print(f"  Listening:  http://{host}:{port}")
    print("  Press Ctrl+C to stop\n")

    handler = partial(UpdateRequestHandler, directory=directory)
    server = HTTPServer((host, port), handler)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down")
        server.shutdown()

    return 0


if __name__ == "__main__":
    sys.exit(main())
