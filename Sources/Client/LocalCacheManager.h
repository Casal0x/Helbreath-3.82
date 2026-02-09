#pragma once

#include <cstdint>
#include <vector>

enum class ConfigCacheType : uint8_t
{
	Items  = 0,
	Magic  = 1,
	Skills = 2,
	Npcs   = 3,
	COUNT  = 4
};

class LocalCacheManager
{
public:
	static LocalCacheManager& Get();

	void Initialize();
	void Shutdown();

	bool     HasCache(ConfigCacheType type) const;
	uint32_t GetHash(ConfigCacheType type) const;

	void AccumulatePacket(ConfigCacheType type, const char* pData, uint32_t size);
	bool FinalizeAndSave(ConfigCacheType type);

	using PacketCallback = bool(*)(char* pData, uint32_t size, void* ctx);
	bool ReplayFromCache(ConfigCacheType type, PacketCallback cb, void* ctx);

	void ResetAccumulator(ConfigCacheType type);

	bool IsReplaying() const { return m_bIsReplaying; }

private:
	static constexpr uint32_t CACHE_MAGIC   = 0x48424346;
	static constexpr uint32_t CACHE_VERSION = 1;

	struct CacheHeader
	{
		uint32_t magic;
		uint32_t version;
		uint32_t configType;
		uint32_t crc32;
		uint32_t payloadSize;
	};

	struct CacheState
	{
		bool     hasCache;
		uint32_t hash;
	};

	struct Accumulator
	{
		std::vector<uint8_t> data;
		bool active;
	};

	CacheState  m_state[4]{};
	Accumulator m_accum[4]{};
	bool m_bIsReplaying = false;

	const char* _GetFilename(ConfigCacheType type) const;
	bool _LoadHeader(ConfigCacheType type);
	uint32_t _ComputePayloadHash(const std::vector<uint8_t>& data) const;
};
