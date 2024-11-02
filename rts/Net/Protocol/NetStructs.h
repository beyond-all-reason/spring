#pragma once

#include <span>
#include <cstddef>
#include <map>
#include <tuple>

#include "NetMessageTypes.h"
#include "System/Net/RawPacket.h"
#include "System/creg/creg.h"

using SharedPacket = std::shared_ptr<netcode::RawPacket>;

class BaseNetStruct;

struct NetStructDefinition {
    NetStructDefinition() = default;
    NetStructDefinition(const BaseNetStruct* bns);
    std::map<uint32_t, std::tuple<std::string, size_t>> membersMap;
    uint32_t totalSize;
};

class BaseNetStruct
{
    CR_DECLARE(BaseNetStruct)
public:
    BaseNetStruct() = default;
    BaseNetStruct(uint8_t packetType_)
        : packetType(packetType_)
    {}
    SharedPacket ToSharedPacket() const;
    static std::unique_ptr<BaseNetStruct> FromSharedPacket(SharedPacket sp);
protected:
    virtual const NetStructDefinition& GetNetStructDefinition() const { return DEFAULT; }
private:
    static const inline auto DEFAULT = NetStructDefinition{};
    uint8_t packetType;
};

#define GET_NETSTRUCT_DEFINITION(T) \
const NetStructDefinition& GetNetStructDefinition() const override { \
        static const T dummy{}; \
        static const NetStructDefinition ndf(&dummy); \
        return ndf; \
}

#define DECLARE_NETSTRUCT(T) \
CR_DECLARE_DERIVED(T) \
GET_NETSTRUCT_DEFINITION(T)

template<uint8_t PacketType>
class NetStruct : public BaseNetStruct {};


/// Net Structures

template<>
class NetStruct<NETMSG_KEYFRAME> : public BaseNetStruct
{
    DECLARE_NETSTRUCT(NetStruct<NETMSG_KEYFRAME>)
public:
    NetStruct() = default;
    NetStruct(int32_t frameNum_)
        : BaseNetStruct(NETMSG_KEYFRAME)
        , frameNum(frameNum_)
    {}
private:
    int32_t frameNum;
};

template<>
class NetStruct<NETMSG_NEWFRAME> : public BaseNetStruct
{
    GET_NETSTRUCT_DEFINITION(NetStruct<NETMSG_NEWFRAME>)
public:
    NetStruct()
        : BaseNetStruct(NETMSG_NEWFRAME)
    {}
};

template<>
class NetStruct<NETMSG_QUIT> : public BaseNetStruct
{
    DECLARE_NETSTRUCT(NetStruct<NETMSG_QUIT>)
public:
    NetStruct() = default;
    NetStruct(const std::string& reason_)
        : BaseNetStruct(NETMSG_QUIT)
        , reason(reason_)
    {}
private:
    std::string reason;
};


#undef GET_NETSTRUCT_DEFINITION
#undef DECLARE_NETSTRUCT