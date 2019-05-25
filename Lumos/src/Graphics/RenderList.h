#pragma once
#include "LM.h"
#include "Entity/Entity.h"
#include "Maths/Frustum.h"
#include "Maths/Maths.h"

namespace lumos
{

	//Maximum number of elements added to the renderlist per frame
	// - Will be added on future frames instead to share workload and prevent lock-ups
#define MAX_LIST_CHANGE_PER_FRAME 300

	//Sort opaque objects front to back to reduce over drawing - tie up between slow sorting or slow rendering, in the current
	//usage the sorting is almost always the bottlekneck and overdraw on the Gfx card is negligible.
	// - Transparent objects however always need to be sorted in order to correctly blend with background objects.
#define SORT_OPAQUE_LIST FALSE

	struct LUMOS_EXPORT RenderList_Object
	{
		float cam_dist_sq;
		std::weak_ptr<Entity> target_obj;
	};

	class LUMOS_EXPORT RenderList
	{
	public:

		RenderList();
		virtual ~RenderList();

		//RenderList Factory
		//  - Attempts to create new renderlist - returns false if max-renderlists (32) have already previously been allocated.
		static bool AllocateNewRenderList(RenderList* renderlist, bool supportsTransparency);

		//Updates all current objects 'distance' to camera
		void UpdateCameraWorldPos(const maths::Vector3& cameraPos);

		//Sort lists based on camera position using insertion sort. With frame coherency,
		// the list should be 'almost' sorted each frame and only a few elements need to be swapped in this function.
		void SortLists();

		//Removes all objects no longer inside the frustum
		void RemoveExcessObjects(const maths::Frustum& frustum);

		//Called when object moves inside the frustum (Inserts via insertion sort)
		void InsertObject(std::shared_ptr<Entity>& obj);

		//Misc. Removes a single object from the list, in general just call 'RemoveExcessObjects' and let it remove the object automatically
		void RemoveObject(std::shared_ptr<Entity>& obj);

		//Clears the entire list
		void RemoveAllObjects();

		void Clear();

		//Iterate over each object in the list calling the provided function per-object.
		void RenderOpaqueObjects(const std::function<void(Entity*)>& per_object_func);
		void RenderTransparentObjects(const std::function<void(Entity*)>& per_object_func);

		//Get the bitmask set per Object in the list
		uint BitMask() const { return m_BitMask; }

		int GetOpaqueListSize() const { return static_cast<int>(m_vRenderListOpaque.size()); }
		int GetTransparentListSize() const { return static_cast<int>(m_vRenderListTransparent.size()); }

	protected:
		//Keeps a list of the number of unique render lists as to enforce and maintain the hard limit on 32 renderlists.
		static uint g_NumRenderLists;

		uint m_NumElementsChanged;

		//Bit mask for renderlist, uint leads to 32 booleans (1's or 0's) thus there is a hardlimit of 31 shadow maps along with the main camera render list.
		uint m_BitMask;

		//If false - all transparent objects will be ignored (maybe shadow render passes?)
		bool m_SupportsTransparancy;

		//Last provided camera position for sorting/inserting new objects
		maths::Vector3 m_CameraPos;

		//Sorted renderlists of visible objects
		std::vector<RenderList_Object> m_vRenderListOpaque;
		std::vector<RenderList_Object> m_vRenderListTransparent;

	private:
		//No copying! - the deconstructor updates a global counter of active renderlists so deleting a copy would cause all future renderlist instances to overlap each other
		RenderList(const RenderList& rl) : m_NumElementsChanged(0), m_BitMask(0), m_SupportsTransparancy(false)
		{
		}
	};
}
