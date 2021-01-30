#pragma once


namespace Lumos
{
	struct AudioData
	{
		unsigned char*	Data;
		float	FreqRate = 0.0f;
		double	Length = 0.0;
		uint32_t	BitRate = 0;
		uint32_t	Size = 0;
		uint32_t	Channels = 0;
	};
}
