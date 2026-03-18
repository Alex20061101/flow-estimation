#!/bin/bash P@ssw0rd123

# Server and user
SERVER="imse4090_1@147.8.92.231"


# Remote base path
REMOTE_BASE="/home/imse4090_1/Project_SmartTrafficSystem/Result/RGB/Daily_Hourly_Counts"

# Local destination
LOCAL_BASE="$HOME/Downloads/camera_data"
mkdir -p "$LOCAL_BASE"

# Site-to-camera mapping
declare -A site_camera_map=(
  [10]="18"
  [9]="13"
  [8]="15"
  [7]="12"
  [6]="10"
  [5]="08"
  [4]="0506"
  [3]="19"
  [2]="03"
  [1]="02"
)

# Dates to download
dates=("20250308" "20250309")

# Loop through sites, cameras, and dates
for site in "${!site_camera_map[@]}"; do
  camera="${site_camera_map[$site]}"
  for date in "${dates[@]}"; do
    filename="Cam${camera}_${date}_daily_count.csv"
    remote_path="$REMOTE_BASE/Site$(printf "%02d" $site)/$filename"
    local_path="$LOCAL_BASE/$filename"

    echo "Downloading $remote_path → $local_path"
    scp "$SERVER:$remote_path" "$local_path"
  done
done
