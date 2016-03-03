#include "stdafx.h"
#include "COMInitializer.h"

COMInitializer::COMInitializer() {

}

COMInitializer::~COMInitializer() {
	CoUninitialize();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
HRESULT COMInitializer::InitializeCOM() const {
	return CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}