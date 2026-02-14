#include "Version.h"
#include <cstdio>
#include <format>

namespace hb {
	namespace version {

		const VersionInfo& get()
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

		std::string get_sem_ver()
		{
			const VersionInfo& ver = get();

			if (ver.prerelease[0] != '\0') {
				return std::format("{}.{}.{}-{}", ver.major, ver.minor, ver.patch, ver.prerelease);
			}
			return std::format("{}.{}.{}", ver.major, ver.minor, ver.patch);
		}

		std::string get_display_string()
		{
			return get_sem_ver();
		}

		std::string get_full_string()
		{
			const VersionInfo& ver = get();
			std::string semver = get_sem_ver();

			if (ver.build[0] != '\0') {
				return std::format("{}+{}", semver, ver.build);
			}
			return semver;
		}

	} // namespace version
} // namespace hb