#include <cctype>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <ctime>
#include <unordered_set>
#include <random>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = filesystem;

// ---- Small helpers ----
static vector<string> split_csv_line(const string& line) {
  vector<string> out;
  string cur;
  bool in_quotes = false;
  for (char c : line) {
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

static string trim(const string& s) {
  size_t start = 0;
  while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) ++start;
  size_t end = s.size();
  while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) --end;
  return s.substr(start, end - start);
}

static optional<int> parse_hour_from_time_range(const string& s) {
  if (s.size() >= 2 && isdigit(s[0]) && isdigit(s[1])) {
    int h = (s[0] - '0') * 10 + (s[1] - '0');
    if (0 <= h && h <= 23) return h;
  }
  stringstream ss(s);
  int h = -1;
  ss >> h;
  if (!ss.fail() && 0 <= h && h <= 23) return h;
  return nullopt;
}

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
      return ymd.substr(0, 4) + "-" + ymd.substr(4, 2) + "-" + ymd.substr(6, 2);
    }
  }
  return nullopt;
}

static string to_lower(string s) {
  for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
  return s;
}

static vector<string> split_dirs(const string& s) {
  vector<string> out;
  string cur;
  for (char c : s) {
    if (c == '+') {
      if (!cur.empty()) out.push_back(to_lower(trim(cur)));
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) out.push_back(to_lower(trim(cur)));
  return out;
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

// Sum bicycle counts by direction for one row.
static map<string, double> sum_by_direction(
    const vector<string>& header,
    const vector<string>& row) {
  map<string, double> sums;
  for (size_t i = 1; i < header.size() && i < row.size(); ++i) {
    string col = trim(header[i]);
    auto pos = col.find('_');
    if (pos == string::npos) continue;

    if (col.substr(pos + 1) != "bicycle") continue;
    string dir = to_lower(col.substr(0, pos));

    bool allowed = (dir == "a1" || dir == "a2") ||
                   (dir.size() == 2 && dir[0] == 'b' && dir[1] >= '1' && dir[1] <= '6');
    if (!allowed) continue;

    string valStr = trim(row[i]);
    if (valStr.empty()) continue;

    double val = 0.0;
    try {
      val = stod(valStr);
    } catch (...) {
      continue;
    }
    sums[dir] += val;
  }
  return sums;
}

struct LinkMapping {
  int link = 0;
  string dir_label;   // "forward" or "reverse"
  int site_a = 0;
  string dirs_a;      // e.g. "B3+B5"
  bool has_site_b = false;
  int site_b = 0;
  string dirs_b;      // e.g. "A1"
};



int main(int argc, char** argv) {
  fs::path rgbDir = "Data/RGB";
  fs::path outDir = "Results/LinkDiff";
  string layout = "cam"; // "cam" for RGBtest/CamXX, "site" for A_RGB1-5_STF_Data/SiteXX
  string monthFolder = "";
  optional<string> startDate;
  optional<string> endDate;

  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "--rgb-dir" && i + 1 < argc) {
      rgbDir = argv[++i];
    } else if (arg == "--out-dir" && i + 1 < argc) {
      outDir = argv[++i];
    } else if (arg == "--layout" && i + 1 < argc) {
      layout = argv[++i];
    } else if (arg == "--month" && i + 1 < argc) {
      monthFolder = argv[++i];
    } else if (arg == "--start-date" && i + 1 < argc) {
      startDate = argv[++i];
    } else if (arg == "--end-date" && i + 1 < argc) {
      endDate = argv[++i];
    }
  }

  if (!monthFolder.empty()) {
      rgbDir /= monthFolder;
      outDir = fs::path("Results") / monthFolder / "LinkDiff";
  }

  // Site -> RGB camera names (folder files contain these tokens)
  map<int, vector<string>> siteRgbCams = {
      {1, {"Cam02"}},
      {2, {"Cam03"}},
      {3, {"Cam19"}},
      {4, {"Cam0506"}},
      {5, {"Cam08"}},
      {6, {"Cam10"}},
      {7, {"Cam12"}},
      {8, {"Cam15", "Cam16"}},
      {9, {"Cam13"}},
      {10, {"Cam18"}},
  };

  // Link mapping provided by you (no percent difference)
  vector<LinkMapping> mappings = {
      {1,  "forward", 1,  "A2",     true,  2,  "B3+B5"},
      {1,  "reverse", 1,  "A1",     true,  2,  "B1+B2"},
      {2,  "forward", 2,  "B3+B4",  true,  3,  "B3+B5"},
      {2,  "reverse", 2,  "B1+B6",  true,  3,  "B1+B2"},
      {3,  "forward", 2,  "B5+B6",  true, 10,  "A1"},
      {3,  "reverse", 2,  "B2+B4",  true, 10,  "A2"},
      {4,  "forward", 3,  "B3+B4",  true,  4,  "B3+B5"},
      {4,  "reverse", 3,  "B1+B6",  true,  4,  "B1+B2"},
      {5,  "forward", 4,  "B3+B4",  true,  5,  "B3+B5"},
      {5,  "reverse", 4,  "B1+B6",  true,  5,  "B1+B2"},
      {6,  "forward", 4,  "B5+B6",  false, 0,  ""},
      {6,  "reverse", 4,  "B2+B4",  false, 0,  ""},
      {7,  "forward", 6,  "B4+B5",  true,  7,  "A2"},
      {7,  "reverse", 6,  "B2+B6",  true,  7,  "A1"},
      {8,  "forward", 7,  "A2",     true,  9,  "A1"},
      {8,  "reverse", 7,  "A1",     true,  9,  "A2"},
      {9,  "forward", 9,  "A1",     false, 0,  ""},
      {9,  "reverse", 9,  "A2",     false, 0,  ""},
      {10, "forward", 10, "A1",     false, 0,  ""},
      {10, "reverse", 10, "A2",     false, 0,  ""},
      {13, "forward", 5,  "B3+B4",  true,  6,  "B3+B5"},
      {13, "reverse", 5,  "B1+B6",  true,  6,  "B1+B2"},
  };

  // Dates to process will be derived from filenames we load.
  vector<string> dates;

  // Aggregate totals per site & direction per hour for each date
  // totalsByDateHour[date][hour][site][dir] -> double
  map<string, map<int, map<int, map<string, double>>>> totalsByDateHour;

  auto dateInRange = [&](const string& date) {
    if (startDate && date < *startDate) return false;
    if (endDate && date > *endDate) return false;
    return true;
  };

  for (const auto& [site, cams] : siteRgbCams) {
    if (layout == "site") {
      ostringstream siteName;
      siteName << "Site" << setw(2) << setfill('0') << site;
      fs::path siteDir = rgbDir / siteName.str();
      if (!fs::exists(siteDir) || !fs::is_directory(siteDir)) continue;

      vector<string> camTokens;
      for (const auto& camToken : cams) {
        for (const auto& camFolder : expand_cam_tokens(camToken)) {
          camTokens.push_back(camFolder);
        }
      }

      for (const auto& entry : fs::directory_iterator(siteDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".csv") continue;

        string filename = entry.path().filename().string();
        bool matchesSiteCam = false;
        for (const auto& token : camTokens) {
          if (filename.find(token) != string::npos) {
            matchesSiteCam = true;
            break;
          }
        }
        if (!matchesSiteCam) continue;

        auto dateOpt = parse_date_from_filename(filename);
        if (!dateOpt) continue;
        if (!dateInRange(*dateOpt)) continue;

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
          int hour = *hourOpt;

          auto sums = sum_by_direction(header, row);
          for (const auto& [dir, val] : sums) {
            totalsByDateHour[*dateOpt][hour][site][dir] += val;
          }
        }
      }
    } else {
      for (const auto& camToken : cams) {
        for (const auto& camFolder : expand_cam_tokens(camToken)) {
          fs::path camDir = rgbDir / camFolder;
          if (!fs::exists(camDir) || !fs::is_directory(camDir)) continue;

          for (const auto& entry : fs::directory_iterator(camDir)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".csv") continue;

            string filename = entry.path().filename().string();
            if (filename.find(camFolder) == string::npos) continue;

            auto dateOpt = parse_date_from_filename(filename);
            if (!dateOpt) continue;
            if (!dateInRange(*dateOpt)) continue;

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
              int hour = *hourOpt;

              auto sums = sum_by_direction(header, row);
              for (const auto& [dir, val] : sums) {
                totalsByDateHour[*dateOpt][hour][site][dir] += val;
              }
            }
          }
        }
      }
    }
  }

  // Collect all dates present in the data, in chronological order.
  for (const auto& [date, _] : totalsByDateHour) {
    dates.push_back(date);
  }
  sort(dates.begin(), dates.end());

  fs::create_directories(outDir);

  // Sites with unreliable RGB cameras (do not use these as denominator)
  unordered_set<int> unreliableSites = {3, 5, 7};
  std::mt19937 rng((std::random_device())());

  for (const auto& date : dates) {
    auto itDate = totalsByDateHour.find(date);
    if (itDate == totalsByDateHour.end()) continue;

    // check if any hour has non-zero totals
    bool anyNonZero = false;
    for (const auto& [hour, hourMap] : itDate->second) {
      for (const auto& [site, dirmap] : hourMap) {
        for (const auto& [dir, val] : dirmap) {
          if (fabs(val) > 1e-9) { anyNonZero = true; break; }
        }
        if (anyNonZero) break;
      }
      if (anyNonZero) break;
    }
    if (!anyNonZero) continue;

    fs::path dayPath = outDir / ("rgb_link_diff_" + date + ".csv");
    ofstream out(dayPath);
    if (!out.is_open()) {
      cerr << "Failed to open output: " << dayPath << "\n";
      continue;
    }

    out << "hour,period,link,dir,site_first,dirs_first,total_first,site_second,dirs_second,total_second,denominator_site,denominator_note,diff,rel_diff\n";
    out << fixed << setprecision(2);

    // For each hour (0..23) produce rows for mappings
    for (int hour = 0; hour < 24; ++hour) {
      auto itHour = itDate->second.find(hour);
      if (itHour == itDate->second.end()) continue;
      auto& hourTotals = itHour->second; // map<int site, map<string dir,double>>

      // check if this hour has any non-zero
      bool hourHas = false;
      for (const auto& [site, dmap] : hourTotals) {
        for (const auto& [d, v] : dmap) if (fabs(v) > 1e-9) { hourHas = true; break; }
        if (hourHas) break;
      }
      if (!hourHas) continue;

      for (const auto& m : mappings) {
        auto dirsA = split_dirs(m.dirs_a);
        double totalA = 0.0;
        for (const auto& d : dirsA) {
          totalA += hourTotals[m.site_a][d];
        }

        if (!m.has_site_b) {
          string period = (hour >= 7 && hour <= 19) ? "day" : "night";
          out << hour << "," << period << "," << m.link << "," << m.dir_label << ","
              << m.site_a << "," << m.dirs_a << ","
              << totalA << ",-,-,0,none,only_site,0,\"\"\n";
          continue;
        }

        auto dirsB = split_dirs(m.dirs_b);
        double totalB = 0.0;
        for (const auto& d : dirsB) {
          totalB += hourTotals[m.site_b][d];
        }

        bool aUnreliable = unreliableSites.count(m.site_a) > 0;
        bool bUnreliable = unreliableSites.count(m.site_b) > 0;

        int site_first = m.site_a;
        string dirs_first = m.dirs_a;
        double total_first = totalA;
        int site_second = m.site_b;
        string dirs_second = m.dirs_b;
        double total_second = totalB;
        string denom_note = "";
        int denominator_site = 0;

        if (aUnreliable && !bUnreliable) {
          denominator_site = m.site_b;
          denom_note = "chosen_reliable";
        } else if (bUnreliable && !aUnreliable) {
          site_first = m.site_b;
          dirs_first = m.dirs_b;
          total_first = totalB;
          site_second = m.site_a;
          dirs_second = m.dirs_a;
          total_second = totalA;
          denominator_site = m.site_a;
          denom_note = "chosen_reliable";
        } else if (!aUnreliable && !bUnreliable) {
          std::uniform_int_distribution<int> uni(0, 1);
          int pick = uni(rng);
          if (pick == 0) {
            denominator_site = m.site_b;
            denom_note = "random_choice";
          } else {
            site_first = m.site_b;
            dirs_first = m.dirs_b;
            total_first = totalB;
            site_second = m.site_a;
            dirs_second = m.dirs_a;
            total_second = totalA;
            denominator_site = m.site_a;
            denom_note = "random_choice";
          }
        } else {
          denom_note = "both_unreliable";
          denominator_site = 0;
        }

        double diff = total_first - total_second;
        optional<double> rel;
        if (denominator_site != 0 && fabs(total_second) > 1e-9) {
          rel = diff / total_second;
        }

        string period = (hour >= 7 && hour <= 19) ? "day" : "night";
        out << hour << "," << period << "," << m.link << "," << m.dir_label << ","
            << site_first << "," << dirs_first << ","
            << total_first << ","
            << site_second << "," << dirs_second << ","
            << total_second << ","
            << (denominator_site == 0 ? string("none") : to_string(denominator_site)) << ","
            << denom_note << ","
            << diff << ",";
        if (rel) out << *rel;
        out << "\n";
      }
    }
  }

  cout << "Done. Output written to: " << outDir << "\n";
  return 0;
}
