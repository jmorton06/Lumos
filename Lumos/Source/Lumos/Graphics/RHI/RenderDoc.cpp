#include "Precompiled.h"
#include "RenderDoc.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#include <windows.h>
#endif
#include <renderdoc/app/renderdoc_app.h>
// Based on https://github.com/PanosK92/SpartanEngine/blob/master/runtime/RHI/RHI_RenderDoc.cpp

namespace Lumos
{
    namespace Graphics
    {
        static RENDERDOC_API_1_5_0* rdc_api = nullptr;
        static void* rdc_module             = nullptr;

#ifdef LUMOS_PLATFORM_WINDOWS
        static std::vector<std::wstring> GetInstalledRenderDocDLLPaths()
        {
            std::vector<std::wstring> paths;

            // query registry for all the render doc paths
            static const wchar_t* pszInstallerFolders = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders");

            HKEY hkey;
            LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszInstallerFolders, 0, KEY_READ, &hkey);
            if(status != ERROR_SUCCESS) // ensure installer folders key is successfully opened
                return paths;

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 8192
            TCHAR achClass[MAX_PATH] = TEXT(""); // buffer for class name
            DWORD cchClassName       = MAX_PATH; // size of class string
            DWORD cSubKeys           = 0;        // number of subkeys
            DWORD cbMaxSubKey;                   // longest subkey size
            DWORD cchMaxClass;                   // longest class string
            DWORD cValues;                       // number of values for keyPath
            DWORD cchMaxValue;                   // longest value name
            DWORD cbMaxValueData;                // longest value data
            DWORD cbSecurityDescriptor;          // size of security descriptor
            FILETIME ftLastWriteTime;            // last write time

            wchar_t cbEnumValue[MAX_VALUE_NAME] = TEXT("");

            DWORD i, retCode;

            TCHAR achValue[MAX_VALUE_NAME];
            DWORD cchValue = MAX_VALUE_NAME;

            // Get the class name and the value count.
            retCode = RegQueryInfoKey(
                hkey,                  // keyPath handle
                achClass,              // buffer for class name
                &cchClassName,         // size of class string
                nullptr,               // reserved
                &cSubKeys,             // number of subkeys
                &cbMaxSubKey,          // longest subkey size
                &cchMaxClass,          // longest class string
                &cValues,              // number of values for this keyPath
                &cchMaxValue,          // longest value name
                &cbMaxValueData,       // longest value data
                &cbSecurityDescriptor, // security descriptor
                &ftLastWriteTime);     // last write time

            if(cValues)
            {
                for(i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
                {
                    cchValue    = MAX_VALUE_NAME;
                    achValue[0] = '\0';
                    DWORD type  = REG_SZ;
                    DWORD size;
                    memset(cbEnumValue, '\0', MAX_VALUE_NAME);

                    // MSDN:  https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regenumvaluea
                    // If the data has the REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ type, the string may not have been stored with
                    // the proper null-terminating characters. Therefore, even if the function returns ERROR_SUCCESS, the application
                    // should ensure that the string is properly terminated before using it; otherwise, it may overwrite a buffer.
                    retCode = RegEnumValue(hkey, i,
                                           achValue,
                                           &cchValue,
                                           nullptr,
                                           &type,
                                           nullptr,
                                           &size);

                    if(type != REG_SZ || retCode != ERROR_SUCCESS)
                        continue;

                    retCode = RegQueryInfoKey(
                        hkey,                  // keyPath handle
                        achClass,              // buffer for class name
                        &cchClassName,         // size of class string
                        nullptr,               // reserved
                        &cSubKeys,             // number of subkeys
                        &cbMaxSubKey,          // longest subkey size
                        &cchMaxClass,          // longest class string
                        &cValues,              // number of values for this keyPath
                        &cchMaxValue,          // longest value name
                        &cbMaxValueData,       // longest value data
                        &cbSecurityDescriptor, // security descriptor
                        &ftLastWriteTime);     // last write time

                    std::wstring path(achValue);
                    if(path.find(L"RenderDoc") != std::wstring::npos)
                    {
                        // many paths qualify:
                        //
                        // "C:\\Program Files\\RenderDoc\\plugins\\amd\\counters\\"
                        // "C:\\Program Files\\RenderDoc\\"
                        // "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\RenderDoc\\"
                        //
                        // Only consider the ones the contain the dll we want
                        const std::wstring rdc_dll_path = path += TEXT("renderdoc.dll");
                        WIN32_FIND_DATA find_file_data  = { 0 };
                        HANDLE file_handle              = FindFirstFile(rdc_dll_path.c_str(), &find_file_data);
                        if(file_handle != INVALID_HANDLE_VALUE)
                        {
                            paths.push_back(path);
                        }
                    }
                }
            }

            RegCloseKey(hkey);

            return paths;
        }
#endif

        void RenderDoc::OnPreDeviceCreation()
        {
            // Load RenderDoc module and get a pointer to it's API
            if(rdc_api == nullptr)
            {
                pRENDERDOC_GetAPI rdc_get_api = nullptr;
#ifdef LUMOS_PLATFORM_WINDOWS
                // If RenderDoc is already injected into the engine, use the existing module
                rdc_module = ::GetModuleHandleA("renderdoc.dll");

                // If RenderDoc is not injected, load the module now
                if(rdc_module == nullptr)
                {
                    std::vector<std::wstring> RDocDllPaths = GetInstalledRenderDocDLLPaths();
                    LUMOS_ASSERT(!RDocDllPaths.empty(), "Could not find any install locations for renderdoc.dll");
                    std::wstring module_path = RDocDllPaths[0]; // assuming x64 is reported first
                    rdc_module               = ::LoadLibraryW(module_path.c_str());
                }

                LUMOS_ASSERT(rdc_module != nullptr, "Failed to get RenderDoc module");

                // Get the address of RENDERDOC_GetAPI
                rdc_get_api = (pRENDERDOC_GetAPI)::GetProcAddress(static_cast<HMODULE>(rdc_module), "RENDERDOC_GetAPI");
#else
                UNIMPLEMENTED;
#endif
                LUMOS_ASSERT(rdc_get_api != nullptr, "Failed to RENDERDOC_GetAPI function address from renderdoc.dll");

                int result = rdc_get_api(eRENDERDOC_API_Version_1_5_0, (void**)&rdc_api);
                LUMOS_ASSERT(result != 0, "Failed to get RenderDoc API pointer");
            }

            LUMOS_ASSERT(rdc_api != nullptr, "RenderDoc API has not been initialised");

            // Disable muting of validation/debug layer messages
            rdc_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

            // Disable overlay
            rdc_api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
        }

        void RenderDoc::Shutdown()
        {
            if(rdc_module != nullptr)
            {
#ifdef LUMOS_PLATFORM_WINDOWS
                ::FreeLibrary(static_cast<HMODULE>(rdc_module));
#else
                UNIMPLEMENTED;
#endif
            }
        }

        void RenderDoc::FrameCapture()
        {
            // Ignore the call if RenderDoc is not initialised/disabled
            if(rdc_api == nullptr)
                return;

            // Trigger
            rdc_api->TriggerCapture();

            // If the RenderDoc UI is already running, make sure it's visible.
            if(rdc_api->IsTargetControlConnected())
            {
                LUMOS_LOG_INFO("Bringing RenderDoc to foreground...");
                rdc_api->ShowReplayUI();
                return;
            }

            // If the RenderDoc UI is not running, launch it and connect.
            LUMOS_LOG_INFO("Launching RenderDoc...");
            if(rdc_api->LaunchReplayUI(true, "") == 0)
            {
                LUMOS_LOG_ERROR("Failed to launch RenderDoc");
            }
        }
    }
}
