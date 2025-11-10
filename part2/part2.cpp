// In this solution, we use the external library httplib to implement connectivity to the end-point GET /fapi/v1/aggTrades
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstdlib>
#include "httplib.h"

using namespace std;
using namespace chrono;

// This struct contains the parsed values
struct AggTrade {
    int64_t a, f, l, T;
    string p, q;
    bool m;

    void print() {
        cout << "{ \"a\": " << a
             << ", \"p\": \"" << p
             << "\", \"q\": \"" << q
             << "\", \"f\": " << f
             << ", \"l\": " << l
             << ", \"T\": " << T
             << ", \"m\": " << (m ? "true" : "false")
             << " }\n";
    }
};

// Function for skipping whitespace and other "not interesting" characters
void skip_ws(string& s, size_t& i) {
    while (i < s.size() && (s[i]==' '||s[i]=='\n' ||s[i]=='\t'||s[i]==':')) ++i;
}

// Function for parsing integer fields
int64_t parse_int64_after(string& obj, size_t pos) {
    size_t i = pos; 
    skip_ws(obj, i);
    int64_t v = 0;
    while (i < obj.size() && obj[i] >= '0' && obj[i] <= '9') { 
        //As we read from left to right, we multiply the value of v by 10 and add the new value
        v = v*10 + (obj[i]-'0'); 
        ++i; 
    }
    return v;
}

// Function for parsing string fields
string parse_string_after(string& obj, size_t pos) {
    size_t i = pos; 
    skip_ws(obj, i);
    ++i; // Skip the first "
    size_t start = i;
    while (obj[i] != '"') ++i; //Parse until the closing "
    return obj.substr(start, i - start);
}

// Function for parsing bool field
bool parse_bool_after(string& obj, size_t pos) {
    size_t i = pos; skip_ws(obj, i);
    return obj[i] == 't'; // With the first letter we already know if it's true or false
}

AggTrade parse_trade_object(string& obj) {
    AggTrade t{};
    // We locate field positions to avoid re-scanning the full object later
    size_t pa = obj.find("\"a\"");
    size_t pp = obj.find("\"p\"");
    size_t pq = obj.find("\"q\"");
    size_t pf = obj.find("\"f\"");
    size_t pl = obj.find("\"l\"");
    size_t pT = obj.find("\"T\"");
    size_t pm = obj.find("\"m\"");

    // We parse each field
    t.a = parse_int64_after(obj, obj.find(':', pa)+1);
    t.p = parse_string_after(obj, obj.find(':', pp)+1);
    t.q = parse_string_after(obj, obj.find(':', pq)+1);
    t.f = parse_int64_after(obj, obj.find(':', pf)+1);
    t.l = parse_int64_after(obj, obj.find(':', pl)+1);
    t.T = parse_int64_after(obj, obj.find(':', pT)+1);
    t.m = parse_bool_after(  obj, obj.find(':', pm)+1);
    return t;
}

// Function for parsing the body of the reply
vector<AggTrade> parse_array_of_objects(string& body) {
    vector<AggTrade> out;
    size_t start = 0;
    while (true) {
        // Each trade is located between {}
        size_t open  = body.find('{', start);
        if (open == string::npos) break;
        size_t close = body.find('}', open);
        string obj = body.substr(open, close - open + 1);
        out.push_back(parse_trade_object(obj));
        start = close + 1;
    }
    return out;
}

int main(int argc, char** argv) {
    // The endpoint requires to set the "symbol" parameter. We can also limit the number of requests we want to do.
    string symbol     = (argc >= 2 ? argv[1] : "BTCUSDT");
    int iterations    = (argc >= 3 ? max(1, atoi(argv[2])) : 5);

    httplib::Client cli("https://fapi.binance.com");

    int64_t next_from_id = -1;
    uint64_t total_trades = 0;
    double total_parse_us = 0.0;

    for (int it = 0; it < iterations; ++it) {
        string path = "/fapi/v1/aggTrades?symbol=" + symbol;
        // After first iteration, we can set fromId to avoid duplicates
        if (next_from_id >= 0) path += "&fromId=" + to_string(next_from_id);

        auto res = cli.Get(path);
        if (!res || res->status != 200) { // If we don't receive a positive answer, we wait and try again
            this_thread::sleep_for(milliseconds(50)); 
            continue; 
        }

        // We parse the body
        auto t0 = steady_clock::now();
        string body = res->body;
        vector<AggTrade> trades = parse_array_of_objects(body);
        auto t1 = steady_clock::now();
        
        //We print the results
        for (auto& r : trades) r.print();
        if (!trades.empty()) next_from_id = trades.back().a + 1;

        total_trades += trades.size();
        total_parse_us += duration_cast<duration<double, micro>>(t1 - t0).count();
        // We throttle requests to avoid rate limiting
        this_thread::sleep_for(milliseconds(50));
    }

    double avg_us = total_trades ? total_parse_us / total_trades : 0.0;
    cerr << "Parsed " << total_trades << " trades, avg " << avg_us << " us/trade\n";
    return 0;
}

// The complexity of this solution is O(n), being n the total number of bytes in the body, as we iterate a fixed number of times over it.