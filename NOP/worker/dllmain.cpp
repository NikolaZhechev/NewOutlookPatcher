#include <wrl.h>
#include <wil/com.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Web.Syndication.h>
#include <iostream>
#include <Windows.h>
#include <commctrl.h>
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"
#include "MinHook.h"

#pragma comment(lib, "libMinHook.x64.lib")

void DbgPrintf(LPCWSTR fmt, ...)
{
    va_list marker;
    WCHAR szBuffer[1024];

    va_start(marker, fmt);
    wvsprintf(szBuffer, fmt, marker);
    va_end(marker);

    OutputDebugStringW(szBuffer);
    OutputDebugStringW(L"\r\n");
}

// Example: MinHook signature for CreateCoreWebView2EnvironmentWithOptions
typedef HRESULT(WINAPI* PFN_CreateCoreWebView2EnvironmentWithOptions)(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    ICoreWebView2EnvironmentOptions* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler
    );

// Target function
PFN_CreateCoreWebView2EnvironmentWithOptions pCreateCoreWebView2EnvironmentWithOptions = NULL;
PFN_CreateCoreWebView2EnvironmentWithOptions pCreateCoreWebView2EnvironmentWithOptionsTarget; //original function pointer BEFORE hook do not call this!

#pragma region "Hooks"

HRESULT(*__ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* _this, HRESULT, ICoreWebView2Controller*) = nullptr;
HRESULT STDMETHODCALLTYPE _ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* _this, HRESULT errorCode, ICoreWebView2Controller* createdController) {
    if (createdController != nullptr) {
        winrt::com_ptr<ICoreWebView2> webview;
        winrt::check_hresult(createdController->get_CoreWebView2(webview.put()));

        EventRegistrationToken tkn_NavigationCompleted;
        winrt::check_hresult(webview->add_NavigationCompleted(Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>([](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {

            auto script = L"\
const styleElement = document.createElement('style');\n\
const cssClass = \"\
#OwaContainer, #OwaContainerSlot1, "                                 /* First "email" ad when online */ L"\
.kk1xx._Bfyd.iIsOF.IjQyD, .kk1xx.lHRXq.iIsOF.IjQyD, "                /* First "email" ad when offline */ L"\
.syTot, "                                                            /* Lower left OneDrive subscription banner */ L"\
[id='34318026-c018-414b-abb3-3e32dfb9cc4c'], "                       /* Word button in sidebar */ L"\
[id='c5251a9b-a95d-4595-91ee-a39e6eed3db2'], "                       /* Excel button in sidebar */ L"\
[id='48cb9ead-1c19-4e1f-8ed9-3d60a7e52b18'], "                       /* PowerPoint button in sidebar */ L"\
[id='59391057-d7d7-49fd-a041-d8e4080f05ec'], "                       /* To Do button in sidebar */ L"\
[id='39109bd4-9389-4731-b8d6-7cc1a128d0b3'], "                       /* OneDrive button in sidebar */ L"\
.___1fkhojs.f22iagw.f122n59.f1vx9l62.f1c21dwh.fqerorx.f1i5mqs4, "    /* More apps button in sidebar */ L"\
[id='D64D0004-2A11-442B-9586-F49009D4852B'] { display: none !important; }\";\n\
styleElement.appendChild(document.createTextNode(cssClass));\n\
document.head.appendChild(styleElement);\n\
";
            // .root-192, .splitButtonMenuButton-220 { background-color: transparent !important; color: var(--neutralDark) !important; } " /* Deemphasize New mail button */ L"\

            ::DbgPrintf(script);
            sender->ExecuteScript(script, Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>([&](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
                return S_OK;
                }).Get());

            return S_OK;
            }).Get(), &tkn_NavigationCompleted));

        volatile int dummyF12Enabled = 0;
        const wchar_t* isF12Enabled = L"y_1A36CD25-E20F-4D0D-B1E6-3CC4307E1488";
        if (isF12Enabled[0 + dummyF12Enabled] == L'y') {
            EventRegistrationToken tkn_AcceleratorKeyPressed;
            winrt::check_hresult(createdController->add_AcceleratorKeyPressed(Microsoft::WRL::Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>([](ICoreWebView2Controller* sender, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {

                COREWEBVIEW2_KEY_EVENT_KIND kind;
                winrt::check_hresult(args->get_KeyEventKind(&kind));
                if (kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_UP) {
                    UINT key;
                    winrt::check_hresult(args->get_VirtualKey(&key));
                    if (key == VK_F12) {
                        winrt::check_hresult(args->put_Handled(true));
                        winrt::com_ptr<ICoreWebView2> webview;
                        winrt::check_hresult(sender->get_CoreWebView2(webview.put()));
                        webview->OpenDevToolsWindow();
                    }
                }

                return S_OK;
                }).Get(), &tkn_AcceleratorKeyPressed));
        }

        /*
        HWND parentWindow = nullptr;
        createdController->get_ParentWindow(&parentWindow);
        ::SetLastError(0);
        __WndProc = reinterpret_cast<LRESULT(*)(HWND, UINT, WPARAM, LPARAM)>(::GetWindowLongPtrW(parentWindow, GWLP_WNDPROC));
        if (::GetLastError() == ERROR_SUCCESS && __WndProc) {
            ::SetWindowLongPtrW(parentWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_WndProc));
        }
        */
    }

    ::DbgPrintf(L"Hello from _ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke");
    return __ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke(_this, errorCode, createdController);
}

HRESULT(*__ICoreWebView2Environment_CreateCoreWebView2Controller)(ICoreWebView2Environment*, HWND, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*) = nullptr;
HRESULT STDMETHODCALLTYPE _ICoreWebView2Environment_CreateCoreWebView2Controller(ICoreWebView2Environment* _this, HWND parentWindow, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* controllerCompletedHandler) {  
    void** controllerCompletedHandlerVtbl = *(void***)controllerCompletedHandler;
    if (controllerCompletedHandlerVtbl[3] != _ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke) {
        ::DbgPrintf(L"Patching controllerCompletedHandlerVtbl");
        DWORD oldProtect = 0;
        if (::VirtualProtect(&controllerCompletedHandlerVtbl[3], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            __ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke = reinterpret_cast<HRESULT(*)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*, HRESULT, ICoreWebView2Controller*)>(controllerCompletedHandlerVtbl[3]);
            controllerCompletedHandlerVtbl[3] = _ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke;
            ::VirtualProtect(&controllerCompletedHandlerVtbl[3], sizeof(uintptr_t), oldProtect, &oldProtect);
        }
    }

    ::DbgPrintf(L"Hello from _ICoreWebView2Environment_CreateCoreWebView2Controller");
    return __ICoreWebView2Environment_CreateCoreWebView2Controller(_this, parentWindow, controllerCompletedHandler);
}

HRESULT(*__ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* _this, HRESULT, ICoreWebView2Environment*) = nullptr;
HRESULT STDMETHODCALLTYPE _ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* _this, HRESULT errorCode, ICoreWebView2Environment* createdEnvironment) {
    void** createdEnvironmentVtbl = *(void***)createdEnvironment;
    if (createdEnvironmentVtbl[3] != _ICoreWebView2Environment_CreateCoreWebView2Controller) {
        ::DbgPrintf(L"Patching createdEnvironmentVtbl");
        DWORD oldProtect = 0;
        if (::VirtualProtect(&createdEnvironmentVtbl[3], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            __ICoreWebView2Environment_CreateCoreWebView2Controller = reinterpret_cast<HRESULT(*)(ICoreWebView2Environment*, HWND, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*)>(createdEnvironmentVtbl[3]);
            createdEnvironmentVtbl[3] = _ICoreWebView2Environment_CreateCoreWebView2Controller;
            ::VirtualProtect(&createdEnvironmentVtbl[3], sizeof(uintptr_t), oldProtect, &oldProtect);
        }
    }

    ::DbgPrintf(L"Hello from _ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke");
    return __ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke(_this, errorCode, createdEnvironment);
}

HRESULT(*__CreateCoreWebView2EnvironmentWithOptions)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*) = nullptr;
STDAPI _CreateCoreWebView2EnvironmentWithOptions(PCWSTR browserExecutableFolder, PCWSTR userDataFolder, ICoreWebView2EnvironmentOptions* environmentOptions, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler) {  
    void** environmentCreatedHandlerVtbl = *(void***)environmentCreatedHandler;
    if (environmentCreatedHandlerVtbl[3] != _ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke) {
        ::DbgPrintf(L"Patching environmentCreatedHandlerVtbl");
        DWORD oldProtect = 0;
        if (::VirtualProtect(&environmentCreatedHandlerVtbl[3], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            __ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke = reinterpret_cast<HRESULT(*)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*, HRESULT, ICoreWebView2Environment*)>(environmentCreatedHandlerVtbl[3]);
            environmentCreatedHandlerVtbl[3] = _ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke;
            ::VirtualProtect(&environmentCreatedHandlerVtbl[3], sizeof(uintptr_t), oldProtect, &oldProtect);
        }
    }

    if (!__CreateCoreWebView2EnvironmentWithOptions) {
        auto hMod = ::GetModuleHandleW(L"WebView2Loader.dll");
        winrt::check_bool(hMod);
        __CreateCoreWebView2EnvironmentWithOptions = reinterpret_cast<HRESULT(*)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)>(::GetProcAddress(hMod, "CreateCoreWebView2EnvironmentWithOptions"));
        winrt::check_bool(__CreateCoreWebView2EnvironmentWithOptions);
    }
    ::DbgPrintf(L"Hello from _CreateCoreWebView2EnvironmentWithOptions");
    return pCreateCoreWebView2EnvironmentWithOptions(browserExecutableFolder, userDataFolder, environmentOptions, environmentCreatedHandler);
}
#pragma endregion

#pragma region "AppVerifier infrastructure"
#define DLL_PROCESS_VERIFIER 4

typedef struct _RTL_VERIFIER_THUNK_DESCRIPTOR {
    PCHAR ThunkName;
    PVOID ThunkOldAddress;
    PVOID ThunkNewAddress;
} RTL_VERIFIER_THUNK_DESCRIPTOR, * PRTL_VERIFIER_THUNK_DESCRIPTOR;

typedef struct _RTL_VERIFIER_DLL_DESCRIPTOR {
    PWCHAR DllName;
    ULONG DllFlags;
    PVOID DllAddress;
    PRTL_VERIFIER_THUNK_DESCRIPTOR DllThunks;
} RTL_VERIFIER_DLL_DESCRIPTOR, * PRTL_VERIFIER_DLL_DESCRIPTOR;

typedef void (NTAPI* RTL_VERIFIER_DLL_LOAD_CALLBACK) (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved);
typedef void (NTAPI* RTL_VERIFIER_DLL_UNLOAD_CALLBACK) (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved);
typedef void (NTAPI* RTL_VERIFIER_NTDLLHEAPFREE_CALLBACK) (
    PVOID AllocationBase,
    SIZE_T AllocationSize);

typedef struct _RTL_VERIFIER_PROVIDER_DESCRIPTOR {
    ULONG Length;
    PRTL_VERIFIER_DLL_DESCRIPTOR ProviderDlls;
    RTL_VERIFIER_DLL_LOAD_CALLBACK ProviderDllLoadCallback;
    RTL_VERIFIER_DLL_UNLOAD_CALLBACK ProviderDllUnloadCallback;

    PWSTR VerifierImage;
    ULONG VerifierFlags;
    ULONG VerifierDebug;

    PVOID RtlpGetStackTraceAddress;
    PVOID RtlpDebugPageHeapCreate;
    PVOID RtlpDebugPageHeapDestroy;

    RTL_VERIFIER_NTDLLHEAPFREE_CALLBACK ProviderNtdllHeapFreeCallback;
} RTL_VERIFIER_PROVIDER_DESCRIPTOR;

HMODULE HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

RTL_VERIFIER_DLL_DESCRIPTOR noHooks{};

RTL_VERIFIER_THUNK_DESCRIPTOR kernelbaseHooks[] =
{
    { (PCHAR)"LoadLibraryExW", nullptr, HookLoadLibraryExW},
    { nullptr, nullptr, nullptr },
};

RTL_VERIFIER_DLL_DESCRIPTOR dlls[] =
{
    { (PWCHAR)L"kernelbase.dll", 0, nullptr, kernelbaseHooks },
    { nullptr, 0, nullptr, nullptr },
};

RTL_VERIFIER_PROVIDER_DESCRIPTOR desc =
{
    sizeof(desc),
    dlls,
    [](auto, auto, auto, auto) {},
    [](auto, auto, auto, auto) {},
    nullptr, 0, 0,
    nullptr, nullptr, nullptr,
    [](auto, auto) {},
};

#pragma endregion

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        ::DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_PROCESS_VERIFIER:
        *(PVOID*)lpvReserved = &desc;
        break;
    }
    return true;
}

// Detour function
HRESULT WINAPI DetourCreateCoreWebView2EnvironmentWithOptions(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    ICoreWebView2EnvironmentOptions* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler)
{
    // Modify parameters here, e.g., forcing a custom userDataFolder
    return pCreateCoreWebView2EnvironmentWithOptions(
        browserExecutableFolder,
        L"C:\\Custom\\Data\\Path", // Example: Override
        environmentOptions,
        environmentCreatedHandler
    );
}

HMODULE WINAPI HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    auto result = ::LoadLibraryExW(lpLibFileName, hFile, dwFlags);
    DbgPrintf(L"NOP: LoadedLibraryExW - %ls", lpLibFileName);

    if (lstrcmpW(lpLibFileName, L"C:\\Windows\\system32\\uxtheme.dll") == 0)
    {
        MH_STATUS status = MH_Initialize();
        if (status != MH_OK)
        {
            DbgPrintf(L"NOP: Error initialising MinHook 0x%08X", status);
            return result;
        }

        status = MH_CreateHookApiEx(L"WebView2Loader", "CreateCoreWebView2EnvironmentWithOptions", &_CreateCoreWebView2EnvironmentWithOptions, reinterpret_cast<void**>(&pCreateCoreWebView2EnvironmentWithOptions), reinterpret_cast<void**>(&pCreateCoreWebView2EnvironmentWithOptionsTarget));
        if (status != MH_OK)
        {
            DbgPrintf(L"NOP: Error creating hook 0x%08X", status);
            return result;
        }

        status = MH_EnableHook(MH_ALL_HOOKS);
        if (status != MH_OK)
        {
            DbgPrintf(L"NOP: Error enabling hook 0x%08X", status);
            return result;
        }
    }

    return result;
}