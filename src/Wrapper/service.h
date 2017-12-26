#pragma once


VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv);
VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv);
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint);
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl);
VOID SvcReportEvent(LPTSTR szFunction);
