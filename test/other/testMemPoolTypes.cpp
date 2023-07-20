#include "System/MemPoolTypes.h"
#include "System/Log/ILog.h"

#include <vector>
#include <cstddef>

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

namespace {
struct TestData
{
    TestData(uint64_t& counter) :
        counter(++counter) { }
    ~TestData() {
        --counter;
    }
    uint64_t& counter;
};

template <typename T>
struct AllocFixture {
    AllocFixture() : mempool(std::make_unique<T>()) {}

    TestData* alloc() {
        return mempool->template alloc<TestData>(instanceCounter);
    }

    void free(TestData* obj) {
        return mempool->free(obj);
    }

    std::unique_ptr<T> mempool;
    uint64_t instanceCounter=0;
};

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test allocator when full", "[class][template]",
        (StaticMemPool<1,sizeof(TestData)>),
        (FixedDynMemPool<sizeof(TestData), 1, 1>))
{
    AllocFixture<TestType> inst;
    auto& mempool = *inst.mempool;
    auto* obj = inst.alloc();
    REQUIRE(obj);
    REQUIRE(!mempool.can_alloc());

    // StaticMemPool silently fails
    //auto* obj2 = mempool.allocMem(1);
    //REQUIRE(!obj2);
}

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test allocator base functionality", "[class][template]",
        (StaticMemPool<1024,sizeof(TestData)>),
        (FixedDynMemPool<sizeof(TestData), 1, 1024>))
{
    AllocFixture<TestType> inst;
    auto& mempool = *inst.mempool;

    REQUIRE(mempool.can_alloc());

    std::vector<TestData*> allocated;

    for (size_t i =0; i < mempool.NUM_PAGES(); ++i) {
        REQUIRE(mempool.can_alloc());
        auto obj = inst.alloc();
        REQUIRE(mempool.alloced(obj));
        allocated.push_back(obj);
    }

    REQUIRE(inst.instanceCounter == mempool.NUM_PAGES());

    REQUIRE(!mempool.can_alloc());

    for (auto* obj : allocated) {
        REQUIRE(mempool.can_free());
        mempool.free(obj);
    }
    REQUIRE(inst.instanceCounter == 0);
    REQUIRE(!mempool.can_free());
}

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test reuse allocator's memory", "[class][template]",
        (StaticMemPool<1024,sizeof(TestData)>),
        (FixedDynMemPool<sizeof(TestData), 1, 1024>))
{
    AllocFixture<TestType> inst;
    auto& mempool = *inst.mempool;

    std::vector<TestData*> allocated;
    for (size_t i =0; i < mempool.NUM_PAGES(); ++i) {
        auto obj = inst.alloc();
        allocated.push_back(obj);
    }

    std::for_each(allocated.rbegin(), allocated.rend(), [&](auto* obj) {
        mempool.free(obj);
    });

    // verify that new elements reuse previous memory
    for (size_t i = 0; i < mempool.NUM_PAGES(); ++i) {
        auto obj = inst.alloc();
        REQUIRE(obj == allocated[i]);
    }

    for (auto* obj : allocated) {
        mempool.free(obj);
    }
}

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test allocator's clear method", "[class][template]",
        (StaticMemPool<1024,sizeof(TestData)>),
        (FixedDynMemPool<sizeof(TestData), 1, 1024>))
{
    AllocFixture<TestType> inst;
    auto& mempool = *inst.mempool;

    for (size_t i =0; i < mempool.NUM_PAGES(); ++i) {
        auto obj = inst.alloc();
    }

    mempool.clear(); // might not call destructors

    for (size_t i =0; i < mempool.NUM_PAGES(); ++i) {
        REQUIRE(mempool.can_alloc());
        auto obj = inst.alloc();
    }
}

} // unnamed namespace
