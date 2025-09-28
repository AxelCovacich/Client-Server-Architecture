#!/bin/bash
set -e

# Create dedicated user if not created yet, named serveruser
if ! id "serveruser" &>/dev/null; then
    sudo useradd --system --no-create-home --shell /usr/sbin/nologin serveruser
fi

# Grant permissions to the dedicated user over the repo
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
sudo chown -R serveruser:serveruser "$REPO_ROOT"

# Copy systemd unit file and edit paths if necessary
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
sudo cp "$REPO_ROOT/server.service" /etc/systemd/system/server.service
echo "Edit /etc/systemd/system/server.service to match your repo folder paths."

# Reload systemd and enable the service
sudo systemctl daemon-reload
sudo systemctl enable server.service
echo "Done. Use 'sudo systemctl start server.service' to start the server."