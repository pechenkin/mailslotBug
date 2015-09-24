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
    printf("error\n line:%ld\n error:%ld\n", line, err);
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

int startWriter()
{
    DWORD cbWritten;
    HANDLE hMailslotLinter;

    hMailslotLinter = CreateFile(LMS, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hMailslotLinter == INVALID_HANDLE_VALUE)
    {
        dbgError(GetLastError(),__LINE__);
        return 1;
    }

    if (!WriteFile(hMailslotLinter, message, LENMSG, &cbWritten, (LPOVERLAPPED) NULL))
        dbgError(GetLastError(),__LINE__);
    else
        _tprintf(TEXT("writer:%s"), message);

    CloseHandle(hMailslotLinter);

    return 0;
}

int startReader()
{
    HANDLE hMailslotClient;
    LPTSTR  lpszBuffer;
    DWORD cbRead;
    HANDLE hEvent;
    OVERLAPPED stOverlapped  = {0} ;
    DWORD cbTr;

    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent)
    {
        dbgError(GetLastError(),__LINE__);
        return 1;
    };

    stOverlapped.hEvent = hEvent;

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

    dbgSlotInfo(hMailslotClient);
    if (!ReadFile(hMailslotClient, lpszBuffer, LENMSG, &cbRead, &stOverlapped))
    {
        //ERROR_IO_PENDING (997) is not a failure!!
        dbgError(GetLastError(),__LINE__);

        _tprintf(TEXT("starting writer...\n"));
        startWriter();

        if (!GetOverlappedResult( hMailslotClient, &stOverlapped, &cbTr, TRUE))
        {
            dbgError(GetLastError(),__LINE__);
            return 1;
        }
    }

    _tprintf(TEXT("reader:%s"), lpszBuffer);

    free(lpszBuffer);
    CloseHandle(hMailslotClient);
    CloseHandle(hEvent);
    return 0;

}

int main()
{
    startReader();
    return 0;
}
