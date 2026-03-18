import pandas as pd
excel_path = r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx'
df = pd.read_excel(excel_path, sheet_name='Computing example for March', header=None)
# Save to csv for easier reading if needed, or just print
print(df.head(50))
df.to_csv(r'C:\Users\alext\Projects\stf\march_computing_example.csv', index=False)
