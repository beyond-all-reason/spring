#include "System/MemPoolTypes.h"
#include "System/Log/ILog.h"

#include <vector>
#include <cstddef>

#include <catch_amalgamated.hpp>

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

constexpr size_t TEST_ALLOCATOR_SIZE = 1024;

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
		(StaticMemPoolT<1,TestData>),
		(FixedDynMemPoolT<1, 1, TestData>))
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

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test bounded allocator base functionality", "[class][template]",
		(StaticMemPoolT<TEST_ALLOCATOR_SIZE, TestData>),
		(FixedDynMemPoolT<1, TEST_ALLOCATOR_SIZE, TestData>))
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

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test unbounded allocator base functionality", "[class][template]",
		(DynMemPoolT<TestData>))
{
	AllocFixture<TestType> inst;
	auto& mempool = *inst.mempool;

	REQUIRE(mempool.can_alloc());

	std::vector<TestData*> allocated;

	for (size_t i =0; i < TEST_ALLOCATOR_SIZE; ++i) {
		REQUIRE(mempool.can_alloc());
		auto obj = inst.alloc();
		REQUIRE(mempool.alloced(obj));
		allocated.push_back(obj);
	}

	REQUIRE(inst.instanceCounter == TEST_ALLOCATOR_SIZE);

	REQUIRE(mempool.can_alloc()); // unbounded allocator can always alloc

	for (auto* obj : allocated	) {
		REQUIRE(mempool.can_free());
		mempool.free(obj);
	}
	REQUIRE(inst.instanceCounter == 0);
	REQUIRE(!mempool.can_free());
}

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test reuse allocator's memory", "[class][template]",
		(StaticMemPoolT<TEST_ALLOCATOR_SIZE, TestData>),
		(FixedDynMemPoolT<1, TEST_ALLOCATOR_SIZE, TestData>),
		(DynMemPoolT<TestData>))
{
	AllocFixture<TestType> inst;
	auto& mempool = *inst.mempool;

	std::unordered_map<TestData*, int> allocated;
	for (size_t i =0; i < TEST_ALLOCATOR_SIZE; ++i) {
		auto obj = inst.alloc();
		allocated[obj]++;
	}

	for (auto& pair : allocated) {
		TestData* ptr = pair.first;
		mempool.free(ptr);
	};

	for (size_t i = 0; i < TEST_ALLOCATOR_SIZE; ++i) {
		auto obj = inst.alloc();
		REQUIRE(allocated[obj] == 1);
	}

	for (auto& pair : allocated) {
		TestData* ptr = pair.first;
		mempool.free(ptr);
	};
}

// obeying the order is not a functional requirement of allocator
TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test reuse allocator's memory in LIFO order", "[class][template]",
		(StaticMemPoolT<TEST_ALLOCATOR_SIZE, TestData>),
		(FixedDynMemPoolT<1, TEST_ALLOCATOR_SIZE, TestData>),
		(DynMemPoolT<TestData>))
{
	AllocFixture<TestType> inst;
	auto& mempool = *inst.mempool;

	std::vector<TestData*> allocated;
	for (size_t i =0; i < TEST_ALLOCATOR_SIZE; ++i) {
		auto obj = inst.alloc();
		allocated.push_back(obj);
	}

	std::for_each(allocated.rbegin(), allocated.rend(), [&](auto* obj) {
			mempool.free(obj);
			});

	// verify that new elements reuse previous memory
	// in LIFO order
	for (size_t i = 0; i < TEST_ALLOCATOR_SIZE; ++i) {
		auto obj = inst.alloc();
		REQUIRE(obj == allocated[i]);
	}

	for (auto* obj : allocated) {
		mempool.free(obj);
	}
}

TEMPLATE_TEST_CASE_METHOD(AllocFixture, "test allocator's clear method", "[class][template]",
		(StaticMemPoolT<TEST_ALLOCATOR_SIZE, TestData>),
		(FixedDynMemPoolT<1, TEST_ALLOCATOR_SIZE, TestData>))
{
	AllocFixture<TestType> inst;
	auto& mempool = *inst.mempool;

	for (size_t i =0; i < mempool.NUM_PAGES(); ++i) {
		auto obj = inst.alloc();
	}

	// allocators do not call destructors as they do not know object underlying type
	mempool.clear();

	for (size_t i =0; i < mempool.NUM_PAGES(); ++i) {
		REQUIRE(mempool.can_alloc());
		auto obj = inst.alloc();
	}
}

} // unnamed namespace
