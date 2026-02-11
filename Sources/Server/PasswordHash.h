#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>


namespace PasswordHash
{
constexpr int SaltHexLen = 33;  // 16 bytes = 32 hex chars + null
constexpr int HashHexLen = 65;  // 32 bytes = 64 hex chars + null

	inline void BytesToHex(const unsigned char* bytes, size_t len, char* outHex, size_t outSize)
	{
		for (size_t i = 0; i < len && (i * 2 + 2) < outSize; i++) {
			std::snprintf(outHex + i * 2, 3, "%02x", bytes[i]);
		}
	}

	inline bool HexToBytes(const char* hex, unsigned char* outBytes, size_t outLen)
	{
		for (size_t i = 0; i < outLen; i++) {
			unsigned int val = 0;
			if (std::sscanf(hex + i * 2, "%02x", &val) != 1) {
				return false;
			}
			outBytes[i] = static_cast<unsigned char>(val);
		}
		return true;
	}

	inline bool GenerateSalt(char* outSaltHex, size_t outSize)
	{
		if (outSize < SaltHexLen) return false;

		unsigned char saltBytes[16] = {};
		NTSTATUS status = BCryptGenRandom(nullptr, saltBytes, sizeof(saltBytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		if (status != 0) return false;

		std::memset(outSaltHex, 0, outSize);
		BytesToHex(saltBytes, sizeof(saltBytes), outSaltHex, outSize);
		return true;
	}

	inline bool HashPassword(const char* password, const char* saltHex, char* outHashHex, size_t outSize)
	{
		if (outSize < HashHexLen) return false;

		// Construct input: saltHex + password
		char input[256] = {};
		std::snprintf(input, sizeof(input), "%s%s", saltHex, password);
		size_t inputLen = std::strlen(input);

		BCRYPT_ALG_HANDLE hAlg = nullptr;
		BCRYPT_HASH_HANDLE hHash = nullptr;
		unsigned char hashResult[32] = {};
		bool ok = false;

		NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
		if (status != 0) goto cleanup;

		status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
		if (status != 0) goto cleanup;

		status = BCryptHashData(hHash, reinterpret_cast<PUCHAR>(input), static_cast<ULONG>(inputLen), 0);
		if (status != 0) goto cleanup;

		status = BCryptFinishHash(hHash, hashResult, sizeof(hashResult), 0);
		if (status != 0) goto cleanup;

		std::memset(outHashHex, 0, outSize);
		BytesToHex(hashResult, sizeof(hashResult), outHashHex, outSize);
		ok = true;

	cleanup:
		if (hHash) BCryptDestroyHash(hHash);
		if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
		SecureZeroMemory(input, sizeof(input));
		return ok;
	}

	inline bool VerifyPassword(const char* password, const char* saltHex, const char* storedHashHex)
	{
		char computedHash[HashHexLen] = {};
		if (!HashPassword(password, saltHex, computedHash, sizeof(computedHash))) {
			return false;
		}
		return std::strcmp(computedHash, storedHashHex) == 0;
	}
}
