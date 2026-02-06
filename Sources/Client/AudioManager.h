#pragma once

#include "miniaudio.h"
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <memory>

// Maximum sound effects per category
static const int AUDIO_MAX_CHARACTER_SOUNDS = 25;   // C1-C24
static const int AUDIO_MAX_MONSTER_SOUNDS = 160;    // M1-M156
static const int AUDIO_MAX_EFFECT_SOUNDS = 55;      // E1-E53

// Maximum concurrent playing sounds (active sound pool)
static const int AUDIO_MAX_ACTIVE_SOUNDS = 32;

// Sound types matching existing categories
enum class SoundType
{
	Character,  // Combat sounds (C1-C24)
	Monster,    // Magic/Monster sounds (M1-M156)
	Effect      // Environmental/Effect sounds (E1-E53)
};

// Decoded sound data stored in our own memory (bypasses miniaudio resource manager)
struct DecodedSound
{
	void* pData = nullptr;
	ma_uint64 frameCount = 0;
	ma_format format = ma_format_f32;
	ma_uint32 channels = 0;
	ma_uint32 sampleRate = 0;
	bool loaded = false;
};

class AudioManager
{
public:
	static AudioManager& Get();

	// Lifecycle
	bool Initialize();
	void Shutdown();

	// Sound loading - pre-loads all sounds into memory
	void LoadSounds();
	void UnloadSounds();
	void CleanupFinishedSounds();

	// Sound effect playback
	void PlayGameSound(SoundType type, int index, int distance = 0, int pan = 0);
	void PlaySoundLoop(SoundType type, int index);
	void StopSound(SoundType type, int index);
	void StopAllSounds();

	// Background music
	void PlayMusic(const char* trackName);
	void StopMusic();
	bool IsMusicPlaying() const;
	const std::string& GetCurrentMusicTrack() const { return m_currentMusicTrack; }

	// Volume control (0-100 scale)
	void SetMasterVolume(int volume);
	void SetSoundVolume(int volume);
	void SetMusicVolume(int volume);
	void SetAmbientVolume(int volume);
	void SetUIVolume(int volume);
	int GetMasterVolume() const { return m_masterVolume; }
	int GetSoundVolume() const { return m_soundVolume; }
	int GetMusicVolume() const { return m_musicVolume; }
	int GetAmbientVolume() const { return m_ambientVolume; }
	int GetUIVolume() const { return m_uiVolume; }

	// Enable/disable
	void SetMasterEnabled(bool enabled);
	void SetSoundEnabled(bool enabled);
	void SetMusicEnabled(bool enabled);
	void SetAmbientEnabled(bool enabled);
	void SetUIEnabled(bool enabled);
	bool IsMasterEnabled() const { return m_bMasterEnabled; }
	bool IsSoundEnabled() const { return m_bSoundEnabled; }
	bool IsMusicEnabled() const { return m_bMusicEnabled; }
	bool IsAmbientEnabled() const { return m_bAmbientEnabled; }
	bool IsUIEnabled() const { return m_bUIEnabled; }

	// Hardware availability
	bool IsSoundAvailable() const { return m_bSoundAvailable; }

	// Listener position (for positional audio)
	void SetListenerPosition(int worldX, int worldY);
	int GetListenerX() const { return m_listenerX; }
	int GetListenerY() const { return m_listenerY; }

	// Per-frame update
	void Update(uint32_t currentTime);

private:
	AudioManager() = default;
	~AudioManager() = default;
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	// Decode a WAV file into a DecodedSound buffer (bypasses resource manager)
	bool DecodeFile(const char* filePath, DecodedSound& out);
	void FreeDecodedSound(DecodedSound& sound);

	// Convert 0-100 volume to 0.0-1.0
	float VolumeToFloat(int volume) const;

	// Get decoded sound data for a type/index
	DecodedSound* GetDecodedSound(SoundType type, int index);

	// Get the appropriate sound group for a given sound type/index
	ma_sound_group* GetGroupForSound(SoundType type, int index);

	// Check if the appropriate category is enabled for a given sound
	bool IsCategoryEnabled(SoundType type, int index) const;

	// miniaudio engine
	ma_engine m_engine;
	bool m_bSoundAvailable = false;
	bool m_bInitialized = false;

	// Sound effect groups (for separate volume control per category)
	ma_sound_group m_sfxGroup;
	bool m_bSfxGroupInitialized = false;

	ma_sound_group m_ambientGroup;
	bool m_bAmbientGroupInitialized = false;

	ma_sound_group m_uiGroup;
	bool m_bUIGroupInitialized = false;

	// Pre-decoded sound data (our own memory, no resource manager)
	std::array<DecodedSound, AUDIO_MAX_CHARACTER_SOUNDS> m_characterSounds = {};
	std::array<DecodedSound, AUDIO_MAX_MONSTER_SOUNDS> m_monsterSounds = {};
	std::array<DecodedSound, AUDIO_MAX_EFFECT_SOUNDS> m_effectSounds = {};

	// Active sound pool for concurrent playback
	struct ActiveSound {
		ma_audio_buffer_ref bufferRef;
		ma_sound sound;
		bool inUse = false;
		bool soundInitialized = false;
	};
	std::array<ActiveSound, AUDIO_MAX_ACTIVE_SOUNDS> m_activeSounds;

	// Background music sound (still uses resource manager - single file, streamed)
	ma_sound m_bgmSound;
	bool m_bBgmLoaded = false;

	// Current music track name
	std::string m_currentMusicTrack;

	// Volume (0-100)
	int m_masterVolume = 100;
	int m_soundVolume = 100;
	int m_musicVolume = 100;
	int m_ambientVolume = 100;
	int m_uiVolume = 100;

	// Enable flags
	bool m_bMasterEnabled = true;
	bool m_bSoundEnabled = true;
	bool m_bMusicEnabled = true;
	bool m_bAmbientEnabled = true;
	bool m_bUIEnabled = true;

	// Listener position (for positional audio)
	int m_listenerX = 0;
	int m_listenerY = 0;
};
