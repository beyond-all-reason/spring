#include "System/Platform/CpuTopology.h"

#include "System/Log/ILog.h"
#include "System/Platform/ThreadAffinityGuard.h"


#include <algorithm>
#include <bitset>
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#include <cpuid.h>
#elif defined(__aarch64__) || defined(__arm__) || defined(_M_ARM64)
// ARM doesn't need cpuid.h
#else
	#error "Unsupported architecture"
#endif
#include <pthread.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <sched.h>

namespace cpu_topology {

#define MAX_CPUS 32  // Maximum logical CPUs

enum CoreType { CORE_PERFORMANCE, CORE_EFFICIENCY, CORE_UNKNOWN };

// Get number of logical CPUs
int get_cpu_count() {
	return sysconf(_SC_NPROCESSORS_CONF);
}

std::vector<int> get_online_cpus() {
	std::vector<int> cpus;
	std::ifstream file("/sys/devices/system/cpu/online");
	if (file) {
		// This is a comma-seperated list of ranges
		// or single values.
		// Ex: 0,2,4,6 or 0-7 or 0-3,8-15
		std::string line;
		std::getline(file, line);
		std::istringstream ss(line);
		int min_cpu;
		int max_cpu;
		char sep;
		while (ss >> min_cpu) {
			if ((ss >> sep) && sep == '-') {
				// Range of CPUs separted by '-'
				if (!(ss >> max_cpu)) {
					// Should not ever happen (would need to be a malformed online file)
					if (min_cpu >= MAX_CPUS) {
						LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", min_cpu);
					} else {
						cpus.push_back(min_cpu);
					}
					break;
				}
				for (int cpu = min_cpu; cpu <= max_cpu; ++cpu) {
					if (cpu >= MAX_CPUS) {
						LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
						continue;
					}
					cpus.push_back(cpu);
				}
				// Consume the trailing comma
				ss >> sep;
			} else {
				// Single CPU
				if (min_cpu >= MAX_CPUS) {
					LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", min_cpu);
					continue;
				}
				cpus.push_back(min_cpu);
			}
		}
	} else {
		// Fallback in case of permission issues reading from sysfs
		int num_cpus = get_cpu_count();
		for (int cpu = 0; cpu < num_cpus; ++cpu) {
			if (cpu >= MAX_CPUS) {
				LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
				continue;
			}
			cpus.push_back(cpu);
		}
	}
	return cpus;
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

void collect_smt_affinity_masks(int cpu,
								std::bitset<MAX_CPUS> &low_smt_mask,
								std::bitset<MAX_CPUS> &high_smt_mask) {
	std::vector<int> siblings = get_thread_siblings(cpu);
	bool smt_enabled = siblings.size() > 1;
	if (smt_enabled) {
		if (cpu == *std::min_element(siblings.begin(), siblings.end())) {
			low_smt_mask.set(cpu);
		} else {
			high_smt_mask.set(cpu);
		}
	}
}

ThreadPinPolicy GetThreadPinPolicy() {
	return THREAD_PIN_POLICY_ANY_PERF_CORE;
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)

enum Vendor { VENDOR_INTEL, VENDOR_AMD, VENDOR_UNKNOWN };

// Detect CPU vendor (Intel or VENDOR_AMD)
Vendor detect_cpu_vendor() {
	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
	__get_cpuid(0, &eax, &ebx, &ecx, &edx);
	if (ebx == 0x756E6547) return VENDOR_INTEL; // "GenuineIntel"
	if (ebx == 0x68747541) return VENDOR_AMD;   // "AuthenticAMD"
	return VENDOR_UNKNOWN;
}

// Detect Intel core type using CPUID 0x1A
CoreType get_intel_core_type(int cpu) {
	set_cpu_affinity(cpu);
	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
	if (__get_cpuid(0x1A, &eax, &ebx, &ecx, &edx)) {
		uint8_t coreType = ( eax & 0xFF000000 ) >> 24;  // Extract core type

		if (coreType & 0x40) return CORE_PERFORMANCE;
		if (coreType & 0x20) return CORE_EFFICIENCY;
	}
	return CORE_UNKNOWN;
}

// Collect CPU affinity masks for Intel
void collect_intel_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								  std::bitset<MAX_CPUS> &perf_mask,
								  std::bitset<MAX_CPUS> &low_ht_mask,
								  std::bitset<MAX_CPUS> &high_ht_mask) {
	const auto cpus = get_online_cpus();

	for (const auto cpu : cpus) {
		CoreType core_type = get_intel_core_type(cpu);
		// default to performance core.
		if (core_type == CORE_UNKNOWN) core_type = CORE_PERFORMANCE;

		if (core_type == CORE_EFFICIENCY) eff_mask.set(cpu);   // Efficiency Core (E-core)
		else if (core_type == CORE_PERFORMANCE) perf_mask.set(cpu);  // Performance Core (P-core)

		collect_smt_affinity_masks(cpu, low_ht_mask, high_ht_mask);
	}
}

// Collect CPU affinity masks for AMD
void collect_amd_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								std::bitset<MAX_CPUS> &perf_mask,
								std::bitset<MAX_CPUS> &low_smt_mask,
								std::bitset<MAX_CPUS> &high_smt_mask) {
	const auto cpus = get_online_cpus();

	for (const auto cpu : cpus) {
		perf_mask.set(cpu);

		collect_smt_affinity_masks(cpu, low_smt_mask, high_smt_mask);
	}
}

uint32_t get_thread_cache(int cpu) {
	std::ifstream file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index3/size");
	uint32_t sizeInBytes = 0;
	if (file) {
		std::string line;
		std::getline(file, line);
		std::istringstream ss(line);
		ss >> sizeInBytes;
	}
	return sizeInBytes;
}

ProcessorGroupCaches& get_group_cache(ProcessorCaches& processorCaches, uint32_t cacheSize) {
	auto foundCache = std::ranges::find_if
		( processorCaches.groupCaches
		, [cacheSize](const auto& gc) -> bool { return (gc.cacheSizes[2] == cacheSize); });

	if (foundCache == processorCaches.groupCaches.end()) {
		processorCaches.groupCaches.push_back({});
		auto& newCacheGroup = processorCaches.groupCaches[processorCaches.groupCaches.size()-1];
		newCacheGroup.cacheSizes[2] = cacheSize;
		return newCacheGroup;
	}

	return (*foundCache);
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

// Notes.
// Here we are grouping by the cache size, which isn't the same a groups and their cache sizes.
// This is fine what our needs at the moment. We're currently only looking a performance core
// with the most cache for the main thread.
// We are also only looking at L3 caches at the moment.
ProcessorCaches GetProcessorCache() {
	ProcessorCaches processorCaches;
	const auto cpus = get_online_cpus();

	for (const auto cpu : cpus) {
		uint32_t cacheSize = get_thread_cache(cpu);
		ProcessorGroupCaches& groupCache = get_group_cache(processorCaches, cacheSize);

		groupCache.groupMask |= (0x1 << cpu);
	}

	return processorCaches;
}

#elif defined(__aarch64__) || defined(__arm__) || defined(_M_ARM64)

// Get the highest cache index available for a CPU (2 for L2, 3 for L3)
int get_arm_highest_cache_index(int cpu) {
	// Check if L3 exists (index3)
	std::ifstream l3_file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index3/size");
	if (l3_file.good()) {
		return 3; // L3 cache exists
	}
	
	// Check if L2 exists (index2)
	std::ifstream l2_file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index2/size");
	if (l2_file.good()) {
		return 2; // L2 cache exists (highest level)
	}
	
	return -1; // No L2 or L3 cache found
}

// Get cache size for a specific CPU and cache index
uint32_t get_arm_cache_size(int cpu, int index) {
	std::ifstream file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index" + std::to_string(index) + "/size");
	uint32_t sizeInKB = 0;
	if (file) {
		std::string line;
		std::getline(file, line);
		// Parse the size (e.g., "128K" -> 128)
		std::istringstream ss(line);
		ss >> sizeInKB;
		// Convert KB to bytes
		sizeInKB *= 1024;
	}
	return sizeInKB;
}

// Get shared CPU list for a cache index
std::vector<int> get_arm_cache_shared_cpus(int cpu, int index) {
	std::ifstream file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index" + std::to_string(index) + "/shared_cpu_list");
	std::vector<int> shared_cpus;
	if (file) {
		std::string line;
		std::getline(file, line);
		std::istringstream ss(line);
		
		// Parse CPU ranges like "0-1" or "2-5" or single CPUs
		while (!ss.eof()) {
			int start_cpu, end_cpu;
			char dash;
			ss >> start_cpu;
			if (ss.peek() == '-') {
				ss >> dash >> end_cpu;
				for (int i = start_cpu; i <= end_cpu; ++i) {
					shared_cpus.push_back(i);
				}
			} else {
				shared_cpus.push_back(start_cpu);
			}
			// Skip comma if present
			if (ss.peek() == ',') {
				ss.ignore();
			}
		}
	}
	return shared_cpus;
}

// Detect ARM core types based on cache sizes
void detect_arm_core_types(std::bitset<MAX_CPUS> &eff_mask, std::bitset<MAX_CPUS> &perf_mask) {
	int num_cpus = get_cpu_count();
	std::vector<std::pair<int, uint32_t>> cpu_cache_sizes;
	
	// First, determine the highest cache level and collect sizes
	int highest_cache_index = -1;
	for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
		int cache_index = get_arm_highest_cache_index(cpu);
		if (cache_index > highest_cache_index) {
			highest_cache_index = cache_index;
		}
	}
	
	if (highest_cache_index == -1) {
		// No L2 or L3 cache found, all cores are performance cores
		for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
			perf_mask.set(cpu);
		}
		return;
	}
	
	// Collect cache sizes for the highest cache level
	uint32_t min_cache_size = UINT32_MAX;
	for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
		uint32_t cache_size = get_arm_cache_size(cpu, highest_cache_index);
		cpu_cache_sizes.push_back({cpu, cache_size});
		if (cache_size > 0 && cache_size < min_cache_size) {
			min_cache_size = cache_size;
		}
	}
	
	// Classify cores based on cache size
	if (min_cache_size == UINT32_MAX) {
		// No valid cache sizes found, all cores are performance cores
		for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
			perf_mask.set(cpu);
		}
		return;
	}
	
	// Cores with the smallest cache are efficiency cores, others are performance cores
	bool has_different_sizes = false;
	for (const auto& [cpu, cache_size] : cpu_cache_sizes) {
		if (cache_size != min_cache_size && cache_size > 0) {
			has_different_sizes = true;
			break;
		}
	}
	
	if (!has_different_sizes) {
		// All cores have the same cache size, all are performance cores
		for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
			perf_mask.set(cpu);
		}
	} else {
		// Cores with minimum cache are efficiency, others are performance
		for (const auto& [cpu, cache_size] : cpu_cache_sizes) {
			if (cache_size == min_cache_size) {
				eff_mask.set(cpu);
			} else {
				perf_mask.set(cpu);
			}
		}
	}
}

// Collect CPU affinity masks for ARM
void collect_arm_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								std::bitset<MAX_CPUS> &perf_mask,
								std::bitset<MAX_CPUS> &low_smt_mask,
								std::bitset<MAX_CPUS> &high_smt_mask) {
	int num_cpus = get_cpu_count();
	
	// Detect core types based on cache sizes
	detect_arm_core_types(eff_mask, perf_mask);
	
	// Detect SMT
	for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
		collect_smt_affinity_masks(cpu, low_smt_mask, high_smt_mask);
	}
}

ProcessorMasks GetProcessorMasks() {
	ThreadAffinityGuard guard;
	ProcessorMasks processorMasks;

	std::bitset<MAX_CPUS> eff_mask, perf_mask, low_ht_mask, high_ht_mask;

	LOG("Detected ARM CPU.");
	collect_arm_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);

	processorMasks.efficiencyCoreMask = eff_mask.to_ulong();
	processorMasks.performanceCoreMask = perf_mask.to_ulong();
	processorMasks.hyperThreadLowMask = low_ht_mask.to_ulong();
	processorMasks.hyperThreadHighMask = high_ht_mask.to_ulong();

	return processorMasks;
}

// Helper to find or create a cache group for ARM
ProcessorGroupCaches& get_arm_group_cache(ProcessorCaches& processorCaches, uint32_t cacheSize, int cacheLevel, bool useExistingGroup) {
	// Store in the appropriate index based on cache level
	// L2 cache (index2) goes into cacheSizes[1]
	// L3 cache (index3) goes into cacheSizes[2]
	int cacheSizeIndex = (cacheLevel == 3) ? 2 : 1;
	
	// If we should use an existing group (fallback mode when shared_cpu_list is not available)
	if (useExistingGroup) {
		auto foundCache = std::ranges::find_if
			( processorCaches.groupCaches
			, [cacheSize, cacheSizeIndex](const auto& gc) -> bool { 
				return (gc.cacheSizes[cacheSizeIndex] == cacheSize); 
			});

		if (foundCache != processorCaches.groupCaches.end()) {
			return (*foundCache);
		}
	}
	
	// Create a new group
	processorCaches.groupCaches.push_back({});
	auto& newCacheGroup = processorCaches.groupCaches[processorCaches.groupCaches.size()-1];
	newCacheGroup.cacheSizes[cacheSizeIndex] = cacheSize;
	return newCacheGroup;
}

ProcessorCaches GetProcessorCache() {
	ProcessorCaches processorCaches;
	int num_cpus = get_cpu_count();
	
	// First determine the highest cache level available
	int highest_cache_index = -1;
	for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
		int cache_index = get_arm_highest_cache_index(cpu);
		if (cache_index > highest_cache_index) {
			highest_cache_index = cache_index;
		}
	}
	
	if (highest_cache_index == -1) {
		LOG_L(L_WARNING, "No L2 or L3 cache found on ARM system");
		return processorCaches;
	}
	
	// Track which CPUs we've already processed (to handle shared caches)
	std::bitset<MAX_CPUS> processed;
	
	// Process each CPU and group by shared cache
	for (int cpu = 0; cpu < num_cpus && cpu < MAX_CPUS; ++cpu) {
		if (processed.test(cpu)) {
			continue; // Already part of a cache group
		}
		
		uint32_t cacheSize = get_arm_cache_size(cpu, highest_cache_index);
		if (cacheSize == 0) {
			continue; // No cache at this level
		}
		
		// Get all CPUs that share this cache
		std::vector<int> shared_cpus = get_arm_cache_shared_cpus(cpu, highest_cache_index);
		
		// Determine whether to use existing group based on cache size
		// If shared_cpu_list was not available or only contains the current CPU, fall back to grouping by size
		bool useExistingGroup = (shared_cpus.empty() || (shared_cpus.size() == 1 && shared_cpus[0] == cpu));
		
		// Create or find the cache group
		ProcessorGroupCaches& groupCache = get_arm_group_cache(processorCaches, cacheSize, highest_cache_index, useExistingGroup);
		
		// Add CPUs to this group
		if (shared_cpus.empty()) {
			// No shared CPU info available, just add this CPU
			groupCache.groupMask |= (0x1 << cpu);
			processed.set(cpu);
		} else {
			// Add all shared CPUs to this group
			for (int shared_cpu : shared_cpus) {
				if (shared_cpu < MAX_CPUS) {
					groupCache.groupMask |= (0x1 << shared_cpu);
					processed.set(shared_cpu);
				}
			}
		}
	}
	
	// Sort cache groups: larger caches (performance cores) first
	int cacheSizeIndex = (highest_cache_index == 3) ? 2 : 1;
	std::ranges::stable_sort
		( processorCaches.groupCaches
		, [cacheSizeIndex](const auto &lh, const auto &rh) -> bool { 
			return lh.cacheSizes[cacheSizeIndex] > rh.cacheSizes[cacheSizeIndex]; 
		});
	
	return processorCaches;
}

#else
	#error "Unsupported architecture"
#endif

} //namespace cpu_topology
