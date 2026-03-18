#include <algorithm>   // for sort()
#include <cctype>      // for isspace(), isdigit(), tolower()
#include <filesystem>  // for walking through directories and files
#include <fstream>     // for reading/writing files
#include <iomanip>     // for setw(), setfill() (formatting numbers like 01, 02...)
#include <iostream>    // for cout
#include <map>         // like a dictionary: key -> value, sorted by key
#include <optional>    // lets a function return "nothing" instead of a garbage value
#include <set>         // like a list but no duplicates, stays sorted
#include <sstream>     // lets you build strings like you would with cout
#include <string>
#include <vector>      // resizable array

using namespace std;

// shortcut so we dont have to write filesystem:: everywhere
namespace fs = filesystem;

// this is basically a label for each camera source, like "cam03_rgb" or "cam01_thermal"
// the operator< lets us put these in a map/set (they need to be sortable)
struct SourceKey {
  string name;
  bool operator<(const SourceKey& other) const { return name < other.name; }
};

// splits a CSV line by commas, respecting quoted fields
// e.g. "hello,world,123" -> ["hello", "world", "123"]
static vector<string> split_csv_line(const string& line) {
  vector<string> out;
  string cur;
  bool in_quotes = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '"') {
      in_quotes = !in_quotes;
    } else if (c == ',' && !in_quotes) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

// removes spaces/tabs from the start and end of a string
static string trim(const string& s) {
  size_t start = 0;
  while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) {
    ++start;
  }
  size_t end = s.size();
  while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }
  return s.substr(start, end - start);
}

// the time column looks different in RGB vs thermal files:
//   thermal: "10"  (just a number)
//   RGB:     "10:00-10:59"  (a range)
// this function handles both and pulls out the hour as an int
// returns nullopt (meaning "no value") if it cant parse it
static optional<int> parse_hour_from_time_range(const string& s) {
  // grab first two digits for the "10:00-10:59" format
  if (s.size() >= 2 && isdigit(s[0]) && isdigit(s[1])) {
    int h = (s[0] - '0') * 10 + (s[1] - '0');
    if (0 <= h && h <= 23) {
      return h;
    }
  }
  // fallback: try reading it as a plain integer like "10"
  stringstream ss(s);
  int h = -1;
  ss >> h;
  if (!ss.fail() && 0 <= h && h <= 23) {
    return h;
  }
  return nullopt;
}

// filenames look like "Cam01_20250103_daily.csv"
// this scans for the first 8-digit chunk and turns it into "2025-01-03"
static optional<string> parse_date_from_filename(const string& filename) {
  for (size_t i = 0; i + 7 < filename.size(); ++i) {
    bool ok = true;
    for (size_t j = 0; j < 8; ++j) {
      if (!isdigit(static_cast<unsigned char>(filename[i + j]))) {
        ok = false;
        break;
      }
    }
    if (ok) {
      string ymd = filename.substr(i, 8);
      string date = ymd.substr(0, 4) + "-" + ymd.substr(4, 2) + "-" + ymd.substr(6, 2);
      return date;
    }
  }
  return nullopt;
}

// combines date + hour into one string we use as a key, like "2025-01-03 10"
static string make_datetime_key(const string& date, int hour) {
  ostringstream os;
  os << date << " " << setw(2) << setfill('0') << hour;
  return os.str();
}

// turns thermal cam number (e.g. 1) into "cam01"
static string cam_id_to_label(int cam) {
  ostringstream os;
  os << "cam" << setw(2) << setfill('0') << cam;
  return os.str();
}

// turns RGB cam folder name (e.g. "Cam03") into lowercase "cam03"
static string cam_id_to_label_rgb(const string& camRaw) {
  string s = camRaw;
  for (auto& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
  return s;
}

static vector<string> expand_cam_tokens(const string& token) {
  if (token.rfind("Cam", 0) != 0) return {token};
  if (token.size() == 7) {
    string a = token.substr(0, 5);
    string b = string("Cam") + token.substr(5, 2);
    return {a, b};
  }
  return {token};
}

// given one row of CSV data, sums up all the counts per direction
// e.g. if columns are B1_person, B1_bicycle, B1_e-scooter, B2_person...
// it returns {"b1": 98, "b2": 5, ...}  (all classes lumped together)
// only keeps directions a1, a2, b1-b6 — ignores stuff like L1->L2
static map<string, double> sum_by_direction(
    const vector<string>& header,
    const vector<string>& row) {
  map<string, double> sums;
  // start at i=1 to skip the first column (hour/time)
  for (size_t i = 1; i < header.size() && i < row.size(); ++i) {
    string col = trim(header[i]);

    // column names look like "B1_person" — split on the underscore
    auto pos = col.find('_');
    if (pos == string::npos) {
      continue;
    }

    // if the class name is not bicycle
    if (col.substr(pos + 1) != "bicycle") {
      continue;
    }

    // everything before the underscore is the direction
    string dir = col.substr(0, pos);

    // lowercase it so A1/a1 are treated the same
    string dirLower = dir;
    for (auto& c : dirLower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    // only keep a1, a2, b1 through b6
    bool allowed = (dirLower == "a1" || dirLower == "a2") ||
                   (dirLower.size() == 2 && dirLower[0] == 'b' && dirLower[1] >= '1' && dirLower[1] <= '6');
    if (!allowed) {
      continue;
    }
    dir = dirLower;

    string valStr = trim(row[i]);
    if (valStr.empty()) {
      continue;
    }
    double val = 0.0;
    try {
      val = stod(valStr);  // string to double
    } catch (...) {
      continue;  // if it's not a number, skip it
    }
    sums[dir] += val;
  }
  return sums;
}

int main(int argc, char** argv) {
  // default folder paths — can be overridden with command line args
  fs::path thermalDir = "Data/Thermal";
  fs::path rgbDir = "Data/RGB";
  fs::path outDir = "Results/Diff";
  string monthFolder = "";

  // parse optional command line args like: ./diff --out-dir my_output
  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "--thermal-dir" && i + 1 < argc) {
      thermalDir = argv[++i];
    } else if (arg == "--rgb-dir" && i + 1 < argc) {
      rgbDir = argv[++i];
    } else if (arg == "--out-dir" && i + 1 < argc) {
      outDir = argv[++i];
    } else if (arg == "--month" && i + 1 < argc) {
      monthFolder = argv[++i];
    }
  }

  if (!monthFolder.empty()) {
      thermalDir /= monthFolder;
      rgbDir /= monthFolder;
      outDir = fs::path("Results") / monthFolder / "Diff";
  }

  // which RGB camera folder lives under which site folder
  // e.g. site 2 has "Cam03" files inside A_RGB1-5_STF_Data/Site02/
  map<int, vector<string>> siteRgbCams = {
      {2, {"Cam03"}},
      {4, {"Cam0506"}},
      {7, {"Cam12"}},
      {9, {"Cam13"}},
      {10, {"Cam18"}},
  };

  // which thermal cameras belong to which site
  // e.g. site 2 has thermal Cam01 and Cam02
  map<int, vector<int>> siteThermalCams = {
      {2, {1, 2}},
      {4, {3, 4}},
      {7, {5, 6}},
      {9, {7, 8}},
      {10, {9, 10}},
  };

  // this is the big nested dictionary that holds everything:
  //   data[site][direction][datetime][source] = count
  // e.g. data[2]["b1"]["2025-01-03 10"]["cam03_rgb"] = 98
  map<int, map<string, map<string, map<SourceKey, double>>>> data;

  // ---- LOAD THERMAL DATA ----
  for (const auto& [site, cams] : siteThermalCams) {
    for (int cam : cams) {
      // build the folder name like "Cam01", "Cam02" etc.
      ostringstream camDirName;
      camDirName << "Cam" << setw(2) << setfill('0') << cam;
      fs::path camDir = thermalDir / camDirName.str();
      if (!fs::exists(camDir) || !fs::is_directory(camDir)) {
        continue;
      }

      // loop through every CSV file in the camera folder
      for (const auto& entry : fs::directory_iterator(camDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".csv") continue;

        string filename = entry.path().filename().string();
        auto dateOpt = parse_date_from_filename(filename);
        if (!dateOpt) continue;  // skip if we cant find a date in the filename
        // No date filter

        ifstream in(entry.path());
        if (!in.is_open()) continue;

        // first line is the header row with column names
        string line;
        if (!getline(in, line)) continue;
        auto header = split_csv_line(line);

        // every remaining line is one hour of data
        while (getline(in, line)) {
          if (line.empty()) continue;
          auto row = split_csv_line(line);
          if (row.empty()) continue;

          auto hourOpt = parse_hour_from_time_range(trim(row[0]));
          if (!hourOpt) continue;

          auto sums = sum_by_direction(header, row);
          string datetimeKey = make_datetime_key(*dateOpt, *hourOpt);

          // label like "cam01_thermal"
          SourceKey src{cam_id_to_label(cam) + "_thermal"};

          for (const auto& [dir, val] : sums) {
            string outDirKey = dir;

            // special case: thermal cam 1 at site 2 uses A1/A2 labels
            // but they actually correspond to B3/B1 directions
            // so we remap them here to match the RGB camera's labelling
            if (site == 2 && cam == 1) {
              if (dir == "a1") {
                outDirKey = "b3";
              } else if (dir == "a2") {
                outDirKey = "b1";
              }
            }
            data[site][outDirKey][datetimeKey][src] += val;
          }
        }
      }
    }
  }

  // ---- LOAD RGB DATA ----
  for (const auto& [site, cams] : siteRgbCams) {
    for (const auto& camRaw : cams) {
      for (const auto& camFolder : expand_cam_tokens(camRaw)) {
        fs::path camDir = rgbDir / camFolder;
        if (!fs::exists(camDir) || !fs::is_directory(camDir)) {
          continue;
        }

        for (const auto& entry : fs::directory_iterator(camDir)) {
          if (!entry.is_regular_file()) continue;
          if (entry.path().extension() != ".csv") continue;

          // only process files that match this camera name (e.g. "Cam03")
          string filename = entry.path().filename().string();
          if (filename.find(camFolder) == string::npos) {
            continue;
          }

          auto dateOpt = parse_date_from_filename(filename);
          if (!dateOpt) continue;
          // No date filter

          ifstream in(entry.path());
          if (!in.is_open()) continue;

          string line;
          if (!getline(in, line)) continue;
          auto header = split_csv_line(line);

          while (getline(in, line)) {
            if (line.empty()) continue;
            auto row = split_csv_line(line);
            if (row.empty()) continue;

            auto hourOpt = parse_hour_from_time_range(trim(row[0]));
            if (!hourOpt) continue;

            auto sums = sum_by_direction(header, row);
            string datetimeKey = make_datetime_key(*dateOpt, *hourOpt);

            // label like "cam03_rgb"
            SourceKey src{cam_id_to_label_rgb(camFolder) + "_rgb"};
            for (const auto& [dir, val] : sums) {
              data[site][dir][datetimeKey][src] += val;
            }
          }
        }
      }
    }
  }

  // ---- WRITE OUTPUT CSVs ----
  fs::create_directories(outDir);

  // one output file per site + direction combo (e.g. site2_b1.csv)
  for (const auto& [site, byDir] : data) {
    for (const auto& [dir, byDatetime] : byDir) {

      // figure out all camera sources that appear in this site+direction
      set<SourceKey> sources;
      for (const auto& [dt, bySrc] : byDatetime) {
        for (const auto& [src, val] : bySrc) {
          sources.insert(src);
        }
      }
      vector<SourceKey> srcList(sources.begin(), sources.end());

      // split sources into rgb vs thermal so we can compute differences
      vector<SourceKey> rgbSources;
      vector<SourceKey> thermalSources;
      for (const auto& src : srcList) {
        // rfind checks if the string ENDS with "_rgb" or "_thermal"
        if (src.name.size() >= 4 && src.name.rfind("_rgb") == src.name.size() - 4) {
          rgbSources.push_back(src);
        } else if (src.name.size() >= 8 && src.name.rfind("_thermal") == src.name.size() - 8) {
          thermalSources.push_back(src);
        }
      }

      // build output filename like "site2_b1.csv"
      ostringstream fname;
      fname << "site" << site << "_" << dir << ".csv";

      fs::path outPath = outDir / fname.str();
      ofstream out(outPath);
      if (!out.is_open()) continue;

      // write header row: date, hour, then each camera, then diff columns
      out << "date,hour,period";
      for (const auto& src : srcList) {
        out << "," << src.name;
      }
      // diff columns: one for each rgb-vs-thermal pair
      for (const auto& rgb : rgbSources) {
        for (const auto& th : thermalSources) {
          out << ",diff_" << th.name << "_minus_" << rgb.name;
          out << ",pct_error_" << th.name << "_minus_" << rgb.name;
        }
      }
      out << "\n";

      // collect all date+hour keys and sort them chronologically
      vector<string> dts;
      dts.reserve(byDatetime.size());
      for (const auto& [dt, _] : byDatetime) dts.push_back(dt);
      sort(dts.begin(), dts.end());

      // write one row per date+hour
      for (const auto& dt : dts) {
        // dt looks like "2025-01-03 10" — split into date and hour parts
        string date = dt.substr(0, 10);
        string hour = dt.substr(11, 2);
        int hourInt = 0;
        try { hourInt = stoi(hour); } catch (...) { hourInt = -1; }
        string period = (hourInt >= 7 && hourInt <= 19) ? "day" : "night";
        out << date << "," << hour << "," << period;

        const auto& bySrc = byDatetime.at(dt);

        // write each camera's count (0 if that camera has no data for this hour)
        for (const auto& src : srcList) {
          auto it = bySrc.find(src);
          double val = (it != bySrc.end()) ? it->second : 0.0;
          out << "," << val;
        }

        // write the difference columns: thermal_count - rgb_count
        for (const auto& rgb : rgbSources) {
          double rgbVal = 0.0;
          auto itRgb = bySrc.find(rgb);
          if (itRgb != bySrc.end()) {
            rgbVal = itRgb->second;
          }
          for (const auto& th : thermalSources) {
            double thVal = 0.0;
            auto itTh = bySrc.find(th);
            if (itTh != bySrc.end()) {
              thVal = itTh->second;
            }
            double diff = thVal - rgbVal;
            out << "," << diff;
            if (rgbVal != 0.0) {
              ostringstream pct;
              pct << fixed << setprecision(2) << (diff / rgbVal * 100.0);
              out << "," << pct.str() << "%";
            } else {
              out << ",";
            }
          }
        }
        out << "\n";
      }
    }
  }

  cout << "Done. Output written to: " << outDir << "\n";
  return 0;
}
