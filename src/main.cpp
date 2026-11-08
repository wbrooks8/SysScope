#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#ifdef __linux__
#include <fstream>
#include <string>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "Unsupported platform"
#endif

class CpuMonitor {
public:
    // Returns CPU usage in [0, 100] since the previous call, or -1.0 on failure.
    // The first call primes the baseline and returns 0.0.
    double sample() {
        unsigned long long idle = 0;
        unsigned long long total = 0;
        if (!readTimes(idle, total)) return -1.0;

        const unsigned long long deltaIdle = idle - prevIdle_;
        const unsigned long long deltaTotal = total - prevTotal_;
        const bool first = !primed_;

        prevIdle_ = idle;
        prevTotal_ = total;
        primed_ = true;

        if (first || deltaTotal == 0) return 0.0;

        const unsigned long long idleCapped = std::min(deltaIdle, deltaTotal);
        return 100.0 * (1.0 - static_cast<double>(idleCapped) / static_cast<double>(deltaTotal));
    }

private:
    static bool readTimes(unsigned long long &idle, unsigned long long &total);

    unsigned long long prevIdle_ = 0;
    unsigned long long prevTotal_ = 0;
    bool primed_ = false;
};

#ifdef __linux__
inline bool CpuMonitor::readTimes(unsigned long long &idle, unsigned long long &total) {
    std::ifstream file("/proc/stat");
    std::string label;
    unsigned long long v[8] = {}; // user nice system idle iowait irq softirq steal

    file >> label; // skip "cpu"
    for (auto &x: v) file >> x;
    if (!file) return false; // also covers failure to open

    idle = v[3] + v[4]; // idle + iowait
    total = 0;
    for (auto x: v) total += x;
    return true;
}

#elif defined(_WIN32)
inline bool CpuMonitor::readTimes(unsigned long long &idle, unsigned long long &total) {
    FILETIME idleFt, kernelFt, userFt;
    if (!GetSystemTimes(&idleFt, &kernelFt, &userFt)) return false;

    auto toU64 = [](const FILETIME &ft) {
        return (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    idle = toU64(idleFt);
    total = toU64(kernelFt) + toU64(userFt);
    return true;
}
#endif

int main() {
    CpuMonitor cpu;
    cpu.sample(); // prime the baseline

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "CPU Usage: " << cpu.sample() << "%\n";
}
