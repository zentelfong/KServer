#pragma once
#include "KMemPoll.h"

//T 内需要有min_heap_idx成员默认为-1
//需要重载>操作符

template<class T>
class KHeap
{
public:
	KHeap()
	{
		m_p = 0;
		m_n = 0;
		m_a = 0;
	}

	~KHeap()
	{
		if(m_p) kFree(m_p); 
	}

	void Clear()
	{
		if(m_p) kFree(m_p); 
		m_p = 0;
		m_n = 0;
		m_a = 0;
	}

	bool IsEmpty()
	{
		return 0u == m_n;
	}


	int Size()
	{
		return m_n;
	}


	T* Top()
	{
		return m_n ? *m_p : 0;
	}


	bool Push(T* e)
	{
		if(reserve(m_n + 1))
			return false;
		shift_up_(m_n++, e);
		return true;
	}


	T* Pop()
	{
		if(m_n)
		{
			T* e = *m_p;
			shift_down_(0u, m_p[--m_n]);
			e->min_heap_idx = -1;
			return e;
		}
		return 0;
	}


	bool Erase(T* e)
	{
		if(((unsigned int)-1) != e->min_heap_idx)
		{
			T *last = m_p[--m_n];
			unsigned parent = (e->min_heap_idx - 1) / 2;
		   /* we replace e with the last element in the heap.  We might need to
		   shift it upward if it is less than its parent, or downward if it is
		   greater than one or both its children. Since the children are known
		   to be less than the parent, it can't need to shift both up and
		   down. */
			if (e->min_heap_idx > 0 && (*(m_p[parent]) > *last))
				shift_up_(e->min_heap_idx, last);
			else
				shift_down_(e->min_heap_idx, last);
			e->min_heap_idx = -1;
			return true;
		}
		return false;
	}

private:

	int reserve(unsigned n)
	{
		if(m_a < n)
		{
			T** p;
			unsigned a = m_a ? m_a * 2 : 8;
			if(a < n)
				a = n;
			if(!(p = (T**)kRealloc(m_p, a * sizeof *p)))
				return -1;
			m_p = p;
			m_a = a;
		}
		return 0;
	}

	void shift_up_(unsigned hole_index,T* e)
	{
		unsigned parent = (hole_index - 1) / 2;
		while(hole_index && *(m_p[parent]) > *e)
		{
			(m_p[hole_index] = m_p[parent])->min_heap_idx = hole_index;
			hole_index = parent;
			parent = (hole_index - 1) / 2;
		}
		(m_p[hole_index] = e)->min_heap_idx = hole_index;
	}

	void shift_down_(unsigned hole_index,T* e)
	{
		unsigned min_child = 2 * (hole_index + 1);
		while(min_child <= m_n)
		{
			min_child -= min_child == m_n || (*(m_p[min_child]) > *(m_p[min_child - 1]));
			if(!(*e > *(m_p[min_child])))
				break;
			(m_p[hole_index] = m_p[min_child])->min_heap_idx = hole_index;
			hole_index = min_child;
			min_child = 2 * (hole_index + 1);
		}
		shift_up_(hole_index,  e);
	}

	T** m_p;
	unsigned m_n;
	unsigned m_a;
};