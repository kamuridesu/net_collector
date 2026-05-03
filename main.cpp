#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <regex>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

struct SiteMetrics {
    std::string domain;
    double min_lat = 0;
    double avg_lat = 0;
    double max_lat = 0;
    double jitter = 0;
    int packet_loss = 0;
    std::string dns_server_address;
    double dns_resolution_time_ms = 0;
};

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("_popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void parsePing(const std::string& output, SiteMetrics& metrics) {
    std::regex time_regex("tempo[=<]([0-9]+)ms");
    if (output.find("time=") != std::string::npos) {
        time_regex = std::regex("time[=<]([0-9]+)ms");
    }

    std::vector<double> latencies;
    auto words_begin = std::sregex_iterator(output.begin(), output.end(), time_regex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        latencies.push_back(std::stod(match[1].str()));
    }

    if (latencies.size() > 1) {
        double total_diff = 0;
        for (size_t i = 1; i < latencies.size(); ++i) {
            total_diff += std::abs(latencies[i] - latencies[i - 1]);
        }
        metrics.jitter = total_diff / (latencies.size() - 1);
    }

    std::regex ms_regex("= ([0-9]+)ms");
    std::vector<double> summary_stats;
    auto summary_begin = std::sregex_iterator(output.begin(), output.end(), ms_regex);
    auto summary_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = summary_begin; i != summary_end; ++i) {
        std::smatch match = *i;
        summary_stats.push_back(std::stod(match[1].str()));
    }

    if (summary_stats.size() >= 3) {
        metrics.avg_lat = summary_stats.back();
        summary_stats.pop_back();
        metrics.max_lat = summary_stats.back();
        summary_stats.pop_back();
        metrics.min_lat = summary_stats.back();
    }

    std::regex loss_regex("([0-9]+)%");
    std::smatch loss_match;
    if (std::regex_search(output, loss_match, loss_regex)) {
        metrics.packet_loss = std::stoi(loss_match[1].str());
    }
}

std::string getDnsServer() {
    std::string dns_server = "Unknown";
    FIXED_INFO *pFixedInfo = (FIXED_INFO *) malloc(sizeof(FIXED_INFO));
    ULONG ulOutBufLen = sizeof(FIXED_INFO);
    
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pFixedInfo);
        pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
    }
    
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR) {
        dns_server = pFixedInfo->DnsServerList.IpAddress.String;
    }
    free(pFixedInfo);
    return dns_server;
}

void printDashboardHeader() {
    std::cout << "\n================================================================" << std::endl;
    std::cout << " NETWORK DATA COLLECTOR v1.1 " << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << std::left << std::setw(20) << "Domain" 
              << std::setw(15) << "Lat(min/avg/max)" 
              << std::setw(10) << "Jitter" 
              << std::setw(8) << "Loss" 
              << "DNS Time" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
}

void printSiteData(const SiteMetrics& m) {
    std::string lat_str = std::to_string((int)m.min_lat) + "/" + std::to_string((int)m.avg_lat) + "/" + std::to_string((int)m.max_lat);
    std::cout << std::left << std::setw(20) << m.domain 
              << std::setw(15) << lat_str
              << std::fixed << std::setprecision(1) << std::setw(10) << m.jitter
              << std::setw(8) << (std::to_string(m.packet_loss) + "%")
              << std::fixed << std::setprecision(2) << m.dns_resolution_time_ms << "ms" << std::endl;
}

int main() {
    std::vector<std::string> sites = {
        "google.com",
        "tiktok.com",
        "web.whatsapp.com",
        "uol.com.br",
        "chatgpt.com"
    };

    std::vector<SiteMetrics> results;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    std::string default_dns = getDnsServer();

    printDashboardHeader();

    for (const auto& domain : sites) {
        SiteMetrics metrics;
        metrics.domain = domain;

        std::string ping_cmd = "ping " + domain + " -n 10";
        std::string ping_out = exec(ping_cmd.c_str());
        parsePing(ping_out, metrics);

        metrics.dns_server_address = default_dns;

        auto start = std::chrono::high_resolution_clock::now();
        struct addrinfo *result = NULL;
        struct addrinfo hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        DWORD dwRetval = getaddrinfo(domain.c_str(), "80", &hints, &result);
        auto end = std::chrono::high_resolution_clock::now();
        
        if (dwRetval == 0) {
            freeaddrinfo(result);
        }
        
        metrics.dns_resolution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        results.push_back(metrics);
        printSiteData(metrics);
    }

    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << "Saving results to network_data.json..." << std::endl;

    std::ofstream file("network_data.json");
    file << "[\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        file << "  {\n";
        file << "    \"domain\": \"" << r.domain << "\",\n";
        file << "    \"latency\": {\n";
        file << "      \"min\": " << r.min_lat << ",\n";
        file << "      \"avg\": " << r.avg_lat << ",\n";
        file << "      \"max\": " << r.max_lat << "\n";
        file << "    },\n";
        file << "    \"jitter\": " << std::fixed << std::setprecision(2) << r.jitter << ",\n";
        file << "    \"packet_loss\": " << r.packet_loss << ",\n";
        file << "    \"dns\": {\n";
        file << "      \"server_address\": \"" << r.dns_server_address << "\",\n";
        file << "      \"resolution_time_ms\": " << std::fixed << std::setprecision(2) << r.dns_resolution_time_ms << "\n";
        file << "    }\n";
        file << "  }" << (i == results.size() - 1 ? "" : ",") << "\n";
    }
    file << "]";
    file.close();

    std::cout << "Done! Press Enter to exit." << std::endl;
    std::cin.get();

    WSACleanup();
    return 0;
}
