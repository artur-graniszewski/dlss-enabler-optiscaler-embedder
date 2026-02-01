#include <windows.h>
#include <psapi.h>

static HMODULE ModuleFromAddress(const void* p) {
    MEMORY_BASIC_INFORMATION mbi{};
    return (VirtualQuery(p, &mbi, sizeof(mbi)) == sizeof(mbi))
        ? static_cast<HMODULE>(mbi.AllocationBase)
        : nullptr;
}

// UWAGA: bez extern "C", jeśli rename w libce był bez extern "C".
BOOL WINAPI optisc_original_DllMain(HINSTANCE, DWORD, LPVOID);

bool OptiScaler_Init(HMODULE /*self*/, const OptiScalerConfig* cfg) {
    MessageBoxW(nullptr, L"Step 0", L"Debug", MB_OK | MB_ICONINFORMATION);

    // 1) absolutnie użyj HMODULE tej „biblioteki” (sekcji), gdzie jest DllMain OptiScalera
    HMODULE hOpti = ModuleFromAddress(reinterpret_cast<const void*>(&optisc_original_DllMain));
    if (!hOpti) {
        MessageBoxW(nullptr, L"[embed] hOpti == NULL", L"Debug", MB_OK | MB_ICONERROR);
        return false;
    }

    // 2) (opcjonalnie) wyłącz ryzykowne feature’y PRZED DllMain – jak wcześniej proponowałem
    //    jeśli masz dostęp do tych setterów, ale to nie jest wymagane do testu.

    // 3) SEH guard + trace: pokaże kod wyjątku jeśli wali wewnątrz DllMain
    DWORD seh = 0;
    BOOL ok = FALSE;
    __try {
        MessageBoxW(nullptr, L"Step 0b", L"Debug", MB_OK | MB_ICONINFORMATION);
        ok = optisc_original_DllMain(hOpti, DLL_PROCESS_ATTACH, nullptr);
    }
    __except (seh = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        wchar_t msg[128];
        swprintf(msg, 128, L"DllMain crashed. SEH=0x%08X", seh);
        MessageBoxW(nullptr, msg, L"Debug", MB_OK | MB_ICONERROR);
        return false;
    }

    return ok != FALSE;
}

void OptiScaler_Shutdown() {
    HMODULE hOpti = ModuleFromAddress(reinterpret_cast<const void*>(&optisc_original_DllMain));
    if (hOpti) optisc_original_DllMain(hOpti, DLL_PROCESS_DETACH, nullptr);
}