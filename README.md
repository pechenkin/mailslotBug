# mailslotBug
mailslotBug.c  : ReactOS incorrectly reads data from the slot in synchronous mode (TCORE-10188 , fixed in r69236 release)
# mailslotBug2
mailslotBug2.c : Problem with asynchronous reading from a mailslot (TCORE-10245, fixed in rr69475 release)
# conversionBug.c
conversionBug.c : First parameter in wcstombs() and mbstowcs() can be NULL, in this case functions returns the number of characters that would result from conversion (TCORE-10390 fixed in r69682 release)
