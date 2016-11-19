#pragma once
#include <Windows.h>
#include <fstream>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <vector>
#include <string>
#pragma comment(lib, "User32.lib")

using namespace std;
/*
	��� ���丮�� �� ���� ��α��� ���� ��� ���ϵ��� ��θ� ���Ϳ� �ִ� �Լ�.
	ex) data ���� �ȿ� NYT, APW �����ȿ� ���� ���ϵ��� �ִٸ�
	get_file_paths(TEXT(".\\data", paths)); �� ȣ���ϸ� paths ���Ϳ�
	��� ���ϵ��� ��ΰ� ����ȴ�.

	cf) TEXT() �Լ��� char * Ÿ���� ���ڿ��� wstring (unicode string) Ÿ������
	�ٲ��ִ� �Լ��̴�.
*/
namespace utility {
	void get_file_paths(LPCWSTR current, vector<string>& paths);
}