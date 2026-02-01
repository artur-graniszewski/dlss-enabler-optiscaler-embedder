#define DLSSENABLER_NO_REDIRECT_VERSION 1
#include "optiscaler_bypass.h"
#include <windows.h>
#include <string>
#include <filesystem>
#include <cwctype>

static bool iequals(std::wstring_view a, std::wstring_view b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (towlower(a[i]) != towlower(b[i])) return false;
    return true;
}

static bool ShouldAlias(std::wstring_view name)
{
    static constexpr std::wstring_view kList[] = {
        L"dbghelp.dll", L"dxgi.dll", L"version.dll", L"winmm.dll",
        L"d3d11.dll", L"d3d12.dll", L"dlss-enabler.asi"
    };
    for (auto s : kList) if (iequals(name, s)) return true;
    return false;
}

using PFN_GetModuleFileNameW = DWORD(WINAPI*)(HMODULE, LPWSTR, DWORD);

static PFN_GetModuleFileNameW ResolveOriginal_GetModuleFileNameW()
{
    static PFN_GetModuleFileNameW p = []() -> PFN_GetModuleFileNameW {
        HMODULE h = GetModuleHandleW(L"kernel32.dll");
        auto f = reinterpret_cast<PFN_GetModuleFileNameW>(
            GetProcAddress(h, "GetModuleFileNameW"));
        if (!f) {
            h = GetModuleHandleW(L"KernelBase.dll");
            f = reinterpret_cast<PFN_GetModuleFileNameW>(
                GetProcAddress(h, "GetModuleFileNameW"));
        }
        return f;
        }();
    return p;
}

DWORD WINAPI MyGetModuleFileNameW(LPVOID hModule, LPWSTR lpFilename, DWORD nSize)
{
    auto orig = ResolveOriginal_GetModuleFileNameW();
    if (!orig) {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }

    DWORD ret = orig((HMODULE)hModule, lpFilename, nSize);
    if (ret == 0 || ret >= nSize) return ret;

    std::filesystem::path original(lpFilename);
    const std::wstring fname = original.filename().wstring();

    auto iequals = [](std::wstring_view a, std::wstring_view b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (towlower(a[i]) != towlower(b[i])) return false;
        return true;
        };

    auto should_alias = [&](std::wstring_view n) {
        static constexpr std::wstring_view k[] = {
            L"dbghelp.dll", L"dxgi.dll", L"version.dll", L"winmm.dll",
            L"d3d11.dll",   L"d3d12.dll", L"dlss-enabler.asi"
        };
        for (auto s : k) if (iequals(n, s)) return true;
        return false;
        };

    if (should_alias(fname) && !iequals(fname, L"dlss-enabler-upscaler.dll"))
    {
        const std::wstring aliased = (original.parent_path() / L"dlss-enabler-upscaler.dll").wstring();

        if (nSize == 0) { SetLastError(ERROR_INSUFFICIENT_BUFFER); return 0; }

        const size_t need = aliased.size(); // without NUL
        if (need + 1 > nSize) {
            wcsncpy_s(lpFilename, nSize, aliased.c_str(), nSize - 1);
            lpFilename[nSize - 1] = L'\0';
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return nSize; // GetModuleFileNameW semantics for overflow
        }

        wcsncpy_s(lpFilename, nSize, aliased.c_str(), _TRUNCATE);
        return static_cast<DWORD>(aliased.size());
    }

    return ret;
}
BOOL WINAPI MyGetFileVersionInfoW(LPCWSTR lpszFileName, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{

    // MessageBox(NULL, L"Step 1: ok", L"Debug", MB_OK | MB_ICONINFORMATION);
    typedef DWORD(WINAPI* LPFN_GetFileVersionInfoSizeW)(LPCWSTR, LPDWORD);
    typedef BOOL(WINAPI* LPFN_GetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID);
    typedef BOOL(WINAPI* LPFN_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

    // Construct the full path to version.dll in the system32 directory
    wchar_t systemDirectory[MAX_PATH];
    UINT size = GetSystemDirectory(systemDirectory, MAX_PATH);
    return false;
    std::wstring versionDllPath = std::wstring(systemDirectory) + L"\\version.dll";

    // Load version.dll from the system32 directory
    HMODULE hVersionDll = LoadLibraryW(versionDllPath.c_str());
    if (hVersionDll == NULL)
    {
        // std::wcerr << L"Failed to load version.dll" << std::endl;
        return false;
    }

    // Get the addresses of the functions
    LPFN_GetFileVersionInfoSizeW pGetFileVersionInfoSize =
        (LPFN_GetFileVersionInfoSizeW)::GetProcAddress(hVersionDll, "GetFileVersionInfoSizeW");
    LPFN_GetFileVersionInfoW pGetFileVersionInfo =
        (LPFN_GetFileVersionInfoW)::GetProcAddress(hVersionDll, "GetFileVersionInfoW");
    LPFN_VerQueryValueW pVerQueryValue = (LPFN_VerQueryValueW)::GetProcAddress(hVersionDll, "VerQueryValueW");

    if (!pGetFileVersionInfoSize || !pGetFileVersionInfo || !pVerQueryValue)
    {
        // std::wcerr << L"Failed to get function addresses from version.dll" << std::endl;
        FreeLibrary(hVersionDll);
        return false;
    }

    //  Get the size of the version information
    DWORD verHandle = 0;
    DWORD result = pGetFileVersionInfo(lpszFileName, dwHandle, dwLen, lpData);
    FreeLibrary(hVersionDll);

    return result;
}
DWORD WINAPI MyGetFileVersionInfoSizeW(LPCWSTR lpszFileName, LPDWORD lpdwHandle)
{

    //MessageBox(NULL, L"Step 1: ok", L"Debug", MB_OK | MB_ICONINFORMATION);
    typedef DWORD(WINAPI* LPFN_GetFileVersionInfoSizeW)(LPCWSTR, LPDWORD);
    typedef BOOL(WINAPI* LPFN_GetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID);
    typedef BOOL(WINAPI* LPFN_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

    // Construct the full path to version.dll in the system32 directory
    wchar_t systemDirectory[MAX_PATH];
    UINT size = GetSystemDirectory(systemDirectory, MAX_PATH);

    std::wstring versionDllPath = std::wstring(systemDirectory) + L"\\version.dll";

    // Load version.dll from the system32 directory
    HMODULE hVersionDll = LoadLibraryW(versionDllPath.c_str());
    if (hVersionDll == NULL)
    {
        // std::wcerr << L"Failed to load version.dll" << std::endl;
        return false;
    }

    // Get the addresses of the functions
    LPFN_GetFileVersionInfoSizeW pGetFileVersionInfoSize =
        (LPFN_GetFileVersionInfoSizeW)::GetProcAddress(hVersionDll, "GetFileVersionInfoSizeW");
    LPFN_GetFileVersionInfoW pGetFileVersionInfo =
        (LPFN_GetFileVersionInfoW)::GetProcAddress(hVersionDll, "GetFileVersionInfoW");
    LPFN_VerQueryValueW pVerQueryValue = (LPFN_VerQueryValueW)::GetProcAddress(hVersionDll, "VerQueryValueW");

    if (!pGetFileVersionInfoSize || !pGetFileVersionInfo || !pVerQueryValue)
    {
        // std::wcerr << L"Failed to get function addresses from version.dll" << std::endl;
        FreeLibrary(hVersionDll);
        return false;
    }

    // Get the size of the version information
    DWORD verHandle = 0;
    DWORD verSize = pGetFileVersionInfoSize(lpszFileName, lpdwHandle);
    FreeLibrary(hVersionDll);

    //MessageBox(NULL, L"Step 4: ok", L"Debug", MB_OK | MB_ICONINFORMATION);
    return verSize;

}

BOOL WINAPI MyVerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen)
{

    // MessageBox(NULL, L"Step 1: ok", L"Debug", MB_OK | MB_ICONINFORMATION);
    typedef DWORD(WINAPI* LPFN_GetFileVersionInfoSizeW)(LPCWSTR, LPDWORD);
    typedef BOOL(WINAPI* LPFN_GetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID);
    typedef BOOL(WINAPI* LPFN_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

    // Construct the full path to version.dll in the system32 directory
    wchar_t systemDirectory[MAX_PATH];
    UINT size = GetSystemDirectory(systemDirectory, MAX_PATH);
    return false;
    std::wstring versionDllPath = std::wstring(systemDirectory) + L"\\version.dll";

    // Load version.dll from the system32 directory
    HMODULE hVersionDll = LoadLibraryW(versionDllPath.c_str());
    // MessageBox(NULL, L"Step 2: ok", L"Debug", MB_OK | MB_ICONINFORMATION);
    if (hVersionDll == NULL)
    {
        // std::wcerr << L"Failed to load version.dll" << std::endl;
        return false;
    }

    // Get the addresses of the functions
    LPFN_GetFileVersionInfoSizeW pGetFileVersionInfoSize =
        (LPFN_GetFileVersionInfoSizeW)::GetProcAddress(hVersionDll, "GetFileVersionInfoSizeW");
    LPFN_GetFileVersionInfoW pGetFileVersionInfo =
        (LPFN_GetFileVersionInfoW)::GetProcAddress(hVersionDll, "GetFileVersionInfoW");
    LPFN_VerQueryValueW pVerQueryValue = (LPFN_VerQueryValueW)::GetProcAddress(hVersionDll, "VerQueryValueW");

    if (!pGetFileVersionInfoSize || !pGetFileVersionInfo || !pVerQueryValue)
    {
        // std::wcerr << L"Failed to get function addresses from version.dll" << std::endl;
        FreeLibrary(hVersionDll);
        return false;
    }

    //  Get the size of the version information
    DWORD verHandle = 0;
    DWORD verSize = pVerQueryValue(pBlock, lpSubBlock, lplpBuffer, puLen);
    FreeLibrary(hVersionDll);

    return verSize;
}
