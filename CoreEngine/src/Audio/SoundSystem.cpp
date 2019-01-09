#include "JM.h"
#include "SoundSystem.h"
#include "Sound.h"
#include "SoundNode.h"
#include "Utilities/TimeStep.h"
#include "Graphics/Camera/Camera.h"

namespace jm
{

	SoundSystem* SoundSystem::m_Instance = nullptr;

	SoundSystem::SoundSystem(unsigned int channels)
	{
		m_Listener = nullptr;
		m_MasterVolume = 1.0f;

#ifdef JM_OPENAL
		JM_INFO("Creating SoundSystem!");
		JM_INFO("Found the following devices: ", alcGetString(nullptr, ALC_DEVICE_SPECIFIER));

		m_Device = alcOpenDevice(nullptr);

		if (!m_Device)
			JM_INFO("Failed to create SoundSystem! (No valid device!)");

		JM_INFO("SoundSystem created with device: ", alcGetString(m_Device, ALC_DEVICE_SPECIFIER));	//Outputs used OAL device!

		m_Context = alcCreateContext(m_Device, nullptr);

		alcMakeContextCurrent(m_Context);
		alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

		for (unsigned int i = 0; i < channels; ++i)
		{
			ALuint source;

			alGenSources(1, &source);
			const ALenum error = alGetError();

			if (error == AL_NO_ERROR)
			{
				m_Sources.push_back(new OALSource(source));
			}
			else
			{
				break;
			}
		}
#endif

		JM_INFO("SoundSystem has ", m_Sources.size(), " channels available!");
	}

	SoundSystem::~SoundSystem()
	{
		RemoveAllSoundNodes();

		for (auto& source : m_Sources)
		{
#ifdef JM_OPENAL
			alDeleteSources(1, &source->source);
#endif
			delete source;
		}

		m_Sources.clear();

#ifdef JM_OPENAL
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(m_Context);
		alcCloseDevice(m_Device);
#endif
	}

	void SoundSystem::RemoveAllSoundNodes()
	{
		m_TemporaryEmitters.clear();
		m_Emitters.clear();
	}

	void SoundSystem::Update(TimeStep* timeStep)
	{
		UpdateListener();
		UpdateTemporaryEmitters(timeStep->GetMillis());

		for (auto & emitter : m_Emitters)
		{
			m_FrameEmitters.push_back(emitter);
			emitter->UpdateSoundState(timeStep->GetMillis());
		}

		CullNodes();	//First off, remove nodes that are too far away

		if (m_FrameEmitters.size() > m_Sources.size())
		{
			std::sort(m_FrameEmitters.begin(), m_FrameEmitters.end(), SoundNode::CompareNodesByPriority);	//Then sort by priority

			DetachSources(m_FrameEmitters.begin() + (m_Sources.size() + 1), m_FrameEmitters.end());		//Detach m_Sources from nodes that won't be covered this frame
			AttachSources(m_FrameEmitters.begin(), m_FrameEmitters.begin() + (m_Sources.size()));	//And attach m_Sources to nodes that WILL be covered this frame
		}
		else
		{
			AttachSources(m_FrameEmitters.begin(), m_FrameEmitters.end());//And attach m_Sources to nodes that WILL be covered this frame
		}

		m_FrameEmitters.clear();	//We're done for the frame! empty the m_Emitters list
	}

	void SoundSystem::CullNodes()
	{
		for (auto i = m_FrameEmitters.begin(); i != m_FrameEmitters.end();)
		{
			auto length = 0.0f;

			if ((*i)->GetIsGlobal())
			{
				length = 0.0f;
			}
			else
			{
				length = (m_Listener->GetPosition() -
					(*i)->GetPosition()).Length();

				if (length < 0.0f)
					length *= -1.0f;
			}

			if (length > (*i)->GetRadius() || !(*i)->GetSound() || (*i)->GetTimeLeft() < 0)
			{
				(*i)->DetachSource();
				i = m_FrameEmitters.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	void SoundSystem::DetachSources(std::vector<std::shared_ptr<SoundNode>>::iterator from, std::vector<std::shared_ptr<SoundNode>>::iterator to)
	{
		for (auto i = from; i != to; ++i)
		{
			(*i)->DetachSource();
		}
	}

	void SoundSystem::AttachSources(std::vector<std::shared_ptr<SoundNode>>::iterator from, std::vector<std::shared_ptr<SoundNode>>::iterator to)
	{
		for (auto i = from; i != to; ++i)
		{
			if (!(*i)->GetSource())
			{
				(*i)->AttachSource(GetSource());
			}
		}
	}

	OALSource* SoundSystem::GetSource()
	{
		for (auto& s : m_Sources)
		{
			if (!s->inUse)
			{
				return s;
			}
		}
		return nullptr;
	}

	void SoundSystem::SetMasterVolume(float value)
	{
		value = maths::Max(0.0f, value);
		value = maths::Min(1.0f, value);
		m_MasterVolume = value;
#ifdef JM_OPENAL
		alListenerf(AL_GAIN, m_MasterVolume);
#endif
	}

	void SoundSystem::UpdateListener() const
	{
		if (m_Listener)
		{
			maths::Vector3 worldPos = m_Listener->GetPosition();
			maths::Vector3 velocity = m_Listener->GetVelocity();

			maths::Vector3 dirup[2];
			dirup[0] = m_Listener->GetForwardDirection();
			dirup[1] = m_Listener->GetUpDirection();

#ifdef JM_OPENAL
			alListenerfv(AL_POSITION, reinterpret_cast<float*>(&worldPos));
			alListenerfv(AL_VELOCITY, reinterpret_cast<float*>(&velocity));
			alListenerfv(AL_ORIENTATION, reinterpret_cast<float*>(&dirup));
#endif
		}
	}

	void SoundSystem::PlaySound(Sound* s, const maths::Vector3& position)
	{
		auto n = std::make_shared<SoundNode>();
		n->SetLooping(false);
		n->SetPosition(position);
		n->SetSound(s);
		m_TemporaryEmitters.push_back(n);
	}

	void SoundSystem::PlaySound(Sound* s, SoundPriority p)
	{
		auto n = std::make_shared<SoundNode>();
		n->SetLooping(false);
		n->SetSound(s);
		n->SetIsGlobal(true);
		n->SetPriority(p);
		m_TemporaryEmitters.push_back(n);
	}

	void SoundSystem::UpdateTemporaryEmitters(float msec)
	{
		for (auto i = m_TemporaryEmitters.begin(); i != m_TemporaryEmitters.end();)
		{
			if ((*i)->GetTimeLeft() < 0.0f && !(*i)->GetLooping())
			{
				i = m_TemporaryEmitters.erase(i);
			}
			else
			{
				(*i)->Update(msec);
				++i;
			}
		}
	}
}
