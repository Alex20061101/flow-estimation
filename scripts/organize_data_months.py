import os
import shutil
import re

def organize_by_month(base_path):
    if not os.path.exists(base_path): return
    for ctype in ['Thermal', 'RGB']:
        ctype_path = os.path.join(base_path, ctype)
        if not os.path.exists(ctype_path): continue
        
        # Collect all raw csv files out of current structure
        all_csvs = []
        for root, dirs, files in os.walk(ctype_path):
            for file in files:
                if file.endswith('.csv'):
                    all_csvs.append(os.path.join(root, file))
                    
        # Move them to correct month and camera folder
        for src in all_csvs:
            filename = os.path.basename(src)
            m = re.search(r'(Cam\w+)_(\d{4})(\d{2})\d{2}_', filename)
            if m:
                cam = m.group(1)
                year = m.group(2)
                month = m.group(3)
                month_str = f"{year}-{month}"
                
                dest_dir = os.path.join(ctype_path, month_str, cam)
                os.makedirs(dest_dir, exist_ok=True)
                dest = os.path.join(dest_dir, filename)
                
                # Only move if not already perfectly in place
                if src != dest:
                    shutil.move(src, dest)
                    
        # Cleanup empty directories
        for root, dirs, files in os.walk(ctype_path, topdown=False):
            if root == ctype_path: continue
            if not os.listdir(root):
                os.rmdir(root)

if __name__ == '__main__':
    organize_by_month(r'Data')
