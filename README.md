# Flow Estimation Pipeline

This repository contains a suite of C++ and Python scripts designed to aggregate, parse, and compute link-level traffic volumes from raw Thermal and RGB camera counts. The system uses sophisticated data-filling logic to ensure complete 168-hour weekly profiles for traffic modeling.

## 🚀 Key Features
*   **Automated Hourly Fallback:** If a specific day or hour is missing from a camera's data, the engine automatically searches for the same day-of-week and hour across other weeks in the same month to "fill in" the gap.
*   **Modular Architecture:** Strict segregation of `Data/` (Inputs) and `Results/` (Outputs) categorized by months.
*   **Dual-Source Comparison:** Real-time processing of both Thermal and RGB sensor data for verification.
*   **Integrated Heatmaps:** Visual tools to quickly identify which sensors have missing data.
*   **8760-Hour Yearly Generator:** Compiled engine to process two full years (2025-2026) of hourly data for traffic modeling.

---

## 📂 Directory Structure

### 📥 Data (Inputs)
All raw daily camera CSV files must be placed here.
*   `Data/Thermal/[YYYY-MM]/CamXX/` — Thermal sensors folder structure.
*   `Data/RGB/[YYYY-MM]/CamXX/` — RGB sensors folder structure.

### 📤 Results (Outputs)
Processed summary reports and flow estimations.
*   `Results/[YYYY-MM]/Links/` — Link summary CSVs (Bicycles, Persons, E-scooters).
*   `Results/[YYYY-MM]/Diff/` — Comparison reports between sensors.
*   `Results/heatmap.html` — Cross-month data availability visual.

### 📦 Old_Results (Archive)
*   Legacy data previously generated from older versions of the engine has been archived here for historical benchmarking.

---

## 🛠 Usage & Workflow

### 1. Organize New Data
When you receive new raw zip files or unorganized CSVs, run the organizer:
```bash
python scripts/organize_data_months.py
```
This utility automatically parses the dates in the filenames and moves them into the correct `Data/` subfolders.

### 2. Verify Data Coverage
Review any gaps in your input dataset visually:
```bash
python scripts/generate_heatmap.py
```
Open `Results/heatmap.html` in your browser. Green checks indicate successful data capture.

### 3. Build the Engine
Requires C++23. Compilation using `g++` (via WSL on Windows) is recommended:
```bash
wsl g++ -std=c++23 -O3 -o main main.cpp
wsl g++ -std=c++23 -O3 -o database_gen main.cpp
wsl g++ -std=c++23 -O3 -o diff diff.cpp
wsl g++ -std=c++23 -O3 -o rgb_link_diff rgb_link_diff.cpp
```

### 4. Generate Reports

#### Individual Month Run
Run the engine and follow the prompts:
```bash
wsl ./main
```
1.  **Select Class:** 1 (Person), 2 (Bicycle), 3 (Scooter).
2.  **Select Month:** Input in `YYYY-MM` format (e.g., `2026-02`).

#### 8760-Hour Yearly Database Generation
To generate the massive 2025–2026 hourly database (17,520 hours per class):
```bash
wsl ./database_gen database [class_number]
```
-   Class 1: Person
-   Class 2: Bicycle
-   Class 3: E-scooter

The outputs are saved as `Results/Hourly_Database_RGB_[class].csv`, `Results/Hourly_Database_Thermal_[class].csv`, and `Results/Hourly_Comparison_MAPE_[class].csv`.

### 📊 Metric Definitions
-   **BKM (Link Volume):** `(Forward_Direction + Backward_Direction) * Link_Length`.
-   **MAPE (Difference %):** `(Thermal_BKM - RGB_BKM) / RGB_BKM`.
-   **Active Sensor Fallback:** If one camera at a site is inactive (0 data), the system automatically uses the other camera as the primary source rather than averaging with zero.

---

## 🧬 Understanding the Fallback Logic
The `main.cpp` engine is designed to generate a complete Monday-through-Sunday profile.
1.  **Primary Search:** It looks for the first full week of the selected month.
2.  **Row Fallback:** If a specific hour (e.g., 2:00 AM) returns `0` counts across all directions, it enters "Deep Search" mode.
3.  **Cross-Week Substitution:** It iterates through every other instance of that same day-of-week within the current month, beginning with the closest week.
4.  **Completion:** It adopts the first available non-zero data set for that hour, ensuring the final report reflects real traffic patterns even if a single day's file was corrupted.
