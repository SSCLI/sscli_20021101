// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
// ===========================================================================
// File:  atl.h
// 
// ===========================================================================

#ifndef __ATL_H__
#define __ATL_H__

#include "ole2.h"
/////////////////////////////////////////////////////////////////////////////
// COM Smart pointers

template <class T>
class _NoAddRefReleaseOnCComPtr : public T
{
	private:
		STDMETHOD_(ULONG, AddRef)()=0;
		STDMETHOD_(ULONG, Release)()=0;
};

//CComPtrBase provides the basis for all other smart pointers
//The other smartpointers add their own constructors and operators
template <class T>
class CComPtrBase
{
protected:
	CComPtrBase()
	{
		p = NULL;
	}
	CComPtrBase(int nNull)
	{
		(void)nNull;
		p = NULL;
	}
	CComPtrBase(T* lp)
	{
		p = lp;
		if (p != NULL)
			p->AddRef();
	}
public:
	typedef T _PtrClass;
	~CComPtrBase()
	{
		if (p)
			p->Release();
	}
	operator T*() const
	{
		return p;
	}
	T& operator*() const
	{
		return *p;
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&()
	{
		return &p;
	}
	_NoAddRefReleaseOnCComPtr<T>* operator->() const
	{
		return (_NoAddRefReleaseOnCComPtr<T>*)p;
	}
	bool operator!() const
	{
		return (p == NULL);
	}
	bool operator<(T* pT) const
	{
		return p < pT;
	}
	bool operator==(T* pT) const
	{
		return p == pT;
	}

	// Release the interface and set to NULL
	void Release()
	{
		T* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	// Attach to an existing interface (does not AddRef)
	void Attach(T* p2)
	{
		if (p)
			p->Release();
		p = p2;
	}
	// Detach the interface (does not Release)
	T* Detach()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CopyTo(T** ppT)
	{
		if (ppT == NULL)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}

    T* p;
};

template <class T>
class CComPtr : public CComPtrBase<T>
{
public:
	CComPtr()
	{
	}
	CComPtr(int nNull) :
		CComPtrBase<T>(nNull)
	{
	}
	CComPtr(T* lp) :
		CComPtrBase<T>(lp)

	{
	}
	CComPtr(const CComPtr<T>& lp) :
		CComPtrBase<T>(lp.p)
	{
	}
	T* operator=(T* lp)
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
	}
    T* operator=(const CComPtr<T>& lp)
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
	}
};

#define IUNKNOWN_METHODS \
private: ULONG volatile m_dwRef; \
public: \
    virtual ULONG STDMETHODCALLTYPE AddRef( void) { \
	return (ULONG)InterlockedIncrement((volatile LONG*)&m_dwRef); } \
    virtual ULONG STDMETHODCALLTYPE Release( void) { \
	ULONG new_ref = (ULONG)InterlockedDecrement((volatile LONG*)&m_dwRef); \
	if (new_ref == 0) { delete this; return 0; } return new_ref; } \


#define BEGIN_COM_MAP(t) \
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) \
	{ \
		if (ppvObject == NULL) \
		{ \
			return E_POINTER; \
		}

#define COM_INTERFACE_ENTRY(i) \
		if (riid == IID_##i) \
		{ \
			*ppvObject = (i*)this; \
                        this->AddRef(); \
			return S_OK; \
		}

#define END_COM_MAP() \
		return E_NOINTERFACE; \
	} \
	virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0; \
	virtual ULONG STDMETHODCALLTYPE Release( void) = 0;



template <const IID* piid>
class ISupportErrorInfoImpl : public ISupportErrorInfo
{
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		return (riid == *piid) ? S_OK : S_FALSE;
	}
};

inline IUnknown* AtlComPtrAssign(IUnknown** pp, IUnknown* lp)
{
	if (lp != NULL)
		lp->AddRef();
	if (*pp)
		(*pp)->Release();
	*pp = lp;
	return lp;
}

inline IUnknown* AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
{
	IUnknown* pTemp = *pp;
	*pp = NULL;
	if (lp != NULL)
		lp->QueryInterface(riid, (void**)pp);
	if (pTemp)
		pTemp->Release();
	return *pp;
}


class CComMultiThreadModelNoCS
{
public:
	static ULONG WINAPI Increment(LONG *p) {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LONG *p) {return InterlockedDecrement(p);}
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObject : public Base
{
public:
	typedef Base _BaseClass;

	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	~CComObject()
	{
		m_dwRef = -(LONG_MAX/2);
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}

	static HRESULT WINAPI CreateInstance(CComObject<Base>** pp);
};

template <class Base>
HRESULT WINAPI CComObject<Base>::CreateInstance(CComObject<Base>** pp)
{
	ATLASSERT(pp != NULL);
	if (pp == NULL)
		return E_POINTER;
	*pp = NULL;

	HRESULT hRes = E_OUTOFMEMORY;
	CComObject<Base>* p = NULL;
	p = new CComObject<Base>();
	if (p != NULL)
	{
        hRes = NOERROR;
	}
	*pp = p;
	return hRes;
}


// the functions in this class don't need to be virtual because
// they are called from CComObject
class CComObjectRootBase
{
public:
	CComObjectRootBase()
	{
		m_dwRef = 0L;
	}
public:
    LONG m_dwRef;
}; // CComObjectRootBase

template <class ThreadModel>
class CComObjectRootEx : public CComObjectRootBase
{
public:
	typedef ThreadModel _ThreadModel;

    ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
#ifdef _DEBUG
		LONG nRef = _ThreadModel::Decrement(&m_dwRef);
		if (nRef < -(LONG_MAX / 2))
		{
			ATLASSERT(0);
		}
		return nRef;
#else
		return _ThreadModel::Decrement(&m_dwRef);
#endif
	}
}; // CComObjectRootEx

typedef CComMultiThreadModelNoCS CComObjectThreadModel;

typedef CComObjectRootEx<CComObjectThreadModel> CComObjectRoot;

#endif // __ATL_H__
