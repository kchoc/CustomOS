// Page tables
uintptr_t* pml4 = (uintptr_t*)0x1000000;
uintptr_t* pdpt = (uintptr_t*)0x1001000;
uintptr_t* pd = (uintptr_t*)0x1002000;
uintptr_t* pt = (uintptr_t*)0x1003000;