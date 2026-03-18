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

using namespace std;

// === Formulas and Dirs (Simplified copy from main.cpp) ===
vector<vector<string>> thermal_cam_dirs = { {}, {"a1", "a2"}, {"b1", "b2", "b3", "b4", "b5", "b6"}, {"b2", "b3", "b4"}, {"b1", "b5", "b6"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"} };
vector<vector<string>> rgb_cam_dirs = { {}, {"a1", "a2"}, {"a1", "a2"}, {"b1", "b2", "b3", "b4", "b5", "b6"}, {"a1", "a2"}, {"b1", "b2", "b3", "b4", "b5", "b6"}, {}, {"a1", "a2"}, {"b1", "b2", "b3", "b4", "b5", "b6"}, {"a1", "a2"}, {"b1", "b2", "b3", "b4", "b5", "b6"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"a1", "a2"}, {"b1", "b2", "b3", "b4", "b5", "b6"}, {"a1", "a2"} };
vector<double> link_lengths = { 0.0, 485.46, 201.69, 165.82, 258.36, 243.31, 218.62, 243.70, 240.41, 91.69, 632.71, 439.35, 365.94, 252.41, 322.35, 153.77, 206.52, 32.09, 197.33 };

vector<unordered_map<string, double>> rgbCamData(21);
vector<unordered_map<string, double>> thermalCamData(11);
vector<pair<double,double>> link_values(19, {0,0}), link_values_ground(19, {0,0});

static double evalSimpleExpr(const string &s) {
    size_t idx = 0;
    auto skipSpaces = [&]() { while(idx < s.size() && isspace(s[idx])) idx++; };
    function<double()> parseExpression;
    function<double()> parseFactor = [&]() {
        skipSpaces();
        if(idx < s.size() && s[idx] == '(') { idx++; double v = parseExpression(); skipSpaces(); if(idx < s.size() && s[idx] == ')') idx++; return v; }
        if(idx < s.size() && isalpha(s[idx])) {
            string name; while(idx < s.size() && isalpha(s[idx])) name += s[idx++];
            skipSpaces(); if(idx < s.size() && s[idx] == '(') {
                idx++; vector<double> args;
                while(idx < s.size() && s[idx] != ')') { args.push_back(parseExpression()); skipSpaces(); if(idx < s.size() && s[idx] == ',') idx++; skipSpaces(); }
                if(idx < s.size() && s[idx] == ')') idx++;
                if(name == "avg") { double sum = 0; int cnt = 0; for(double v : args) { if(v > 1e-9) { sum += v; cnt++; } } return cnt ? sum/cnt : 0.0; }
            }
            return 0.0;
        }
        string num; while(idx < s.size() && (isdigit(s[idx]) || s[idx] == '.')) num += s[idx++];
        return num.empty() ? 0.0 : stod(num);
    };
    parseExpression = [&]() {
        double val = parseFactor(); skipSpaces();
        while(idx < s.size() && (s[idx] == '+' || s[idx] == '-')) {
            char op = s[idx++]; double next = parseFactor();
            if(op == '+') val += next; else val -= next; skipSpaces();
        }
        return val;
    };
    return parseExpression();
}

double calculateDir(const string &expr, bool ground) {
    if(expr.empty()) return 0.0;
    string expanded = expr;
    // Replace e.g. 3b3 with value
    for(int cam=1; cam<=20; cam++) {
        string prefix = to_string(cam);
        auto it = (ground ? rgbCamData[cam] : (cam <= 10 ? thermalCamData[cam] : unordered_map<string,double>()));
        for(auto& pair : it) {
            string key = prefix + pair.first;
            size_t pos = 0;
            while((pos = expanded.find(key, pos)) != string::npos) {
                expanded.replace(pos, key.length(), to_string(pair.second));
            }
        }
    }
    // Handle link[n]
    for(int n=1; n<=18; n++) {
        string key = "link[" + to_string(n) + "]";
        size_t pos = 0;
        while((pos = expanded.find(key, pos)) != string::npos) {
            double val = (ground ? link_values_ground[n].first : link_values[n].first);
            expanded.replace(pos, key.length(), to_string(val));
        }
    }
    return evalSimpleExpr(expanded);
}

void computeLinks(const vector<pair<string,string>>& formulas, vector<pair<double,double>>& values, bool ground) {
    for(int i=1; i<=18; i++) {
        values[i].first = calculateDir(formulas[i].first, ground);
        values[i].second = calculateDir(formulas[i].second, ground);
    }
}

// ... Formulas ...
vector<pair<string, string>> f_ground = { {"",""}, {"avg((3b3+3b5), 2a2)", "avg((3b1+3b2), 2a1)"}, {"3b3+3b4", "3b1+3b6"}, {"avg((3b5+3b6), 18a1)", "avg((3b2+3b4), 18a2)"}, {"5b3+5b5", "5b1+5b2"}, {"5b3+5b4", "5b1+5b6"}, {"5b5+5b6", "5b2+5b4"}, {"avg((10b4+10b5), 12a1)", "avg((10b2+10b6), 12a2)"}, {"avg((10b4+10b5), 12a1)", "avg((10b2+10b6), 12a2)"}, {"13a1", "13a2"}, {"18a1", "18a2"}, {"2a1", "2a2"}, {"link[2]-link[4]", "link[2]-link[4]"}, {"10b5+10b3", "10b1+10b2"}, {"link[5]-link[6]", "link[5]-link[6]"}, {"16a1", "16a2"}, {"16a2", "16a1"}, {"link[6]+link[14]", "link[12]+link[18]"}, {"15a1", "15a2"} };
vector<pair<string, string>> f_thermal = { {"",""}, {"avg((t2b5+t2b3),t1a1)", "avg((t2b2+t2b1), t1a2)"}, {"t2b4+t2b3", "t2b6+t2b1"}, {"avg((t2b5+t2b6),t9a1,t10a1)", "avg((t2b2+t2b4),t9a2,t10a2)"}, {"avg(t3b3+t3b5, t4b1+t4b2)", "avg(t3b1+t3b2, t4b1+t4b2)"}, {"avg(t3b3+t3b4, t4b1+t4b6)", "avg(t3b1+t3b6, t4b2+t4b4)"}, {"avg(t3b5+t3b6, t4b2+t4b4)", "avg(t3b2+t3b4, t4b4+t4b5)"}, {"avg((t9a1+t9a2), t5a1)", "avg((t9a2+t10a2), t5a2)"}, {"avg((t9a1+t9a2), t6a1)", "avg((t9a2+t10a2), t6a2)"}, {"avg(t7a1, t8a1)", "avg(t7a2, t8a2)"}, {"t9a1", "t9a2"}, {"t1a1", "t1a2"}, {"link[2]-link[4]", "link[2]-link[4]"}, {"t9a1+t10a1", "t9a2+t10a2"}, {"link[5]-link[6]", "link[5]-link[6]"}, {"t7a1", "t7a2"}, {"t7a2", "t7a1"}, {"link[6]+link[14]", "link[12]+link[18]"}, {"t9a1", "t9a2"} };

int main() {
    // Range: 20260205, 20260206
    double totalBkmRgb = 0, totalBkmTh = 0;
    vector<string> dates = {"20260205", "20260206"};
    for(string dt : dates) {
        for(int hr=0; hr<24; hr++) {
            // Load CSVs for all cams ... 
            // (To simplify, we'll assume a helper handles individual hour loading)
            // This script is too long to include data loading. I will use the main engine binary.
        }
    }
}
