#include "LM.h"
#include "RenderList.h"


namespace Lumos
{

	uint RenderList::g_NumRenderLists = 0;

	bool RenderList::AllocateNewRenderList(RenderList* renderlist, bool supportsTransparency)
	{
		if (g_NumRenderLists >= 31)
		{
			LUMOS_CORE_ERROR("Attempting to create more than 32 renderlists!");
			return false;
		}

		renderlist->m_SupportsTransparancy = supportsTransparency;

		g_NumRenderLists++;

		return true;
	}

	RenderList::RenderList()
		: m_NumElementsChanged(0)
		, m_SupportsTransparancy(false)
		, m_CameraPos(0.0f, 0.0f, 0.0f)
	{
		m_BitMask = 0x1 << g_NumRenderLists;
	}

	RenderList::~RenderList()
	{
		RemoveAllObjects();
		g_NumRenderLists--;
	}

	void RenderList::RenderOpaqueObjects(const std::function<void(Entity*)>& per_object_func)
	{
		for (const auto& node : m_vRenderListOpaque)
		{
			per_object_func(node.target_obj.lock().get());
		}
	}

	void RenderList::RenderTransparentObjects(const std::function<void(Entity*)>& per_object_func)
	{
		if (m_SupportsTransparancy)
		{
			for (const auto& node : m_vRenderListTransparent) {
				per_object_func(node.target_obj.lock().get());
			}
		}
	}

	void RenderList::UpdateCameraWorldPos(const maths::Vector3& cameraPos)
	{
		m_NumElementsChanged = 0;
		m_CameraPos = cameraPos;

		//Update all distances to the camera in all render lists
		// Note: For transparent objects we want to sort them back to front, so in order to keep it simple and not change the sorting proceedure we just invert them distances.
		//		 This means that negative far distance is now less than negative near distance.

		auto update_list = [&](std::vector<RenderList_Object>& list, float mul)
		{
#pragma omp parallel for
			for (auto &i : list)
			{
				i.cam_dist_sq = (i.target_obj.lock()->GetComponent<TransformComponent>()->m_Transform.GetWorldMatrix().GetPositionVector() - m_CameraPos).LengthSquared() * mul;
			}
		};

#if SORT_OPAQUE_LIST
		update_list(m_vRenderListOpaque, 1.0f);
#endif

		if (m_SupportsTransparancy)
			update_list(m_vRenderListTransparent, -1.0f);
	}

	void RenderList::SortLists()
	{
		RenderList_Object swap_buffer{};

		auto sort_list = [&](std::vector<RenderList_Object>& list)
		{
			int i = 1, j = 0, size = static_cast<int>(list.size());
			for (; i < size; ++i)
			{
				swap_buffer = list[i];
				j = i - 1;

				while (j >= 0 && list[j].cam_dist_sq > swap_buffer.cam_dist_sq)
				{
					list[j + 1] = list[j];
					j--;
				}

				list[j + 1] = swap_buffer;
			}
		};

#if SORT_OPAQUE_LIST
		sort_list(m_vRenderListOpaque);
#endif

		if (m_SupportsTransparancy)
			sort_list(m_vRenderListTransparent);
	}

	void RenderList::RemoveExcessObjects(const maths::Frustum& frustum)
	{
		auto mark_objects_for_removal = [&](std::vector<RenderList_Object>& list)
		{
			//First iterate over each object in the list and mark it for removal (this can easily be parallised as it does not need any synchronisation)
			const auto size = static_cast<int>(list.size());

#pragma omp parallel for
			for (int i = 0; i < size; ++i)
			{
				if (list[i].target_obj.expired())
					break;

				Entity* entity = list[i].target_obj.lock().get();

				if (!frustum.InsideFrustum(entity->GetComponent<TransformComponent>()->m_Transform.GetWorldMatrix().GetPositionVector(), entity->GetBoundingRadius()))
				{
					entity->GetFrustumCullFlags() &= ~m_BitMask;
				}
			}

			//Next iterate over the list - removing any objects that are no longer inside the frustum
			int n_removed = 0;
			for (int i = 0; i < size; ++i)
			{
				if (list[i].target_obj.expired() || !(list[i].target_obj.lock()->GetFrustumCullFlags() & m_BitMask))
				{
					n_removed++;
				}
				else if (n_removed > 0)
				{
					list[i - n_removed] = list[i];
				}
			}

			if (n_removed > 0)
			{
				//list._Pop_back_n(n_removed);
				for (int i = 0; i < n_removed; i++)
				{
					list.pop_back();
				}
			}
		};

		mark_objects_for_removal(m_vRenderListOpaque);

		if (m_SupportsTransparancy)
			mark_objects_for_removal(m_vRenderListTransparent);
	}

	void RenderList::InsertObject(std::shared_ptr<Entity>& obj)
	{
		const bool isOpaque = true;// obj->IsOpaque();	TODO:
		if (!m_SupportsTransparancy && !isOpaque)
		{
			//This renderlist does not support transparent objects
			return;
		}

		if (m_NumElementsChanged >= MAX_LIST_CHANGE_PER_FRAME)
		{
			//No more list changes this frame - all new elements will have to be added over future frames
			return;
		}
		m_NumElementsChanged++;
		obj->GetFrustumCullFlags() |= m_BitMask;

		auto target_list = &m_vRenderListOpaque;

		RenderList_Object carry_obj;
		carry_obj.target_obj = obj;

#if !SORT_OPAQUE_LIST
		if (!isOpaque)
		{
#endif
			carry_obj.cam_dist_sq = (obj->GetComponent<TransformComponent>()->m_Transform.GetWorldMatrix().GetPositionVector() - m_CameraPos).LengthSquared();

			if (!isOpaque)
			{
				//If the object is transparent, add it to the transparent render list
				target_list = &m_vRenderListTransparent;

				//To cheat the sorting system to always use the same sorting opperand, we just invert all transparent distances so negative far is less than neg near.
				carry_obj.cam_dist_sq = -carry_obj.cam_dist_sq;
			}

			//Find where the object should be inserted
			auto loc = target_list->end();
			for (auto itr = target_list->begin(); itr != target_list->end(); itr++)
			{
				if (itr->cam_dist_sq > carry_obj.cam_dist_sq)
				{
					loc = itr;
					break;
				}
			}
			target_list->insert(loc, carry_obj);

#if !SORT_OPAQUE_LIST
		}
		else
		{
			m_vRenderListOpaque.push_back(carry_obj);
		}
#endif
	}

	void RenderList::RemoveObject(std::shared_ptr<Entity>& obj)
	{
		const bool isOpaque = true;// obj->IsOpaque(); TODO:
		if (!m_SupportsTransparancy && !isOpaque)
		{
			//This renderlist does not support transparent objects
			return;
		}

		auto target_list = isOpaque ? &m_vRenderListOpaque : &m_vRenderListTransparent;
		uint new_size = static_cast<uint>(target_list->size());

		bool found = false;
		uint idx = 0;
		for (uint i = 0; !found && i < new_size; ++i)
		{
			if ((*target_list)[i].target_obj.lock().get() == obj.get())
			{
				found = true;
				idx = i;
				break;
			}
		}

		if (found)
		{
			new_size--; //As we want to remove an object from the end of the list

			//Move all elements after the 'to be deleted element' forward, so we can safely pop the last element without losing anyone
			for (uint i = idx; i < new_size; ++i)
			{
				(*target_list)[i] = (*target_list)[i + 1];
			}

			target_list->pop_back();
		}
	}

	void RenderList::RemoveAllObjects()
	{
		auto unmark_objects = [&](std::vector<RenderList_Object>& list)
		{
			//First iterate over each object in the list and mark it for removal (this can easily be parallised as it does not need any synchronisation)
			const auto size = static_cast<int>(list.size());

#pragma omp parallel for
			for (int i = 0; i < size; ++i)
			{
				if (!list[i].target_obj.expired())
				{
					Entity* obj = list[i].target_obj.lock().get();
					obj->GetFrustumCullFlags() &= ~m_BitMask;
				}
			}
		};
		unmark_objects(m_vRenderListOpaque);
		m_vRenderListOpaque.clear();

		unmark_objects(m_vRenderListTransparent);
		m_vRenderListTransparent.clear();
	}

	void RenderList::Clear()
	{
		m_vRenderListOpaque.clear();
		m_vRenderListTransparent.clear();
	}
}
