#pragma once
#pragma once
#include <winnt.h>

/// <summary>
/// Very simple class that will initialize COM on creation, and uninitialize
/// COM on destruction.
/// </summary>
class COMInitializer
{
public:
	/// <summary>
	/// Default constructor.
	/// </summary>
	COMInitializer();
	/// <summary>
	/// Destructor that will call CoUninitialize.
	/// </summary>
	~COMInitializer();
	/// <summary>
	/// Initializes COM.
	/// </summary>
	/// <returns>The result of COM initialization.</returns>
	HRESULT InitializeCOM() const;
};