/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ISound.h"

#include <cstring> //memset
#include <array>

#ifndef   NO_SOUND
#include "OpenAL/Sound.h"
#endif // NO_SOUND
#include "Null/NullSound.h"

#include "SoundLog.h"
#include "System/Config/ConfigHandler.h"

#include "SoundChannels.h"
#include "Null/NullAudioChannel.h"
#ifndef NO_SOUND
#include "OpenAL/AudioChannel.h"
#endif

#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"

CONFIG(bool, Sound).defaultValue(true).description("Enables (OpenAL) or disables sound.");

CONFIG(bool, UseEFX     ).defaultValue( true).safemodeValue(false);
CONFIG(bool, UseSDLAudio).defaultValue( true).safemodeValue(false).headlessValue(0).description("If enabled, OpenAL-soft only renders audio into a SDL buffer and playback is done by the SDL audio layer, i.e. SDL handles the hardware");

// defined here so spring-headless contains them, too (default & headless should contain the same set of configtags!)
CONFIG(int, MaxSounds).defaultValue(128).headlessValue(1).minimumValue(1).description("Maximum sounds played in parallel.");
CONFIG(int, PitchAdjust).defaultValue(0).description("Adjusts sound pitch proportional to [if set to 1, the square root of] game speed. Set to 2 for linear scaling.");

CONFIG(int, snd_volmaster).defaultValue(60).minimumValue(0).maximumValue(200).description("Master sound volume.");
CONFIG(int, snd_volgeneral).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"general\" sound channel.");
CONFIG(int, snd_volunitreply).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"unit reply\" sound channel.");
CONFIG(int, snd_volbattle).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"battle\" sound channel.");
CONFIG(int, snd_volui).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"ui\" sound channel.");
CONFIG(int, snd_volmusic).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"music\" sound channel.");
CONFIG(int, snd_volambient).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"ambient\" sound channel.");
CONFIG(float, snd_airAbsorption).defaultValue(0.1f);

CONFIG(std::string, snd_device).defaultValue("").description("Sets the used output device. See \"Available Devices\" section in infolog.txt.");


#ifndef NO_SOUND
using SelAudioChannel = AudioChannel;
#else
using SelAudioChannel = NullAudioChannel;
#endif
alignas(SelAudioChannel) static std::array<std::array<std::byte, sizeof(SelAudioChannel)>, ChannelType::CHANNEL_COUNT> audioChannelMem;


ISound* ISound::singleton = nullptr;

void ISound::Initialize(bool reload, bool forceNullSound)
{
#ifndef NO_SOUND
	if (!IsNullAudio() && !forceNullSound) {
		for (size_t i = 0; i < Channels.size(); ++i) {
			Channels[i] = new (audioChannelMem[i].data()) AudioChannel();
		}

		if (!reload) {
			SCOPED_ONCE_TIMER("ISound::Init::New");
			assert(singleton == nullptr);
			singleton = new CSound();
		}
		{
			SCOPED_ONCE_TIMER("ISound::Init::Dev");

			// sound device is initialized in a thread, must wait
			// for it to finish (otherwise LoadSoundDefs can fail)
			singleton->Init();

			while (!singleton->CanLoadSoundDefs()) {
				LOG("[ISound::%s] spawning sound-thread (%.1fms)", __func__, (__timer.GetDuration()).toMilliSecsf());

				if (singleton->SoundThreadQuit()) {
					// no device or context found, fallback
					ChangeOutput(true);
					break;
				}

				spring_sleep(spring_msecs(100));
			}
		}
	} else
#endif // NO_SOUND
	{
		if (!reload) {
			assert(singleton == nullptr);
			singleton = new NullSound();
		}

		for (size_t i = 0; i < Channels.size(); ++i) {
			Channels[i] = new (audioChannelMem[i].data()) NullAudioChannel();
		}
	}
}

void ISound::Shutdown(bool reload)
{
	// kill thread before setting singleton pointer to null
	if (singleton != nullptr)
		singleton->Kill();

	if (!reload)
		spring::SafeDelete(singleton);

	for (size_t i = 0; i < Channels.size(); ++i) {
		spring::SafeDestruct(Channels[i]);
		std::memset(audioChannelMem[i].data(), 0, audioChannelMem[i].size());
	}
}


bool ISound::IsNullAudio()
{
	return !configHandler->GetBool("Sound");
}


bool ISound::ChangeOutput(bool forceNullSound)
{
	// FIXME: on reload, sound-ids change (depends on order when they are requested, see GetSoundId()/GetSoundItem()
	if (IsNullAudio()) {
		LOG_L(L_ERROR, "[ISound::%s] re-enabling sound isn't supported yet!", __func__);
		return true;
	}

	Shutdown(false);
	configHandler->Set("Sound", IsNullAudio() || forceNullSound);
	Initialize(false, forceNullSound);

	return (IsNullAudio() || forceNullSound);
}

bool ISound::LoadSoundDefs(LuaParser* defsParser)
{
	return (singleton->LoadSoundDefsImpl(defsParser));
}

