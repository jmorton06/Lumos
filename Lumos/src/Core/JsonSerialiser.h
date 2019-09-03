#pragma once
#include "LM.h"
#include <jsonhpp/json.hpp>

namespace Lumos
{
	class JsonSerialiser
	{
	public:
		template <typename T> static void Serialise(const std::string& path, T& obj);
		static void Serialise(const std::string& path, nlohmann::json& obj);

		template <typename T> static T* Deserialise(const std::string& path);

		static nlohmann::json Load(const std::string& path);
	};

	inline void JsonSerialiser::Serialise(const std::string& path, nlohmann::json& obj)
	{
		//check if there is anything serialized
		if (obj.is_null()) {
			throw std::invalid_argument("The passed object cannot be serialized");
		}
		
		//Write to file
		std::ofstream stream;
		stream.open(path, std::fstream::out);
		stream << obj;


	}

	template <typename T>
	void JsonSerialiser::Serialise(const std::string& path, T& obj)
	{
		//Convert the object instance to json data
		nlohmann::json j = obj.Serialise();
		JsonSerialiser::Serialise(path, j);
	}

	inline nlohmann::json JsonSerialiser::Load(const std::string& path)
	{
		std::ifstream stream(path);
		if (!stream.good())
		{
			LUMOS_LOG_CRITICAL("Serializer can't serialize cus can't read file : {0}", path);
			return nullptr;
		}

		nlohmann::json input;
		stream >> input;
		if (input.is_null())
		{
			LUMOS_LOG_CRITICAL("File is either a non-json file or corrupted");
		}

		return input;
	}


	template <typename T>
	T* JsonSerialiser::Deserialise(const std::string& path)
	{
		nlohmann::json input = Load(path);

		//Check if the object is serializing its typeID
		const auto iterator = input.find("typeID");
		if (iterator == input.end())
		{
			LUMOS_LOG_CRITICAL("The object you are serialising is not serialising its typeID!");
		}
		else if (input.is_null())
		{
			LUMOS_LOG_CRITICAL("File is either a non-json file or corrupted");
		}

		//Create instance of the type that is specified in the json file under the "typeID" member
		//auto instance = TypeRegister::createInstance(input["typeID"]);
		//if (instance == nullptr)
		//	return nullptr;
		//Serialisable* deserialisedObject = static_cast<Serialisable*>(instance.get());
		//instance.release();
		////Load json data into the instance
		//deserialisedObject->Deserialise(input);
		////Cast into the desired type
		//T* obj = static_cast<T*>(deserialisedObject);
		//return obj;

		return nullptr;
	}
}