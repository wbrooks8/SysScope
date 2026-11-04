
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

struct CpuTimes {
  long user{};
  long nice{};
  long system{};
  long idle{};
  long iowait{};
  long irq{};
  long softirq{};
  long steal{};
  long guest{};
  long guest_nice{};
};

CpuTimes readCpu() {
  std::ifstream stat("/proc/stat"); // opens the procfs file
  std::string line;
  std::getline(stat, line); // read first line
  CpuTimes t{};             // zero-initialize all fields
  std::istringstream ss(line.substr(4)); //string the leading "cpu"
  ss >> t.user >> t.nice >> t.system >> t.idle
       >> t.iowait >> t.irq >> t.softirq >> t.steal
       >> t.guest >> t.guest_nice;      // parse the 10 numeric fields
  return t;

}

// sum all fields to get total
long total(const CpuTimes& t) {
  return t.user + t.nice + t.system + t.idle + t.iowait
       + t.irq + t.softirq + t.steal + t.guest + t.guest_nice;
}

int main() {
  while (true) {
    auto t1 = readCpu();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto t2 = readCpu();

    long dt = total(t2) - total(t1);
    long di = t2.idle - t1.idle;

    double utilization = (1.0 - static_cast<double>(di) / static_cast<double>(dt)) * 100.0;

    std::cout << "Utilization: " << utilization << "%\n";


  }

  std::cout << "SysScope - Linux System Monitor\n";
  std::cout << "Version 0.1.0\n";

  return 0;
}
