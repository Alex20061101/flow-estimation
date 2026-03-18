
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <functional>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <filesystem>
#include <cstdio>

using namespace std;

vector<vector<string>> thermal_cam_dirs = {
    {}, // placeholder for 0 index
    {"a1", "a2"}, // cam1
    {"b1", "b2", "b3", "b4", "b5", "b6"}, // cam2
    {"b2", "b3", "b4"}, // cam3
    {"b1", "b5", "b6"}, // cam4
    {"a1", "a2"}, // cam5
    {"a1", "a2"}, // cam6
    {"a1", "a2"}, // cam7
    {"a1", "a2"}, // cam8
    {"a1", "a2"}, // cam9
    {"a1", "a2"}  // cam10
};

vector<vector<string>> rgb_cam_dirs = {
    {}, // placeholder for 0 index
    {"a1", "a2"}, // cam1
    {"a1", "a2"}, // cam2
    {"b1", "b2", "b3", "b4", "b5", "b6"}, // cam3
    {"a1", "a2"}, // cam4
    {"b1", "b2", "b3", "b4", "b5", "b6"}, // cam5
    {}, // cam6
    {"a1", "a2"}, // cam7
    {"b1", "b2", "b3", "b4", "b5", "b6"}, // cam8
    {"a1", "a2"}, // cam9
    {"b1", "b2", "b3", "b4", "b5", "b6"}, // cam10
    {"a1", "a2"}, // cam11
    {"a1", "a2"}, // cam12
    {"a1", "a2"}, // cam13
    {"a1", "a2"}, // cam14
    {"a1", "a2"}, // cam15
    {"a1", "a2"}, // cam16
    {"a1", "a2"}, // cam17
    {"a1", "a2"}, // cam18
    {"b1", "b2", "b3", "b4", "b5", "b6"}, // cam19
    {"a1", "a2"}  // cam20
};

vector<double> link_lengths = {
    0.0,
    485.46, 201.69, 165.82, 258.36, 243.31, 218.62, 243.70, 240.41, 91.69, 632.71,
    439.35, 365.94, 252.41, 322.35, 153.77, 206.52, 32.09, 197.33
};

// Formulas are 1-indexed. link_formulas[1] is Link 1.
vector<pair<string, string>> link_formulas = {
    {"", ""}, // placeholder
    {"avg((t2b5+t2b3),t1b3)", "avg((t2b2+t2b1),t1b1)"}, // 1: (1,2)
    {"avg((t2b4+t2b3),t1b3)", "avg((t2b6+t2b1),t1b1)"}, // 2: (2,3)
    {"avg((t2b5+t2b6),t9a1,t10a1)", "avg((t2b2+t2b4),t9a2,t10a2)"}, // 3: (2,10)
    {"avg(t3b3,t4b3)+avg(t3b5,t4b5)", "avg(t3b1,t4b1)+avg(t3b2,t4b2)"}, // 4: (3,4) (5or6 logic)
    {"avg(t3b3,t4b3)+avg(t3b4,t4b4)", "avg(t3b1,t4b1)+avg(t3b6,t4b6)"}, // 5: (4,5)
    {"avg(t3b5,t4b5)+avg(t3b6,t4b6)", "avg(t3b2,t4b2)+avg(t3b4,t4b4)"}, // 6: (4,11)
    {"avg(t5a2,t6a2)", "avg(t5a1,t6a1)"}, // 7: (6,7)
    {"avg(t5a2,t6a2)", "avg(t5a1,t6a1)"}, // 8: (7,9)
    {"avg(t7a1,t8a1)", "avg(t7a2,t8a2)"}, // 9: (9,12)
    {"avg(t9a1,t10a1)", "avg(t9a2,t10a2)"}, // 10: (10,12)
    {"2a1", "2a2"}, // 11
    {"glink[2]-glink[4]", "glink[2]-glink[4]"}, // 12
    {"10b5+10b3", "10b1+10b2"}, // 13
    {"glink[5]-glink[6]", "glink[5]-glink[6]"}, // 14
    {"16a1", "16a2"}, // 15
    {"16a2", "16a1"}, // 16
    {"glink[6]+glink[14]", "glink[12]+glink[18]"}, // 17 (Actually computed in computeLink17, but kept for clarity)
    {"15a1", "15a2"}  // 18
};

vector<pair<string, string>> link_formulas_ground = {
    {"", ""}, // placeholder
    {"avg((3b3+3b5),2a2)", "avg((3b1+3b2),2a1)"}, // 1: (1,2)
    {"3b3+3b4", "3b1+3b6"}, // 2: (2,3)
    {"avg((3b5+3b6),18a1)", "avg((3b2+3b4),18a2)"}, // 3: (2,10)
    {"avg(5b3,6b3)+avg(5b5,6b5)", "avg(5b1,6b1)+avg(5b2,6b2)"}, // 4: (3,4)
    {"avg(5b3,6b3)+avg(5b4,6b4)", "avg(5b1,6b1)+avg(5b6,6b6)"}, // 5: (4,5)
    {"avg(5b5,6b5)+avg(5b6,6b6)", "avg(5b2,6b2)+avg(5b4,6b4)"}, // 6: (4,11)
    {"avg((10b4+10b5),12a2)", "avg((10b2+10b6),12a1)"}, // 7: (6,7)
    {"avg((10b4+10b5),12a2)", "avg((10b2+10b6),12a1)"}, // 8: (7,9)
    {"13a1", "13a2"}, // 9: (9,12)
    {"18a1", "18a2"}, // 10: (10,12)
    {"2a1", "2a2"}, // 11
    {"link[2]-link[4]", "link[2]-link[4]"}, // 12
    {"10b5+10b3", "10b1+10b2"}, // 13
    {"link[5]-link[6]", "link[5]-link[6]"}, // 14
    {"16a1", "16a2"}, // 15
    {"16a2", "16a1"}, // 16
    {"link[6]+link[14]", "link[12]+link[18]"}, // 17
    {"15a1", "15a2"}  // 18
};


// === Print Helpers ===

void printCamData(const vector<unordered_map<string, double> >& camData) {
    for (size_t cam = 1; cam < camData.size(); ++cam) {
        cout << "Camera " << cam << ":" << endl;
        if (camData[cam].empty()) {
            cout << "  (no data)" << endl;
            continue;
        }
        for (const auto& kv : camData[cam]) {
            cout << "  Direction " << kv.first << ": " << kv.second << endl;
        }
        cout << endl;
    }
}

// Print computed link values to stdout
void printLinkValues(const vector<pair<double,double> >& values, const string& label) {
    cout << "\n--- " << label << " ---\n";
    cout << fixed << setprecision(2);
    for (size_t i = 1; i < values.size(); ++i) {
        const auto &pr = values[i];
        double sum = pr.first + pr.second;
        cout << "Link " << i << ": formula1=" << pr.first << ", formula2=" << pr.second
             << ", sum=" << sum << "\n";
    }
    cout << defaultfloat;
}

// === Data Processing ===

// A vector of maps. Each element in the vector corresponds to a camera.
// Each camera has a map, which stores amount of bicycle/scooter/people in a certain direction.
// Access camera 1 direction a1 by rgbCamData[1]["a1"]
vector<unordered_map<string, double> > rgbCamData(21);
vector<unordered_map<string, double> > thermalCamData(11);

// Evaluate a simple arithmetic expression consisting of numbers, +, / and parentheses.
static double evalSimpleExpr(const string &s) {
    size_t i = 0;

    auto skipSpaces = [&]() {
        while (i < s.size() && isspace((unsigned char)s[i])) ++i;
    };

    function<double()> parseExpression; // handles '+' and '-'
    function<double()> parseTerm;       // handles '/'
    function<double()> parseFactor;     // number, parentheses, or function call

    auto isMissing = [&](double v) {
        // If you truly use exact 0 as "missing", this is fine.
        // If you want tolerance:
        return fabs(v) < 1e-12;
    };

    auto parseIdentifier = [&]() -> string {
        size_t start = i;
        while (i < s.size() && (isalnum((unsigned char)s[i]) || s[i] == '_')) ++i;
        return s.substr(start, i - start);
    };

    parseFactor = [&]() -> double {
        skipSpaces();

        // Parentheses
        if (i < s.size() && s[i] == '(') {
            ++i;
            double val = parseExpression();
            skipSpaces();
            if (i < s.size() && s[i] == ')') ++i;
            return val;
        }

        // Function call: name(...)
        if (i < s.size() && (isalpha((unsigned char)s[i]) || s[i] == '_')) {
            string name = parseIdentifier();
            for (char &ch : name) ch = (char)tolower((unsigned char)ch);
            skipSpaces();

            if (i < s.size() && s[i] == '(') {
                ++i; // '('
                vector<double> args;
                skipSpaces();

                if (i < s.size() && s[i] != ')') {
                    while (true) {
                        args.push_back(parseExpression());
                        skipSpaces();
                        if (i < s.size() && s[i] == ',') { ++i; skipSpaces(); continue; }
                        break;
                    }
                }

                if (i < s.size() && s[i] == ')') ++i;

                if (name == "avgnz" || name == "avg") {
                    double sum = 0.0;
                    int cnt = 0;
                    for (double v : args) {
                        if (!isMissing(v)) { sum += v; ++cnt; }
                    }
                    return (cnt > 0) ? (sum / (double)cnt) : 0.0;
                }

                // Unknown function -> 0 (or throw if you prefer)
                return 0.0;
            }

            // Bare identifier not expected
            return 0.0;
        }

        // Number (with optional sign)
        int sign = 1;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
            if (s[i] == '-') sign = -1;
            ++i;
        }

        bool found = false;
        double num = 0.0;

        while (i < s.size() && isdigit((unsigned char)s[i])) {
            found = true;
            num = num * 10.0 + (s[i] - '0');
            ++i;
        }

        if (i < s.size() && s[i] == '.') {
            ++i;
            double place = 0.1;
            while (i < s.size() && isdigit((unsigned char)s[i])) {
                found = true;
                num += (s[i] - '0') * place;
                place *= 0.1;
                ++i;
            }
        }

        if (!found) return 0.0;
        return sign * num;
    };

    parseTerm = [&]() -> double {
        double value = parseFactor();
        while (true) {
            skipSpaces();
            if (i < s.size() && s[i] == '/') {
                ++i;
                double rhs = parseFactor();
                if (rhs != 0.0) value /= rhs;
                else value = 0.0; // keep your old behavior
            } else break;
        }
        return value;
    };

    parseExpression = [&]() -> double {
        double value = parseTerm();
        while (true) {
            skipSpaces();
            if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
                char op = s[i++];
                double rhs = parseTerm();
                value = (op == '+') ? (value + rhs) : (value - rhs);
            } else break;
        }
        return value;
    };

    return parseExpression();
}

// Storage for computed link values: pair(firstFormula, secondFormula). index 0 unused
vector<pair<double,double>> link_values;
vector<pair<double,double>> link_values_ground;

// Calculate a direction/formula value.
double calculateDir(const string& formula, const vector<pair<double,double>>& values, bool useFirst = true) {
    if (formula.empty()) return 0.0;

    string expr;
    for (size_t i = 0; i < formula.size();) {
        char c = formula[i];

        // Handle link[n] references
        if (formula.compare(i, 5, "link[") == 0) {
            i += 5;
            size_t start = i;
            while (i < formula.size() && isdigit((unsigned char)formula[i])) ++i;
            int idx = 0;
            if (i > start) idx = stoi(formula.substr(start, i - start));
            if (i < formula.size() && formula[i] == ']') ++i;
            double value = 0.0;
            if (idx >= 0 && idx < (int)values.size()) value = (useFirst ? values[idx].first : values[idx].second);
            ostringstream oss;
            oss << setprecision(12) << value;
            expr += oss.str();
            continue;
        }

        // Handle glink[n] references (force use of link_values_ground)
        if (formula.compare(i, 6, "glink[") == 0) {
            i += 6;
            size_t start = i;
            while (i < formula.size() && isdigit((unsigned char)formula[i])) ++i;
            int idx = 0;
            if (i > start) idx = stoi(formula.substr(start, i - start));
            if (i < formula.size() && formula[i] == ']') ++i;
            double value = 0.0;
            if (idx >= 0 && idx < (int)link_values_ground.size()) value = (useFirst ? link_values_ground[idx].first : link_values_ground[idx].second);
            ostringstream oss;
            oss << setprecision(12) << value;
            expr += oss.str();
            continue;
        }

        // Handle numbers and directions (with optional 't' prefix for thermal cameras)
        if (isdigit((unsigned char)c) || c == 't' || c == 'T') {
            bool useThermal = false;
            if (c == 't' || c == 'T') {
                useThermal = true;
                ++i;
                if (i >= formula.size()) { expr.push_back('t'); break; }
                c = formula[i];
            }

            // parse camera number
            if (!isdigit((unsigned char)c)) {
                // malformed; treat literal
                expr.push_back(useThermal ? 't' : c);
                ++i;
                continue;
            }
            size_t start = i;
            while (i < formula.size() && isdigit((unsigned char)formula[i])) ++i;
            string camStr = formula.substr(start, i - start);
            int cam = stoi(camStr);

            // parse direction (a/b + number)
            if (i < formula.size() && (formula[i] == 'a' || formula[i] == 'b')) {
                char dirType = formula[i++];
                size_t dstart = i;
                while (i < formula.size() && isdigit((unsigned char)formula[i])) ++i;
                string dirNum = formula.substr(dstart, i - dstart);
                string dirKey = string(1, dirType) + dirNum;
                double value = 0.0;
                if (!useThermal) {
                    if (cam >= 1 && cam < (int)rgbCamData.size()) {
                        auto it = rgbCamData[cam].find(dirKey);
                        if (it != rgbCamData[cam].end()) value = it->second;
                    }
                } else {
                    if (cam >= 1 && cam < (int)thermalCamData.size()) {
                        auto it = thermalCamData[cam].find(dirKey);
                        if (it != thermalCamData[cam].end()) value = it->second;
                    }
                }
                ostringstream oss;
                oss << setprecision(12) << value;
                expr += oss.str();
            } else {
                // it's just a camera index used as a number
                expr += camStr;
            }
        } else if (c == ' ' || c == '\t') {
            ++i;
        } else {
            // operators like '+', '/', parentheses, or other characters
            expr.push_back(c);
            ++i;
        }
    }

    return evalSimpleExpr(expr);
}

// Compute all link values from `link_formulas` using `calculateDir` and store them
void computeLinkValues(const vector<pair<string,string>>& formulas, vector<pair<double,double>>& values) {
    values.clear();
    values.resize(formulas.size(), {0.0,0.0});
    for (size_t i = 1; i < formulas.size(); ++i) {
        const auto& p = formulas[i];
        double v1 = calculateDir(p.first, values, true);
        double v2 = calculateDir(p.second, values, false);
        values[i] = {v1, v2};
    }
}

// === Data Reading with Day-of-Week Fallback ===

// Convert YYYYMMDD string to day of week: 0=Monday, 6=Sunday
static int dateStrToDoW(const string& date) {
    int y = stoi(date.substr(0,4));
    int m = stoi(date.substr(4,2));
    int d = stoi(date.substr(6,2));
    // Tomohiko Sakamoto's algorithm (0=Sunday)
    static const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (m < 3) y--;
    int dow = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    return (dow + 6) % 7; // convert to 0=Monday
}

// Get all dates in a month that share the same day-of-week as primaryDate,
// sorted by proximity to primaryDate (closest week first)
static vector<string> getSameDowDates(const string& primaryDate, int year, int month) {
    int targetDow = dateStrToDoW(primaryDate);
    int primaryDay = stoi(primaryDate.substr(6,2));

    // Days in month
    int daysInMonth = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) daysInMonth = 30;
    else if (month == 2) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        daysInMonth = leap ? 29 : 28;
    }

    vector<string> result;
    for (int d = 1; d <= daysInMonth; ++d) {
        char buf[9]; snprintf(buf, sizeof(buf), "%04d%02d%02d", year, month, d);
        string ds(buf, 8);
        if (dateStrToDoW(ds) == targetDow) result.push_back(ds);
    }

    // Sort by proximity to primaryDate (primary first, then nearest weeks)
    sort(result.begin(), result.end(), [&](const string& a, const string& b) {
        int da = abs(stoi(a.substr(6,2)) - primaryDay);
        int db = abs(stoi(b.substr(6,2)) - primaryDay);
        return da < db;
    });
    return result;
}

// Read per-hour data from a CSV file for a specific target class and direction set.
// Returns map<hour(0-23), map<dirKey, value>>. Hours not present in file are absent.
static map<int, map<string,double>> readCsvHoursForClass(
    const string& filePath,
    const vector<string>& dirs,
    const string& target)
{
    map<int, map<string,double>> result;
    ifstream f(filePath);
    if (!f.is_open()) return result;

    string headerLine;
    if (!getline(f, headerLine)) return result;

    vector<string> headers;
    {
        istringstream hs(headerLine); string h;
        while (getline(hs, h, ',')) {
            while (!h.empty() && isspace((unsigned char)h.front())) h.erase(h.begin());
            while (!h.empty() && isspace((unsigned char)h.back())) h.pop_back();
            headers.push_back(h);
        }
    }

    // Map column -> direction index (-1 if irrelevant)
    vector<int> colToDir(headers.size(), -1);
    for (size_t c = 0; c < headers.size(); ++c) {
        string low = headers[c];
        for (auto& ch : low) ch = (char)tolower((unsigned char)ch);
        size_t us = low.find('_');
        if (us == string::npos) continue;
        string dirPart = low.substr(0, us);
        string typePart = low.substr(us + 1);
        if (typePart.find(target) == string::npos) continue;
        for (size_t d = 0; d < dirs.size(); ++d) {
            if (dirs[d] == dirPart) { colToDir[c] = (int)d; break; }
        }
    }

    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        istringstream ss(line); string tok;
        size_t col = 0; int hour = -1;
        map<string,double> hourVals;
        while (getline(ss, tok, ',')) {
            while (!tok.empty() && isspace((unsigned char)tok.front())) tok.erase(tok.begin());
            while (!tok.empty() && isspace((unsigned char)tok.back())) tok.pop_back();
            if (col == 0) {
                try { hour = stoi(tok.size() >= 2 ? tok.substr(0,2) : tok); } catch(...) { hour = -1; }
            } else if (col < colToDir.size() && colToDir[col] != -1) {
                double val = 0.0;
                if (!tok.empty()) { try { val = stod(tok); } catch(...) {} }
                hourVals[dirs[colToDir[col]]] += val;
            }
            ++col;
        }
        if (hour >= 0 && hour < 24) result[hour] = hourVals;
    }
    return result;
}

// Check if an hour's data has any non-zero value across all directions
static bool hourHasData(const map<string,double>& hourVals) {
    for (auto& [d, v] : hourVals) if (v != 0.0) return true;
    return false;
}

// Helper: read all RGB camera files for the requested class (1=person,2=bicycle,3=scooter)
// Uses day-of-week fallback: if a file is missing or an hour is all-zero,
// it tries other weeks in the same month with the same day-of-week.
// Helper: Get best representation of 24 hours for all RGB cameras for a specific date,
// using day-of-week fallbacks if data is missing or zero.
// Returns map<hour, vector<map<dir, count>>> where vector index is camId.
static vector<vector<unordered_map<string, double>>> getBestHourlyRgbData(int classNumber, string primaryDate, const string& monthFolder) {
    vector<vector<unordered_map<string, double>>> hourlyFull(24, vector<unordered_map<string, double>>(21));

    int year  = stoi(primaryDate.substr(0,4));
    int month = stoi(primaryDate.substr(4,2));
    vector<string> candidates = getSameDowDates(primaryDate, year, month);

    string target;
    if (classNumber == 1) target = "person";
    else if (classNumber == 2) target = "bicycle";
    else target = "scooter";

    string baseDir = string("Data/RGB/") + (monthFolder.empty() ? "" : monthFolder + "/");

    for (int i = 1; i <= 20; ++i) {
        string padded = (to_string(i).size() == 1) ? "0" + to_string(i) : to_string(i);
        string camBase = baseDir + "Cam" + padded + "/Cam" + padded;

        map<int, map<string,double>> bestFileHours; // Best hour-by-hour found so far for this camera

        for (const string& cdate : candidates) {
            string path1 = camBase + "_" + cdate + "_daily_count.csv";
            string path2 = camBase + "_" + cdate + "_daily.csv";
            auto fileHours = readCsvHoursForClass(path1, rgb_cam_dirs[i], target);
            if (fileHours.empty()) fileHours = readCsvHoursForClass(path2, rgb_cam_dirs[i], target);

            for (auto& [hr, dirVals] : fileHours) {
                if (bestFileHours.count(hr) == 0 || !hourHasData(bestFileHours[hr])) {
                    if (bestFileHours.count(hr) == 0 || hourHasData(dirVals))
                        bestFileHours[hr] = dirVals;
                }
            }
        }

        for (auto& [hr, dirVals] : bestFileHours) {
            for (auto& [dir, val] : dirVals) {
                hourlyFull[hr][i][dir] = val;
            }
        }
    }
    return hourlyFull;
}

// Same for Thermal
static vector<vector<unordered_map<string, double>>> getBestHourlyThermalData(int classNumber, string primaryDate, const string& monthFolder) {
    vector<vector<unordered_map<string, double>>> hourlyFull(24, vector<unordered_map<string, double>>(11));

    int year  = stoi(primaryDate.substr(0,4));
    int month = stoi(primaryDate.substr(4,2));
    vector<string> candidates = getSameDowDates(primaryDate, year, month);

    string target;
    if (classNumber == 1) target = "person";
    else if (classNumber == 2) target = "bicycle";
    else target = "scooter";

    string baseDir = string("Data/Thermal/") + (monthFolder.empty() ? "" : monthFolder + "/");

    for (int i = 1; i <= 10; ++i) {
        string camNum = (to_string(i).size() == 1) ? "0" + to_string(i) : to_string(i);
        string camBase = baseDir + "Cam" + camNum + "/Cam" + camNum;
        map<int, map<string,double>> bestFileHours;

        for (const string& cdate : candidates) {
            string path = camBase + "_" + cdate + "_daily.csv";
            auto fileHours = readCsvHoursForClass(path, thermal_cam_dirs[i], target);
            for (auto& [hr, dirVals] : fileHours) {
                if (bestFileHours.count(hr) == 0 || !hourHasData(bestFileHours[hr])) {
                    if (bestFileHours.count(hr) == 0 || hourHasData(dirVals))
                        bestFileHours[hr] = dirVals;
                }
            }
        }
        for (auto& [hr, dirVals] : bestFileHours) {
            for (auto& [dir, val] : dirVals) {
                hourlyFull[hr][i][dir] = val;
            }
        }
    }
    return hourlyFull;
}

static bool readRgbDataForClass(int classNumber, string primaryDate, const string& monthFolder) {
    auto best = getBestHourlyRgbData(classNumber, primaryDate, monthFolder);
    for (int i = 1; i <= 20; i++)
        for (const auto& dir : rgb_cam_dirs[i]) rgbCamData[i][dir] = 0.0;
    for (int hr = 0; hr < 24; ++hr) {
        auto& camList = best[hr];
        for (int i = 1; i <= 20; ++i) {
            for (auto& [dir, val] : camList[i]) rgbCamData[i][dir] += val;
        }
    }
    return true;
}

static bool readThermalDataForClass(int classNumber, string primaryDate, const string& monthFolder) {
    auto best = getBestHourlyThermalData(classNumber, primaryDate, monthFolder);
    for (int i = 1; i <= 10; i++)
        for (const auto& dir : thermal_cam_dirs[i]) thermalCamData[i][dir] = 0.0;
    for (int hr = 0; hr < 24; ++hr) {
        auto& camList = best[hr];
        for (int i = 1; i <= 10; ++i) {
            for (auto& [dir, val] : camList[i]) thermalCamData[i][dir] += val;
        }
    }
    return true;
}

// Single entry point to read both RGB and Thermal data for a given class
static void readData(int classNumber, string date, const string& monthFolder) {
    readRgbDataForClass(classNumber, date, monthFolder);
    readThermalDataForClass(classNumber, date, monthFolder);
}

// === Data Calculation ===
//


// Picks the first consecutive week of a given month using Zeller's congruence algorithm
void pickDates(vector<string> &dates, int year, int month) {
    if (month < 1 || month > 12) return;

    int zMonth = month;
    int zYear = year;
    if (zMonth < 3) { zMonth += 12; zYear -= 1; }

    int y = zYear % 100;
    int c = zYear / 100;

    int first_monday = -1;
    // Find the first Monday of the month
    for (int i = 1; i <= 7; ++i) {
        int h = (i + (13 * (zMonth + 1)) / 5 + y + y/4 + c/4 + 5 * c) % 7; // Zeller's algo
        if (h == 2) { first_monday = i; break; }
    }

    if (first_monday == -1) return;

    // Produce consecutive 7 days starting from first_monday
    for (int k = 0; k < 7; ++k) {
        int day = first_monday + k; // A continuous Monday through Sunday week
        char buf[11];
        snprintf(buf, sizeof(buf), "%04d%02d%02d", year, month, day);
        if ((size_t)k < dates.size()) dates[k] = buf;
    }
}

// Returns the X value for a certain day of the month for a specific link
// returns a specific X_(j, d, m)
double calcLinkX(int link, vector<pair<double,double>>& values) {
    return values[link].first + values[link].second;
}

// Generate wide CSV summary: rows=links, columns=X1..Xn,Z,F1..Fn and Group Scaling Factors
static void writeLinkSummaryWide(const vector<vector<double>>& Xs, const vector<string>& dates, int month, const string &label, const string& monthFolder, int classNumber) {
    // Global link_lengths used instead of local vector.

    auto safeLen = [&](size_t link) -> double {
        if (link < link_lengths.size()) return (double)link_lengths[link];
        return 0.0;
    };

    size_t numDates = dates.size();
    if (Xs.size() <= 1) {
        cerr << "No link data to write." << endl;
        return;
    }

    size_t numLinks = Xs.size() - 1;
    size_t baseLinks = min<size_t>(10, numLinks);

    vector<double> Z(numLinks + 1, 0.0);
    vector<vector<double>> F(numLinks + 1, vector<double>(numDates, 0.0));

    // Compute Z and F for base links (1..10)
    for (size_t i = 1; i <= baseLinks; ++i) {
        double sum = 0.0;
        for (size_t d = 0; d < numDates; ++d) sum += Xs[i][d];
        Z[i] = (numDates > 0) ? (sum / (double)numDates) : 0.0;
        for (size_t d = 0; d < numDates; ++d) {
            F[i][d] = (Xs[i][d] != 0.0) ? (Z[i] / Xs[i][d]) : 0.0;
        }
    }

    // Group scaling factors per day = average(F[1..10][day])
    vector<double> groupSF(numDates, 0.0);
    for (size_t d = 0; d < numDates; ++d) {
        double s = 0.0;
        for (size_t i = 1; i <= baseLinks; ++i) s += F[i][d];
        groupSF[d] = (baseLinks > 0) ? (s / (double)baseLinks) : 0.0;
    }

    // Variance/std/eps of base-link Fs per day (your existing logic)
    vector<double> varF(numDates, 0.0);
    vector<double> stdF(numDates, 0.0);
    vector<double> eps(numDates, 0.0);
    for (size_t d = 0; d < numDates; ++d) {
        if (baseLinks <= 1) { varF[d] = 0.0; stdF[d] = 0.0; eps[d] = 0.0; continue; }

        double sumsq = 0.0;
        for (size_t i = 1; i <= baseLinks; ++i) sumsq += F[i][d] * F[i][d];

        double numerator = sumsq - (double)baseLinks * groupSF[d] * groupSF[d];
        varF[d] = (baseLinks > 1) ? (numerator / (double)(baseLinks - 1)) : 0.0;
        if (varF[d] < 0.0 && varF[d] > -1e-12) varF[d] = 0.0;

        stdF[d] = (varF[d] > 0.0) ? sqrt(varF[d] / baseLinks) : 0.0;
        eps[d] = (groupSF[d] != 0.0) ? (2.228 * stdF[d] / groupSF[d]) : 0.0;
    }

    // For links beyond first 10, compute Z as SUMPRODUCT(Xs[link][d], groupSF[d]) / numDates
    for (size_t i = baseLinks + 1; i <= numLinks; ++i) {
        double sp = 0.0;
        for (size_t d = 0; d < numDates; ++d) sp += Xs[i][d] * groupSF[d];
        Z[i] = (numDates > 0) ? (sp / (double)numDates) : 0.0;
    }

    // =========================
    // BKM: Z bar and T (Excel)
    // =========================

    // Z bar:
    // -> weighted average of Z for links 11..14 and 17..18, weighted by lengths.
    vector<size_t> zbarLinks;
    for (size_t k = 11; k <= 14; ++k) zbarLinks.push_back(k);
    zbarLinks.push_back(17);
    zbarLinks.push_back(18);

    double zbarNum = 0.0;
    double zbarDen = 0.0;
    for (size_t k : zbarLinks) {
        if (k <= numLinks) {
            double L = safeLen(k);
            zbarNum += L * Z[k];
            zbarDen += L;
        }
    }
    double Zbar = (zbarDen != 0.0) ? (zbarNum / zbarDen) : 0.0;

    // T:
    // T:
    // =SUMPRODUCT(C3:C12,M3:M12)+D35*SUM(C25:C32)
    // -> sum_{1..10}(L*Z) + Zbar * sum_{11..18}(L)
    double sumCoreLenZ = 0.0;
    for (size_t k = 1; k <= min<size_t>(10, numLinks); ++k) {
        sumCoreLenZ += safeLen(k) * Z[k];
    }

    double sumCoverageLen = 0.0;
    for (size_t k = 11; k <= min<size_t>(18, numLinks); ++k) {
        sumCoverageLen += safeLen(k);
    }

    double T = sumCoreLenZ + Zbar * sumCoverageLen;

    // build filename like "Links_<Label>_<Class>_2025-01.csv"
    string timeLabel = monthFolder.empty() ? "All" : monthFolder;
    string className = "Object";
    if (classNumber == 1) className = "person";
    else if (classNumber == 2) className = "bicycle";
    else if (classNumber == 3) className = "e-scooter";

    string outDir = string("Results/") + timeLabel + "/Links";
    std::filesystem::create_directories(outDir);
    string outName = outDir + "/Links_" + label + "_" + className + "_" + timeLabel + ".csv";
    ofstream out(outName);
    if (!out.is_open()) {
        cerr << "Failed to open " << outName << " for writing." << endl;
        return;
    }

    out << "Link";
    for (size_t d = 0; d < numDates; ++d) out << ",X" << (d + 1);
    out << ",Z";
    for (size_t d = 0; d < numDates; ++d) out << ",F" << (d + 1);
    out << "\n";

    for (size_t i = 1; i <= numLinks; ++i) {
        out << fixed << setprecision(2);
        out << i;
        for (size_t d = 0; d < numDates; ++d) out << "," << Xs[i][d];
        out << "," << Z[i];

        out << setprecision(12);
        for (size_t d = 0; d < numDates; ++d) {
            if (i <= baseLinks) out << "," << F[i][d];
            else out << ",";
        }
        out << "\n";
    }
    out << defaultfloat;

    out << "\nGroup Scaling Factors";
    out << setprecision(12);
    for (size_t d = 0; d < numDates; ++d) out << "," << groupSF[d];
    out << "\n";

    out << "Variance";
    for (size_t d = 0; d < numDates; ++d) out << "," << varF[d];
    out << "\n";

    out << "StdDev";
    for (size_t d = 0; d < numDates; ++d) out << "," << stdF[d];
    out << "\n";

    out << "Epsilon";
    for (size_t d = 0; d < numDates; ++d) out << "," << eps[d];
    out << "\n";

    // Append BKM section
    out << "\nBKM\n";
    out << fixed << setprecision(2);
    out << "Z_bar," << Zbar << "\n";
    out << "T," << T << "\n";

    out.close();

    cout << "Wrote CSV: " << outName << endl;
    cout << fixed << setprecision(2);
    cout << "BKM: Z_bar=" << Zbar << "  T=" << T << "\n";
    cout << defaultfloat;
}

void generateFullYearlyDatabase(int classNumber) {
    string className = (classNumber == 1) ? "person" : (classNumber == 2 ? "bicycle" : "scooter");
    string outRgb = "Results/Hourly_Database_RGB_" + className + ".csv";
    string outThermal = "Results/Hourly_Database_Thermal_" + className + ".csv";
    string outMape = "Results/Hourly_Comparison_MAPE_" + className + ".csv";

    ofstream fRgb(outRgb), fTh(outThermal), fMp(outMape);
    
    auto writeHeaders = [&](ofstream& f, bool mape) {
        f << "Date,Hour";
        for (int i = 1; i <= 18; ++i) f << ",Link" << i << (mape ? "_MAPE" : "_BKM");
        f << "\n";
    };
    writeHeaders(fRgb, false); writeHeaders(fTh, false); writeHeaders(fMp, true);

    vector<int> years = {2025, 2026};
    for (int y : years) {
        for (int m = 1; m <= 12; ++m) {
            int days = 31;
            if (m == 4 || m == 6 || m == 9 || m == 11) days = 30;
            else if (m == 2) days = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 29 : 28;

            string monthFolder = to_string(y) + "-" + (m < 10 ? "0" : "") + to_string(m);
            cout << "Processing month: " << monthFolder << endl;

            for (int d = 1; d <= days; ++d) {
                char buf[9]; snprintf(buf, sizeof(buf), "%04d%02d%02d", y, m, d);
                string primaryDate(buf, 8);

                auto hourlyRgb = getBestHourlyRgbData(classNumber, primaryDate, monthFolder);
                auto hourlyTh = getBestHourlyThermalData(classNumber, primaryDate, monthFolder);

                for (int hr = 0; hr < 24; ++hr) {
                    char dateBuf[11]; snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", y, m, d);
                    fRgb << dateBuf << "," << hr;
                    fTh << dateBuf << "," << hr;
                    fMp << dateBuf << "," << hr;

                    // Compute RGB Links
                    rgbCamData = hourlyRgb[hr];
                    computeLinkValues(link_formulas_ground, link_values_ground);
                    
                    // Compute Thermal Links
                    thermalCamData = hourlyTh[hr];
                    computeLinkValues(link_formulas, link_values);

                    for (int i = 1; i <= 18; ++i) {
                        double L = (i < (int)link_lengths.size()) ? link_lengths[i] : 0.0;
                        double rVal = (link_values_ground[i].first + link_values_ground[i].second) * L;
                        double tVal = (link_values[i].first + link_values[i].second) * L;
                        double mape = (rVal != 0.0) ? (tVal - rVal) / rVal : 0.0;

                        fRgb << "," << fixed << setprecision(2) << rVal;
                        fTh << "," << fixed << setprecision(2) << tVal;
                        fMp << "," << fixed << setprecision(4) << mape;
                    }
                    fRgb << "\n"; fTh << "\n"; fMp << "\n";
                }
            }
        }
    }
    cout << "Databases created in Results/ folder." << endl;
}

int main(int argc, char** argv) {
    if (argc > 1 && string(argv[1]) == "database") {
        int classNumber = (argc > 2) ? stoi(argv[2]) : 1;
        generateFullYearlyDatabase(classNumber);
        return 0;
    }
    cout << "---- Select the class number ----" << endl;
    int classNumber;
    cin >> classNumber;
    int numberOfClasses = 3;

    cout << "Enter month (YYYY-MM), e.g. 2025-03: ";
    string monthFolder;
    cin >> monthFolder;

    int year = 0, month = 0;
    if (monthFolder.size() >= 7 && monthFolder[4] == '-') {
        try {
            year = stoi(monthFolder.substr(0,4));
            month = stoi(monthFolder.substr(5,2));
        } catch (...) { year = 0; month = 0; }
    }

    vector<string> dates(7, string());
    if (year > 0 && month > 0) pickDates(dates, year, month);

    size_t numLinks = (link_formulas_ground.size() > 0 ? link_formulas_ground.size() - 1 : 0);
    vector<vector<double>> Xs_ground(numLinks + 1, vector<double>(dates.size(), 0.0));
    vector<vector<double>> Xs_thermal(numLinks + 1, vector<double>(dates.size(), 0.0));

    for (size_t di = 0; di < dates.size(); ++di) {
        string date = dates[di];
        if (date.empty()) continue;
        cout << "--- DATE: " << date << " --- " << endl;

        auto hourlyRgb = getBestHourlyRgbData(classNumber, date, monthFolder);
        auto hourlyTh = getBestHourlyThermalData(classNumber, date, monthFolder);

        // Sum for whole day to match legacy output
        for (int hr = 0; hr < 24; ++hr) {
            rgbCamData = hourlyRgb[hr];
            computeLinkValues(link_formulas_ground, link_values_ground);
            thermalCamData = hourlyTh[hr];
            computeLinkValues(link_formulas, link_values);

            for (size_t i = 1; i <= numLinks; ++i) {
                Xs_ground[i][di] += (link_values_ground[i].first + link_values_ground[i].second);
                Xs_thermal[i][di] += (link_values[i].first + link_values[i].second);
            }
        }
        printLinkValues(link_values_ground, "RGB (Sum of Day)");
        printLinkValues(link_values, "Thermal (Sum of Day)");
    }

    writeLinkSummaryWide(Xs_ground, dates, month, string("RGB"), monthFolder, classNumber);
    writeLinkSummaryWide(Xs_thermal, dates, month, string("Thermal"), monthFolder, classNumber);

    return 0;
}
