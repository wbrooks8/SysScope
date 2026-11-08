#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>

unsigned long long FileTimeToInt64(const FILETIME &ft) {
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return li.QuadPart;
}
#endif

double readCpu() {
#ifdef __linux__
    static unsigned long long prevIdle = 0, prevTotal = 0;

    std::ifstream file("/proc/stat");
    if (!file.is_open()) return -1.0;

    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);

    std::string cpuLabel;
    iss >> cpuLabel; // Skip "cpu" string

    std::vector<long long> values;
    long long val;
    while (iss >> val) {
        values.push_back(val);
    }

    if (values.size() < 4) return -1.0;

    long long idle = values[3] + (values.size() > 4 ? values[4] : 0); // idle + iowait
    long long total = std::accumulate(values.begin(), values.end(), 0LL);

    if (prevTotal == 0) {
        prevIdle = idle;
        prevTotal = total;
        return 0.0;
    }

    unsigned long long deltaTotal = total - prevTotal;
    unsigned long long deltaIdle = idle - prevIdle;

    prevIdle = idle;
    prevTotal = total;

    if (deltaTotal == 0) return 0.0;

    double usage = 100.0 * (1.0 - (static_cast<double>(deltaIdle) / static_cast<double>(deltaTotal)));
    return std::clamp(usage, 0.0, 100.0);

#elif defined(_WIN32)
    static FILETIME prevIdle = {0, 0}, prevKernel = {0, 0}, prevUser = {0, 0};
    FILETIME idle, kernel, user;

    if (!GetSystemTimes(&idle, &kernel, &user)) return -1.0;

    unsigned long long idleTicks = FileTimeToInt64(idle);
    unsigned long long kernelTicks = FileTimeToInt64(kernel);
    unsigned long long userTicks = FileTimeToInt64(user);

    unsigned long long prevIdleTicks = FileTimeToInt64(prevIdle);
    unsigned long long prevKernelTicks = FileTimeToInt64(prevKernel);
    unsigned long long prevUserTicks = FileTimeToInt64(prevUser);

    unsigned long long totalTicks = kernelTicks + userTicks;
    unsigned long long prevTotal = prevKernelTicks + prevUserTicks;

    if (prevTotal == 0) {
        prevIdle = idle;
        prevKernel = kernel;
        prevUser = user;
        return 0.0;
    }

    unsigned long long deltaTotal = totalTicks - prevTotal;
    if (deltaTotal == 0) return 0.0;

    unsigned long long deltaIdle = idleTicks - prevIdleTicks;

    prevIdle = idle;
    prevKernel = kernel;
    prevUser = user;

    if (deltaIdle > deltaTotal) deltaIdle = deltaTotal;

    double usage = 100.0 * (1.0 - (static_cast<double>(deltaIdle) / static_cast<double>(deltaTotal)));
    return std::clamp(usage, 0.0, 100.0);

#else
#error "Unsupported platform"
#endif
}

int main() {
    // First call initializes baseline (returns 0.0)
    readCpu();

    // Wait a second to allow a meaningful delta measurement
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Second call returns actual CPU usage over that interval
    std::cout << "CPU Usage: " << readCpu() << "%" << std::endl;

    return 0;
}