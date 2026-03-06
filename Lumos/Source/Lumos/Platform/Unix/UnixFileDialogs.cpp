#include "Core/OS/FileDialogs.h"

#if defined(LUMOS_PLATFORM_LINUX)
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <array>

namespace Lumos
{
    static bool HasCommand(const char* cmd)
    {
        std::string check = std::string("which ") + cmd + " > /dev/null 2>&1";
        return system(check.c_str()) == 0;
    }

    static std::string ExecCommand(const std::string& cmd)
    {
        std::array<char, 4096> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if(!pipe)
            return "";
        while(fgets(buffer.data(), buffer.size(), pipe) != nullptr)
            result += buffer.data();
        int status = pclose(pipe);
        if(status != 0)
            return "";
        // Trim trailing newline
        while(!result.empty() && (result.back() == '\n' || result.back() == '\r'))
            result.pop_back();
        return result;
    }

    // Build zenity --file-filter="Supported|*.png *.jpg" or kdialog filter "*.png *.jpg"
    static std::string BuildZenityFilter(const std::string& filter)
    {
        if(filter.empty())
            return "";

        std::string pattern;
        std::istringstream stream(filter);
        std::string ext;
        bool first = true;
        while(std::getline(stream, ext, ','))
        {
            while(!ext.empty() && ext[0] == ' ') ext.erase(0, 1);
            while(!ext.empty() && ext.back() == ' ') ext.pop_back();
            if(ext.empty()) continue;
            if(!first) pattern += " ";
            pattern += "*." + ext;
            first = false;
        }

        return " --file-filter='Files|" + pattern + "' --file-filter='All Files|*'";
    }

    static std::string BuildKDialogFilter(const std::string& filter)
    {
        if(filter.empty())
            return "";

        std::string pattern;
        std::istringstream stream(filter);
        std::string ext;
        bool first = true;
        while(std::getline(stream, ext, ','))
        {
            while(!ext.empty() && ext[0] == ' ') ext.erase(0, 1);
            while(!ext.empty() && ext.back() == ' ') ext.pop_back();
            if(ext.empty()) continue;
            if(!first) pattern += " ";
            pattern += "*." + ext;
            first = false;
        }

        return " '" + pattern + "'";
    }

    std::string FileDialogs::OpenFile(const std::string& filter, const std::string& defaultPath)
    {
        if(HasCommand("zenity"))
        {
            std::string cmd = "zenity --file-selection --title='Open File'";
            cmd += BuildZenityFilter(filter);
            if(!defaultPath.empty())
                cmd += " --filename='" + defaultPath + "/'";
            return ExecCommand(cmd);
        }

        if(HasCommand("kdialog"))
        {
            std::string cmd = "kdialog --getopenfilename";
            if(!defaultPath.empty())
                cmd += " '" + defaultPath + "'";
            else
                cmd += " .";
            cmd += BuildKDialogFilter(filter);
            return ExecCommand(cmd);
        }

        return "";
    }

    std::string FileDialogs::SaveFile(const std::string& filter, const std::string& defaultPath, const std::string& defaultName)
    {
        if(HasCommand("zenity"))
        {
            std::string cmd = "zenity --file-selection --save --confirm-overwrite --title='Save File'";
            cmd += BuildZenityFilter(filter);
            if(!defaultPath.empty() || !defaultName.empty())
            {
                std::string filename = defaultPath;
                if(!filename.empty() && filename.back() != '/')
                    filename += "/";
                filename += defaultName;
                cmd += " --filename='" + filename + "'";
            }
            return ExecCommand(cmd);
        }

        if(HasCommand("kdialog"))
        {
            std::string cmd = "kdialog --getsavefilename";
            std::string filename = defaultPath;
            if(!filename.empty() && filename.back() != '/')
                filename += "/";
            filename += defaultName;
            if(!filename.empty())
                cmd += " '" + filename + "'";
            else
                cmd += " .";
            cmd += BuildKDialogFilter(filter);
            return ExecCommand(cmd);
        }

        return "";
    }

    std::string FileDialogs::PickFolder(const std::string& defaultPath)
    {
        if(HasCommand("zenity"))
        {
            std::string cmd = "zenity --file-selection --directory --title='Select Folder'";
            if(!defaultPath.empty())
                cmd += " --filename='" + defaultPath + "/'";
            return ExecCommand(cmd);
        }

        if(HasCommand("kdialog"))
        {
            std::string cmd = "kdialog --getexistingdirectory";
            if(!defaultPath.empty())
                cmd += " '" + defaultPath + "'";
            else
                cmd += " .";
            return ExecCommand(cmd);
        }

        return "";
    }
}
#endif
