// ipcalc.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <nlohmann/json.hpp>
#include <getopt.h>

using namespace std;
using json = nlohmann::json;

uint32_t ipToInt(const string& ip) {
    stringstream ss(ip);
    string octet;
    uint32_t result = 0;
    while (getline(ss, octet, '.')) {
        result = (result << 8) + stoi(octet);
    }
    return result;
}

string intToIP(uint32_t num) {
    return to_string((num >> 24) & 0xff) + "." +
           to_string((num >> 16) & 0xff) + "." +
           to_string((num >> 8) & 0xff) + "." +
           to_string(num & 0xff);
}

uint32_t prefixToMask(int prefix) {
    return (~0u << (32 - prefix)) & 0xffffffff;
}

int maskToPrefix(uint32_t mask) {
    int count = 0;
    while (mask) {
        count++;
        mask &= mask - 1;
    }
    return count;
}

string ipBinary(uint32_t num) {
    auto byteToBin = [](uint8_t b) {
        string s;
        for (int i = 7; i >= 0; i--) s += (b & (1 << i)) ? '1' : '0';
        return s;
    };
    return byteToBin((num >> 24) & 0xff) + "." +
           byteToBin((num >> 16) & 0xff) + "." +
           byteToBin((num >> 8) & 0xff) + "." +
           byteToBin(num & 0xff);
}

struct Result {
    string ip, mask, network, broadcast, wildcard;
    string first_usable, last_usable;
    int prefix, total_hosts, usable_hosts;
};

Result calculate(const string& input, const string& maskStr) {
    uint32_t ipInt, maskInt;
    int prefix;
    size_t pos = input.find('/');
    if (pos != string::npos) {
        string ipPart = input.substr(0, pos);
        string prefStr = input.substr(pos+1);
        ipInt = ipToInt(ipPart);
        prefix = stoi(prefStr);
        maskInt = prefixToMask(prefix);
    } else if (!maskStr.empty()) {
        ipInt = ipToInt(input);
        if (maskStr.find('.') != string::npos) {
            maskInt = ipToInt(maskStr);
            prefix = maskToPrefix(maskInt);
        } else {
            prefix = stoi(maskStr);
            maskInt = prefixToMask(prefix);
        }
    } else {
        throw runtime_error("Invalid input");
    }
    uint32_t networkInt = ipInt & maskInt;
    uint32_t broadcast = networkInt | (~maskInt & 0xffffffff);
    uint32_t wildcard = ~maskInt & 0xffffffff;
    int total = 1 << (32 - prefix);
    int usable = max(0, total - 2);
    string first = (networkInt == broadcast) ? "" : intToIP(networkInt + 1);
    string last = (networkInt == broadcast) ? "" : intToIP(broadcast - 1);
    Result r;
    r.ip = intToIP(ipInt);
    r.mask = intToIP(maskInt);
    r.prefix = prefix;
    r.network = intToIP(networkInt);
    r.broadcast = intToIP(broadcast);
    r.wildcard = intToIP(wildcard);
    r.first_usable = first;
    r.last_usable = last;
    r.total_hosts = total;
    r.usable_hosts = usable;
    return r;
}

vector<map<string,string>> subnets(const Result& base, int newPrefix) {
    if (newPrefix <= base.prefix) throw runtime_error("New prefix must be larger than original");
    int num = 1 << (newPrefix - base.prefix);
    uint32_t step = 1u << (32 - newPrefix);
    uint32_t netInt = ipToInt(base.network);
    vector<map<string,string>> subs;
    for (int i=0; i<num; i++) {
        uint32_t n = netInt + i * step;
        Result sub = calculate(intToIP(n), to_string(newPrefix));
        map<string,string> m;
        m["network"] = sub.network;
        m["prefix"] = to_string(sub.prefix);
        m["first_usable"] = sub.first_usable.empty() ? "none" : sub.first_usable;
        m["last_usable"] = sub.last_usable.empty() ? "none" : sub.last_usable;
        subs.push_back(m);
    }
    return subs;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: ipcalc <ip>[/prefix] [mask] [--subnet PREFIX] [--bin] [--json]\n";
        return 1;
    }
    string input = argv[1];
    string mask;
    bool bin = false, jsonOut = false;
    int subnetPrefix = -1;
    for (int i=2; i<argc; i++) {
        string arg = argv[i];
        if (arg == "--subnet" && i+1 < argc) subnetPrefix = stoi(argv[++i]);
        else if (arg == "--bin") bin = true;
        else if (arg == "--json") jsonOut = true;
        else if (arg[0] != '-') mask = arg;
    }
    Result res;
    try {
        res = calculate(input, mask);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    vector<map<string,string>> subs;
    if (subnetPrefix != -1) {
        try {
            subs = subnets(res, subnetPrefix);
        } catch (const exception& e) {
            cerr << "Subnet error: " << e.what() << "\n";
            return 1;
        }
    }
    json j;
    j["ip"] = res.ip;
    j["mask"] = res.mask;
    j["prefix"] = res.prefix;
    j["network"] = res.network;
    j["broadcast"] = res.broadcast;
    j["wildcard"] = res.wildcard;
    j["first_usable"] = res.first_usable.empty() ? nullptr : json(res.first_usable);
    j["last_usable"] = res.last_usable.empty() ? nullptr : json(res.last_usable);
    j["total_hosts"] = res.total_hosts;
    j["usable_hosts"] = res.usable_hosts;
    if (bin) {
        j["binary_ip"] = ipBinary(ipToInt(res.ip));
        j["binary_mask"] = ipBinary(ipToInt(res.mask));
        j["binary_network"] = ipBinary(ipToInt(res.network));
    }
    if (!subs.empty()) {
        json subArr = json::array();
        for (auto& m : subs) {
            json item;
            item["network"] = m["network"];
            item["prefix"] = m["prefix"];
            item["first_usable"] = m["first_usable"];
            item["last_usable"] = m["last_usable"];
            subArr.push_back(item);
        }
        j["subnets"] = subArr;
    }
    if (jsonOut) {
        cout << j.dump(2) << "\n";
        return 0;
    }
    cout << "\nIP Calculator\n";
    cout << "IP Address:     " << res.ip << "\n";
    cout << "Subnet Mask:    " << res.mask << " (/" << res.prefix << ")\n";
    cout << "Network:        " << res.network << "\n";
    cout << "Broadcast:      " << res.broadcast << "\n";
    cout << "Wildcard Mask:  " << res.wildcard << "\n";
    if (!res.first_usable.empty()) {
        cout << "First Usable:   " << res.first_usable << "\n";
        cout << "Last Usable:    " << res.last_usable << "\n";
    } else {
        cout << "First Usable:   (none)\n";
        cout << "Last Usable:    (none)\n";
    }
    cout << "Total Hosts:    " << res.total_hosts << "\n";
    cout << "Usable Hosts:   " << res.usable_hosts << "\n";
    if (bin) {
        cout << "\nBinary:\n";
        cout << "IP:      " << j["binary_ip"].get<string>() << "\n";
        cout << "Mask:    " << j["binary_mask"].get<string>() << "\n";
        cout << "Network: " << j["binary_network"].get<string>() << "\n";
    }
    if (!subs.empty()) {
        cout << "\nSubnets (/" << subnetPrefix << "):\n";
        for (auto& item : subs) {
            cout << "  " << item["network"] << "/" << item["prefix"] << "   (" << item["first_usable"] << " - " << item["last_usable"] << ")\n";
        }
    }
    return 0;
}
