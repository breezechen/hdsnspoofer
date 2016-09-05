#include "stdafx.h"

#include <windows.h>
#include "funcs.h"

const TCHAR srvName[] = TEXT("snspoofer");
const TCHAR srvFilePath[] = TEXT("system32\\drivers\\snspoofer.sys");
const DWORD srvStartType = 1;


char Hex(TCHAR wch)
{
	if (wch <= '9' && wch >= '0') {
		return wch - '0';
	}

	if (wch <= 'F' && wch >= 'A') {
		return wch - 'A' + 0xA;
	}

	if (wch <= 'f' && wch >= 'a') {
		return wch - 'a' + 0xa;
	}
	return 0;
}

VOID ToHexStr(const UCHAR* bytes, ULONG len, TCHAR* buf)
{
	ULONG i = 0;
	UCHAR m, n;
	for (i = 0; i < len; ++i) {
		m = bytes[i] / 0x10;
		n = bytes[i] % 0x10;
		buf[i * 2] = m > 9 ? ('A' + m - 0xa) : ('0' + m);
		buf[i * 2 + 1] = n > 9 ? ('A' + n - 0xa) : ('0' + n);
	}
}

bool GetSNInfo(HDSNInfo& origin, HDSNInfo& changeTo)
{
	HKEY key;
	LONG rst = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\services\\snspoofer")
		, 0, KEY_WOW64_64KEY | KEY_READ, &key);
	if (ERROR_SUCCESS != rst) {
		return false;
	}

	bool ret = false;
	do 
	{
		TCHAR str[4];
		str[0] = 's';
		str[1] = 'n';
		str[2] = '0';
		str[3] = 0;

		TCHAR buf[82];
		DWORD cb = 82 * sizeof(TCHAR);
		DWORD type;
		int i = 0;
		for (; i < HD_MAX_COUNT; i++)
		{
			str[2] = '0' + i;
			rst = RegQueryValueEx(key, str, 0, &type, (LPBYTE)buf, &cb);
			if (rst != ERROR_SUCCESS) {
				break;
			}
			for (int j = 0; j < SN_MAX_LEN; j++)
			{
				origin.sn[i][j] = Hex(buf[j * 2]) * 0x10 + Hex(buf[j * 2 + 1]);
				changeTo.sn[i][j] = Hex(buf[SN_MAX_LEN*2 + 1 + j * 2]) * 0x10 + Hex(buf[SN_MAX_LEN * 2 + 1 + j * 2 + 1]);
			}
			origin.sn[i][SN_MAX_LEN] = 0;
			changeTo.sn[i][SN_MAX_LEN] = 0;
		}
		origin.count = changeTo.count = i;
		ret = true;
	} while (0);

	if (key) {
		RegCloseKey(key);
	}
	return ret;
}


bool SpoofHDSN(const HDSNInfo& originSN, const HDSNInfo& newSN)
{
	HKEY key;
	LONG rst = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\services\\snspoofer")
		, 0, KEY_WOW64_64KEY | KEY_WRITE, &key);
	if (ERROR_SUCCESS != rst) {
		return false;
	}

	bool ret = false;
	do
	{
		TCHAR str[4];
		str[0] = 's';
		str[1] = 'n';
		str[2] = '0';
		str[3] = 0;

		TCHAR buf[82 + 1];
		TCHAR* ptr;
		DWORD cb = 82 * sizeof(TCHAR);
		int i = 0;
		for (; i < newSN.count; i++)
		{
			str[2] = '0' + i;
			ptr = buf;
			ToHexStr(originSN.sn[i], SN_MAX_LEN, ptr);
			ptr += (SN_MAX_LEN * 2);
			*ptr++ = '|';
			ToHexStr(newSN.sn[i], SN_MAX_LEN, ptr);
			ptr += (SN_MAX_LEN * 2);
			*ptr++ = 0;

			rst = RegSetValueEx(key, str, 0, REG_SZ, (const BYTE*)buf, cb);
			if (rst != ERROR_SUCCESS) {
				break;
			}
		}
		if (i == newSN.count) {
			ret = true;
		}
	} while (0);

	if (key) {
		RegCloseKey(key);
	}
	return ret;
}

bool InstallService()
{
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	bool ret = false;

	do 
	{
		hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
		if (NULL == hSCManager) {
			break;;
		}

		hService = CreateService(hSCManager, srvName, srvName,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			srvStartType,
			SERVICE_ERROR_NORMAL,
			srvFilePath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);
		if (hService == NULL) {
			hService = OpenService(hSCManager, srvName, SERVICE_ALL_ACCESS);
			if (hService == NULL) {
				break;
			}
		}

		if (StartService(hService, 0, NULL) == NULL) {
			break;
		}


		SERVICE_STATUS_PROCESS ssStatus;
		DWORD dwBytesNeeded;

		if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus,
			sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
			break;
		}

		DWORD dwStartTickCount = GetTickCount();
		DWORD dwOldCheckPoint = ssStatus.dwCheckPoint;
		DWORD dwWaitTime;
		while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
		{
			dwWaitTime = ssStatus.dwWaitHint / 10;
			if (dwWaitTime < 1000)
				dwWaitTime = 1000;
			else if (dwWaitTime > 10000)
				dwWaitTime = 10000;

			Sleep(dwWaitTime);
			if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus,
				sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
				break;
			}

			if (ssStatus.dwCheckPoint > dwOldCheckPoint)
			{
				dwStartTickCount = GetTickCount();
				dwOldCheckPoint = ssStatus.dwCheckPoint;
			}
			else
			{
				if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
				{
					break;
				}
			}
		}

		if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
			ret = true;
		}
		
	} while (0);

	if (!ret) {
		UninstallService();
	}
	if (hService) {
		CloseServiceHandle(hService);
	}
	if (hSCManager) {
		CloseServiceHandle(hSCManager);
	}

	return ret;
}

bool UninstallService()
{
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	bool ret = false;

	do
	{
		hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
		if (NULL == hSCManager) {
			break;;
		}

		hService = OpenService(hSCManager, srvName, SERVICE_ALL_ACCESS);
		if (hService == NULL) {
			break;
		}

		SERVICE_STATUS ss;
		if (ControlService(hService, SERVICE_CONTROL_STOP, &ss) == NULL) {
			break;
		}

		DeleteService(hService);



		ret = true;

	} while (0);

	if (hService) {
		CloseServiceHandle(hService);
	}
	if (hSCManager) {
		CloseServiceHandle(hSCManager);
	}

	return ret;
}

bool IsServiceInstalled()
{
	bool ret = false;
	SC_HANDLE schSCManager = OpenSCManager(
		NULL,                   
		NULL,                    
		SC_MANAGER_ALL_ACCESS);  
	if (NULL == schSCManager) {	
		return ret;
	}
	SC_HANDLE schService = OpenService(
		schSCManager,         
		srvName,         
		SERVICE_ALL_ACCESS); 
	if (schService != NULL) {
		SERVICE_STATUS_PROCESS ssStatus;
		DWORD dwBytesNeeded;
		if (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus,
			sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
			if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING) {
				ret = true;
			}
		}
	} 

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return ret;
}

bool GenRandomSN(int count, HDSNInfo& info)
{
	const char chs[] = " 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	srand((unsigned int)time(0));
	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < SN_MAX_LEN; j++)
		{
			info.sn[i][j] = chs[rand() % (sizeof(chs)-1)];
		}
		info.sn[i][SN_MAX_LEN] = 0;
	}
	info.count = count;
	return true;
}
