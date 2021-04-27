#include "Precompiled.h"
#include "CommandLine.h"

#include <algorithm>
#include <iomanip>

namespace Lumos
{
#if 0
    argument_t::argument_t()
		: m_Enabled{false}, m_LongName{}, m_ArgValue{}, m_ShortName{0} {}
	
    argument_t::argument_t(std::string longName, char shortName, std::string argVal)
		: m_Enabled{true}, m_LongName{std::move(longName)}, m_ArgValue{std::move(argVal)}, m_ShortName{shortName} {}
	
    void argument_t::AsCheck() const
	{
        if(!m_Enabled) 
			LUMOS_LOG_ERROR("Option {0} is not provided. Type --help or -h for help", m_LongName);
        
        if(m_ArgValue.empty())
			LUMOS_LOG_ERROR("Option {0} does not have an argument. Type --help or -h for help", m_LongName);
    }
	
    std::string argument_t::AsString() const
    {
        AsCheck();
        return m_ArgValue;
    }
	
    int argument_t::AsInteger() const
    {
        AsCheck();
        return stoi(m_ArgValue);
    }
	
	 float argument_t::AsFloat() const
    {
        AsCheck();
        return stof(m_ArgValue);
    }
#endif

    CommandLine::CommandLine()
    {
        m_Description = "Lumos Command Line Parser";
    }

    CommandLine::CommandLine(std::string description)
        : m_Description(std::move(description))
    {
    }

    void CommandLine::AddArgument(const std::vector<std::string>& flags,
        const Value& value, const std::string& help)
    {
        m_Arguments.emplace_back(Argument { flags, value, help });
    }

    void CommandLine::PrintHelp(std::ostream& os) const
    {

        // Print the general description.
        os << m_Description << std::endl;

        // Find the argument with the longest combined flag length (in order
        // to align the help messages).

        uint32_t maxFlagLength = 0;

        for(auto const& argument : m_Arguments)
        {
            uint32_t flagLength = 0;
            for(const auto& flag : argument.m_Flags)
            {
                // Plus comma and space.
                flagLength += static_cast<uint32_t>(flag.size()) + 2;
            }

            maxFlagLength = std::max(maxFlagLength, flagLength);
        }

        // Now print each argument.
        for(const auto& argument : m_Arguments)
        {
            std::string flags;
            for(auto const& flag : argument.m_Flags)
            {
                flags += flag + ", ";
            }

            // Remove last comma and space and add padding according to the
            // longest flags in order to align the help messages.
            std::stringstream sstr;
            sstr << std::left << std::setw(maxFlagLength)
                 << flags.substr(0, flags.size() - 2);

            // Print the help for each argument. This is a bit more involved
            // since we do line wrapping for long descriptions.
            size_t spacePos = 0;
            size_t lineWidth = 0;
            while(spacePos != std::string::npos)
            {
                size_t nextspacePos = argument.m_Help.find_first_of(' ', spacePos + 1);
                sstr << argument.m_Help.substr(spacePos, nextspacePos - spacePos);
                lineWidth += nextspacePos - spacePos;
                spacePos = nextspacePos;

                if(lineWidth > 60)
                {
                    os << sstr.str() << std::endl;
                    sstr = std::stringstream();
                    sstr << std::left << std::setw(maxFlagLength - 1) << " ";
                    lineWidth = 0;
                }
            }
        }
    }

    void CommandLine::Parse(int argc, char** argv)
    {
        m_Argc = argc;
        m_Argv = argv;

        // Skip the first argument (name of the program).
        int i = 1;
        while(i < argc)
        {
            // First we have to identify wether the value is separated by a space
            // or a '='.
            std::string flag(argv[i]);
            std::string value;
            bool valueIsSeparate = false;

            // If there is an '=' in the flag, the part after the '=' is actually
            // the value.
            size_t equalPos = flag.find('=');
            if(equalPos != std::string::npos)
            {
                value = flag.substr(equalPos + 1);
                flag = flag.substr(0, equalPos);
            }
            // Else the following argument is the value.
            else if(i + 1 < argc)
            {
                value = argv[i + 1];
                valueIsSeparate = true;
            }

            // Search for an argument with the provided flag.
            bool foundArgument = false;

            for(auto const& argument : m_Arguments)
            {
                if(std::find(argument.m_Flags.begin(), argument.m_Flags.end(), flag)
                    != std::end(argument.m_Flags))
                {

                    foundArgument = true;

                    // In the case of booleans, there must not be a value present.
                    // So if the value is neither 'true' nor 'false' it is considered
                    // to be the next argument.
                    if(std::holds_alternative<bool*>(argument.m_Value))
                    {
                        if(!value.empty() && value != "true" && value != "false")
                        {
                            valueIsSeparate = false;
                        }
                        *std::get<bool*>(argument.m_Value) = (value != "false");
                    }
                    // In all other cases there must be a value.
                    else if(value.empty())
                    {
                        throw std::runtime_error(
                            "Failed to parse command line arguments: "
                            "Missing value for argument \""
                            + flag + "\"!");
                    }
                    // For a std::string, we take the entire value.
                    else if(std::holds_alternative<std::string*>(argument.m_Value))
                    {
                        *std::get<std::string*>(argument.m_Value) = value;
                    }
                    // In all other cases we use a std::stringstream to
                    // convert the value.
                    else
                    {
                        std::visit(
                            [&value](auto&& arg)
                            {
                                std::stringstream sstr(value);
                                sstr >> *arg;
                            },
                            argument.m_Value);
                    }

                    break;
                }
            }

            // Print a warning if there was an unknown argument.
            if(!foundArgument)
            {
                LUMOS_LOG_ERROR("Ignoring unknown command line argument {0}", flag);

                // Advance to the next flag.
                i++;

                // If the value was separated, we have to advance our index once more.
                if(foundArgument && valueIsSeparate)
                {
                    i++;
                }
            }
        }
    }
}