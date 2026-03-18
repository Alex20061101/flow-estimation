import pandas as pd
excel_path = r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx'
xl = pd.ExcelFile(excel_path)
for sheet in xl.sheet_names:
    print(f"\nScanning sheet: {sheet}")
    df = pd.read_excel(excel_path, sheet_name=sheet, header=None)
    # Search for "t1", "t2", "Cam01", "Cam02", "Thermal"
    mask = df.stack().astype(str).str.contains(r't\d|thermal|Cam01|Cam02', case=False)
    if mask.any():
        print(f"FOUND thermal-related markers in sheet: {sheet}")
        indices = mask[mask].index
        for idx in indices[:10]: # Print first 10 matches
            print(f"Row {idx[0]}, Col {idx[1]}: {df.iloc[idx[0], idx[1]]}")
    else:
        print(f"No thermal markers in {sheet}")
