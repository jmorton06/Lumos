#pragma once
#include "LM.h"
#include "Maths/Maths.h"
#include "SoundPriority.h"

#include <AL/al.h>
#include <AL/alc.h>

namespace Lumos
{
	class SoundNode;
    class Sound;
    class Camera;
	struct TimeStep;

	struct LUMOS_EXPORT OALSource
	{
		ALuint	source;
		bool	inUse;

		explicit OALSource(ALuint src)
		{
			source = src;
			inUse = false;
		}
	};

	class LUMOS_EXPORT SoundSystem
	{
	public:
		static void Initialise(unsigned int channels = 8)
		{
			m_Instance = new SoundSystem(channels);
		}

		static void Destroy()
		{
			delete m_Instance;
		}

		static SoundSystem* Instance()
		{
			return m_Instance;
		}

		void		SetListener(Camera* l)	{ m_Listener = l; }
		Camera*		GetListener() const { return m_Listener; }

		void		AddSoundNode(std::shared_ptr<SoundNode>& s)	{ m_Emitters.push_back(s); }

		void		Update(TimeStep* timeStep);
		void		SetMasterVolume(float value);

		void		PlaySound(Sound* s, const maths::Vector3& position);
		void		PlaySound(Sound* s, SoundPriority p);
		void		RemoveAllSoundNodes();

	protected:

		explicit SoundSystem(unsigned int channels = 128);
		~SoundSystem();

		void		UpdateListener() const;

		static void	DetachSources(std::vector<std::shared_ptr<SoundNode>>::iterator from, std::vector<std::shared_ptr<SoundNode>>::iterator to);
		void		AttachSources(std::vector<std::shared_ptr<SoundNode>>::iterator from, std::vector<std::shared_ptr<SoundNode>>::iterator to);

		void		CullNodes();

		OALSource*	GetSource();

		void		UpdateTemporaryEmitters(float msec);

		std::vector<OALSource*>	m_Sources;

		std::vector<std::shared_ptr<SoundNode>>	m_Emitters;			//List of all m_Emitters
		std::vector<std::shared_ptr<SoundNode>> m_FrameEmitters;		//List of m_Emitters that can be heard in frame
		std::vector<std::shared_ptr<SoundNode>>	m_TemporaryEmitters;

		Camera*				m_Listener;

		ALCcontext*			m_Context;
		ALCdevice*			m_Device;

		float				m_MasterVolume;

		static SoundSystem* m_Instance;
	};
}
