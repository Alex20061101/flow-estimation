import pandas as pd
excel_path = r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx'
df = pd.read_excel(excel_path, sheet_name='March 1st', header=None)
print("March 1st - Headers (First 5 rows):")
print(df.head(5))
# Export to csv for deep inspection
df.to_csv(r'C:\Users\alext\Projects\stf\march_1st_check.csv', index=False)
