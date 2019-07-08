#include "LM.h"
#include "App/ISystem.h"

namespace Lumos
{
	class Entity;

	class LUMOS_EXPORT EntityManager
	{
		friend class Entity;
	public:
		EntityManager() = default;
		~EntityManager() { Clear(); }

		void Clear();
		Entity* CreateEntity(const String& name = "");
		void DestroyEntity(Entity* entity);

		const std::vector<Entity*>& GetEntities() const { return m_Entities; }

        void SetSignature(Entity* entity, Signature signature)
        {
            m_Signatures[entity] = signature;
        }
        
        Signature GetSignature(Entity* entity)
        {
            return m_Signatures[entity];
        }
        
	private:
		std::vector<Entity*> m_Entities;
        std::unordered_map<Entity*, Signature> m_Signatures{};
	};
}
