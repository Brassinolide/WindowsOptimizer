#pragma warning (disable:4996)
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
using namespace std;

BOOL Run_as_System(LPCWSTR run) {
	//��Ȩ��Debug�Ի�ȡ���̾��
	//https://blog.csdn.net/zuishikonghuan/article/details/47746451
	HANDLE hToken;
	LUID Luid;
	TOKEN_PRIVILEGES tp;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Luid);
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = Luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, false, &tp, sizeof(tp), NULL, NULL);
	CloseHandle(hToken);

	//ö�ٽ��̻�ȡlsass.exe��ID��winlogon.exe��ID�����������еĿ���ֱ�Ӵ򿪾����ϵͳ����
	DWORD idL, idW;
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Process32First(hSnapshot, &pe)) {
		do {
			if (0 == wcscmp(pe.szExeFile, L"lsass.exe")) {
				idL = pe.th32ProcessID;
			}
			else if (0 == wcscmp(pe.szExeFile, L"winlogon.exe")) {
				idW = pe.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &pe));
	}
	CloseHandle(hSnapshot);

	//��ȡ���������lsass����winlogon
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, idL);
	if (!hProcess)hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, idW);
	HANDLE hTokenx;
	//��ȡ����
	OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hTokenx);
	//��������
	DuplicateTokenEx(hTokenx, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hToken);
	CloseHandle(hProcess);
	CloseHandle(hTokenx);
	//������Ϣ
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFOW);
	si.lpDesktop = (LPWSTR)L"winsta0\\default";//��ʾ����
	//�������̣�������CreateProcessAsUser���򱨴�1314����Ȩ
	BOOL ret = CreateProcessWithTokenW(hToken, LOGON_NETCREDENTIALS_ONLY, NULL, (LPWSTR)run, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	CloseHandle(hToken);

	return ret;
}

int wmain(int argc, wchar_t** argv) {
	if (argc != 2) {
		printf("usage: sysrun.exe path_to_exe");
		return 1;
	}

	Run_as_System(argv[1]);
}
