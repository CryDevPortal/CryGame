// #include <CryModuleDefs.h>

#include "StdAfx.h"

#if defined(CRY_ENABLE_RC_HELPER)

#undef RC_EXECUTABLE
#define RC_EXECUTABLE			"rc.exe"

#include "ResourceCompilerHelper.h"
#include "EngineSettingsManager.h"
#include "LineStreamBuffer.h"

#pragma warning (disable:4312)

#include <windows.h>		
#include <shellapi.h> //ShellExecute()


// pseudo-variable that represents the DOS header of the module
EXTERN_C IMAGE_DOS_HEADER __ImageBase;


static CEngineSettingsManager *g_pSettingsManager = 0;

//////////////////////////////////////////////////////////////////////////
CResourceCompilerHelper::CResourceCompilerHelper(const wchar_t* moduleName) : 
m_bErrorFlag(false)
{
	if (!g_pSettingsManager)
	{
		// FIXME: memory leak (ask Timur)
		g_pSettingsManager = new CEngineSettingsManager(moduleName);
	}
}


//////////////////////////////////////////////////////////////////////////
CResourceCompilerHelper::~CResourceCompilerHelper()
{
}


//////////////////////////////////////////////////////////////////////////
void CResourceCompilerHelper::GetRootPathUtf16(bool pullFromRegistry, SettingsManagerHelpers::CWCharBuffer wbuffer)
{
	if (pullFromRegistry)
	{
		g_pSettingsManager->GetRootPathUtf16(wbuffer);
	}
	else
	{
		g_pSettingsManager->GetValueByRef("ENG_RootPath", wbuffer);
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceCompilerHelper::GetRootPathAscii(bool pullFromRegistry, SettingsManagerHelpers::CCharBuffer buffer)
{
	wchar_t wbuffer[MAX_PATH];

	GetRootPathUtf16(pullFromRegistry, SettingsManagerHelpers::CWCharBuffer(wbuffer, sizeof(wbuffer)));

	SettingsManagerHelpers::GetAsciiFilename(wbuffer, buffer);
}

//////////////////////////////////////////////////////////////////////////
void CResourceCompilerHelper::ResourceCompilerUI( void* hParent )
{
	g_pSettingsManager->CallSettingsDialog(hParent);
}


class ResourceCompilerLineHandler
{
public:
	ResourceCompilerLineHandler(IResourceCompilerListener* listener): m_listener(listener) {}
	void HandleLine(const char* line)
	{
		if (m_listener && line)
		{
			// Check the first character to see if it's a warning or error.
			IResourceCompilerListener::MessageSeverity severity = IResourceCompilerListener::MessageSeverity_Info;

			if ((line[0] == 'E') && (line[1]==':'))
			{
				line += 2;  // skip the prefix
				severity = IResourceCompilerListener::MessageSeverity_Error;
			}
			else if ((line[0] == 'W') && (line[1]==':'))
			{
				line += 2;  // skip the prefix
				severity = IResourceCompilerListener::MessageSeverity_Warning;
			}
			else if ((line[0] == ' ') && (line[1]==' '))
			{
				line += 2;  // skip the prefix
			}

			// skip thread prefix
			while(*line == ' ')
			{
				++line;
			}
			while(isdigit(*line))
			{
				++line;
			}
			if (*line == '>')
			{
				++line;
			}

			// skip time
			while(*line == ' ')
			{
				++line;
			}
			while(isdigit(*line))
			{
				++line;
			}
			if(*line == ':')
			{
				++line;
			}
			while(isdigit(*line))
			{
				++line;
			}
			if(*line == ' ')
			{
				++line;
			}

			m_listener->OnRCMessage(severity, line);
		}
	}

private:
	IResourceCompilerListener* m_listener;
};


//////////////////////////////////////////////////////////////////////////
CResourceCompilerHelper::ERcCallResult CResourceCompilerHelper::CallResourceCompiler(
	const char* szFileName, 
	const char* szAdditionalSettings, 
	IResourceCompilerListener* listener, 
	bool bMayShowWindow, 
	CResourceCompilerHelper::ERcExePath rcExePath, 
	bool bSilent,
	bool bNoUserDialog,
	const wchar_t* szWorkingDirectory,
	const wchar_t* szRootPath)
{
	// make command for execution
	SettingsManagerHelpers::CFixedString<wchar_t, MAX_PATH*3> wRemoteCmdLine;

	if (!szAdditionalSettings)
	{
		szAdditionalSettings = "";
	}

	wchar_t szRemoteDirectory[512];
	{
		wchar_t pathBuffer[512];
		switch (rcExePath)
		{
		case eRcExePath_registry:
			GetRootPathUtf16(true, SettingsManagerHelpers::CWCharBuffer(pathBuffer, sizeof(pathBuffer)));
			break;
		case eRcExePath_settingsManager:
			GetRootPathUtf16(false, SettingsManagerHelpers::CWCharBuffer(pathBuffer, sizeof(pathBuffer)));
			break;
		case eRcExePath_currentFolder:
			wcscpy(pathBuffer, L".");
			break;
		case eRcExePath_customPath:
			wcscpy(pathBuffer, szRootPath);
			break;
		default:
			return eRcCallResult_notFound;
		}

		if (!pathBuffer[0])
		{
			wcscpy(pathBuffer, L".");
		}

		swprintf_s(szRemoteDirectory, L"%s/Bin32/rc", pathBuffer);
	}

	if (!szFileName)
	{
		wRemoteCmdLine.appendAscii("\"");
		wRemoteCmdLine.append(szRemoteDirectory);
		wRemoteCmdLine.appendAscii("/rc.exe\" /userdialog=0 ");
		wRemoteCmdLine.appendAscii(szAdditionalSettings);
	}
	else
	{
		wRemoteCmdLine.appendAscii("\"");
		wRemoteCmdLine.append(szRemoteDirectory);
		wRemoteCmdLine.appendAscii("/rc.exe\" \"");
		wRemoteCmdLine.appendAscii(szFileName);
		wRemoteCmdLine.appendAscii("\" ");
		wRemoteCmdLine.appendAscii(bNoUserDialog ? "/userdialog=0 " : "/userdialog=1 ");
		wRemoteCmdLine.appendAscii(szAdditionalSettings);
	}

	// Create a pipe to read the stdout of the RC.
	SECURITY_ATTRIBUTES saAttr;
	::memset(&saAttr, 0, sizeof(saAttr));
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = 0;
	HANDLE hChildStdOutRd, hChildStdOutWr;
	CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &saAttr, 0);
	SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN
	HANDLE hChildStdInRd, hChildStdInWr;
	CreatePipe(&hChildStdInRd, &hChildStdInWr, &saAttr, 0);
	SetHandleInformation(hChildStdInWr, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN

	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwX = 100;
	si.dwY = 100;
	si.hStdError = hChildStdOutWr;
	si.hStdOutput = hChildStdOutWr;
	si.hStdInput = hChildStdInRd;
	si.dwFlags = STARTF_USEPOSITION | STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	bool bShowWindow = false;
	{
		wchar_t buffer[20];
		g_pSettingsManager->GetValueByRef("ShowWindow", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));
		bShowWindow = (wcscmp(buffer, L"true") == 0);
	}

	if (!CreateProcessW(
		NULL,                   // No module name (use command line).
		const_cast<wchar_t*>(wRemoteCmdLine.c_str()), // Command line.
		NULL,                   // Process handle not inheritable.
		NULL,                   // Thread handle not inheritable.
		TRUE,                   // Set handle inheritance to TRUE.
		(bMayShowWindow && bShowWindow) ? 0  : CREATE_NO_WINDOW, // creation flags.
		NULL,                   // Use parent's environment block.
		szWorkingDirectory?szWorkingDirectory:szRemoteDirectory,  // Set starting directory.
		&si,                    // Pointer to STARTUPINFO structure.
		&pi ))                  // Pointer to PROCESS_INFORMATION structure.
	{
		/*
		This code block is commented out instead of being deleted because it's
		good to have at hand for a debugging session.

		const size_t charsInMessageBuffer = 32768;   // msdn about FormatMessage(): "The output buffer cannot be larger than 64K bytes."
		wchar_t szMessageBuffer[charsInMessageBuffer] = L"";   
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, szMessageBuffer, charsInMessageBuffer, NULL);
		GetCurrentDirectoryW(charsInMessageBuffer, szMessageBuffer);
		*/

		if (!bSilent)
		{
			MessageBoxA(0, "ResourceCompiler was not found.\n\nPlease verify CryENGINE RootPath.", "Error", MB_ICONERROR|MB_OK);
		}

		return eRcCallResult_notFound;
	}

	// Close the pipe that writes to the child process, since we don't actually have any input for it.
	CloseHandle(hChildStdInWr);

	// Read all the output from the child process.
	CloseHandle(hChildStdOutWr);
	ResourceCompilerLineHandler lineHandler(listener);
	LineStreamBuffer lineBuffer(&lineHandler, &ResourceCompilerLineHandler::HandleLine);
	for (;;)
	{
		char buffer[2048];
		DWORD bytesRead;
		if (!ReadFile(hChildStdOutRd, buffer, sizeof(buffer), &bytesRead, NULL) || (bytesRead == 0))
		{
			break;
		}
		lineBuffer.HandleText(buffer, bytesRead);
	} 

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	bool ok = true;
	DWORD exitCode = 1;
	{
		if ((GetExitCodeProcess(pi.hProcess, &exitCode) == 0) || (exitCode != 0))
		{
			ok = false;
		}	
	}

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	if (exitCode == eRcExitCode_Crash)
	{
		return eRcCallResult_crash;
	}

	if (lineBuffer.IsTruncated())
	{
		return eRcCallResult_error;
	}

	return ok ? eRcCallResult_success : eRcCallResult_error;
}


//////////////////////////////////////////////////////////////////////////
void* CResourceCompilerHelper::AsyncCallResourceCompiler(
	const char* szFileName, 
	const char* szAdditionalSettings, 
	bool bMayShowWindow, 
	CResourceCompilerHelper::ERcExePath rcExePath, 
	bool bSilent,
	bool bNoUserDialog,
	const wchar_t* szWorkingDirectory,
	const wchar_t* szRootPath)
{
	// make command for execution
	SettingsManagerHelpers::CFixedString<wchar_t, MAX_PATH*3> wRemoteCmdLine;

	if (!szAdditionalSettings)
	{
		szAdditionalSettings = "";
	}

	wchar_t szRemoteDirectory[512];
	{
		wchar_t pathBuffer[512];
		switch (rcExePath)
		{
		case eRcExePath_registry:
			GetRootPathUtf16(true, SettingsManagerHelpers::CWCharBuffer(pathBuffer, sizeof(pathBuffer)));
			break;
		case eRcExePath_settingsManager:
			GetRootPathUtf16(false, SettingsManagerHelpers::CWCharBuffer(pathBuffer, sizeof(pathBuffer)));
			break;
		case eRcExePath_currentFolder:
			wcscpy(pathBuffer, L".");
			break;
		case eRcExePath_customPath:
			wcscpy(pathBuffer, szRootPath);
			break;
		default:
			return NULL;
		}

		if (!pathBuffer[0])
		{
			wcscpy(pathBuffer, L".");
		}

		swprintf_s(szRemoteDirectory, L"%s/Bin32/rc", pathBuffer);
	}

	if (!szFileName)
	{
		wRemoteCmdLine.appendAscii("\"");
		wRemoteCmdLine.append(szRemoteDirectory);
		wRemoteCmdLine.appendAscii("/rc.exe\" /userdialog=0 ");
		wRemoteCmdLine.appendAscii(szAdditionalSettings);
	}
	else
	{
		wRemoteCmdLine.appendAscii("\"");
		wRemoteCmdLine.append(szRemoteDirectory);
		wRemoteCmdLine.appendAscii("/rc.exe\" \"");
		wRemoteCmdLine.appendAscii(szFileName);
		wRemoteCmdLine.appendAscii("\" ");
		wRemoteCmdLine.appendAscii(bNoUserDialog ? "/userdialog=0 " : "/userdialog=1 ");
		wRemoteCmdLine.appendAscii(szAdditionalSettings);
	}

	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwX = 100;
	si.dwY = 100;
	si.dwFlags = STARTF_USEPOSITION;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	bool bShowWindow = false;
	{
		wchar_t buffer[20];
		g_pSettingsManager->GetValueByRef("ShowWindow", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));
		bShowWindow = (wcscmp(buffer, L"true") == 0);
	}

	if (!CreateProcessW(
		NULL,                   // No module name (use command line).
		const_cast<wchar_t*>(wRemoteCmdLine.c_str()), // Command line.
		NULL,                   // Process handle not inheritable.
		NULL,                   // Thread handle not inheritable.
		TRUE,                   // Set handle inheritance to TRUE.
		(bMayShowWindow && bShowWindow) ? 0  : CREATE_NO_WINDOW, // creation flags.
		NULL,                   // Use parent's environment block.
		szWorkingDirectory?szWorkingDirectory:szRemoteDirectory,  // Set starting directory.
		&si,                    // Pointer to STARTUPINFO structure.
		&pi ))                  // Pointer to PROCESS_INFORMATION structure.
	{
		if (!bSilent)
		{
			MessageBoxA(0, "ResourceCompiler was not found.\n\nPlease verify CryENGINE RootPath.", "Error", MB_ICONERROR|MB_OK);
		}

		return NULL;
	}

	return pi.hProcess;
}


//////////////////////////////////////////////////////////////////////////
bool CResourceCompilerHelper::IsPrefer32Bit()
{ 
	bool b = false;

	wchar_t buffer[20];
	g_pSettingsManager->GetValueByRef("EDT_Prefer32Bit", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));
	b = (wcscmp(buffer, L"true") == 0);

	return b;
}


//////////////////////////////////////////////////////////////////////////
bool CResourceCompilerHelper::InvokeResourceCompiler( const char *szSrcFile, const bool bWindow ) const
{
	bool bRet = true;

	// make command for execution

	wchar_t wMasterCDDir[512];
	GetCurrentDirectoryW(512,wMasterCDDir);

	SettingsManagerHelpers::CFixedString<wchar_t, 512> wRemoteCmdLine;
	SettingsManagerHelpers::CFixedString<wchar_t, 512> wDir;

	wRemoteCmdLine.appendAscii("Bin32/rc/");
	wRemoteCmdLine.appendAscii(RC_EXECUTABLE);
	wRemoteCmdLine.appendAscii(" \"");
	wRemoteCmdLine.append(wMasterCDDir);
	wRemoteCmdLine.appendAscii("\\");
	wRemoteCmdLine.appendAscii(szSrcFile);
	wRemoteCmdLine.appendAscii("\" /userdialog=0");

	wDir.append(wMasterCDDir);
	wDir.appendAscii("\\Bin32\\rc");

	STARTUPINFOW si;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.dwX = 100;
	si.dwY = 100;
	si.dwFlags = STARTF_USEPOSITION;

	PROCESS_INFORMATION pi;
	ZeroMemory( &pi, sizeof(pi) );

	if (!CreateProcessW( 
		NULL,     // No module name (use command line). 
		const_cast<wchar_t*>(wRemoteCmdLine.c_str()), // Command line. 
		NULL,     // Process handle not inheritable. 
		NULL,     // Thread handle not inheritable. 
		FALSE,    // Set handle inheritance to FALSE. 
		bWindow?0:CREATE_NO_WINDOW,	// creation flags. 
		NULL,     // Use parent's environment block. 
		wDir.c_str(),  // Set starting directory. 
		&si,      // Pointer to STARTUPINFO structure.
		&pi))     // Pointer to PROCESS_INFORMATION structure.
	{
		bRet = false;
	}

	// Wait until child process exits.
	WaitForSingleObject( pi.hProcess, INFINITE );

	// Close process and thread handles. 
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return bRet;
}


//////////////////////////////////////////////////////////////////////////
void CResourceCompilerHelper::GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer wbuffer)
{
	if (wbuffer.getSizeInElements() <= 0)
	{
		return;
	}

	g_pSettingsManager->GetRootPathUtf16(wbuffer);

	SettingsManagerHelpers::CFixedString<wchar_t, 1024> editorExe;
	editorExe = wbuffer.getPtr();

	if (editorExe.length() <= 0)
	{
		MessageBoxA(NULL, "Can't Find the Editor.\nPlease, setup correct CryENGINE root path in the engine settings dialog", "Error", MB_ICONERROR | MB_OK);
		ResourceCompilerUI(0);
		g_pSettingsManager->GetRootPathUtf16(wbuffer);
		editorExe = wbuffer.getPtr();

		if (editorExe.length() <= 0)
		{
			wbuffer[0] = 0;
			return;
		}
	}

	g_pSettingsManager->GetValueByRef("EDT_Prefer32Bit", wbuffer);
	if (wcscmp(wbuffer.getPtr(), L"true") == 0)
	{
		editorExe.appendAscii("/Bin32/Editor.exe");
	}
	else
	{
		editorExe.appendAscii("/Bin64/Editor.exe");
	}

	const size_t sizeToCopy = (editorExe.length() + 1) * sizeof(wbuffer[0]);
	if (sizeToCopy > wbuffer.getSizeInBytes())
	{
		wbuffer[0] = 0;
	}
	else
	{
		memcpy(wbuffer.getPtr(), editorExe.c_str(), sizeToCopy);
	}
}


//////////////////////////////////////////////////////////////////////////
bool CResourceCompilerHelper::IsError() const
{
	return m_bErrorFlag;
}


//////////////////////////////////////////////////////////////////////////
#if defined(_ICRY_PAK_HDR_)

void CResourceCompilerHelper::ProcessIfNeeded(
	const char* originalFilename, 
	char* processedFilename, 
	size_t processedFilenameSizeInBytes)
{
	char sDestFile[512];
	GetOutputFilename(originalFilename, sDestFile, sizeof(sDestFile));

	for(uint32 dwIndex=0;;++dwIndex)		// check for all input files
	{
		char sSrcFile[512];
		GetInputFilename(originalFilename, dwIndex, sSrcFile, sizeof(sSrcFile));

		if(sSrcFile[0] == 0)
		{
			break;					// last input file
		}

		// compile if there is no destination
		// compare date of destination and source , recompile if needed
		// load dds header, check hash-value of the compile settings in the dds file, recompile if needed (not done yet)

		CDebugAllowFileAccess dafa;
		FILE* pDestFile = gEnv->pCryPak->FOpen(sDestFile,"rb");
		FILE* pSrcFile = gEnv->pCryPak->FOpen(sSrcFile,"rb");
		dafa.End();

		// files from the pak file do not count as date comparison do not seem to work there
		if(pDestFile)
		{
			if(gEnv->pCryPak->IsInPak(pDestFile))
			{
				gEnv->pCryPak->FClose(pDestFile);
				pDestFile=0;
			}
		}

		bool bInvokeResourceCompiler=false;

		// is there no destination file?
		if(pSrcFile && !pDestFile)
			bInvokeResourceCompiler=true;

		// if both files exist, is the source file newer?
		if(pDestFile && pSrcFile)
		{
			bInvokeResourceCompiler=true;

			ICryPak::FileTime timeSrc = gEnv->pCryPak->GetModificationTime(pSrcFile);
			ICryPak::FileTime timeDest = gEnv->pCryPak->GetModificationTime(pDestFile);

			if(timeDest>=timeSrc)
				bInvokeResourceCompiler=false;
		}

		if(pDestFile)
		{
			gEnv->pCryPak->FClose(pDestFile);pDestFile=0;
		}
		if(pSrcFile)
		{
			gEnv->pCryPak->FClose(pSrcFile);pSrcFile=0;
		}

		if(bInvokeResourceCompiler)
		{
			// Adjust filename so that they are global.
			char sFullSrcFilename[MAX_PATH];
			gEnv->pCryPak->AdjustFileName( sSrcFile,sFullSrcFilename,0 );
			// call rc.exe
			if(!InvokeResourceCompiler(sFullSrcFilename,false))		// false=no window
			{
				// rc failed
				m_bErrorFlag=true;
				assert(!pSrcFile);		// internal error
				assert(!pDestFile);
				strncpy_s(processedFilename, processedFilenameSizeInBytes, originalFilename, _TRUNCATE);
				return;
			}
		}

		assert(!pSrcFile);		// internal error
		assert(!pDestFile);
	}

	// load without using RC (e.g. TGA)
	strncpy_s(processedFilename, processedFilenameSizeInBytes, sDestFile, _TRUNCATE);
}


void CResourceCompilerHelper::GetInputFilename(
	const char* filename, 
	const unsigned int index,
	char* inputFilename, 
	size_t inputFilenameSizeInBytes)
{
	if (inputFilename == 0 || inputFilenameSizeInBytes < sizeof(inputFilename[0]))
	{
		return;
	}

	const char *ext = GetExtension(filename);

	if (ext && stricmp(ext, "dds") == 0)
	{
		switch(index)
		{
		case 0:
			ReplaceExtension(filename, "tif", inputFilename, inputFilenameSizeInBytes);
			return;
		case 1:
			ReplaceExtension(filename, "srf", inputFilename, inputFilenameSizeInBytes);
			return;
		default:
			inputFilename[0] = 0;
			return;
		}		
	}

	if (index > 0)
	{
		inputFilename[0] = 0;
		return;
	}

	strncpy_s(inputFilename, inputFilenameSizeInBytes, filename, _TRUNCATE);
}
#endif



//////////////////////////////////////////////////////////////////////////
bool CResourceCompilerHelper::CallEditor(void** pEditorWindow, void* hParent, const char* pWindowName, const char* pFlag)
{
	HWND window = ::FindWindowA(NULL, pWindowName);
	if (window)
	{
		*pEditorWindow = window;
		return true;
	}
	else
	{
		*pEditorWindow = 0;

		wchar_t buffer[512] = { L'\0' };
		GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));

		SettingsManagerHelpers::CFixedString<wchar_t, 256> wFlags;
		SettingsManagerHelpers::ConvertUtf8ToUtf16(pFlag, wFlags.getBuffer());
		wFlags.setLength(wcslen(wFlags.c_str()));

		if (buffer[0] != '\0')		
		{
			INT_PTR hIns = (INT_PTR)ShellExecuteW(NULL, L"open", buffer, wFlags.c_str(), NULL, SW_SHOWNORMAL);
			if(hIns > 32)
			{
				return true;
			}
			else
			{
				MessageBoxA(0,"Editor.exe was not found.\n\nPlease verify CryENGINE root path.","Error",MB_ICONERROR|MB_OK);
				ResourceCompilerUI(hParent);
				GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));
				ShellExecuteW(NULL, L"open", buffer, wFlags.c_str(), NULL, SW_SHOWNORMAL);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
const char* CResourceCompilerHelper::GetCallResultDescription(ERcCallResult result)
{
	switch (result)
	{
	case eRcCallResult_success:
		return "Success.";
	case eRcCallResult_notFound:
		return "ResourceCompiler executable was not found.";
	case eRcCallResult_error:
		return "ResourceCompiler exited with an error.";
	case eRcCallResult_crash:
		return "ResourceCompiler crashed! Please report this. Include source asset and this log in the report.";
	default: 
		return "Unexpected failure in ResultCompilerHelper.";
	}
}

//////////////////////////////////////////////////////////////////////////
CEngineSettingsManager* CResourceCompilerHelper::GetSettingsManager()
{
	return g_pSettingsManager;
}

bool CResourceCompilerHelper::GetInstalledBuildPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path)
{
	return g_pSettingsManager->GetInstalledBuildRootPathUtf16(index, name, path);
}

bool CResourceCompilerHelper::GetInstalledBuildPathAscii(const int index, SettingsManagerHelpers::CCharBuffer name, SettingsManagerHelpers::CCharBuffer path)
{
	wchar_t wName[MAX_PATH];
	wchar_t wPath[MAX_PATH];
	if( GetInstalledBuildPathUtf16(index, SettingsManagerHelpers::CWCharBuffer(wName, sizeof(wName)), SettingsManagerHelpers::CWCharBuffer(wPath, sizeof(wPath))) )
	{
		SettingsManagerHelpers::GetAsciiFilename(wName, name);
		SettingsManagerHelpers::GetAsciiFilename(wPath, path);
		return true;
	}
	return false;
}

#endif //(CRY_ENABLE_RC_HELPER)










































































