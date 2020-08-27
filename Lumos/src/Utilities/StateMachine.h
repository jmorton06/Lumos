#pragma once

#include "State.h"

namespace Lumos
{
	class StateMachine
	{
		enum class PushState
		{
			None,
			Pause,
			Suspend,
			Terminate
		};

	public:
		StateMachine();
		~StateMachine();

		void Pop();
		void PopExact(const Ref<State>& state);

		void Push(const Ref<State>& state);

		void Update();
		void Pause();
		void Resume();

	private:
		std::vector<Ref<State>> m_States;
		Ref<State> m_CurrentState = nullptr;
	};
}