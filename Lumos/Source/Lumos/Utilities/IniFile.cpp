#include "Precompiled.h"
#include "IniFile.h"
#include "Core/OS/FileSystem.h"
#include "Core/StringUtilities.h"

#include <filesystem>
#include <fstream>

Lumos::IniFile::IniFile(const std::string& filePath)
    : m_FilePath(filePath)
{
    Load();
}

void Lumos::IniFile::Reload()
{
    RemoveAll();
    Load();
}

bool Lumos::IniFile::Remove(const std::string& key)
{
    if(IsKeyExisting(key))
    {
        m_Data.erase(key);
        return true;
    }

    return false;
}

void Lumos::IniFile::RemoveAll()
{
    m_Data.clear();
}

bool Lumos::IniFile::IsKeyExisting(const std::string& key) const
{
    return m_Data.find(key) != m_Data.end();
}

void Lumos::IniFile::RegisterPair(const std::string& key, const std::string& value)
{
    RegisterPair(std::make_pair(key, value));
}

void Lumos::IniFile::RegisterPair(const std::pair<std::string, std::string>& pair)
{
    m_Data.insert(pair);
}

std::vector<std::string> Lumos::IniFile::GetFormattedContent() const
{
    std::vector<std::string> result;

    for(const auto& [key, value] : m_Data)
        result.push_back(key + "=" + value);

    return result;
}

void Lumos::IniFile::Load()
{
    auto fileString = Lumos::FileSystem::ReadTextFile(m_FilePath);
    auto lines = Lumos::StringUtilities::GetLines(fileString);

    for(auto& line : lines)
    {
        if(IsValidLine(line))
        {
            line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
            RegisterPair(ExtractKeyAndValue(line));
        }
    }
}

void Lumos::IniFile::Rewrite() const
{
    std::stringstream stream;
    for(const auto& [key, value] : m_Data)
        stream << key << "=" << value << std::endl;

    FileSystem::WriteTextFile(m_FilePath, stream.str());
}

std::pair<std::string, std::string> Lumos::IniFile::ExtractKeyAndValue(const std::string& p_line) const
{
    std::string key;
    std::string value;

    std::string* currentBuffer = &key;

    for(auto& c : p_line)
    {
        if(c == '=')
            currentBuffer = &value;
        else
            currentBuffer->push_back(c);
    }

    return std::make_pair(key, value);
}

bool Lumos::IniFile::IsValidLine(const std::string& attributeLine) const
{
    if(attributeLine.size() == 0)
        return false;

    if(attributeLine[0] == '#' || attributeLine[0] == ';' || attributeLine[0] == '[')
        return false;

    if(std::count(attributeLine.begin(), attributeLine.end(), '=') != 1)
        return false;

    return true;
}

bool Lumos::IniFile::StringToBoolean(const std::string& value) const
{
    return (value == "1" || value == "T" || value == "t" || value == "True" || value == "true");
}
