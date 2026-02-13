#include "LocalCacheManager.h"
#include "CRC32.h"

#include <cstdio>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <direct.h>
#endif

LocalCacheManager& LocalCacheManager::Get()
{
	static LocalCacheManager instance;
	return instance;
}

void LocalCacheManager::Initialize()
{
#ifdef _WIN32
	_mkdir("cache");
#endif
	for (int i = 0; i < static_cast<int>(ConfigCacheType::COUNT); i++) {
		m_state[i] = {};
		m_accum[i] = {};
		_LoadHeader(static_cast<ConfigCacheType>(i));
	}
}

void LocalCacheManager::Shutdown()
{
	for (int i = 0; i < static_cast<int>(ConfigCacheType::COUNT); i++) {
		m_state[i] = {};
		m_accum[i].data.clear();
		m_accum[i].active = false;
	}
}

bool LocalCacheManager::HasCache(ConfigCacheType type) const
{
	return m_state[static_cast<int>(type)].hasCache;
}

uint32_t LocalCacheManager::GetHash(ConfigCacheType type) const
{
	return m_state[static_cast<int>(type)].hash;
}

void LocalCacheManager::AccumulatePacket(ConfigCacheType type, const char* pData, uint32_t size)
{
	if (m_bIsReplaying) return;

	auto& acc = m_accum[static_cast<int>(type)];
	acc.active = true;

	if (size > 65535) return;
	uint16_t len = static_cast<uint16_t>(size);
	const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&len);
	acc.data.push_back(lenBytes[0]);
	acc.data.push_back(lenBytes[1]);
	acc.data.insert(acc.data.end(),
		reinterpret_cast<const uint8_t*>(pData),
		reinterpret_cast<const uint8_t*>(pData) + size);
}

bool LocalCacheManager::FinalizeAndSave(ConfigCacheType type)
{
	if (m_bIsReplaying) return false;

	int idx = static_cast<int>(type);
	auto& acc = m_accum[idx];
	if (!acc.active || acc.data.empty()) return false;

	uint32_t hash = _ComputePayloadHash(acc.data);

	CacheHeader hdr{};
	hdr.magic = CACHE_MAGIC;
	hdr.version = CACHE_VERSION;
	hdr.configType = static_cast<uint32_t>(type);
	hdr.crc32 = hash;
	hdr.payloadSize = static_cast<uint32_t>(acc.data.size());

	std::ofstream file(_GetFilename(type), std::ios::binary);
	if (!file) return false;

	file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
	file.write(reinterpret_cast<const char*>(acc.data.data()), acc.data.size());

	if (!file) return false;

	m_state[idx].hasCache = true;
	m_state[idx].hash = hash;

	acc.data.clear();
	acc.active = false;

	return true;
}

bool LocalCacheManager::ReplayFromCache(ConfigCacheType type, PacketCallback cb, void* ctx)
{
	m_bIsReplaying = true;

	std::ifstream file(_GetFilename(type), std::ios::binary);
	if (!file) {
		m_bIsReplaying = false;
		return false;
	}

	CacheHeader hdr{};
	if (!file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr))) {
		m_bIsReplaying = false;
		return false;
	}

	if (hdr.magic != CACHE_MAGIC || hdr.version != CACHE_VERSION) {
		m_bIsReplaying = false;
		return false;
	}

	std::vector<uint8_t> payload(hdr.payloadSize);
	if (!file.read(reinterpret_cast<char*>(payload.data()), hdr.payloadSize)) {
		m_bIsReplaying = false;
		return false;
	}

	uint32_t check = hb::shared::util::hb_crc32(payload.data(), payload.size());
	if (check != hdr.crc32) {
		m_bIsReplaying = false;
		return false;
	}

	size_t offset = 0;
	while (offset + 2 <= payload.size()) {
		uint16_t pktLen = 0;
		std::memcpy(&pktLen, payload.data() + offset, 2);
		offset += 2;

		if (offset + pktLen > payload.size()) break;

		if (!cb(reinterpret_cast<char*>(payload.data() + offset), pktLen, ctx)) {
			m_bIsReplaying = false;
			return false;
		}
		offset += pktLen;
	}

	m_bIsReplaying = false;
	return true;
}

void LocalCacheManager::ResetAccumulator(ConfigCacheType type)
{
	auto& acc = m_accum[static_cast<int>(type)];
	acc.data.clear();
	acc.active = false;
}

const char* LocalCacheManager::_GetFilename(ConfigCacheType type) const
{
	switch (type) {
	case ConfigCacheType::Items:  return "cache\\{7a3f8b2e-4d1c-9e5a-b6f0-2c8d4e1a3b5f}.bin";
	case ConfigCacheType::Magic:  return "cache\\{d9e2a1c4-8f37-4b6d-a5c0-1e9f3d7b2a4c}.bin";
	case ConfigCacheType::Skills: return "cache\\{b4c8e6f1-2a5d-4739-8e1b-6f0c3d9a5e2b}.bin";
	case ConfigCacheType::Npcs:   return "cache\\{e3a7f5d2-1b8c-4e6a-9d0f-5c2b7a4e8f1d}.bin";
	default: return "";
	}
}

bool LocalCacheManager::_LoadHeader(ConfigCacheType type)
{
	int idx = static_cast<int>(type);
	m_state[idx].hasCache = false;
	m_state[idx].hash = 0;

	std::ifstream file(_GetFilename(type), std::ios::binary);
	if (!file) return false;

	CacheHeader hdr{};
	if (!file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr))) {
		return false;
	}

	if (hdr.magic != CACHE_MAGIC || hdr.version != CACHE_VERSION) {
		return false;
	}

	if (hdr.configType != static_cast<uint32_t>(type)) {
		return false;
	}

	m_state[idx].hasCache = true;
	m_state[idx].hash = hdr.crc32;
	return true;
}

uint32_t LocalCacheManager::_ComputePayloadHash(const std::vector<uint8_t>& data) const
{
	return hb::shared::util::hb_crc32(data.data(), data.size());
}
