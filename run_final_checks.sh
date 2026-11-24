#!/bin/bash
set -euo pipefail

echo "================================="
echo " VALVEWISE FINAL VALIDATION"
echo "================================="

echo "===== CLEAN BUILD ====="
make clean
make all

echo "===== GRAPH + PARSER + STATE + API TESTS ====="
make test

echo "===== ADVANCED GRAPH TESTS ====="
make test-advanced

echo "===== FINAL SCENARIO TESTS ====="
python3 tests/test_final_validation.py

echo "===== REAL EPANET VALIDATION ====="
python3 tests/validate_real_network.py

echo "===== BENCHMARK ====="
make benchmark

echo "===== FRONTEND SYNTAX ====="

if command -v node >/dev/null 2>&1; then
    node --check frontend/js/network.js
    node --check frontend/js/comparison.js
    echo "JavaScript syntax passed."
else
    echo "Node unavailable; JavaScript syntax check skipped."
fi

echo "===== GENERATE REPORT ====="
python3 tests/generate_final_report.py

echo "===== VERIFY REPORT ====="
test -s docs/PERSON1_VALIDATION_REPORT.md
test -s build/final_benchmark.csv
test -s build/net3_failure_report.csv

echo "===== GIT COMMIT ====="

git add \
backend \
frontend \
tests \
docs \
Makefile \
README.md

if ! git diff --cached --quiet; then
    git commit -m "Complete Person 1 dashboard and final validation"
else
    echo "No new changes to commit."
fi

echo "===== GITHUB PUSH ====="
git push origin person1-network-engine

echo "===== FINAL STATUS ====="
git status
git log -1 --oneline

echo "================================="
echo " PERSON 1 VALIDATION COMPLETED"
echo "================================="
