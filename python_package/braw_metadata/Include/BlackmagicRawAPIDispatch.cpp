/* -LICENSE-START-
** Copyright (c) 2017 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
**
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/
/* BlackmagicRawAPIDispatch.cpp */

#include "BlackmagicRawAPI.h"
#include <mach-o/dyld.h>
#include <libgen.h>
#include <pthread.h>

#define kBlackmagicRawAPI_FrameworkName		"BlackmagicRawAPI.framework"

class BmdMutex
{
public:
	inline explicit BmdMutex() { pthread_mutex_init(&m_mutex, NULL); }
	inline ~BmdMutex() { pthread_mutex_destroy(&m_mutex); }
	inline void lock() { pthread_mutex_lock(&m_mutex); }
	inline void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
	pthread_mutex_t m_mutex;
};

class BmdScopedLock
{
public:
	inline explicit BmdScopedLock(BmdMutex& mutex) : m_mutex(mutex) { m_mutex.lock(); }
	inline ~BmdScopedLock() { m_mutex.unlock(); }

private:
	BmdMutex& m_mutex;
};

typedef IBlackmagicRawFactory* (*CreateRawFactoryFunc)(void);
typedef HRESULT (*VariantInitFunc)(Variant*);
typedef HRESULT (*VariantClearFunc)(Variant*);
typedef SafeArray* (*SafeArrayCreateFunc)(BlackmagicRawVariantType, uint32_t, SafeArrayBound*);
typedef HRESULT (*SafeArrayGetVartypeFunc)(SafeArray*, BlackmagicRawVariantType*);
typedef HRESULT (*SafeArrayGetLBoundFunc)(SafeArray*, uint32_t, long*);
typedef HRESULT (*SafeArrayGetUBoundFunc)(SafeArray*, uint32_t, long*);
typedef HRESULT (*SafeArrayAccessDataFunc)(SafeArray*, void**);
typedef HRESULT (*SafeArrayUnaccessDataFunc)(SafeArray*);
typedef HRESULT (*SafeArrayDestroyFunc)(SafeArray*);

static CFBundleRef							gBundleRef								= NULL;
static CreateRawFactoryFunc					gCreateBlackmagicRawFactoryInstance		= NULL;
static VariantInitFunc						gVariantInit							= NULL;
static VariantClearFunc						gVariantClear							= NULL;
static SafeArrayCreateFunc					gSafeArrayCreate						= NULL;
static SafeArrayGetVartypeFunc				gSafeArrayGetVartype					= NULL;
static SafeArrayGetLBoundFunc				gSafeArrayGetLBound						= NULL;
static SafeArrayGetUBoundFunc				gSafeArrayGetUBound						= NULL;
static SafeArrayAccessDataFunc				gSafeArrayAccessData					= NULL;
static SafeArrayUnaccessDataFunc			gSafeArrayUnaccessData					= NULL;
static SafeArrayDestroyFunc					gSafeArrayDestroy						= NULL;

static BmdMutex								gBlackmagicRawMutex;

static void TryLoadBlackmagicRawAPI(CFURLRef bundleURL)
{
	gBundleRef = CFBundleCreate(kCFAllocatorDefault, bundleURL);
	if (gBundleRef != NULL)
	{
		gCreateBlackmagicRawFactoryInstance = (CreateRawFactoryFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("CreateBlackmagicRawFactoryInstance"));
		gVariantInit = (VariantInitFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("VariantInit"));
		gVariantClear = (VariantClearFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("VariantClear"));
		gSafeArrayCreate = (SafeArrayCreateFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("SafeArrayCreate"));
		gSafeArrayGetVartype = (SafeArrayGetVartypeFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("SafeArrayGetVartype"));
		gSafeArrayGetLBound = (SafeArrayGetLBoundFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("SafeArrayGetLBound"));
		gSafeArrayGetUBound = (SafeArrayGetUBoundFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("SafeArrayGetUBound"));
		gSafeArrayAccessData = (SafeArrayAccessDataFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("SafeArrayAccessData"));
		gSafeArrayUnaccessData = (SafeArrayUnaccessDataFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("SafeArrayUnaccessData"));
		gSafeArrayDestroy = (SafeArrayDestroyFunc)CFBundleGetFunctionPointerForName(gBundleRef, CFSTR("SafeArrayDestroy"));
	}
}

IBlackmagicRawFactory* CreateBlackmagicRawFactoryInstanceFromBundle (void)
{
	BmdScopedLock lock(gBlackmagicRawMutex);

	if (gCreateBlackmagicRawFactoryInstance == NULL)
	{
		CFBundleRef mainBundle = CFBundleGetMainBundle();
		if (mainBundle != NULL)
		{
			CFRetain(mainBundle);
			CFURLRef frameworksBundleURL = CFBundleCopyPrivateFrameworksURL(mainBundle);
			if (frameworksBundleURL != NULL)
			{
				CFURLRef bundleURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, frameworksBundleURL, CFSTR(kBlackmagicRawAPI_FrameworkName), false);
				CFRelease(frameworksBundleURL);
				if (bundleURL != NULL)
				{
					TryLoadBlackmagicRawAPI(bundleURL);
					CFRelease(bundleURL);
				}
			}
			CFRelease(mainBundle);
		}
	}

	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return NULL;

	return gCreateBlackmagicRawFactoryInstance();
}

IBlackmagicRawFactory* CreateBlackmagicRawFactoryInstance (void)
{
	IBlackmagicRawFactory* factory = NULL;

	if (gCreateBlackmagicRawFactoryInstance == NULL)
	{
		factory = CreateBlackmagicRawFactoryInstanceFromBundle();
		if (factory != NULL)
			return factory;
	}

	if (gCreateBlackmagicRawFactoryInstance == NULL)
	{
		factory = CreateBlackmagicRawFactoryInstanceFromExeRelativePath(NULL);
		if (factory != NULL)
			return factory;
	}

	BmdScopedLock lock(gBlackmagicRawMutex);

	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return NULL;

	return gCreateBlackmagicRawFactoryInstance();
}

IBlackmagicRawFactory* CreateBlackmagicRawFactoryInstanceFromPath (CFStringRef loadPath)
{
	BmdScopedLock lock(gBlackmagicRawMutex);

	if ((loadPath != NULL) && (gCreateBlackmagicRawFactoryInstance == NULL))
	{
		CFURLRef pathURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, loadPath, kCFURLPOSIXPathStyle, true);
		if (pathURL != NULL)
		{
			CFURLRef bundleURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, pathURL, CFSTR(kBlackmagicRawAPI_FrameworkName), false);
			if (bundleURL != NULL)
			{
				TryLoadBlackmagicRawAPI(bundleURL);
				CFRelease(bundleURL);
			}
			CFRelease(pathURL);
		}
	}

	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return NULL;

	return gCreateBlackmagicRawFactoryInstance();
}

IBlackmagicRawFactory* CreateBlackmagicRawFactoryInstanceFromExeRelativePath (CFStringRef relativePath)
{
	BmdScopedLock lock(gBlackmagicRawMutex);

	if (gCreateBlackmagicRawFactoryInstance == NULL)
	{
		uint32_t size = PATH_MAX;
		char path[PATH_MAX];
		int retVal = _NSGetExecutablePath(path, &size);
		if (retVal != 0)
			return nullptr;

		char resolvedPath[PATH_MAX];
		char* absolutePathName = realpath(path, resolvedPath);
		if (absolutePathName == nullptr)
			return nullptr;

		char* absoluteDirectory = dirname(absolutePathName);

		CFStringRef executablePath = CFStringCreateWithCString(NULL, absoluteDirectory, kCFStringEncodingUTF8);
		CFMutableStringRef loadPath = CFStringCreateMutable(NULL, 0);
		CFStringAppend(loadPath, executablePath);

		if (relativePath != NULL)
		{
			CFStringAppend(loadPath, CFSTR("/"));
			CFStringAppend(loadPath, relativePath);
		}

		CFURLRef pathURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, loadPath, kCFURLPOSIXPathStyle, true);
		if (pathURL != NULL)
		{
			CFURLRef bundleURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, pathURL, CFSTR(kBlackmagicRawAPI_FrameworkName), false);
			if (bundleURL != NULL)
			{
				TryLoadBlackmagicRawAPI(bundleURL);
				CFRelease(bundleURL);
			}
			CFRelease(pathURL);
		}

		if (loadPath != NULL)
			CFRelease(loadPath);
		if (executablePath != NULL)
			CFRelease(executablePath);
	}

	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return NULL;

	return gCreateBlackmagicRawFactoryInstance();
}

HRESULT VariantInit (Variant* variant)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gVariantInit(variant);
}

HRESULT VariantClear (Variant* variant)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gVariantClear(variant);
}

SafeArray* SafeArrayCreate (BlackmagicRawVariantType variantType, uint32_t dimensions, SafeArrayBound* safeArrayBound)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return NULL;

	return gSafeArrayCreate(variantType, dimensions, safeArrayBound);
}

HRESULT SafeArrayGetVartype (SafeArray* safeArray, BlackmagicRawVariantType* variantType)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gSafeArrayGetVartype(safeArray, variantType);
}

HRESULT SafeArrayGetLBound (SafeArray* safeArray, uint32_t dimensions, long* lBound)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gSafeArrayGetLBound(safeArray, dimensions, lBound);
}

HRESULT SafeArrayGetUBound (SafeArray* safeArray, uint32_t dimensions, long* uBound)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gSafeArrayGetUBound(safeArray, dimensions, uBound);
}

HRESULT SafeArrayAccessData (SafeArray* safeArray, void** outData)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gSafeArrayAccessData(safeArray, outData);
}

HRESULT SafeArrayUnaccessData (SafeArray* safeArray)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gSafeArrayUnaccessData(safeArray);
}

HRESULT SafeArrayDestroy (SafeArray* safeArray)
{
	if (gCreateBlackmagicRawFactoryInstance == NULL)
		return E_FAIL;

	return gSafeArrayDestroy(safeArray);
}
