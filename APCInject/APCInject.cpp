// APCInject.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <Windows.h>  
#include <TlHelp32.h>  
#include<iostream>
using namespace std;

#define MAX_PATH 1024
typedef long(__fastcall *pfnRtlAdjustPrivilege64)(ULONG, ULONG, ULONG, PVOID);
pfnRtlAdjustPrivilege64 RtlAdjustPrivilege;
// ���ڴ洢ע��ģ��DLL��·��ȫ��
char* DllFullPath;

int InjectDllWithApc(char* DllFullPath, ULONG pid)
{
#ifdef _WIN64   // x64 OpenProcess��Ȩ����
	RtlAdjustPrivilege = (pfnRtlAdjustPrivilege64)GetProcAddress((HMODULE)(GetModuleHandle(L"ntdll.dll")), "RtlAdjustPrivilege");
	if (RtlAdjustPrivilege == NULL)
	{
		return FALSE;
	}
	BOOLEAN dwRetVal = 0;
	RtlAdjustPrivilege(20, 1, 0, &dwRetVal);  //����Ȩ��
#endif

	HANDLE hProcess, hThread, hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32 = { 0 };

	HMODULE hDll = GetModuleHandle(L"Kernel32.dll");
	int len = strlen(DllFullPath) + 1;
	//��Ŀ����̣���Ŀ�����д��DLL		
	hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, TRUE, pid);
	if (hProcess == NULL)
	{
		printf("failed to open process!!\n");
		return 0;
	}
	//��Ŀ����������ڴ�  
	PVOID pszLibFileRemote = (char *)VirtualAllocEx(hProcess, NULL, lstrlen((LPCWSTR)DllFullPath) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (pszLibFileRemote != NULL)
	{
		//��DLLд����Ŀ���������Ŀռ�  
		if (WriteProcessMemory(hProcess, pszLibFileRemote, (void *)DllFullPath, lstrlen((LPCWSTR)DllFullPath) + 1, NULL))
		{
			HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
			THREADENTRY32 te32;
			//�����߳̿���
			hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			if (hThreadSnap == INVALID_HANDLE_VALUE)
				return 1;
			te32.dwSize = sizeof(THREADENTRY32);
			if (!Thread32First(hThreadSnap, &te32))
			{
				CloseHandle(hThreadSnap);
				return 1;
			}
			do
			{
				//����Ŀ����̵��߳�  
				if (te32.th32OwnerProcessID == pid)
				{
					printf("TID:%d\n", te32.th32ThreadID);

					hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, te32.th32ThreadID);
					if (hThread != 0)
					{//Ŀ���̲߳���APC  
						if (QueueUserAPC((PAPCFUNC)LoadLibraryA, hThread, (DWORD)pszLibFileRemote))
						{
							printf("����APC�ɹ�\n");
						}
						else
						{
							printf("����APCʧ��\n");
							return 1;
						}
						CloseHandle(hThread);
					}
				}
			} while (Thread32Next(hThreadSnap, &te32));
			CloseHandle(hThreadSnap);
		}
	}
	CloseHandle(hProcess);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	ULONG32  ulProcessID = 0;
	printf("Input ProcessID\r\n");
	cin >> ulProcessID;    		//������Ҫע��Ľ���ID
#ifdef  _WIN64		
	DllFullPath = "E:\\Dll64.dll";
#else				
	DllFullPath = "E:\\Dll.dll";
#endif
	InjectDllWithApc(DllFullPath, ulProcessID);
	return 0;
}
