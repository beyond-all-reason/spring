#include "System/Platform/CpuTopology.h"

#include "System/Log/ILog.h"
#include "System/Platform/ThreadAffinityGuard.h"


#include <algorithm>
#include <bitset>
#include <cpuid.h>
#include <pthread.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <sched.h>

namespace cpu_topology {

#define MAX_CPUS 32  // Maximum logical CPUs
	
enum Vendor { VENDOR_INTEL, VENDOR_AMD, VENDOR_UNKNOWN };

enum CoreType { CORE_PERFORMANCE, CORE_EFFICIENCY, CORE_UNKNOWN };

// Detect CPU vendor (Intel or VENDOR_AMD)
Vendor detect_cpu_vendor() {
    unsigned int eax, ebx, ecx, edx;
    __get_cpuid(0, &eax, &ebx, &ecx, &edx);
    if (ebx == 0x756E6547) return VENDOR_INTEL; // "GenuineIntel"
    if (ebx == 0x68747541) return VENDOR_AMD;   // "AuthenticAMD"
    return VENDOR_UNKNOWN;
}

// Get number of logical CPUs
int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_CONF);
}

// Set CPU affinity to a specific core
void set_cpu_affinity(uint32_t cpu) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);

    pthread_t thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(mask), &mask) != 0) {
        perror("pthread_setaffinity_np");
    }
}

// Detect Intel core type using CPUID 0x1A
CoreType get_intel_core_type(int cpu) {
    set_cpu_affinity(cpu);
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(0x1A, &eax, &ebx, &ecx, &edx)) {
        uint8_t coreType = ( eax & 0xFF000000 ) >> 24;  // Extract core type

        if (coreType & 0x40) return CORE_PERFORMANCE;
        if (coreType & 0x20) return CORE_EFFICIENCY;
    }
    return CORE_UNKNOWN;
}

// Get thread siblings for a CPU
std::vector<int> get_thread_siblings(int cpu) {
    std::ifstream file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/thread_siblings_list");
    std::vector<int> siblings;
    if (file) {
        std::string line;
        std::getline(file, line);
        std::istringstream ss(line);
        int sibling;
        char sep;
        while (ss >> sibling) {
            siblings.push_back(sibling);
            ss >> sep;  // Skip separator (comma or other)
        }
    }
    return siblings;
}

// Collect CPU affinity masks for Intel
void collect_intel_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
                                  std::bitset<MAX_CPUS> &perf_mask,
                                  std::bitset<MAX_CPUS> &low_ht_mask,
                                  std::bitset<MAX_CPUS> &high_ht_mask) {
    int num_cpus = get_cpu_count();

    for (int cpu = 0; cpu < num_cpus; ++cpu) {
        if (cpu >= MAX_CPUS) {
            LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
            continue;
        }

        CoreType core_type = get_intel_core_type(cpu);
        // default to performance core.
        if (core_type == CORE_UNKNOWN) core_type = CORE_PERFORMANCE;

        if (core_type == CORE_EFFICIENCY) eff_mask.set(cpu);   // Efficiency Core (E-core)
        else if (core_type == CORE_PERFORMANCE) perf_mask.set(cpu);  // Performance Core (P-core)

        std::vector<int> siblings = get_thread_siblings(cpu);
        bool smt_enabled = siblings.size() > 1;
        if (smt_enabled) {
            if (!siblings.empty() && cpu == *std::min_element(siblings.begin(), siblings.end())) {
                low_ht_mask.set(cpu);
            } else {
                high_ht_mask.set(cpu);
            }
        }
    }
}

// Collect CPU affinity masks for AMD
void collect_amd_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
                                std::bitset<MAX_CPUS> &perf_mask,
                                std::bitset<MAX_CPUS> &low_ht_mask,
                                std::bitset<MAX_CPUS> &high_ht_mask) {
    int num_cpus = get_cpu_count();

    for (int cpu = 0; cpu < num_cpus; ++cpu) {
        if (cpu >= MAX_CPUS) {
            LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
            continue;
        }

        perf_mask.set(cpu);

        std::vector<int> siblings = get_thread_siblings(cpu);
        bool smt_enabled = siblings.size() > 1;
        if (smt_enabled) {
            if (!siblings.empty() && cpu == *std::min_element(siblings.begin(), siblings.end())) {
                low_ht_mask.set(cpu);
            } else {
                high_ht_mask.set(cpu);
            }
        }
    }
}

ProcessorMasks GetProcessorMasks() {
    ThreadAffinityGuard guard;
    ProcessorMasks processorMasks;

    std::bitset<MAX_CPUS> eff_mask, perf_mask, low_ht_mask, high_ht_mask;
    Vendor cpu_vendor = detect_cpu_vendor();

    if (cpu_vendor == VENDOR_INTEL) {
        LOG("Detected Intel CPU.");
        collect_intel_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);
    } else if (cpu_vendor == VENDOR_AMD) {
        LOG("Detected AMD CPU.");
        collect_amd_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);
    } else {
        LOG_L(L_WARNING, "Unknown or unsupported CPU vendor.");
    }

    processorMasks.efficiencyCoreMask = eff_mask.to_ulong();
    processorMasks.performanceCoreMask = perf_mask.to_ulong();
    processorMasks.hyperThreadLowMask = low_ht_mask.to_ulong();
    processorMasks.hyperThreadHighMask = high_ht_mask.to_ulong();

    return processorMasks;
}

} //namespace cpu_topology
