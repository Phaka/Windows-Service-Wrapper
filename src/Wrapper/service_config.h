#pragma once

VOID __stdcall DoQuerySvc(LPCTSTR pszServiceName);
VOID __stdcall DoUpdateSvcDesc(LPCTSTR pszServiceName);
VOID __stdcall DoDisableSvc(LPCTSTR pszServiceName);
VOID __stdcall DoEnableSvc(LPCTSTR pszServiceName);
VOID __stdcall DoDeleteSvc(LPCTSTR pszServiceName);