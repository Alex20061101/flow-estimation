import pandas as pd
import os
from pathlib import Path

# Config
BASE_DIR = Path("Data")
LENGTHS = [
    0.0, 485.46, 201.69, 165.82, 258.36, 243.31, 218.62, 243.70, 240.41, 91.69, 632.71,
    439.35, 365.94, 252.41, 322.35, 153.77, 206.52, 32.09, 197.33
]

def load_all_data_for_day(sensor_type, date_str, cls):
    # Load all cams for this date
    month = f"{date_str[:4]}-{date_str[4:6]}"
    cams = {}
    cam_dirs = os.listdir(BASE_DIR / sensor_type / month)
    for cam_dir in cam_dirs:
        cam_id = int(cam_dir.replace("Cam", "t").replace("Cam", "").replace("t","")) # handle t1, t2 or Cam01
        if cam_dir.startswith("t"):
             cam_id = int(cam_dir[1:])
        elif cam_dir.startswith("Cam"):
             cam_id = int(cam_dir[3:])
        
        path = BASE_DIR / sensor_type / month / cam_dir / f"{cam_dir}_{date_str}_daily.csv"
        if not path.exists():
            path = BASE_DIR / sensor_type / month / cam_dir / f"{cam_dir}_{date_str}_daily_count.csv"
        
        if path.exists():
            df = pd.read_csv(path)
            # Store hourly counts for each direction
            hourly = []
            for h in range(24):
                counts = {}
                row = df[df['hour'].astype(int) == h]
                if not row.empty:
                    for col in df.columns:
                        if cls in col.lower():
                            d = col.split('_')[0].lower()
                            counts[d] = row[col].values[0]
                hourly.append(counts)
            cams[cam_id] = hourly
    return cams

def avg_active(*args):
    vals = [v for v in args if v > 1e-6]
    return sum(vals)/len(vals) if vals else 0.0

def compute_bkm(cams, cls):
    total_bkm = 0
    # Day total
    for h in range(24):
        def g(cam, d):
            if cam not in cams: return 0.0
            return cams[cam][h].get(d, 0.0)
        
        # formulas (RGB simplified mapping)
        lx = [0]*19
        ly = [0]*19
        
        # Link 1: avg(3b3+b5, 2a2)
        lx[1] = avg_active(g(3, "b3") + g(3, "b5"), g(2, "a2"))
        ly[1] = avg_active(g(3, "b1") + g(3, "b2"), g(2, "a1"))
        # Link 2
        lx[2] = g(3, "b3") + g(3, "b4")
        ly[2] = g(3, "b1") + g(3, "b6")
        # Link 3
        lx[3] = avg_active(g(3, "b5") + g(3, "b6"), g(18, "a1"))
        ly[3] = avg_active(g(3, "b2") + g(3, "b4"), g(18, "a2"))
        # Link 4 (5or6)
        lx[4] = avg_active(g(5, "b3") + g(5, "b5"), g(6, "b3") + g(6, "b5"), g(8, "b3") + g(8, "b5"))
        ly[4] = avg_active(g(5, "b1") + g(5, "b2"), g(6, "b1") + g(6, "b2"), g(8, "b1") + g(8, "b2"))
        # Link 5
        lx[5] = avg_active(g(5, "b3") + g(5, "b4"), g(6, "b3") + g(6, "b4"), g(8, "b3") + g(8, "b4"))
        ly[5] = avg_active(g(5, "b1") + g(5, "b6"), g(6, "b1") + g(6, "b6"), g(8, "b1") + g(8, "b6"))
        # Link 6
        lx[6] = avg_active(g(5, "b5") + g(5, "b6"), g(6, "b5") + g(6, "b6"), g(8, "b5") + g(8, "b6"))
        ly[6] = avg_active(g(5, "b2") + g(5, "b4"), g(6, "b2") + g(6, "b4"), g(8, "b2") + g(8, "b4"))
        # Link 11
        lx[11] = g(2, "a1")
        ly[11] = g(2, "a2")
        # Link 12 (Derived)
        lx[12] = lx[2] - lx[4]
        ly[12] = ly[2] - ly[4]
        # Link 17
        lx[17] = lx[6] + (lx[5] - lx[6]) # Link 14 = L5-L6
        ly[17] = ly[12] + ly[18]
        # ... Other links 7,8,9,10,13,14,15,16,18
        lx[7] = avg_active(g(10, "b4") + g(10, "b5"), g(12, "a2"))
        ly[7] = avg_active(g(10, "b2") + g(10, "b6"), g(12, "a1"))
        lx[8] = lx[7]
        ly[8] = ly[7]
        lx[9] = g(13, "a1")
        ly[9] = g(13, "a2")
        lx[10] = g(18, "a1")
        ly[10] = g(18, "a2")
        lx[13] = g(10, "b5") + g(10, "b3")
        ly[13] = g(10, "b1") + g(10, "b2")
        lx[14] = lx[5] - lx[6]
        ly[14] = ly[5] - ly[6]
        lx[15] = g(16, "a1")
        ly[15] = g(16, "a2")
        lx[16] = g(16, "a2")
        ly[16] = g(16, "a1")
        lx[18] = g(15, "a1")
        ly[18] = g(15, "a2")
        
        for i in range(1, 19):
            total_bkm += (lx[i] + ly[i]) * LENGTHS[i]
            
    return total_bkm

dates = ["20260205", "20260206"]
for cls in ["person", "bicycle", "e-scooter"]:
    rgb_total = 0
    th_total = 0
    for d in dates:
        rgb_data = load_all_data_for_day("RGB", d, cls)
        th_data = load_all_data_for_day("Thermal", d, cls)
        rgb_total += compute_bkm(rgb_data, cls)
        th_total += compute_bkm(th_data, cls)
    
    print(f"Class: {cls}")
    print(f"  Total RGB BKM: {rgb_total:.2f}")
    print(f"  Total Thermal BKM: {th_total:.2f}")
