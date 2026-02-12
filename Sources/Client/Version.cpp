#include "Version.h"
#include <cstdio>
#include <format>

namespace hb {
	namespace version {

		const VersionInfo& Get()
		{
			static const VersionInfo kVersion{
				HB_VERSION_MAJOR,
				HB_VERSION_MINOR,
				HB_VERSION_PATCH,
				HB_VERSION_PRERELEASE,
				HB_VERSION_BUILD
			};
			return kVersion;
		}

		std::string GetSemVer()
		{
			const VersionInfo& ver = Get();

			if (ver.prerelease[0] != '\0') {
				return std::format("{}.{}.{}-{}", ver.major, ver.minor, ver.patch, ver.prerelease);
			}
			return std::format("{}.{}.{}", ver.major, ver.minor, ver.patch);
		}

		std::string GetDisplayString()
		{
			return GetSemVer();
		}

		std::string GetFullString()
		{
			const VersionInfo& ver = Get();
			std::string semver = GetSemVer();

			if (ver.build[0] != '\0') {
				return std::format("{}+{}", semver, ver.build);
			}
			return semver;
		}

	} // namespace version
} // namespace hb