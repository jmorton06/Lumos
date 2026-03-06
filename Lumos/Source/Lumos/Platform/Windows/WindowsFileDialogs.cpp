#include "Precompiled.h"
#include "Core/OS/FileDialogs.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#include <Windows.h>
#include <commdlg.h>
#include <ShlObj.h>
#include <string>
#include <sstream>
#include <vector>

namespace Lumos
{
    // Converts "png,jpg,bmp" to "Image Files\0*.png;*.jpg;*.bmp\0All Files\0*.*\0"
    static std::string BuildFilterString(const std::string& filter)
    {
        if(filter.empty())
        {
            std::string result;
            result += "All Files";
            result += '\0';
            result += "*.*";
            result += '\0';
            result += '\0';
            return result;
        }

        std::istringstream stream(filter);
        std::string ext;
        std::string pattern;
        bool first = true;

        while(std::getline(stream, ext, ','))
        {
            while(!ext.empty() && ext[0] == ' ') ext.erase(0, 1);
            while(!ext.empty() && ext.back() == ' ') ext.pop_back();
            if(ext.empty()) continue;

            if(!first) pattern += ";";
            pattern += "*." + ext;
            first = false;
        }

        std::string result;
        result += "Supported Files";
        result += '\0';
        result += pattern;
        result += '\0';
        result += "All Files";
        result += '\0';
        result += "*.*";
        result += '\0';
        result += '\0';
        return result;
    }

    std::string FileDialogs::OpenFile(const std::string& filter, const std::string& defaultPath)
    {
        OPENFILENAMEA ofn;
        char szFile[MAX_PATH] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;

        std::string filterStr = BuildFilterString(filter);
        ofn.lpstrFilter = filterStr.c_str();
        ofn.nFilterIndex = 1;

        if(!defaultPath.empty())
            ofn.lpstrInitialDir = defaultPath.c_str();

        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if(GetOpenFileNameA(&ofn))
            return std::string(szFile);

        return "";
    }

    std::string FileDialogs::SaveFile(const std::string& filter, const std::string& defaultPath, const std::string& defaultName)
    {
        OPENFILENAMEA ofn;
        char szFile[MAX_PATH] = { 0 };

        if(!defaultName.empty())
        {
            strncpy(szFile, defaultName.c_str(), MAX_PATH - 1);
            szFile[MAX_PATH - 1] = '\0';
        }

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;

        std::string filterStr = BuildFilterString(filter);
        ofn.lpstrFilter = filterStr.c_str();
        ofn.nFilterIndex = 1;

        if(!defaultPath.empty())
            ofn.lpstrInitialDir = defaultPath.c_str();

        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if(GetSaveFileNameA(&ofn))
            return std::string(szFile);

        return "";
    }

    std::string FileDialogs::PickFolder(const std::string& defaultPath)
    {
        BROWSEINFOA bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.hwndOwner = NULL;
        bi.lpszTitle = "Select Folder";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if(pidl)
        {
            char path[MAX_PATH];
            if(SHGetPathFromIDListA(pidl, path))
            {
                CoTaskMemFree(pidl);
                return std::string(path);
            }
            CoTaskMemFree(pidl);
        }
        return "";
    }
}
#endif
