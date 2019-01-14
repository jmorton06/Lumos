#pragma once
#include "LM.h"
#include "Sound.h"

#define BUFFERLENGTH 32768

#ifdef OGGVORBIS 
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#else
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#endif

namespace Lumos
{

	class LUMOS_EXPORT OggSound : public Sound
	{
	public:
		OggSound();
		virtual ~OggSound();

		bool LoadFromOgg(const String& filename);

		double	StreamData(unsigned int buffer, double timeLeft) override;

	protected:
		FILE*			m_FileHandle;
#ifdef OGGVORBIS
		OggVorbis_File	m_StreamHandle;
#else
		stb_vorbis*		m_StreamHandle;
		stb_vorbis_info m_VorbisInfo;
#endif
	};

}
