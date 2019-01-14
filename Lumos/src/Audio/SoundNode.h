#pragma once
#include "LM.h"
#include "Sound.h"
#include "SoundSystem.h"
#include "SoundPriority.h"
#include "Maths/MathsUtilities.h"

#define NUM_STREAM_BUFFERS 3

namespace Lumos
{
	class LUMOS_EXPORT SoundNode
	{
	public:
		SoundNode();
		explicit SoundNode(Sound* s);
		virtual ~SoundNode();

		void			Reset();

		void			SetSound(Sound *s);
		Sound*			GetSound() const { return m_Sound; }

		void			SetPriority(SoundPriority p) { m_Priority = p; }
		SoundPriority	GetPriority() const { return m_Priority; }

		void			SetVelocity(const maths::Vector3& vel)	 { m_Velocity = vel; }
		maths::Vector3	GetVelocity() const { return m_Velocity; }

		void			SetPosition(const maths::Vector3& pos)	 { m_Position = pos; }
		maths::Vector3	GetPosition() const { return m_Position; }

		void			SetVolume(float volume)		 { m_Volume = maths::Min(1.0f, maths::Max(0.0f, volume)); }
		float			GetVolume() const { return m_Volume; }

		void			SetLooping(bool state)		 { m_IsLooping = state; }
		bool			GetLooping() const { return m_IsLooping; }

		void			SetPaused(bool state)		 { m_Paused = state; }
		bool			GetPaused() const { return m_Paused; }

		void			SetRadius(float value)		 { m_Radius = maths::Max(0.0f, value); }
		float			GetRadius() const { return m_Radius; }

		float			GetPitch() const { return m_Pitch; }
		void			SetPitch(float value)		 { m_Pitch = value; }

		float			GetReferenceDistance() const { return m_ReferenceDistance; }
		void			SetReferenceDistance(float value)		 { m_ReferenceDistance = value; }

		bool			GetIsGlobal() const { return m_IsGlobal; }
		void			SetIsGlobal(bool value)		 { m_IsGlobal = value; }

		bool			GetStationary() const { return m_Stationary; }
		void			SetStationary(bool value)	 { m_Stationary = value; }

		double			GetTimeLeft() const { return m_TimeLeft; }

		OALSource*		GetSource() const { return m_OALSource; }

		void			UpdateSoundState(float msec);

		static bool		CompareNodesByPriority(std::shared_ptr<SoundNode>& a, std::shared_ptr<SoundNode>& b);

		void			AttachSource(OALSource* s);
		void			DetachSource();

		virtual void	Update(float msec);

		void Pause() { }//alSourcePause(m_OALSource->source); m_Paused = true;
	
		void Resume() { };// alSourcePlay(m_OALSource->source);  m_Paused = false;
	
		void Stop() const {};// alSourceStop(m_OALSource->source);


	protected:

		Sound*			m_Sound;
		OALSource*		m_OALSource;
		SoundPriority	m_Priority;
		maths::Vector3	m_Position;
		maths::Vector3	m_Velocity;
		float			m_Volume;
		float			m_Radius;
		float			m_Pitch;
		bool			m_IsLooping;
		bool			m_IsGlobal;
		double			m_TimeLeft;
		bool			m_Paused;
		float			m_ReferenceDistance;
		bool			m_Stationary;
		double			m_StreamPos;
		ALuint			m_StreamBuffers[NUM_STREAM_BUFFERS];
	};

}
