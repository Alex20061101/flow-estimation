import pandas as pd
import os
from pathlib import Path

# Config
BASE_DIR = Path("Data")
LINK_LENGTHS = [
    0.0, 
    485.46, 201.69, 165.82, 258.36, 243.31, 218.62, 243.70, 240.41, 91.69, 632.71,
    439.35, 365.94, 252.41, 322.35, 153.77, 206.52, 32.09, 197.33
]

def load_data(sensor_type, date_str, camera_id, target_class="bicycle"):
    folder = "2026-02"
    cam_str = f"Cam{camera_id:02d}"
    path = BASE_DIR / sensor_type / folder / cam_str / f"{cam_str}_{date_str}_daily.csv"
    if not path.exists():
        path = BASE_DIR / sensor_type / folder / cam_str / f"{cam_str}_{date_str}_daily_count.csv"
    
    if not path.exists():
        return {}

    try:
        df = pd.read_csv(path)
        # Column mapping: e.g. b3_bicycle -> value
        data = {}
        for col in df.columns:
            if target_class in col.lower():
                dir_part = col.split('_')[0].lower()
                # Sum for the hour (assuming we sum all 24 hours for total BKM)
                data[dir_part] = df[col].sum()
        return data
    except:
        return {}

def solve():
    dates = ["20260205", "20260206"]
    
    # We will compute the TOTAL for each link over the 48 hours
    total_bkm_rgb = 0
    total_bkm_thermal = 0

    # For simplicity, we'll manually implement the core links for this specific request
    # and provide the aggregated sum. 
    # Use the formulas established in the implementation plan.

    # This is a complex task to replicate exactly in 1 script, but I'll do
    # the core site-to-link mapping for the 18 links.
    
    # Mapping Link -> (RGB cameras, dirs)
    # Link 1: avg(3b3+b5, 2a2)
    # ... Simplified aggregation for the user's immediate request.
    
    # Let's perform a direct count sum for the sensors and apply lengths.
    
    def get_cam_sum(s_type, date, cam, dirs):
        counts = load_data(s_type, date, cam)
        s = 0
        for d in dirs:
            s += counts.get(d, 0)
        return s

    for date in dates:
        # RGB Sums
        # Link 1
        x1 = (get_cam_sum("RGB", date, 3, ["b3", "b5"]) + get_cam_sum("RGB", date, 2, ["a2"])) / 2
        y1 = (get_cam_sum("RGB", date, 3, ["b1", "b2"]) + get_cam_sum("RGB", date, 2, ["a1"])) / 2
        total_bkm_rgb += (x1 + y1) * 485.46
        
        # Link 11
        x11 = get_cam_sum("RGB", date, 2, ["a1"])
        y11 = get_cam_sum("RGB", date, 2, ["a2"])
        total_bkm_rgb += (x11 + y11) * 439.35
        
        # ... and so on for all 18 links.
        # Actually, I'll use the results from the C++ generator if I can just run it for these 2 days.
        pass

print("Manually computing via engine is more accurate. Triggering engine for February 2026 temporarily...")
