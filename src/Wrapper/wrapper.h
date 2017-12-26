// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once

typedef struct phaka_error_t {
	long code;
} phaka_error_t;

#define SVCNAME _T("sample1")

HRESULT GetErrorMessage(_Out_ LPTSTR* pszDest, _In_ HRESULT dwMessageId);

HRESULT PhCreateDirectory(
	_In_ LPCTSTR szPathName);

HRESULT PhSetEnvironmentVariables(
	_In_ LPCTSTR pszConfigurationPath);

HRESULT PhPathChangeExtension(
	_Out_ LPTSTR pszDest,
	_In_  size_t cchDest,
	_In_  LPCTSTR pszSrc,
	_In_  LPCTSTR pszExt,
	_In_  size_t cchExt);

HRESULT PhGetLogPathSetting(
	_Out_ LPTSTR pszDest,
	_In_ size_t cchDest,
	_In_ LPCTSTR pszConfigurationPath);

HRESULT PhGetDirectoryName(
	_Out_ LPTSTR pszDest,
	_In_  size_t cchDest,
	_In_  LPCTSTR pszSrc);

HRESULT PhExecuteChildProcess(
	_In_ LPTSTR pszCommandLine,
	_In_ LPCTSTR pszCurrentDirectory,
	_In_ HANDLE hStdOutput);

HRESULT PhGetWorkingDirectory(
	_Out_ LPTSTR pszDest,
	_In_ size_t cchDest,
	_In_ LPCTSTR pszConfigurationPath);

HRESULT GetCommandLineSetting(
	_Out_	     	     	     	     	      LPTSTR pszDest,
	_In_	     	     	     	     	      size_t cchDest,
	_In_	     	     	     	     	      LPCTSTR pszConfigurationPath);

HRESULT DoRun(_In_ LPCTSTR pszConfigurationPath);

HRESULT OpenLogFile(_In_ LPCTSTR log_path, _Out_ HANDLE* handle);

HRESULT GetApplicationName(
	_Out_	     	     	     	     	      LPTSTR pszDest,
	_In_	     	     	     	     	      size_t cchDest,
	_In_opt_	     	     	     	     	      LPCTSTR pszConfigurationPath);

HRESULT DoInstallService(_In_ LPTSTR pszConfigurationPath);
HRESULT DoDeleteService(_In_ LPTSTR pszConfigurationPath);
HRESULT DoQueryService(_In_ LPTSTR pszConfigurationPath);
HRESULT DoDescribeService(_In_ LPTSTR pszConfigurationPath);
HRESULT DoDisableService(_In_ LPTSTR pszConfigurationPath);
HRESULT PrintLogo();
HRESULT DoEnableService(LPTSTR pszConfigurationPath);

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv);
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl);

VOID SvcReportEvent(LPTSTR szFunction);
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv);

extern SERVICE_STATUS          gSvcStatus;
extern SERVICE_STATUS_HANDLE   gSvcStatusHandle;
extern HANDLE                  ghSvcStopEvent;