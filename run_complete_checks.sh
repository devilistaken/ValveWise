#!/bin/bash
set -euo pipefail

echo "===== BUILD PERSON 1 ENGINE ====="
make all

echo "===== BUILD OPTIMIZER ENGINE ====="
make -C integration/reference/teammate/backend

echo "===== C AND API TESTS ====="
make test
make test-advanced

echo "===== OPTIMIZER IMPROVEMENT ====="
python3 tests/test_optimizer_improvement.py

echo "===== UNIFIED API TESTS ====="
python3 -m unittest discover -s tests -p 'test_unified.py' -v

echo "===== COMPLETE INTEGRATION ====="
if [ -f tests/test_complete_integration.py ]; then
    python3 -m unittest discover -s tests -p 'test_complete_integration.py' -v
else
    echo "Complete integration test file missing"
    exit 1
fi

echo "===== JAVASCRIPT SYNTAX ====="
node --check frontend/js/unified.js
node --check frontend/js/network.js
node --check frontend/js/comparison.js
node --check frontend/js/optimizer.js

echo "===== FINAL VALIDATION ====="
python3 tests/test_final_validation.py

echo "===== SUCCESS ====="
echo "ALL REQUIRED CHECKS PASSED"
