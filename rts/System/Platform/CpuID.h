/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CPUID_H
#define CPUID_H

#include "CpuTopology.h"

#if defined(__GNUC__)
	#define _noinline __attribute__((__noinline__))
#else
	#define _noinline
#endif

#include <cstdint>

namespace springproc {
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);

	/** Class to detect the processor topology, more specifically,
	    for now it can detect the number of real (not hyper threaded
	    core.

	    It uses 'cpuid' instructions to query the information. It
	    implements both the new (i7 and above) and legacy (from P4 on
	    methods).

	    The implementation is done only for Intel processor for now, as at
	    the time of the writing it was not clear how to achieve a similar
	    result for AMD CMT multithreading.

	    This file is based on the following documentations from Intel:
	    - "Intel® 64 Architecture Processor Topology Enumeration"
	      (Kuo_CpuTopology_rc1.rh1.final.pdf)
	    - "Intel® 64 and IA-32 Architectures Software Developer’s Manual
	     Volume 3A: System Programming Guide, Part 1"
	      (64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf)
	    - "Intel® 64 and IA-32 Architectures Software Developer’s Manual
	     Volume 2A: Instruction Set Reference, A-M"
	      (64-ia-32-architectures-software-developer-vol-2a-manual.pdf) */

	class CPUID {
	public:
		static CPUID& GetInstance();

		/** Total number of cores in the system. This excludes SMT/HT
		    cores. */
		int GetNumPhysicalCores() const { return numPhysicalCores; }
		int GetNumPerformanceCores() const { return numPerformanceCores; }
		int GetNumLogicalCores() const { return numLogicalCores; }

		bool HasHyperThreading() const { return smtDetected; };

		cpu_topology::ProcessorMasks GetAvailableProcessorAffinityMask() const { return processorMasks; };

		// Logical processor masks and the L3 cache they have access to. The list is sorted groups with largest cache
		// first.
		cpu_topology::ProcessorCaches GetProcessorCaches() const { return processorCaches; }

	private:
		CPUID();

		void EnumerateCores();

		int numLogicalCores;
		int numPhysicalCores;
		int numPerformanceCores;

		cpu_topology::ProcessorMasks processorMasks;
		cpu_topology::ProcessorCaches processorCaches;

		bool smtDetected;
	};

}

#endif // CPUID_H
