#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <tchar.h>

#define LVER TEXT("5")
#define LMS TEXT("\\\\.\\mailslot\\LinterMailslot"LVER)

static const LPTSTR message = TEXT("message\n");
#define LENMSG ((DWORD) (lstrlen(message)+1)*sizeof(TCHAR))

#define MSGCNT 5

static inline void dbgError(DWORD err, DWORD line)
{        
    printf("error\n line:%ld\n error%ld\n", line, err);
}

static inline DWORD dbgSlotInfo(HANDLE hMailslot)
{
   DWORD lpMaxMessageSize, lpNextSize, lpMessageCount, lpReadTimeout;
   if (!(GetMailslotInfo(hMailslot,
                        &lpMaxMessageSize,
                        &lpNextSize,
                        &lpMessageCount,
                        &lpReadTimeout)))
       dbgError(GetLastError(),__LINE__);
    else
       printf("slot status: NextSize=%ld MessageCount=%ld\n",lpNextSize,lpMessageCount);
    return lpMessageCount;
}

DWORD WINAPI sltWriter( LPVOID lpParam )
{
    DWORD i;
    DWORD cbWritten;
    HANDLE hMailslotLinter;
    
    hMailslotLinter = CreateFile(LMS, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   
    if (hMailslotLinter == INVALID_HANDLE_VALUE)
    {
        dbgError(GetLastError(),__LINE__);
        return 1;
    }
    
    for(i=0; i<MSGCNT; i++)
    {
        Sleep(1000);
        if (!WriteFile(hMailslotLinter, message, LENMSG, &cbWritten, (LPOVERLAPPED) NULL))
            dbgError(GetLastError(),__LINE__);
        else
            _tprintf(TEXT("writer:%s"), message);
    }
    
    CloseHandle(hMailslotLinter); 

    return 0;         
}

DWORD WINAPI sltReader( LPVOID lpParam )
{
    DWORD i=0;
    HANDLE hMailslotClient;
    LPTSTR  lpszBuffer;
    DWORD cbRead;
    
    hMailslotClient = CreateMailslot(LMS, 0L, MAILSLOT_WAIT_FOREVER, (LPSECURITY_ATTRIBUTES) NULL);
    if (hMailslotClient == INVALID_HANDLE_VALUE)
    {
        dbgError(GetLastError(),__LINE__);
        return 1;
    }
    
    if (!(lpszBuffer=(LPTSTR) malloc(LENMSG)))
    {
        dbgError(GetLastError(),__LINE__);
        return 1;
    }
    while (i<MSGCNT)
    {
        dbgSlotInfo(hMailslotClient);
        if (!ReadFile(hMailslotClient, lpszBuffer, LENMSG, &cbRead, NULL))
            dbgError(GetLastError(),__LINE__);
        else
        {
            _tprintf(TEXT("reader:%s"), lpszBuffer);
            i++;
        }
    }
    
    free(lpszBuffer);
    CloseHandle(hMailslotClient); 
    return 0;
    
}

int main()
{
    HANDLE  hThreadArray[2]; 
    hThreadArray[0] = CreateThread(NULL,0, sltReader, NULL, 0, NULL);
    if (hThreadArray[0]==INVALID_HANDLE_VALUE)
    {
        dbgError(GetLastError(),__LINE__);
        return 1;
    };
    hThreadArray[1] = CreateThread(NULL,0, sltWriter, NULL, 0, NULL);
    if (hThreadArray[1]==INVALID_HANDLE_VALUE)
    {
        dbgError(GetLastError(),__LINE__);
        return 1;
    };
    WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
    CloseHandle(hThreadArray[0]);
    CloseHandle(hThreadArray[1]);
    return 0;
}
