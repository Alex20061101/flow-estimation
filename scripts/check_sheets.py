import pandas as pd
excel_path = r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx'
xl = pd.ExcelFile(excel_path)
print(xl.sheet_names)
