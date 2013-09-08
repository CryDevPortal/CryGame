////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ResourceCompilerHelper.h
//  Version:     v1.00
//  Created:     12/07/2004 by MartinM.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef NOT_USE_CRY_MEMORY_MANAGER
	#include "CryModuleDefs.h"												// ECryModule
#endif

#ifndef __RESOURCECOMPILERHELPER_H__
#define __RESOURCECOMPILERHELPER_H__
#pragma once

#include "SettingsManagerHelpers.h"
#include <stdio.h>     // strlen()

class IResourceCompilerListener
{
public:
	enum MessageSeverity
	{
		MessageSeverity_Debug,
		MessageSeverity_Info,
		MessageSeverity_Warning,
		MessageSeverity_Error
	};
	virtual void OnRCMessage(MessageSeverity severity, const char* text) = 0;
	virtual ~IResourceCompilerListener(){}
};


class CEngineSettingsManager;

enum ERcExitCode
{
	eRcExitCode_Success = 0,   // must be 0
	eRcExitCode_Error = 1,
	eRcExitCode_FatalError = 100,
	eRcExitCode_Crash = 101,
	eRcExitCode_UserFixing = 200,
};

//////////////////////////////////////////////////////////////////////////
// Provides settings and functions to make calls to RC.
class CResourceCompilerHelper
{
public:
	enum ERcCallResult
	{
		eRcCallResult_success,
		eRcCallResult_notFound,
		eRcCallResult_error,
		eRcCallResult_crash
	};

	enum ERcExePath
	{
		eRcExePath_currentFolder,
		eRcExePath_registry,
		eRcExePath_settingsManager,
		eRcExePath_customPath
	};

#if (CRY_ENABLE_RC_HELPER)				// run compiler only on developer platform

	// Loads EngineSettingsManager to get settings information
	CResourceCompilerHelper(const wchar_t* moduleName=0);

	~CResourceCompilerHelper();

#if defined(_ICRY_PAK_HDR_)
	// checks file date and existence
	// Return:
	//   fills processedFilename[] with name of the file that should be loaded
	void ProcessIfNeeded(
		const char* originalFilename, 
		char* processedFilename, 
		size_t processedFilenameSizeInBytes);
#endif // _ICRY_PAK_HDR_

	void GetRootPathUtf16(bool pullFromRegistry, SettingsManagerHelpers::CWCharBuffer wbuffer);
	void GetRootPathAscii(bool pullFromRegistry, SettingsManagerHelpers::CCharBuffer buffer);

	void ResourceCompilerUI( void* hParent );

	bool IsPrefer32Bit();

	CEngineSettingsManager* GetSettingsManager();

	void GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer wbuffer);

	bool IsError() const;

	//
	// Arguments:
	//   szFileName null terminated ABSOLUTE file path or 0 can be used to test for rc.exe existance, relative path needs to be relative to bin32/rc directory
	//   szAdditionalSettings - 0 or e.g. "/refresh" or "/refresh /xyz=56"
	//
	ERcCallResult CallResourceCompiler(
		const char* szFileName=0, 
		const char* szAdditionalSettings=0, 
		IResourceCompilerListener* listener=0, 
		bool bMayShowWindow=true, 
		ERcExePath rcExePath=eRcExePath_registry, 
		bool bSilent=false,
		bool bNoUserDialog=false,
		const wchar_t* szWorkingDirectory=0,
		const wchar_t* szRootPath=0);

	void* AsyncCallResourceCompiler(
		const char* szFileName=0, 
		const char* szAdditionalSettings=0, 
		bool bMayShowWindow=true, 
		ERcExePath rcExePath=eRcExePath_registry, 
		bool bSilent=false,
		bool bNoUserDialog=false,
		const wchar_t* szWorkingDirectory=0,
		const wchar_t* szRootPath=0);

	bool CallEditor(
		void** pEditorWindow,
		void* hParent,
		const char* pWndName,
		const char* pFlag);

	static const char* GetCallResultDescription(ERcCallResult result);

	bool GetInstalledBuildPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path);
	bool GetInstalledBuildPathAscii(const int index, SettingsManagerHelpers::CCharBuffer name, SettingsManagerHelpers::CCharBuffer path);

private:
	bool m_bErrorFlag;

private:
#if defined(_ICRY_PAK_HDR_)
	// Arguments:
	//   szFilePath - could be source or destination filename
	//   dwIndex - used to iterator through all input filenames, start with 0 and increment by 1
	// Return:
	//   fills inputFilename[] with a filename (or empty string if  that was the last input format)
	static void CResourceCompilerHelper::GetInputFilename(
		const char* filename, 
		const unsigned int index,
		char* inputFilename, 
		size_t inputFilenameSizeInBytes);
#endif

	// Arguments:
	//   szDataFolder usually DATA_FOLDER = "Game"
	bool InvokeResourceCompiler( const char *szSrcFile, const bool bWindow ) const; 

#endif // CRY_ENABLE_RC_HELPER

private:
	// little helper function (to stay independent)
	static const char* GetExtension(const char* in)
	{
		const size_t len = strlen(in);
		for(const char* p = in + len-1; p >= in; --p)
		{
			switch(*p)
			{
			case ':':
			case '/':
			case '\\':
				// we've reached a path separator - it means there's no extension in this name
				return 0;
			case '.':
				// there's an extension in this file name
				return p+1;
			}
		}
		return 0;
	}	

	// little helper function (to stay independent)
	static void ReplaceExtension(const char* path, const char* new_ext, char* buffer, size_t bufferSizeInBytes)
	{
		const char* const ext = GetExtension(path);
    
		SettingsManagerHelpers::CFixedString<char, 512> p;
		if(ext)
		{
			p.set(path, ext - path);
			p.append(new_ext);
		}
		else
		{
			p.set(path);
			p.append(".");
			p.append(new_ext);
		}
    
		strncpy_s(buffer, bufferSizeInBytes, p.c_str(), _TRUNCATE);
	}

public:
	// Arguments:
	//   szFilePath - could be source or destination filename
	static void GetOutputFilename(const char* szFilePath, char* buffer, size_t bufferSizeInBytes)
	{
		const char* const ext = GetExtension(szFilePath);

		if (ext)
		{
			if (stricmp(ext, "tif") == 0 ||
				stricmp(ext, "srf") == 0)
			{
				ReplaceExtension(szFilePath, "dds", buffer, bufferSizeInBytes);
				return;
			}
		}

		strncpy_s(buffer, bufferSizeInBytes, szFilePath, _TRUNCATE);
	}

	// only for image formats supported by the resource compiler
	// Arguments:
	//   szExtension - e.g. ".tif", can be 0
	static bool IsImageFormat(const char* szExtension)
	{
		if (szExtension)
		{
			if (stricmp(szExtension, "dds") == 0   ||  // DirectX surface format
				stricmp(szExtension, "tif") == 0)      // Crytek resource compiler image input format	
			{
				return true;
			}
		}
		return false;
	}
};


#endif // __RESOURCECOMPILERHELPER_H__
