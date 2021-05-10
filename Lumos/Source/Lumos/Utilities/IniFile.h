#pragma once

namespace Lumos
{
    class IniFile
    {
    public:
        IniFile(const std::string& filePath);
        ~IniFile() = default;

        void Reload();
        void Rewrite() const;

        template <typename T>
        T Get(const std::string& key);
        template <typename T>
        T GetOrDefault(const std::string& key, T defaultT);
        template <typename T>
        bool Set(const std::string& key, const T& value);
        template <typename T>
        bool Add(const std::string& key, const T& value);
        template <typename T>
        bool SetOrAdd(const std::string& key, const T& value);

        bool Remove(const std::string& key);
        void RemoveAll();
        bool IsKeyExisting(const std::string& key) const;
        std::vector<std::string> GetFormattedContent() const;

        void RegisterPair(const std::string& key, const std::string& value);
        void RegisterPair(const std::pair<std::string, std::string>& pair);

        void Load();

        std::pair<std::string, std::string> ExtractKeyAndValue(const std::string& attributeLine) const;
        bool IsValidLine(const std::string& attributeLine) const;
        bool StringToBoolean(const std::string& value) const;

    private:
        std::string m_FilePath;
        std::unordered_map<std::string, std::string> m_Data;
    };

    template <typename T>
    inline T IniFile::Get(const std::string& key)
    {
        if constexpr(std::is_same<bool, T>::value)
        {
            if(!IsKeyExisting(key))
                return false;

            return StringToBoolean(m_Data[key]);
        }
        else if constexpr(std::is_same<std::string, T>::value)
        {
            if(!IsKeyExisting(key))
                return std::string("NULL");

            return m_Data[key];
        }
        else if constexpr(std::is_integral<T>::value)
        {
            if(!IsKeyExisting(key))
                return static_cast<T>(0);

            return static_cast<T>(std::atoi(m_Data[key].c_str()));
        }
        else if constexpr(std::is_floating_point<T>::value)
        {
            if(!IsKeyExisting(key))
                return static_cast<T>(0.0f);

            return static_cast<T>(std::atof(m_Data[key].c_str()));
        }
        else
        {
            LUMOS_ASSERT(false, "The given type must be : bool, integral, floating point or string");
            return T();
        }
    }

    template <typename T>
    inline T IniFile::GetOrDefault(const std::string& key, T defaultT)
    {
        return IsKeyExisting(key) ? Get<T>(key) : defaultT;
    }

    template <typename T>
    inline bool IniFile::Set(const std::string& key, const T& value)
    {
        if(IsKeyExisting(key))
        {
            if constexpr(std::is_same<bool, T>::value)
            {
                m_Data[key] = value ? "true" : "false";
            }
            else if constexpr(std::is_same<std::string, T>::value)
            {
                m_Data[key] = value;
            }
            else if constexpr(std::is_integral<T>::value)
            {
                m_Data[key] = std::to_string(value);
            }
            else if constexpr(std::is_floating_point<T>::value)
            {
                m_Data[key] = std::to_string(value);
            }
            else
            {
                LUMOS_ASSERT(false, "Type not supported");
            }

            return true;
        }

        return false;
    }

    template <typename T>
    inline bool IniFile::SetOrAdd(const std::string& key, const T& value)
    {
        if(IsKeyExisting(key))
            return Set(key, value);
        else
            return Add(key, value);
    }

    template <typename T>
    inline bool IniFile::Add(const std::string& key, const T& value)
    {
        if(!IsKeyExisting(key))
        {
            if constexpr(std::is_same<bool, T>::value)
            {
                RegisterPair(key, value ? "true" : "false");
            }
            else if constexpr(std::is_same<std::string, T>::value)
            {
                RegisterPair(key, value);
            }
            else if constexpr(std::is_integral<T>::value)
            {
                RegisterPair(key, std::to_string(value));
            }
            else if constexpr(std::is_floating_point<T>::value)
            {
                RegisterPair(key, std::to_string(value));
            }
            else
            {
                LUMOS_ASSERT(false, "Type not supported");
            }

            return true;
        }

        return false;
    }
}
