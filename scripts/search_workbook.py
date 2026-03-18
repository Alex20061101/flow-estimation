import pandas as pd
excel_path = r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx'
xl = pd.ExcelFile(excel_path)
for sheet in xl.sheet_names:
    df = pd.read_excel(excel_path, sheet_name=sheet, header=None)
    # Search for "Thermal" (case insensitive)
    mask = df.stack().astype(str).str.contains("Thermal|T\d", case=False).any()
    if mask:
        print(f"FOUND matching 'Thermal' or 'T' reference in sheet: {sheet}")
        # Print a few rows around where it might be
        for r in range(len(df)):
            for c in range(len(df.columns)):
                cell_val = str(df.iloc[r, c])
                if "Thermal" in cell_val:
                    print(f"Row {r}, Col {c}: {cell_val}")
