#pragma once

#include <activation.h>

#define ACTIVATABLE_OBJECT1(cnt, obj, clsname)	static com::ActivationObject<obj > Host##cnt (clsname);
#define ACTIVATABLE_OBJECT(obj, clsname)	ACTIVATABLE_OBJECT1(__COUNTER__,obj, clsname)

namespace com {

class ActivationFactory : public ::IActivationFactory
{
public:
	ActivationFactory(const wchar_t *className);
	STDMETHODIMP_(ULONG) AddRef() { return 42; }
	STDMETHODIMP_(ULONG) Release() { return 42; }
	STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObj) {
		if(!ppvObj) return E_POINTER;
		if(iid == IID_IUnknown)				{ *ppvObj = static_cast<IUnknown*>(this); return S_OK; }
		if(iid == IID_IInspectable)			{ *ppvObj = static_cast<IInspectable*>(this); return S_OK; }
		if(iid == IID_IActivationFactory)	{ *ppvObj = static_cast<IActivationFactory*>(this); return S_OK; }
		return *ppvObj = NULL, E_NOINTERFACE;
	}
	STDMETHODIMP GetIids(ULONG *iidCount, IID **iids) {
		if(!iidCount || !iids)	return E_INVALIDARG;
		*iidCount = 1;
		*iids = static_cast<IID*>(::CoTaskMemAlloc(sizeof(IID)));
		if(!*iids)	return E_OUTOFMEMORY;
		**iids = IID_IActivationFactory;
		return S_OK;
	}
	STDMETHODIMP GetRuntimeClassName(HSTRING *className) { return ::WindowsCreateString(_className, ::wcslen(_className), className); }
	STDMETHODIMP GetTrustLevel(::TrustLevel *trustLevel) { return trustLevel ? *trustLevel = ::TrustLevel::BaseTrust, S_OK : E_INVALIDARG; }
	STDMETHODIMP GetFactory(HSTRING className, IActivationFactory** factory) {
		const wchar_t* name = ::WindowsGetStringRawBuffer(className, nullptr);
		return ::wcscmp(name, _className) == 0 ? QueryInterface(IID_IActivationFactory, reinterpret_cast<void**>(factory)) : _next ? _next->GetFactory(className, factory) : E_NOINTERFACE;
	}
	STDMETHODIMP ActivateInstance(IInspectable **instance) { return E_NOTIMPL; }
protected:
	ActivationFactory*	_next;
	const wchar_t*		_className;
};

template<class T> class ActivationObject : public ActivationFactory
{
public:
	ActivationObject(const wchar_t *className) : ActivationFactory(className) {};
	STDMETHODIMP ActivateInstance(IInspectable **instance) {
		if(!instance || *instance)	return E_INVALIDARG;
		T* pobj = new T();
		if(!pobj)					return E_OUTOFMEMORY;
		pobj->AddRef();
		HRESULT hr = pobj->QueryInterface(IID_IInspectable, (void **)instance);
		pobj->Release();
		return hr;
	}
};

}
