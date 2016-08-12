#pragma once

#include "KMemPoll.h"

#ifndef uthash_malloc
#define uthash_malloc(sz) kMalloc(sz)
#endif

#ifndef uthash_free
#define uthash_free(ptr,sz) kFree(ptr)
#endif

#include "uthash.h"

//hash ±í
template<class T>
class KHashMap
{
public:
	typedef unsigned int key_t;
	typedef T data_t;

	struct KHashMapImpl
	{
		UT_hash_handle hh;
		data_t* value;
		key_t   key;
	};

	KHashMap(){
		m_hashMap=NULL;
	}

	~KHashMap()
	{
		Clear();
	}

	void Clear()
	{
		if (!m_hashMap)
			return;
		KHashMapImpl *val=NULL,*tmp=NULL;
		HASH_ITER(hh, m_hashMap, val, tmp) 
		{
			HASH_DEL(m_hashMap,val);
			kFree(val);
		}
		m_hashMap=NULL;
	}


	T* Find(key_t key)
	{
		KHashMapImpl* val=NULL;
		HASH_FIND_INT( m_hashMap, &key,val);
		if (val)
			return val->value;
		else
			return NULL;
	}

	void Set(key_t key,data_t* value)
	{
		KHashMapImpl* val=NULL;
		HASH_FIND_INT(m_hashMap, &key,val);	
		if (val)
		{
			HASH_DEL(m_hashMap,val);
			kFree(val);
		}
		val = (struct KHashMapImpl*)kMalloc(sizeof(KHashMapImpl));
		val->value=value;
		val->key=key;
		HASH_ADD_INT(m_hashMap,key,val);
	}

	bool Remove(key_t key)
	{
		KHashMapImpl* val=NULL;
		HASH_FIND_INT( m_hashMap, &key,val);
		if(val)
		{
			HASH_DEL(m_hashMap,val);
			kFree(val);
			return true;
		}
		else
			return false;
	}

	int GetSize() const
	{
		return HASH_COUNT(m_hashMap);
	}

	//µü´úÆ÷
	class Iterator
	{
	public:
		Iterator(KHashMapImpl* data)
			:m_data(data)
		{
		}
		Iterator(const Iterator& rfs)
		{
			m_data=rfs.m_data;
		}
		Iterator& operator++()
		{
			if(m_data)
				m_data=(struct KHashMapImpl*)(m_data->hh.next);
			return *this;
		}
		bool operator==(const Iterator& rfs)
		{
			return m_data==rfs.m_data;
		}
		bool operator!=(const Iterator& rfs)
		{
			return m_data!=rfs.m_data;
		}
		data_t* operator*()
		{
			if(m_data)
				return m_data->value;
			else
				return NULL;
		}
		data_t* Data()
		{
			if(m_data)
				return m_data->value;
			else
				return NULL;
		}
		const char* Key()
		{
			if(m_data)
				return m_data->key;
			else
				return NULL;
		}
	private:
		friend class KHashMap;
		KHashMapImpl* m_data;
	};

	Iterator Begin()
	{
		return Iterator(m_hashMap);
	}

	Iterator End()
	{
		return Iterator(NULL);
	}


	bool Remove(Iterator* it)
	{
		if(it)
		{
			HASH_DEL(m_hashMap,it->m_data);
			kFree(it->m_data);
			return true;
		}
		else
			return false;
	}

private:
	KHashMap(const KHashMap&){}
	KHashMapImpl* m_hashMap;
};

