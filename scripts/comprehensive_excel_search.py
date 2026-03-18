import pandas as pd
excel_path = r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx'
df = pd.read_excel(excel_path, sheet_name='Computing example for March', header=None)
# Find rows containing "Thermal"
thermal_rows = df[df.apply(lambda row: row.astype(str).str.contains('Thermal', case=False).any(), axis=1)]
print("Rows with 'Thermal':")
print(thermal_rows)

# If no "Thermal" found, let's just print every 10th row label
print("\nRow 0-200 labels (Col 0 and 1):")
print(df.iloc[0:200, 0:2])
