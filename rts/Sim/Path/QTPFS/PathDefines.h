#ifndef QTPFS_DEFINES_HDR
#define QTPFS_DEFINES_HDR

#include <cstdint>
#include <limits>

// #define QTPFS_NO_LOADSCREEN
#define QTPFS_SUPPORT_PARTIAL_SEARCHES
// #define QTPFS_TRACE_PATH_SEARCHES
#define QTPFS_SMOOTH_PATHS
// #define QTPFS_CONSERVATIVE_NODE_SPLITS
// #define QTPFS_DEBUG_NODE_HEAP

#define QTPFS_CORNER_CONNECTED_NODES

// #define QTPFS_SLOW_ACCURATE_TESSELATION
// #define QTPFS_ORTHOPROJECTED_EDGE_TRANSITIONS
#define QTPFS_ENABLE_MICRO_OPTIMIZATION_HACKS
// #define QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES

static constexpr int QTPFS_MAX_SMOOTHING_ITERATIONS = 1;

static constexpr int QTPFS_MAX_NETPOINTS_PER_NODE_EDGE = 1;
static constexpr float QTPFS_NETPOINT_EDGE_SPACING_SCALE = (1.0f / (QTPFS_MAX_NETPOINTS_PER_NODE_EDGE + 1));

static constexpr float QTPFS_POSITIVE_INFINITY = (std::numeric_limits<float>::infinity());
static constexpr float QTPFS_CLOSED_NODE_COST = (1 << 24);

static constexpr int QTPFS_LAST_FRAME = (std::numeric_limits<int>::max());

static constexpr uint32_t QTPFS_MAX_NODE_SIZE = 64;
static constexpr uint32_t QTPFS_BAD_ROOT_NODE_SIZE = 32;

static constexpr uint32_t QTPFS_SHARE_PATH_MIN_SIZE = 2;
static constexpr uint32_t QTPFS_SHARE_PATH_MAX_SIZE = 16;
static constexpr uint32_t QTPFS_PARTIAL_SHARE_PATH_MAX_SIZE = 32;

static constexpr uint32_t QTPFS_MAP_DAMAGE_SIZE = 16;

// Though there are four quads per level, having nothing is like a 5th state. So 3 bits, not 2, is needed per level.
static constexpr uint32_t QTPFS_NODE_NUMBER_SHIFT_STEP = 3;

namespace QTPFS {
    constexpr int SEARCH_DIRS = 2;

	struct PathHashType {
	private:
		uint64_t low;
		uint64_t high;
	public:
		PathHashType()
			: low(0)
			, high(0)
		{}

		constexpr PathHashType(uint64_t _low, uint64_t _high)
			: low(_low)
			, high(_high)
		{}

		bool operator==(const PathHashType& other) const {
			return (high == other.high) ? (low == other.low) : false;
		}

		bool operator!=(const PathHashType& other) const {
			return (high != other.high) ? true : (low != other.low);
		}

        bool operator<(const PathHashType& other) const {
			return (high < other.high) ? true : ((high == other.high) ? (low < other.low) : false);
		}
	};

    constexpr uint64_t BAD_HASH_PART = std::numeric_limits<std::uint64_t>::max();
    constexpr PathHashType BAD_HASH{BAD_HASH_PART, BAD_HASH_PART};
}

#endif

