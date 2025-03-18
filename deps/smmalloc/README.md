# smmalloc 
[![Actions Status](https://github.com/SergeyMakeev/smmalloc/workflows/build/badge.svg)](https://github.com/SergeyMakeev/smmalloc/actions) [![Build status](https://ci.appveyor.com/api/projects/status/8c3oqiywaa8firfy?svg=true)](https://ci.appveyor.com/project/SergeyMakeev/smmalloc/) [![codecov](https://codecov.io/gh/SergeyMakeev/smmalloc/branch/master/graph/badge.svg?token=DKANC80IO5)](https://codecov.io/gh/SergeyMakeev/smmalloc) ![MIT](https://img.shields.io/badge/license-MIT-blue.svg)

## About

smmalloc is a fast and efficient "proxy" allocator designed to handle many small allocations/deallocations in heavy multithreaded scenarios.
The allocator created for using in applications where the performance is critical such as video games.
Designed to speed up the typical memory allocation pattern in C++ such as many small allocations and deletions.
This is a proxy allocator means that smmalloc handles only specific allocation sizes and pass-through all other allocations to generic heap allocator.

## Commercial games using smmalloc

- Warface (PS4, X1, Nintendo Switch)


## Features

* Near zero size overhead for handled allocations (allocator does not store any per-allocation meta information)
* Blazing fast (~40 cycles per operation) when allocator thread cache is used (configurable)
* Very simple to integrate on any platform. Just make your current allocator as fallback allocator and let the smmalloc handle specific allocations and speed up your application.
* Highly scalable in multithread environment.

## Performance

Here is an example of performance comparison for several different allocators.  
Platform: Windows 10, Intel Core i7-2600

Threads | #1 | #2 | #3 | #4 | #5
--- | --- | --- | --- |--- |---
crt | 23853865 | 15410410 | 15993655 | 14124607 | 14636381
rpmalloc | 75866414 | 52689298 | 52606215 |46058909 | 38706739
hoard | 65922032 | 46605339 | 42874516 | 34404618 | 27629651
ltalloc | 62525965 | 52315981 | 41634992 | 33557726 | 27333887
smmalloc | 92615384 | 70584046 | 66352324 | 47087501 | 38303161
smmalloc (no thread cache) | 49295774 | 25991465 | 11342809 | 8615216 | 6455889
dlmalloc + mutex | 31313394 | 5858632 | 3824636 | 3354672 | 2135141
dlmalloc | 106304079 | 0 | 0 | 0 | 0

![Performance comparison](https://raw.githubusercontent.com/SergeyMakeev/smmalloc/master/Images/i7_results.png)


Here is an example of performance comparison for several different allocators.  
Platform: Playstation 4

Threads | #1 | #2 | #3 | #4
--- | --- | --- | --- |--- 
mspace | 4741379 | 956729 | 457264 | 366920
crt | 4444444 | 853385 | 419009 | 332095
ltalloc | 28571429 | 25290698 | 19248174 | 14683637
smmalloc | 36065574 | 29333333 | 25202412 | 21868691
smmalloc (no thread cache)  | 22916667 | 8527132 | 5631815 | 4198497
dlmalloc + mutex | 8058608 | 1848623 | 579845 | 564604
dlmalloc | 35483871 | 0 | 0 | 0

![Performance comparison](https://raw.githubusercontent.com/SergeyMakeev/smmalloc/master/Images/ps4_results.png)

## Usage

**_sm_allocator_create** - create allocator instance  
**_sm_allocator_destroy** - destroy allocator instance  
**_sm_allocator_thread_cache_create** - create thread cache for current thread  
**_sm_allocator_thread_cache_destroy** - destroy thread cache for current thread  
**_sm_malloc** - allocate aligned memory block  
**_sm_free** - free memory block  
**_sm_realloc** - reallocate memory block  
**_sm_msize** - get usable memory size  

Tiny code example
```cpp

// create allocator to handle 16, 32, 48 and 64 allocations (4 buckets, 16Mb each) 
sm_allocator space = _sm_allocator_create(4, (16 * 1024 * 1024));

// allocate 19 bytes with 16 bytes alignment
void* p = _sm_malloc(space, 19, 16);

// free memory
_sm_free(space, p)

// destroy allocator
_sm_allocator_destroy(space);

```
