#!/bin/bash
set -euo pipefail

make all
make -C integration/reference/teammate/backend

python3 -m unittest discover \
-s tests -p 'test_unified.py' -v

python3 backend/api/server.py
