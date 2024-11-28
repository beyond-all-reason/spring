
# only one of the two targets needs this
remove_definitions(-DNO_SOUND)

# Dummy Sound implementaiton
set(noSoundSources
		IAudioChannel.cpp
		ISound.cpp
		Null/SoundChannels.cpp
		Null/NullSound.cpp
	)

add_library(no-sound STATIC EXCLUDE_FROM_ALL ${noSoundSources})
set_target_properties(no-sound PROPERTIES COMPILE_FLAGS "-DNO_SOUND")
target_link_libraries(no-sound Tracy::TracyClient)
target_link_libraries(no-sound mio::mio)

# Define default sound implementation
if    (NO_SOUND)
	set(sound-impl no-sound PARENT_SCOPE)
else  (NO_SOUND)
	set(sound-impl sound PARENT_SCOPE)
endif (NO_SOUND)


# Real Sound implementaiton
if    (NOT NO_SOUND)
	set(soundSources
			ISound.cpp
			IAudioChannel.cpp
			Null/NullSound.cpp
			OpenAL/ALShared.cpp
			OpenAL/EFX.cpp
			OpenAL/EFXfuncs.cpp
			OpenAL/EFXPresets.cpp
			OpenAL/AudioChannel.cpp
			OpenAL/OggDecoder.cpp
			OpenAL/Mp3Decoder.cpp
			OpenAL/MusicStream.cpp
			OpenAL/Sound.cpp
			OpenAL/SoundChannels.cpp
			OpenAL/SoundBuffer.cpp
			OpenAL/SoundItem.cpp
			OpenAL/SoundSource.cpp
			OpenAL/VorbisShared.cpp
		)

	find_package_static(OpenAL REQUIRED)
	include_directories(${OPENAL_INCLUDE_DIR})
	find_package_static(OggVorbis 1.3.4 REQUIRED)

	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIR})

	include_directories(${CMAKE_SOURCE_DIR}/include/)
	include_directories(${CMAKE_SOURCE_DIR}/include/AL)

	add_library(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	target_link_libraries(sound vorbis::vorbisfile vorbis::vorbis Ogg::ogg)
	target_link_libraries(sound ${OPENAL_LIBRARY})
	target_link_libraries(sound Tracy::TracyClient)
	target_link_libraries(sound mio::mio)
endif (NOT NO_SOUND)
