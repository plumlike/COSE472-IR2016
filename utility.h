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
	모든 디렉토리의 최 하위 경로까지 가서 모든 파일들의 경로를 벡터에 넣는 함수.
	ex) data 폴더 안에 NYT, APW 폴더안에 각각 파일들이 있다면
	get_file_paths(TEXT(".\\data", paths)); 를 호출하면 paths 벡터에
	모든 파일들의 경로가 저장된다.

	cf) TEXT() 함수는 char * 타입의 문자열을 wstring (unicode string) 타입으로
	바꿔주는 함수이다.
*/
namespace utility {
	void get_file_paths(LPCWSTR current, vector<string>& paths);
}