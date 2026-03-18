import pandas as pd
excel_path = r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx'
df = pd.read_excel(excel_path, sheet_name='Computing example for March', header=None)
# Look further right (columns 20+) and further down (rows 40+)
print("Columns 20-40:")
print(df.iloc[:50, 20:41])
print("\nRows 40-100:")
print(df.iloc[40:100, :20])
