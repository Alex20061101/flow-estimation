import os
import shutil
import re

MONTH_MAP = {
    'January': '01', 'February': '02', 'March': '03', 'April': '04',
    'May': '05', 'June': '06', 'July': '07', 'August': '08',
    'September': '09', 'October': '10', 'November': '11', 'December': '12'
}

def migrate():
    legacy_dirs = ['Monthly_Links_Thermal', 'Monthly_Links_Ground_Truth']
    
    for l_dir in legacy_dirs:
        if not os.path.exists(l_dir):
            continue
            
        label = "Thermal" if "Thermal" in l_dir else "GroundTruth"
        
        for root, dirs, files in os.walk(l_dir):
            for file in files:
                if not file.endswith('.csv'): continue
                
                # root is something like 'Monthly_Links_Thermal\\bicycle'
                class_name = os.path.basename(root)
                
                # file is something like 'Links_Thermal_April.csv'
                m = re.search(r'Links_.*?_([A-Za-z]+)\.csv', file)
                if m:
                    month_english = m.group(1)
                    if month_english in MONTH_MAP:
                        month_num = MONTH_MAP[month_english]
                        yyyymm = f"2025-{month_num}"
                        
                        # Target: Results/2025-04/Links/Links_Thermal_bicycle_2025-04.csv
                        target_dir = os.path.join("Results", yyyymm, "Links")
                        os.makedirs(target_dir, exist_ok=True)
                        
                        target_filename = f"Links_{label}_{class_name}_{yyyymm}.csv"
                        target_path = os.path.join(target_dir, target_filename)
                        
                        src_path = os.path.join(root, file)
                        shutil.move(src_path, target_path)
                        print(f"Moved: {src_path} -> {target_path}")
                        
        # Delete tree if empty
        for root, dirs, files in os.walk(l_dir, topdown=False):
            if not os.listdir(root):
                os.rmdir(root)
                
if __name__ == '__main__':
    migrate()
