#include "NetStructs.h"



CR_BIND(BaseNetStruct, )
CR_REG_METADATA(BaseNetStruct, (
	CR_MEMBER_BEGINFLAG(CM_NoSerialize),
		CR_MEMBER(packetType),
	CR_MEMBER_ENDFLAG(CM_NoSerialize)
))

CR_BIND_DERIVED(NetStruct<NETMSG_KEYFRAME>, BaseNetStruct, )
CR_REG_METADATA(NetStruct<NETMSG_KEYFRAME>, (
	CR_MEMBER_BEGINFLAG(CM_NoSerialize),
		CR_MEMBER(frameNum),
	CR_MEMBER_ENDFLAG(CM_NoSerialize)
))

CR_BIND_DERIVED(NetStruct<NETMSG_QUIT>, BaseNetStruct, )
CR_REG_METADATA(NetStruct<NETMSG_QUIT>, (
	CR_MEMBER_BEGINFLAG(CM_NoSerialize),
		CR_MEMBER(reason),
	CR_MEMBER_ENDFLAG(CM_NoSerialize)
))


NetStructDefinition::NetStructDefinition(const BaseNetStruct* bns)
{
	totalSize = 0;
	for (const auto* cl = bns->GetClass(); cl != nullptr; cl = cl->baseClass) {
		for (const auto& member : cl->members) {
			const auto memberSize = member.type->GetSize();
			totalSize += static_cast<uint32_t>(memberSize);
			membersMap[member.offset] = std::make_tuple(std::string(member.name), memberSize);
		}
	}
}


SharedPacket BaseNetStruct::ToSharedPacket() const
{
	const auto& netStructDef = GetNetStructDefinition();
	SharedPacket sharedPacket = std::make_shared<netcode::RawPacket>(netStructDef.totalSize);
	for (const auto& [offset, data] : netStructDef.membersMap) {
		const auto size = std::get<size_t>(data); //size

		std::span<const uint8_t> s = std::span(
			reinterpret_cast<const uint8_t*>(this) + offset,
			size
		);
		*sharedPacket << s;
	}

	return sharedPacket;
}

std::unique_ptr<BaseNetStruct> BaseNetStruct::FromSharedPacket(SharedPacket sharedPacket)
{
	const auto pt = *reinterpret_cast<decltype(packetType)*>(sharedPacket->data);

	std::unique_ptr<BaseNetStruct> ns = nullptr;

	switch (pt)
	{
	case NETMSG_KEYFRAME: {
		ns = std::make_unique<NetStruct<NETMSG_KEYFRAME>>();
	} break;
	default:
		break;
	}

	if (ns == nullptr)
		return nullptr;

	const auto& netStructDef = ns->GetNetStructDefinition();
	if (netStructDef.totalSize != sharedPacket->length)
		return nullptr;

	auto* ptr = reinterpret_cast<uint8_t*>(ns.get());
	size_t pos = 0;
	for (const auto & [offset, data] : netStructDef.membersMap) {
		const auto size = std::get<size_t>(data); //size

		for (size_t rpos = 0; rpos < size; ++rpos) {
			*(ptr + offset + rpos) = sharedPacket->data[pos + rpos];
		}
		pos += size;
	}

	return ns;
}