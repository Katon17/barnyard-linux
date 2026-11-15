#include "../src/Platform/Linux/TTypes_Linux.h"
static_assert(sizeof(DWORD) == 4);
static_assert(sizeof(LONG) == 4);
static_assert(sizeof(ULONG) == 4);
static_assert(sizeof(FILETIME) == 8);
static_assert(sizeof(ULARGE_INTEGER) == 8);
static_assert(sizeof(ULONG_PTR) == sizeof(void*));
int main() { return 0; }
