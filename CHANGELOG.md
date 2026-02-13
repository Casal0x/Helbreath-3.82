# Changelog

### Infrastructure
- Lowercased all 566 files and 8 directories in Binaries/Game/ for case-sensitive filesystem compatibility (sprites, sounds, maps, cache, music, fonts, save, contents)
- Updated 291 source code string literals to reference lowercase file paths (sprite pak names, sound files, cache paths, font/music/config paths)
- Added runtime lowercasing of server-sent map names before building map data file paths
