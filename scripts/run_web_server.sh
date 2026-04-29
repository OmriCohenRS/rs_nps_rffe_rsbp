#!/usr/bin/env bash

# ------------------------------------------------------------
# Run AFE11612 Web App
# ------------------------------------------------------------

# Exit immediately on error
set -e

# Resolve repo root (script may be run from anywhere)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

WEB_APP_DIR="$REPO_ROOT/front_end/afe11612_web_app"

# Sanity checks
if [ ! -d "$WEB_APP_DIR" ]; then
  echo "ERROR: Web app directory not found:"
  echo "  $WEB_APP_DIR"
  exit 1
fi

if [ ! -f "$WEB_APP_DIR/server.py" ]; then
  echo "ERROR: server.py not found in web app directory"
  exit 1
fi

echo "Starting AFE11612 web server"
echo "Repo root:    $REPO_ROOT"
echo "Web app dir:  $WEB_APP_DIR"
echo

# Go to web app directory
cd "$WEB_APP_DIR"

# Run uvicorn in hardware-safe mode
exec python3 -m uvicorn server:app \
  --host 0.0.0.0 \
  --port 8000 \
  --loop asyncio \
  --workers 1