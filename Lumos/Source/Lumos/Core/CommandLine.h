#pragma once

#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "Utilities/TSingleton.h"

namespace Lumos
{
    // examples:
    // --string="Foo Bar"
    // --string "Foo Bar"
    // --help
    // --help=false
    // --help true

    enum class ArgumentRequirement
    {
        NONE = 0,
        REQUIRE = 1,
        OPTIONAL = 2
    };
#if 0
    struct option_t
    {
        const char* description;
        const char* longOption;
        char shortOption;
        ArgumentRequirement needsArgument;
        
        explicit operator option() const
        {
            return { longOption, (int)needsArgument, "", shortOption};
        }
    };
	
	class argument_t
	{
		public:
		argument_t();
		argument_t(std::string  longName, char shortName, std::string argValue);
		explicit operator bool() const { return m_Enabled; }
		
		std::string AsString() const;
		int AsInteger() const;
		float AsFloat() const;
		
		private:
		bool m_Enabled;
		std::string m_LongName;
		std::string m_ArgValue;
		char m_ShortName;
		void AsCheck() const;
	};
#endif
    class CommandLine : public ThreadSafeSingleton<CommandLine>
    {
        friend class TSingleton<CommandLine>;

    public:
        // These are the possible variables the options may point to. Bool and
        // std::string are handled in a special way, all other values are parsed
        // with a std::stringstream. This std::variant can be easily extended if
        // the stream operator>> is overloaded. If not, you have to add a special
        // case to the parse() method.
        typedef std::variant<int32_t*,
            uint32_t*,
            double*,
            float*,
            bool*,
            std::string*>
            Value;

        // The description is printed as part of the help message.

        CommandLine();
        explicit CommandLine(std::string description);

        // Adds a possible option. A typical call would be like this:
        // bool printHelp = false;
        // cmd.addArgument({"--help", "-h"}, &printHelp, "Print this help message");
        // Then, after parse() has been called, printHelp will be true if the user
        // provided the flag.
        void AddArgument(const std::vector<std::string>& flags,
            const Value& value, const std::string& help);

        // Prints the description given to the constructor and the help
        // for each option.
        void PrintHelp(std::ostream& os = std::cout) const;

        // The command line arguments are traversed from start to end. That means,
        // if an option is set multiple times, the last will be the one which is
        // finally used. This call will throw a std::runtime_error if a value is
        // missing for a given option. Unknown flags will cause a warning on
        // std::cerr.
        void Parse(int argc, char** argv);

#if 0
		const std::map<std::string, argument_t>& GetArguments(std::vector<option_t>& possibleOptions);
#endif

    private:
        struct Argument
        {
            std::vector<std::string> m_Flags;
            Value m_Value;
            std::string m_Help;
        };

        std::string m_Description;
        std::vector<Argument> m_Arguments;

        int m_Argc;
        char** m_Argv;

#if 0
		std::map<std::string, argument_t> m_Arguments_t;
#endif
    };

}