#pragma once

void wrapper_service_install(LPCTSTR name);
void wrapper_service_query(LPCTSTR name);
void wrapper_service_update(LPCTSTR name);
void wrapper_service_disable(LPCTSTR name);
void wrapper_service_enable(LPCTSTR name);
void wrapper_service_delete(LPCTSTR name);
void wrapper_service_start(LPCTSTR name);
void wrapper_service_dacl(LPCTSTR name);
void wrapper_service_stop(LPCTSTR name);
void wrapper_service_run(LPTSTR name);