import os
import glob
import re
from collections import defaultdict

def scan_data(base_path):
    data = defaultdict(lambda: defaultdict(list))
    for root, dirs, files in os.walk(base_path):
        for file in files:
            if file.endswith('.csv') or file.endswith('.xlsx'):
                # Extract camera and date e.g. Cam02_20250101_daily.csv
                m = re.search(r'(Cam\w+)_(\d{8})_', file)
                if m:
                    cam = m.group(1)
                    date = m.group(2)
                    date_str = f"{date[:4]}-{date[4:6]}-{date[6:]}"
                    
                    if 'Thermal' in root:
                        ctype = 'Thermal'
                    elif 'RGB' in root:
                        ctype = 'RGB'
                    else:
                        ctype = 'Other'
                    
                    if cam not in data[ctype][date_str]:
                        data[ctype][date_str].append(cam)
    return data

def generate_html(data):
    html = ["<html><head><style>"]
    html.append("body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f8f9fa; padding: 20px; }")
    html.append("h1 { color: #333; text-align: center; }")
    html.append("h2 { color: #555; border-bottom: 2px solid #ddd; padding-bottom: 5px; margin-top: 40px; }")
    html.append("table { border-collapse: collapse; margin-top: 10px; background-color: white; box-shadow: 0 1px 3px rgba(0,0,0,0.2); }")
    html.append("th, td { border: 1px solid #eee; padding: 8px 12px; text-align: center; font-size: 14px; }")
    html.append("th { background-color: #2c3e50; color: white; position: sticky; top: 0; }")
    html.append(".date-col { font-weight: bold; background-color: #ecf0f1; color: #2c3e50; text-align: left; }")
    html.append(".yes { background-color: #2ecc71; color: transparent; font-size: 0; position: relative; }")
    html.append(".yes::after { content: '✓'; color: white; font-size: 14px; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); }")
    html.append(".no { background-color: #e74c3c; color: transparent; font-size: 0; position: relative; }")
    html.append(".no::after { content: '✕'; color: white; font-size: 14px; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); }")
    html.append("</style></head><body>")
    html.append("<h1>Data Availability Heatmap</h1>")
    
    for ctype, dates_dict in data.items():
        html.append(f"<h2>{ctype} Cameras</h2>")
        
        all_dates = sorted(list(dates_dict.keys()))
        if not all_dates:
            html.append("<p>No data found.</p>")
            continue
            
        all_cams = set()
        for cams in dates_dict.values():
            all_cams.update(cams)
        all_cams = sorted(list(all_cams))
        
        html.append("<table><tr><th>Date</th>")
        for cam in all_cams:
            html.append(f"<th>{cam}</th>")
        html.append("</tr>")
        
        for date in all_dates:
            html.append(f"<tr><td class='date-col'>{date}</td>")
            for cam in all_cams:
                if cam in dates_dict[date]:
                    html.append("<td class='yes'>Y</td>")
                else:
                    html.append("<td class='no'>N</td>")
            html.append("</tr>")
        html.append("</table>")
        
    html.append("</body></html>")
    
    with open('Results/heatmap.html', 'w', encoding='utf-8') as f:
        f.write('\n'.join(html))
    print("Heatmap successfully generated at: Results/heatmap.html")

if __name__ == '__main__':
    if not os.path.exists('Results'):
        os.makedirs('Results')
    data = scan_data('Data')
    generate_html(data)
