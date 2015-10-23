#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <windows.h>

#define BUFFER_SIZE 64

void MultiByteToWideCharTest()
{
    char rosStr[BUFFER_SIZE] = "Reactos";
    int cnt = 0;
    
    cnt = MultiByteToWideChar(CP_ACP,0,rosStr,-1,NULL,0);
    printf("MultiByteToWideChar:%d\n",cnt);
}

void WideCharToMultiByteTest()
{
    wchar_t rosStr[BUFFER_SIZE] = L"Reactos";
    int cnt = 0;
    
    cnt = WideCharToMultiByte(CP_ACP,0,rosStr,-1,NULL,0,NULL,NULL);
    printf("WideCharToMultiByte:%d\n",cnt);
}

void mbstowcsTest()
{
    char rosStr[BUFFER_SIZE] = "Reactos";
    int cnt = 0;
    
    cnt = mbstowcs(NULL,rosStr,BUFFER_SIZE);
    printf("mbstowcs:%d\n",cnt);
}

void wcstombsTest()
{
    wchar_t rosStr[BUFFER_SIZE] = L"Reactos";
    int cnt = 0;
    
    cnt = wcstombs(NULL,rosStr,BUFFER_SIZE);
    printf("wcstombs:%d\n",cnt);
}


int main()
{
    MultiByteToWideCharTest();
    WideCharToMultiByteTest(); 
#ifdef TEST1
    mbstowcsTest();
#endif
#ifdef TEST2
    wcstombsTest();
#endif    
    return 0;
}
