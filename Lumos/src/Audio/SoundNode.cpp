#include "LM.h"
#include "SoundNode.h"
#include "Graphics/Camera/Camera.h"

namespace Lumos
{

	SoundNode::SoundNode()
	{
		Reset();
	}

	SoundNode::SoundNode(Sound* s)
	{
		Reset();
		SetSound(s);
	}

	void SoundNode::Reset()
	{
		m_Priority = SOUNDPRIORTY_LOW;
		m_Pitch = 1.0f;
		m_Volume = 1.0f;
		m_Radius = 50.0f;
		m_TimeLeft = 0.0f;
		m_IsLooping = true;
		m_OALSource = nullptr;
		m_Sound = nullptr;
		m_Paused = false;
		m_StreamPos = 0;
		m_IsGlobal = false;
		m_Stationary = false;
		m_ReferenceDistance = 0.0f;
		m_Velocity = maths::Vector3(0.0f);

		for (unsigned int i = 0; i < NUM_STREAM_BUFFERS; ++i)
		{
			m_StreamBuffers[i] = 0;
		}
	}

	SoundNode::~SoundNode()
	{
		DetachSource();
	}

	bool SoundNode::CompareNodesByPriority(std::shared_ptr<SoundNode>& a, std::shared_ptr<SoundNode>& b)
	{
		return a->m_Priority > b->m_Priority;
	}

	void SoundNode::SetSound(Sound *s)
	{
		m_Sound = s;
		DetachSource();
		if (m_Sound)
		{
			m_TimeLeft = m_Sound->GetLength();

#ifdef LUMOS_OPENAL
			if (m_Sound->IsStreaming())
			{
				alGenBuffers(NUM_STREAM_BUFFERS, m_StreamBuffers);
			}
			else
			{
				alDeleteBuffers(NUM_STREAM_BUFFERS, m_StreamBuffers);
			}
#endif
		}
	}

	void SoundNode::AttachSource(OALSource* s)
	{
		m_OALSource = s;

		if (!m_OALSource)
		{
			return;
		}

		m_OALSource->inUse = true;

#ifdef LUMOS_OPENAL
		alSourceStop(m_OALSource->source);
		alSourcef(m_OALSource->source, AL_MAX_DISTANCE, m_Radius);
		alSourcef(m_OALSource->source, AL_ROLLOFF_FACTOR, 1.0f);
		alSourcef(m_OALSource->source, AL_REFERENCE_DISTANCE, m_ReferenceDistance);
#endif
		if (m_Sound->IsStreaming())
		{
			m_StreamPos = m_TimeLeft;
			int numBuffered = 0;
			while (numBuffered < NUM_STREAM_BUFFERS)
			{
				double streamed = m_Sound->StreamData(m_StreamBuffers[numBuffered], m_StreamPos);

				if (streamed)
				{
					m_StreamPos -= streamed;
					numBuffered++;
				}
				else
				{
					break;
				}
			}
#ifdef LUMOS_OPENAL
			alSourceQueueBuffers(m_OALSource->source, numBuffered, &m_StreamBuffers[0]);
#endif
		}
		else
		{
#ifdef LUMOS_OPENAL
			alSourcei(m_OALSource->source, AL_BUFFER, m_Sound->GetBuffer());
			alSourcef(m_OALSource->source, AL_SEC_OFFSET, static_cast<ALfloat>((m_Sound->GetLength() / 1000.0) - (m_TimeLeft / 1000.0)));
			alSourcePlay(m_OALSource->source);
#endif
		}

#ifdef LUMOS_OPENAL
		alSourcePlay(m_OALSource->source);
#endif
	}

	void SoundNode::DetachSource()
	{
		if (!m_OALSource)
		{
			return;
		}

		m_OALSource->inUse = false;
#ifdef LUMOS_OPENAL
		alSourcef(m_OALSource->source, AL_GAIN, 0.0f);
		alSourceStop(m_OALSource->source);
		alSourcei(m_OALSource->source, AL_BUFFER, 0);

		if (m_Sound && m_Sound->IsStreaming())
		{
			int numProcessed = 0;
			ALuint tempBuffer = 0;
			alGetSourcei(m_OALSource->source, AL_BUFFERS_PROCESSED, &numProcessed);

			while (numProcessed--)
			{
				alSourceUnqueueBuffers(m_OALSource->source, 1, &tempBuffer);
			}
		}
#endif

		m_OALSource = nullptr;
	}

	void SoundNode::UpdateSoundState(float msec)
	{
		if (m_Sound)
		{
			m_TimeLeft -= (msec * m_Pitch);

			if (m_IsLooping)
			{
				while (m_TimeLeft < 0)
				{
					m_TimeLeft += m_Sound->GetLength();
				}
			}

			if (m_OALSource)
			{
#ifdef LUMOS_OPENAL
				alSourcef(m_OALSource->source, AL_GAIN, m_Volume);
				alSourcef(m_OALSource->source, AL_PITCH, m_Pitch);
				alSourcef(m_OALSource->source, AL_MAX_DISTANCE, m_Radius);
				alSourcef(m_OALSource->source, AL_REFERENCE_DISTANCE, m_ReferenceDistance);
#endif
				maths::Vector3 position;
				maths::Vector3 velocity;

				if (m_IsGlobal)
				{
					position = SoundSystem::Instance()->GetListener()->GetPosition();
				}
				else
				{
					position = GetPosition();
				}

				if (m_Stationary)
				{
					velocity = maths::Vector3(0.0f);
				}
				else
				{
					velocity = m_Velocity;
				}

#ifdef LUMOS_OPENAL
				alSourcefv(m_OALSource->source, AL_POSITION, reinterpret_cast<float*>(&position));

				if (m_Sound->IsStreaming())
				{
					int numProcessed;
					alGetSourcei(m_OALSource->source, AL_BUFFERS_PROCESSED, &numProcessed);
					alSourcei(m_OALSource->source, AL_LOOPING, 0);

					while (numProcessed-- && m_StreamPos > 0)
					{	//The && prevents clipping at the end of sounds!
						ALuint freeBuffer;

						alSourceUnqueueBuffers(m_OALSource->source, 1, &freeBuffer);

						m_StreamPos -= m_Sound->StreamData(freeBuffer, m_StreamPos);
						alSourceQueueBuffers(m_OALSource->source, 1, &freeBuffer);

						if (m_StreamPos < 0 && m_IsLooping)
						{
							m_StreamPos = m_Sound->GetLength();
						}
					}
				}
				else
				{
					alSourcei(m_OALSource->source, AL_LOOPING, m_IsLooping ? 1 : 0);
				}
#endif
			}
		}
	}

	void SoundNode::Update(float msec)
	{
        auto sharedPtr = std::shared_ptr<SoundNode>(this);
		SoundSystem::Instance()->AddSoundNode(sharedPtr);
	}
}
