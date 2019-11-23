#pragma once
#include "lmpch.h"
#include "Sound.h"
#include "Maths/Maths.h"

namespace Lumos
{
	class LUMOS_EXPORT SoundNode
	{
	public:
		static SoundNode* Create();
		SoundNode();
		SoundNode(Sound* s);
		virtual ~SoundNode();

		void			Reset();

		Sound*			GetSound() const { return m_Sound; }

		void			SetVelocity(const Maths::Vector3& vel) { m_Velocity = vel; }
		Maths::Vector3	GetVelocity() const { return m_Velocity; }

		void			SetPosition(const Maths::Vector3& pos) { m_Position = pos; }
		Maths::Vector3	GetPosition() const { return m_Position; }

		void			SetVolume(float volume) { m_Volume = Maths::Min(1.0f, Maths::Max(0.0f, volume)); }
		float			GetVolume() const { return m_Volume; }

		void			SetLooping(bool state) { m_IsLooping = state; }
		bool			GetLooping() const { return m_IsLooping; }

		void			SetPaused(bool state) { m_Paused = state; }
		bool			GetPaused() const { return m_Paused; }

		void			SetRadius(float value) { m_Radius = Maths::Max(0.0f, value); }
		float			GetRadius() const { return m_Radius; }

		float			GetPitch() const { return m_Pitch; }
		void			SetPitch(float value) { m_Pitch = value; }

		float			GetReferenceDistance() const { return m_ReferenceDistance; }
		void			SetReferenceDistance(float value) { m_ReferenceDistance = value; }

		bool			GetIsGlobal() const { return m_IsGlobal; }
		void			SetIsGlobal(bool value) { m_IsGlobal = value; }

		bool			GetStationary() const { return m_Stationary; }
		void			SetStationary(bool value) { m_Stationary = value; }

		double			GetTimeLeft() const { return m_TimeLeft; }

		virtual void OnUpdate(float msec) = 0;
		virtual void Pause() = 0;
		virtual void Resume() = 0;
		virtual void Stop() = 0;
		virtual void SetSound(Sound *s);

	protected:

		Sound*			m_Sound;
		Maths::Vector3	m_Position;
		Maths::Vector3	m_Velocity;
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
	};

}
