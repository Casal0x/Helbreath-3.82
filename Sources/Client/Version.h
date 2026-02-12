#pragma once

#include <string>

namespace hb {
	namespace version {

#ifndef HB_VERSION_MAJOR
#define HB_VERSION_MAJOR 3
#endif

#ifndef HB_VERSION_MINOR
#define HB_VERSION_MINOR 82
#endif

#ifndef HB_VERSION_PATCH
#define HB_VERSION_PATCH 0
#endif

#ifndef HB_VERSION_PRERELEASE
#define HB_VERSION_PRERELEASE ""
#endif

#ifndef HB_VERSION_BUILD
#define HB_VERSION_BUILD ""
#endif

		struct VersionInfo {
			int major;
			int minor;
			int patch;
			const char* prerelease;
			const char* build;
		};

		const VersionInfo& Get();
		std::string GetSemVer();
		std::string GetDisplayString();
		std::string GetFullString();

	} // namespace version
} //namespace hb