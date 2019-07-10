#pragma once

namespace Lumos
{
	struct AudioData
	{
		unsigned char*	Data;
		float	FreqRate = 0.0f;
		double	Length = 0.0;
		u32	BitRate = 0;
		u32	Size = 0;
		u32	Channels = 0;
	};
}