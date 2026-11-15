#!/usr/bin/env bash
# Requires preview package installed plus Xvfb/xauth development test dependencies.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p reports
barnyard-linux-preview --version
barnyard-linux-preview --status > reports/installed-preview-status.json
dpkg -V barnyard-linux-preview
desktop-file-validate /usr/share/applications/barnyard-linux-preview.desktop
BARNYARD_PREVIEW_TEST_DIR=/usr/lib/barnyard-linux-preview \
  xvfb-run -a -s '-screen 0 1280x720x24' /usr/bin/python3 -B tests/test_preview_gui.py \
  > reports/installed-preview-gui-tests.txt 2>&1
xvfb-run -a -s '-screen 0 1280x720x24' barnyard-linux-gpu-check --frames 5 --hidden > reports/installed-gles-probe.json
cat reports/installed-preview-gui-tests.txt
cat reports/installed-gles-probe.json
echo 'PASS: installed preview tools; NOT gameplay or X270 hardware validation.'
