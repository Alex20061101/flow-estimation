#!/bin/bash
set -e

months=("2025-01" "2025-02" "2025-03" "2025-04" "2026-01" "2026-02" "2026-03")
classes=("1" "2" "3")

g++ -std=c++23 -O2 -o main main.cpp

for m in "${months[@]}"; do
  for c in "${classes[@]}"; do
    echo "Processing class=$c month=$m"
    echo -e "$c\n$m" | ./main 2>&1 | tail -3
    echo "---"
  done
done

echo "ALL DONE"
