#include "lmpch.h"
#include "Reference.h"

namespace Lumos
{
    RefCount::RefCount()
    {
        m_Refcount.init();
        m_RefcountInit.init();
		m_WeakRefcount.init(0);
    }
    
    RefCount::~RefCount()
    {
    }
    
    bool RefCount::InitRef()
    {
        if (reference())
        {
            if (m_RefcountInit.get() > 0)
            {
                m_RefcountInit.unref();
                unreference(); // first referencing is already 1, so compensate for the ref above
            }
            
            return true;
        }
        else
        {
            
            return false;
        }
    }
    
    int RefCount::GetReferenceCount() const
    {
        return m_Refcount.get();
    }

	int RefCount::GetWeakReferenceCount() const
	{
		return m_WeakRefcount.get();
	}
    
    bool RefCount::reference()
    {
        bool success = m_Refcount.ref();
        
        return success;
    }
    
    bool RefCount::unreference()
    {
        bool die = m_Refcount.unref();
        
        return die;
    }

	bool RefCount::weakReference()
	{
		bool success = m_WeakRefcount.ref();

		return success;
	}

	bool RefCount::weakUnreference()
	{
		bool die = m_WeakRefcount.unref() && m_Refcount.count == 0;

		return die;
	}
}
