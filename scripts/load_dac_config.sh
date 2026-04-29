#!/bin/bash

set -e

# Resolve script directory (works from anywhere)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CONFIG_SRC="$SCRIPT_DIR/../config/afe_dac_config.json"
CONFIG_DST="/etc/afe11612"
CONFIG_FILE="$CONFIG_DST/afe_config.json"

echo "Installing AFE config..."

# Check file exists
if [ ! -f "$CONFIG_SRC" ]; then
    echo "Error: config file not found: $CONFIG_SRC"
    exit 1
fi

# Create directory
sudo mkdir -p "$CONFIG_DST"

# Copy file
sudo cp "$CONFIG_SRC" "$CONFIG_FILE"

# Set permissions
sudo chmod 644 "$CONFIG_FILE"
sudo chown root:root "$CONFIG_FILE"

echo "Config installed to $CONFIG_FILE"