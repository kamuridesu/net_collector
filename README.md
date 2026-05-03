# Network Data Collector

A lightweight C++ utility designed to monitor and collect real-time network performance metrics. This tool is specifically tailored for evaluating internet service provider (ISP) quality by measuring latency, jitter, and DNS performance across popular web services.

## 🚀 Features

- **Multi-Site Monitoring**: Automatically tests connectivity to major platforms:
  - Google
  - TikTok
  - WhatsApp
  - UOL
  - ChatGPT
- **Comprehensive Metrics**:
  - **Latency**: Captures Minimum, Average, and Maximum round-trip times (RTT).
  - **Jitter**: Calculates the variation in latency between consecutive pings.
  - **Packet Loss**: Tracks the percentage of failed requests.
  - **DNS Resolution**: Measures the time taken to resolve domain names using the system's configured DNS.
- **Dual Output**:
  - **Live Dashboard**: A clean, formatted terminal interface for real-time monitoring.
  - **JSON Export**: Automatically saves all results to `network_data.json` for easy integration with web dashboards or analysis tools.
- **Windows Optimized**: Uses native Win32 APIs and WinSock2 for accurate system-level data.

## 🛠️ Requirements

- **Operating System**: Windows 10/11
- **Compiler**: GCC (MinGW-w64) or MSVC
- **Dependencies**: 
  - `Ws2_32.lib` (Windows Sockets)
  - `Iphlpapi.lib` (IP Helper API)

## 📦 Compilation

To compile the project using G++, run the following command in your terminal:

```bash
g++ main.cpp -o net_collector.exe -lws2_32 -liphlpapi
```

*Note: If you have a resource file for the application icon, include it in the compilation:*
```bash
windres resource.rc -O coff -o resource.res
g++ main.cpp resource.res -o net_collector.exe -lws2_32 -liphlpapi
```

## 📊 Output Format

The tool generates a `network_data.json` file structured as follows:

```json
[
  {
    "domain": "google.com",
    "latency": {
      "min": 15,
      "avg": 18,
      "max": 25
    },
    "jitter": 1.2,
    "packet_loss": 0,
    "dns": {
      "server_address": "192.168.1.1",
      "resolution_time_ms": 45.5
    }
  }
]
```

## 📝 License

This project is open-source and intended for personal network diagnostic use.
