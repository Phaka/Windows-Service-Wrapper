#pragma once

VOID SvcInstall(LPCTSTR pszServiceName);
VOID DoQuerySvc(LPCTSTR pszServiceName);
VOID DoUpdateSvcDesc(LPCTSTR pszServiceName);
VOID DoDisableSvc(LPCTSTR pszServiceName);
VOID DoEnableSvc(LPCTSTR pszServiceName);
VOID DoDeleteSvc(LPCTSTR pszServiceName);