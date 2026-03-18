import openpyxl

wb = openpyxl.load_workbook(r'C:\Users\alext\Projects\stf\Calculation Example for March_updated_13March.xlsx', data_only=False)
sheet = wb['Computing example for March']

# Let's read the first 40 rows and first 10 columns to find formula patterns
for row in range(1, 41):
    row_data = []
    for col in range(1, 11):
        cell = sheet.cell(row=row, column=col)
        val = cell.value
        # If it's a formula, it starts with '='
        row_data.append(str(val))
    print(f"Row {row}: {' | '.join(row_data)}")
