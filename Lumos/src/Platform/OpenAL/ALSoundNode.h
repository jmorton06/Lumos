#pragma once

#include "LM.h"

#include "Audio/SoundNode.h"

#include <AL/al.h>

#define NUM_STREAM_BUFFERS 3

namespace lumos
{
	class ALSoundNode : public SoundNode
	{
	public:
		ALSoundNode();
		virtual ~ALSoundNode();

		void OnUpdate(float msec) override;
		void Pause() override;
		void Resume() override;
		void Stop() override;
		void SetSound(Sound *s) override;

	private:
		ALuint m_Source;
		ALuint m_StreamBuffers[NUM_STREAM_BUFFERS];
	};
}