#include "AudioManager.h"
#include <cstdio>
#include <cstdlib>

AudioManager& AudioManager::Get()
{
	static AudioManager instance;
	return instance;
}

bool AudioManager::Initialize(HWND hWnd)
{
	(void)hWnd; // Not needed for miniaudio

	if (m_bInitialized)
		return m_bSoundAvailable;

	// Initialize miniaudio engine
	ma_engine_config engineConfig = ma_engine_config_init();

	ma_result result = ma_engine_init(&engineConfig, &m_engine);
	if (result != MA_SUCCESS)
	{
		m_bSoundAvailable = false;
		m_bInitialized = true;
		return false;
	}

	// Initialize sound effect group for separate volume control
	result = ma_sound_group_init(&m_engine, 0, NULL, &m_sfxGroup);
	if (result == MA_SUCCESS)
	{
		m_bSfxGroupInitialized = true;
		ma_sound_group_set_volume(&m_sfxGroup, VolumeToFloat(m_soundVolume));
	}

	// Initialize ambient sound group
	result = ma_sound_group_init(&m_engine, 0, NULL, &m_ambientGroup);
	if (result == MA_SUCCESS)
	{
		m_bAmbientGroupInitialized = true;
		ma_sound_group_set_volume(&m_ambientGroup, VolumeToFloat(m_ambientVolume));
	}

	// Initialize UI sound group
	result = ma_sound_group_init(&m_engine, 0, NULL, &m_uiGroup);
	if (result == MA_SUCCESS)
	{
		m_bUIGroupInitialized = true;
		ma_sound_group_set_volume(&m_uiGroup, VolumeToFloat(m_uiVolume));
	}

	// Apply master volume to the engine
	ma_engine_set_volume(&m_engine, VolumeToFloat(m_masterVolume));

	m_bSoundAvailable = true;
	m_bInitialized = true;
	return true;
}

bool AudioManager::DecodeFile(const char* filePath, DecodedSound& out)
{
	// Decode to f32 at the engine's sample rate so playback doesn't need resampling
	ma_uint32 engineSampleRate = ma_engine_get_sample_rate(&m_engine);
	ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, engineSampleRate);
	ma_decoder decoder;

	ma_result result = ma_decoder_init_file(filePath, &decoderConfig, &decoder);
	if (result != MA_SUCCESS)
		return false;

	ma_uint64 totalFrames = 0;
	result = ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
	if (result != MA_SUCCESS || totalFrames == 0)
	{
		ma_decoder_uninit(&decoder);
		return false;
	}

	ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(decoder.outputFormat, decoder.outputChannels);
	size_t dataSize = (size_t)(totalFrames * bytesPerFrame);

	void* pData = std::malloc(dataSize);
	if (pData == nullptr)
	{
		ma_decoder_uninit(&decoder);
		return false;
	}

	ma_uint64 framesRead = 0;
	result = ma_decoder_read_pcm_frames(&decoder, pData, totalFrames, &framesRead);

	out.pData = pData;
	out.frameCount = framesRead;
	out.format = decoder.outputFormat;
	out.channels = decoder.outputChannels;
	out.sampleRate = decoder.outputSampleRate;
	out.loaded = true;

	ma_decoder_uninit(&decoder);
	return true;
}

void AudioManager::FreeDecodedSound(DecodedSound& sound)
{
	if (sound.pData != nullptr)
	{
		std::free(sound.pData);
		sound.pData = nullptr;
	}
	sound.frameCount = 0;
	sound.loaded = false;
}

void AudioManager::LoadSounds()
{
	if (!m_bSoundAvailable)
		return;

	char filename[64];

	// Load Character sounds (C1-C24)
	for (int i = 1; i < AUDIO_MAX_CHARACTER_SOUNDS; i++)
	{
		std::sprintf(filename, "sounds\\C%d.wav", i);
		DecodeFile(filename, m_characterSounds[i]);
	}

	// Load Monster/Magic sounds (M1-M156)
	for (int i = 1; i < AUDIO_MAX_MONSTER_SOUNDS; i++)
	{
		std::sprintf(filename, "sounds\\M%d.wav", i);
		DecodeFile(filename, m_monsterSounds[i]);
	}

	// Load Effect sounds (E1-E53)
	for (int i = 1; i < AUDIO_MAX_EFFECT_SOUNDS; i++)
	{
		std::sprintf(filename, "sounds\\E%d.wav", i);
		DecodeFile(filename, m_effectSounds[i]);
	}
}

void AudioManager::Shutdown()
{
	if (!m_bInitialized)
		return;

	// Stop and unload music
	StopMusic();

	// Unload all sounds
	UnloadSounds();

	// Uninitialize sound groups
	if (m_bSfxGroupInitialized)
	{
		ma_sound_group_uninit(&m_sfxGroup);
		m_bSfxGroupInitialized = false;
	}
	if (m_bAmbientGroupInitialized)
	{
		ma_sound_group_uninit(&m_ambientGroup);
		m_bAmbientGroupInitialized = false;
	}
	if (m_bUIGroupInitialized)
	{
		ma_sound_group_uninit(&m_uiGroup);
		m_bUIGroupInitialized = false;
	}

	// Uninitialize engine
	if (m_bSoundAvailable)
	{
		ma_engine_uninit(&m_engine);
	}

	m_bInitialized = false;
	m_bSoundAvailable = false;
}

void AudioManager::UnloadSounds()
{
	// Stop and uninit active sounds
	for (auto& active : m_activeSounds)
	{
		if (active.inUse)
		{
			if (active.soundInitialized)
			{
				ma_sound_stop(&active.sound);
				ma_sound_uninit(&active.sound);
				active.soundInitialized = false;
			}
			active.inUse = false;
		}
	}

	// Free decoded Character sounds
	for (int i = 0; i < AUDIO_MAX_CHARACTER_SOUNDS; i++)
		FreeDecodedSound(m_characterSounds[i]);

	// Free decoded Monster sounds
	for (int i = 0; i < AUDIO_MAX_MONSTER_SOUNDS; i++)
		FreeDecodedSound(m_monsterSounds[i]);

	// Free decoded Effect sounds
	for (int i = 0; i < AUDIO_MAX_EFFECT_SOUNDS; i++)
		FreeDecodedSound(m_effectSounds[i]);

	// Stop music
	StopMusic();
}

void AudioManager::CleanupFinishedSounds()
{
	for (auto& active : m_activeSounds)
	{
		if (active.inUse && active.soundInitialized && !ma_sound_is_playing(&active.sound))
		{
			ma_sound_uninit(&active.sound);
			active.soundInitialized = false;
			active.inUse = false;
		}
	}
}

float AudioManager::VolumeToFloat(int volume) const
{
	// Convert 0-100 to 0.0-1.0
	if (volume <= 0) return 0.0f;
	if (volume >= 100) return 1.0f;
	return volume / 100.0f;
}

DecodedSound* AudioManager::GetDecodedSound(SoundType type, int index)
{
	switch (type)
	{
	case SoundType::Character:
		if (index >= 0 && index < AUDIO_MAX_CHARACTER_SOUNDS && m_characterSounds[index].loaded)
			return &m_characterSounds[index];
		break;
	case SoundType::Monster:
		if (index >= 0 && index < AUDIO_MAX_MONSTER_SOUNDS && m_monsterSounds[index].loaded)
			return &m_monsterSounds[index];
		break;
	case SoundType::Effect:
		if (index >= 0 && index < AUDIO_MAX_EFFECT_SOUNDS && m_effectSounds[index].loaded)
			return &m_effectSounds[index];
		break;
	}
	return nullptr;
}

ma_sound_group* AudioManager::GetGroupForSound(SoundType type, int index)
{
	if (type == SoundType::Effect)
	{
		// Ambient: E38 (rain loop)
		if (index == 38)
			return &m_ambientGroup;

		// UI: E14 (click), E23 (notification), E24 (transaction), E25 (war notification), E29 (item place), E53 (error)
		if (index == 14 || index == 23 || index == 24 || index == 25 || index == 29 || index == 53)
			return &m_uiGroup;
	}

	// Everything else (all C sounds, all M sounds, remaining E sounds) → effects group
	return &m_sfxGroup;
}

bool AudioManager::IsCategoryEnabled(SoundType type, int index) const
{
	if (type == SoundType::Effect)
	{
		if (index == 38)
			return m_bAmbientEnabled;

		if (index == 14 || index == 23 || index == 24 || index == 25 || index == 29 || index == 53)
			return m_bUIEnabled;
	}

	return m_bSoundEnabled;
}

void AudioManager::PlaySound(SoundType type, int index, int distance, int pan)
{
	if (!m_bSoundAvailable || !IsCategoryEnabled(type, index))
		return;

	// Clean up finished sounds first
	CleanupFinishedSounds();

	// Get the decoded sound data
	DecodedSound* pDecoded = GetDecodedSound(type, index);
	if (pDecoded == nullptr)
		return;

	// Calculate volume based on distance
	float volume = 1.0f;

	// Distance attenuation (reduce by 10% per distance unit, max 10 units)
	if (distance > 0)
	{
		if (distance > 10) distance = 10;
		volume *= (1.0f - (distance * 0.1f));
	}

	// Don't play if too quiet
	if (volume < 0.01f)
		return;

	// Find a free slot in the active sound pool
	ActiveSound* pSlot = nullptr;
	for (auto& active : m_activeSounds)
	{
		if (!active.inUse)
		{
			pSlot = &active;
			break;
		}
	}

	// No free slot - try to reclaim a finished one
	if (pSlot == nullptr)
	{
		for (auto& active : m_activeSounds)
		{
			if (active.soundInitialized && !ma_sound_is_playing(&active.sound))
			{
				ma_sound_uninit(&active.sound);
				active.soundInitialized = false;
				active.inUse = false;
				pSlot = &active;
				break;
			}
		}
	}

	// Still no slot available - skip this sound
	if (pSlot == nullptr)
		return;

	// Create an audio buffer ref pointing to our decoded data
	ma_result result = ma_audio_buffer_ref_init(pDecoded->format, pDecoded->channels, pDecoded->pData, pDecoded->frameCount, &pSlot->bufferRef);
	if (result != MA_SUCCESS)
		return;

	// Create a sound from the audio buffer ref (bypasses resource manager entirely)
	ma_sound_group* pGroup = GetGroupForSound(type, index);
	result = ma_sound_init_from_data_source(&m_engine, &pSlot->bufferRef, MA_SOUND_FLAG_NO_SPATIALIZATION, pGroup, &pSlot->sound);
	if (result != MA_SUCCESS)
		return;

	pSlot->inUse = true;
	pSlot->soundInitialized = true;

	// Set volume for this instance
	ma_sound_set_volume(&pSlot->sound, volume);

	// Apply panning (-100 to 100 maps to -1.0 to 1.0)
	if (pan != 0)
	{
		float panValue = pan / 100.0f;
		if (panValue < -1.0f) panValue = -1.0f;
		if (panValue > 1.0f) panValue = 1.0f;
		ma_sound_set_pan(&pSlot->sound, panValue);
	}

	// Start playback
	ma_sound_start(&pSlot->sound);
}

void AudioManager::PlaySoundLoop(SoundType type, int index)
{
	if (!m_bSoundAvailable || !IsCategoryEnabled(type, index))
		return;

	// Get the decoded sound data
	DecodedSound* pDecoded = GetDecodedSound(type, index);
	if (pDecoded == nullptr)
		return;

	// Clean up finished sounds first
	CleanupFinishedSounds();

	// Find a free slot
	ActiveSound* pSlot = nullptr;
	for (auto& active : m_activeSounds)
	{
		if (!active.inUse)
		{
			pSlot = &active;
			break;
		}
	}

	if (pSlot == nullptr)
		return;

	// Create an audio buffer ref pointing to our decoded data
	ma_result result = ma_audio_buffer_ref_init(pDecoded->format, pDecoded->channels, pDecoded->pData, pDecoded->frameCount, &pSlot->bufferRef);
	if (result != MA_SUCCESS)
		return;

	// Create a sound from the audio buffer ref
	ma_sound_group* pGroup = GetGroupForSound(type, index);
	result = ma_sound_init_from_data_source(&m_engine, &pSlot->bufferRef, MA_SOUND_FLAG_NO_SPATIALIZATION, pGroup, &pSlot->sound);
	if (result != MA_SUCCESS)
		return;

	pSlot->inUse = true;
	pSlot->soundInitialized = true;
	ma_sound_set_looping(&pSlot->sound, MA_TRUE);
	ma_sound_start(&pSlot->sound);
}

void AudioManager::StopSound(SoundType type, int index)
{
	// With the buffer-ref system, we can't easily identify which active sound
	// corresponds to a specific type/index. This would require additional tracking.
	(void)type;
	(void)index;
}

void AudioManager::StopAllSounds()
{
	// Stop and uninit all active sounds
	for (auto& active : m_activeSounds)
	{
		if (active.inUse)
		{
			if (active.soundInitialized)
			{
				ma_sound_stop(&active.sound);
				ma_sound_uninit(&active.sound);
				active.soundInitialized = false;
			}
			active.inUse = false;
		}
	}
}

void AudioManager::PlayMusic(const char* trackName)
{
	if (!m_bSoundAvailable)
		return;

	if (trackName == nullptr || trackName[0] == '\0')
		return;

	// Check if already playing this track
	if (m_bBgmLoaded && m_currentMusicTrack == trackName)
		return;

	// Stop existing music
	StopMusic();

	// Don't play if music is disabled
	if (!m_bMusicEnabled)
		return;

	// Build full path
	std::string filename = std::string("music\\") + trackName + ".wav";

	// Initialize the music sound - stream from disk (music files are large)
	ma_uint32 flags = MA_SOUND_FLAG_STREAM;
	ma_result result = ma_sound_init_from_file(&m_engine, filename.c_str(), flags, NULL, NULL, &m_bgmSound);

	if (result != MA_SUCCESS)
	{
		m_bBgmLoaded = false;
		return;
	}

	m_bBgmLoaded = true;
	m_currentMusicTrack = trackName;

	// Set volume and looping
	ma_sound_set_volume(&m_bgmSound, VolumeToFloat(m_musicVolume));
	ma_sound_set_looping(&m_bgmSound, MA_TRUE);

	// Start playback
	ma_sound_start(&m_bgmSound);
}

void AudioManager::StopMusic()
{
	if (m_bBgmLoaded)
	{
		ma_sound_stop(&m_bgmSound);
		ma_sound_uninit(&m_bgmSound);
		m_bBgmLoaded = false;
	}
	m_currentMusicTrack.clear();
}

bool AudioManager::IsMusicPlaying() const
{
	if (!m_bBgmLoaded)
		return false;

	return ma_sound_is_playing(&m_bgmSound) == MA_TRUE;
}

void AudioManager::SetMasterVolume(int volume)
{
	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	m_masterVolume = volume;

	// Engine volume scales all output (SFX groups + music)
	if (m_bInitialized && m_bSoundAvailable)
	{
		ma_engine_set_volume(&m_engine, m_bMasterEnabled ? VolumeToFloat(volume) : 0.0f);
	}
}

void AudioManager::SetMasterEnabled(bool enabled)
{
	m_bMasterEnabled = enabled;

	if (m_bInitialized && m_bSoundAvailable)
	{
		ma_engine_set_volume(&m_engine, enabled ? VolumeToFloat(m_masterVolume) : 0.0f);
	}
}

void AudioManager::SetSoundVolume(int volume)
{
	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	m_soundVolume = volume;

	// Set sound group volume (affects all sounds in the group)
	if (m_bSfxGroupInitialized)
	{
		ma_sound_group_set_volume(&m_sfxGroup, VolumeToFloat(volume));
	}
}

void AudioManager::SetMusicVolume(int volume)
{
	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	m_musicVolume = volume;

	// Update currently playing music volume
	if (m_bBgmLoaded)
	{
		ma_sound_set_volume(&m_bgmSound, VolumeToFloat(volume));
	}
}

void AudioManager::SetSoundEnabled(bool enabled)
{
	m_bSoundEnabled = enabled;

	// Stop all sounds if disabling
	if (!enabled)
	{
		StopAllSounds();
	}
}

void AudioManager::SetMusicEnabled(bool enabled)
{
	bool wasEnabled = m_bMusicEnabled;
	m_bMusicEnabled = enabled;

	// Stop music if disabling
	if (wasEnabled && !enabled)
	{
		StopMusic();
	}
}

void AudioManager::SetAmbientVolume(int volume)
{
	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	m_ambientVolume = volume;

	if (m_bAmbientGroupInitialized)
	{
		ma_sound_group_set_volume(&m_ambientGroup, VolumeToFloat(volume));
	}
}

void AudioManager::SetUIVolume(int volume)
{
	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	m_uiVolume = volume;

	if (m_bUIGroupInitialized)
	{
		ma_sound_group_set_volume(&m_uiGroup, VolumeToFloat(volume));
	}
}

void AudioManager::SetAmbientEnabled(bool enabled)
{
	m_bAmbientEnabled = enabled;

	// Stop ambient sounds if disabling (stop all and let non-ambient ones replay naturally)
	if (!enabled)
	{
		// Mute the ambient group by setting volume to 0
		if (m_bAmbientGroupInitialized)
		{
			ma_sound_group_set_volume(&m_ambientGroup, 0.0f);
		}
	}
	else
	{
		// Restore ambient group volume
		if (m_bAmbientGroupInitialized)
		{
			ma_sound_group_set_volume(&m_ambientGroup, VolumeToFloat(m_ambientVolume));
		}
	}
}

void AudioManager::SetUIEnabled(bool enabled)
{
	m_bUIEnabled = enabled;

	if (!enabled)
	{
		if (m_bUIGroupInitialized)
		{
			ma_sound_group_set_volume(&m_uiGroup, 0.0f);
		}
	}
	else
	{
		if (m_bUIGroupInitialized)
		{
			ma_sound_group_set_volume(&m_uiGroup, VolumeToFloat(m_uiVolume));
		}
	}
}

void AudioManager::SetListenerPosition(int worldX, int worldY)
{
	m_listenerX = worldX;
	m_listenerY = worldY;
}

void AudioManager::Update(uint32_t currentTime)
{
	(void)currentTime;
	// Periodically clean up finished sounds
	CleanupFinishedSounds();
}
