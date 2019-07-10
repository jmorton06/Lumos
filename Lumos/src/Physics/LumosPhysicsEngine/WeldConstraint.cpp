#include "LM.h"
#include "Maths/Maths.h"
#include "WeldConstraint.h"
#include "PhysicsObject3D.h"


namespace Lumos
{

	WeldConstraint::WeldConstraint(PhysicsObject3D *obj1, PhysicsObject3D *obj2)
		: m_pObj1(obj1)
		, m_pObj2(obj2)
        , m_positionOffset(obj2->GetPosition() - obj1->GetPosition())
        , m_orientation(obj2->GetOrientation())
	{
	}

	void WeldConstraint::ApplyImpulse()
	{
		// Position
		Maths::Vector3 pos(m_positionOffset);
		Maths::Quaternion::RotatePointByQuaternion(m_pObj1->GetOrientation(), pos);
		pos += m_pObj1->GetPosition();
		m_pObj2->SetPosition(pos);

		// Orientation
		m_pObj2->SetOrientation(m_pObj1->GetOrientation() * m_orientation);
	}

	void WeldConstraint::DebugDraw() const
	{
	}
}
