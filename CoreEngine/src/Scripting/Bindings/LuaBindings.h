#pragma once
//Include all bindings for each classes
#include "JM.h"
#include "Scripting/Bindings/CameraBindings/CameraThirdPersonBinding.h"
#include "EntityBinding.h"
#include "MathsBindings/Vector2Binding.h"
#include "MathsBindings/Vector3Binding.h"
#include "MathsBindings/Vector4Binding.h"
#include "MathsBindings/Matrix4Binding.h"

#include "CameraBindings/CameraBinding.h"
#include "CameraBindings/CameraFPSBinding.h"
#include "CameraBindings/Camera2DBinding.h"
#include "CameraBindings/CameraMayaBinding.h"
#include "SceneBinding.h"

namespace jm
{
	namespace luabindings
	{
		inline void BindAll()
		{
			EntityBinding::Bind();
			Vector2Binding::Bind();
			Vector3Binding::Bind();
			Vector4Binding::Bind();
			Matrix4Binding::Bind();

			CameraBinding::Bind();
			Camera2DBinding::Bind();
			CameraFPSBinding::Bind();
			CameraMayaBinding::Bind();
			CameraThirdPersonBinding::Bind();
			SceneBinding::Bind();
		}
	}
}
