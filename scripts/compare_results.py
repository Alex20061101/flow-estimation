import os
import pandas as pd
import glob

def compare():
    old_files = glob.glob('Old_Results/*/Links/*.csv')
    diff_count = 0
    match_count = 0
    
    with open('benchmark_report.txt', 'w') as out:
        for old_path in old_files:
            # old_path: Old_Results/2025-01/Links/...
            new_path = old_path.replace('Old_Results', 'Results')
            
            if not os.path.exists(new_path):
                out.write(f"MISSING IN NEW RUN: {new_path}\n")
                diff_count += 1
                continue
                
            df_old = pd.read_csv(old_path)
            df_new = pd.read_csv(new_path)
            
            try:
                pd.testing.assert_frame_equal(df_old, df_new, check_exact=False, atol=1e-2)
                match_count += 1
            except AssertionError as e:
                diff_count += 1
                out.write(f"DIFFERENCE FOUND IN: {new_path}\n{e}\n\n")
                
        out.write(f"\n--- Benchmark Summary ---\n")
        out.write(f"Perfect Matches: {match_count}\n")
        out.write(f"Differences: {diff_count}\n")
        print(f"Benchmark complete. Matches: {match_count}, Diffs: {diff_count}")

if __name__ == '__main__':
    compare()
