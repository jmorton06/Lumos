#pragma once
#include "Utilities/TSingleton.h"

#include <utility>
#include <vector>
#include <map>
#include <sstream>

#ifdef LUMOS_PLATFORM_UNIX
#include <getopt.h>
#endif


namespace Lumos
{

    enum class ArgumentRequirement
    {
        NONE = 0,
        REQUIRE = 1,
        OPTIONAL = 2
    };

    struct option_t
    {
        const char* description;
        const char* longOption;
        char shortOption;
        ArgumentRequirement needsArgument;
        
        explicit operator option() const
        {
            return { longOption, (int)needsArgument, nullptr, shortOption};
        }
    };

    class argument_t
    {
    public:
        argument_t();
        argument_t(std::string  _long_name, char _short_name, std::string  _argval);
        explicit operator bool() const { return m_Enabled; }
        [[nodiscard]] std::string AsString() const;
        [[nodiscard]] int AsInteger() const;
        
    private:
        bool m_Enabled;
        std::string m_LongName;
        std::string m_ArgValue;
        char m_ShortName;
        void AsCheck() const;
    };


    class CommandLineArguments : public ThreadSafeSingleton<CommandLineArguments>
    {
        friend class TSingleton<CommandLineArguments>;
    public:
        CommandLineArguments() = default;
        ~CommandLineArguments() = default;
        
        void Init(int argc, char** argv)
        {
            m_Argc = argc;
            m_Argv = argv;
        }
        
        const std::map<std::string, argument_t>& GetArguments(std::vector<option_t>& possibleOptions);
        void PrintHelp(const std::vector<option_t>& options);

    private:
        std::map<std::string, argument_t> m_Arguments;
        int m_Argc;
        char** m_Argv;
    };
}

