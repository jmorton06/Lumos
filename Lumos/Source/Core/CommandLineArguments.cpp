#include "Precompiled.h"
#include "CommandLineArguments.h"
#include <iomanip>

namespace Lumos
{

    argument_t::argument_t()
            : m_Enabled{false}, m_LongName{}, m_ArgValue{}, m_ShortName{0} {}

    argument_t::argument_t(std::string _long_name, char _short_name, std::string _argval)
            : m_Enabled{true}, m_LongName{std::move(_long_name)}, m_ArgValue{std::move(_argval)}, m_ShortName{_short_name} {}

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

    std::string GetOptString(const std::vector<option_t>& options)
    {
        std::stringstream optstring{":"}; // Start optstring with a ':' to enable optional arguments
        std::for_each(options.begin(), options.end(),
                [&optstring](const option_t& option)
                {
                    optstring << option.shortOption << (option.needsArgument >= ArgumentRequirement::REQUIRE ? ":" : "");
                });
        return optstring.str();
    }

    std::unique_ptr<option> get_long_options(const std::vector<option_t>& options)
    {
        auto* long_options = new option[options.size()];
        int i = 0;
        std::for_each(options.begin(), options.end(),
                [&long_options, &i](const option_t& o) {
                    long_options[i++] = static_cast<option>(o);
                });
        return std::unique_ptr<option>(long_options);
    }

    void add_help_option(std::vector<option_t>& options)
    {
        options.push_back({"Print this message", "help", 'h', ArgumentRequirement::NONE });
    }

    void CommandLineArguments::PrintHelp(const std::vector<option_t>& options)
    {
        std::for_each(options.begin(),options.end(),
                [](const option_t& o)
        {
                    std::stringstream lft{},rght{};
                    lft << " -" << o.shortOption << ", --" << o.longOption;
                    rght << "| " << o.description;
                    printf("%-32s%-30s\n", lft.str().c_str(), rght.str().c_str());
                });
    }

    const std::map<std::string, argument_t>& CommandLineArguments::GetArguments(std::vector<option_t>& possible_options)
    {
#ifdef LUMOS_PLATFORM_UNIX

        opterr = 0;

        add_help_option(possible_options);
        int option_index                = 0;
        auto long_opts                  = get_long_options(possible_options);
        std::string optstring           = GetOptString(possible_options);
        int c = 0;
        while(c != -1)
        {
            c = getopt_long(m_Argc, m_Argv, optstring.c_str(), long_opts.get(), &option_index);
            auto element = std::find_if(possible_options.begin(), possible_options.end(),
                                     [&c](const option_t &o) { return o.shortOption == c; });
            if (element != possible_options.end())
                m_Arguments[element->longOption] = argument_t(
                        element->longOption,
                        element->shortOption,
                        element->needsArgument >= ArgumentRequirement::REQUIRE ? optarg == NULL ? "" : optarg : "");
        }
#endif
        return m_Arguments;
    }
}
