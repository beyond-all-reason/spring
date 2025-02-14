#include "System/Platform/CpuTopology.h"

#include "System/Log/ILog.h"

#include <bit>
#include <windows.h>

namespace cpu_topology {

// Mingw v12 is the minimum version that gives us a GetLogicalProcessorInformationEx() with efficiency core
// detection. Unfortunately mingw v12 also introduces a ticking time bomb change that can cause the toolchain to
// produce a crashing exe because of a change in static initialized variable space.
// See bug report for more information: https://sourceforge.net/p/mingw-w64/bugs/992/
// The net result is that we have to use GetProcAddress() on the kernel32 ourselves and provide a modified set
// of data definitions with the information we need.
// We can do away with this if 1) mingw fix their bug or 2) the build system shifts over to using MSVC.
namespace spring_overrides {

    typedef struct _GROUP_AFFINITY {
        KAFFINITY Mask;
        WORD      Group;
        WORD      Reserved[3];
        } GROUP_AFFINITY, *PGROUP_AFFINITY;

    typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP {
        RelationProcessorCore,
        RelationNumaNode,
        RelationCache,
        RelationProcessorPackage,
        RelationGroup,
        RelationProcessorDie,
        RelationNumaNodeEx,
        RelationProcessorModule,
        RelationAll = 0xffff
        } LOGICAL_PROCESSOR_RELATIONSHIP;

    typedef struct _PROCESSOR_RELATIONSHIP {
        BYTE Flags;
        BYTE EfficiencyClass;
        BYTE Reserved[20];
        WORD GroupCount;
        GROUP_AFFINITY GroupMask[ANYSIZE_ARRAY];
        } PROCESSOR_RELATIONSHIP,*PPROCESSOR_RELATIONSHIP;

    struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {
        LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
        DWORD Size;
        __C89_NAMELESS union {
            PROCESSOR_RELATIONSHIP Processor;
            //NUMA_NODE_RELATIONSHIP NumaNode;
            //CACHE_RELATIONSHIP Cache;
            //GROUP_RELATIONSHIP Group;
        };
        };

        typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;
    
    #if _WIN32_WINNT >= 0x0601
    typedef BOOL (WINAPI *GetLogicalProcessorInformationExFunc)(
        LOGICAL_PROCESSOR_RELATIONSHIP,
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
        PDWORD);
    //WINBASEAPI WINBOOL WINAPI GetLogicalProcessorInformationEx (LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer, PDWORD ReturnedLength);
    #endif
}


ProcessorMasks GetProcessorMasks() {
    spring_overrides::GetLogicalProcessorInformationExFunc Local_GetLogicalProcessorInformationEx;
    spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = NULL;
    spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr = NULL;
    DWORD returnLength = 0;
    DWORD byteOffset = 0;
    BOOL done = FALSE;
    ProcessorMasks processorMasks;

    Local_GetLogicalProcessorInformationEx = (spring_overrides::GetLogicalProcessorInformationExFunc) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),
        "GetLogicalProcessorInformationEx");
    if (NULL == Local_GetLogicalProcessorInformationEx)
    {
        LOG("GetLogicalProcessorInformation is not supported.\n");
        return processorMasks;
    }

    while (!done)
    {
        DWORD rc = Local_GetLogicalProcessorInformationEx(spring_overrides::RelationProcessorCore, buffer, &returnLength);

        if (FALSE == rc)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if (buffer)
                    free(buffer);

                buffer = (spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)malloc(returnLength);

                if (NULL == buffer)
                    return processorMasks;
            }
            else
                return processorMasks;
        }
        else
            done = TRUE;
    }
    ptr = buffer;

    while (byteOffset < returnLength)
    {
        if (ptr->Relationship == spring_overrides::RelationProcessorCore)
        {
            const uint32_t supportedMask = static_cast<uint32_t>(ptr->Processor.GroupMask[0].Mask);
            if (supportedMask == 0) {
                LOG("Info: Processor group %d has a thread mask outside of the supported range."
                    , int(ptr->Processor.GroupCount));
                break;
            }

            const bool hyperThreading = !std::has_single_bit(supportedMask);
            if (hyperThreading) {
                processorMasks.hyperThreadLowMask |= ( 1 << std::countr_zero(supportedMask) );
                processorMasks.hyperThreadHighMask |= ( 0x80000000 >> std::countl_zero(supportedMask) );
            }

            if (ptr->Processor.EfficiencyClass){
                processorMasks.efficiencyCoreMask |= supportedMask;
            } else {
                processorMasks.performanceCoreMask |= supportedMask;
            }
        }
        byteOffset += ptr->Size;
        ptr = (spring_overrides::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(((char*)buffer) + byteOffset);
    }

    if (buffer)
        free(buffer);

    return processorMasks;
}

} //namespace cpu_topology
