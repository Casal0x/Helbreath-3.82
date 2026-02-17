# Client version check via ping and per-project build counters

Client now sends its version in every ping packet. Server detects outdated clients and warns them every 5 minutes to relaunch and update. Linux builds no longer auto-increment the build counter; only Windows builds increment via --increment-version flag.

Build counters are now per-project (build_counter_client.txt, build_counter_server.txt) instead of a single shared counter. Each vcxproj has a pre-build event that increments only its own counter, so VS IDE builds work correctly. The build.ps1 script increments both.
