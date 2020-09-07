#pragma once



#include "ShaderUniform.h"
#include "ShaderResource.h"

#define SHADER_VERTEX_INDEX 0
#define SHADER_UV_INDEX 1
#define SHADER_MASK_UV_INDEX 2
#define SHADER_TID_INDEX 3
#define SHADER_MID_INDEX 4
#define SHADER_COLOR_INDEX 5

#define SHADER_UNIFORM_PROJECTION_MATRIX_NAME "sys_ProjectionMatrix"
#define SHADER_UNIFORM_VIEW_MATRIX_NAME "sys_ViewMatrix"
#define SHADER_UNIFORM_MODEL_MATRIX_NAME "sys_ModelMatrix"
#define SHADER_UNIFORM_TEXTURE_MATRIX_NAME "sys_TextureMatrix"

namespace Lumos
{
	namespace Graphics
	{
		enum class ShaderType : int
		{
			VERTEX,
			FRAGMENT,
			GEOMETRY,
			TESSELLATION_CONTROL,
			TESSELLATION_EVALUATION,
			COMPUTE,
			UNKNOWN
		};

		struct ShaderEnumClassHash
		{
			template<typename T>
			std::size_t operator()(T t) const
			{
				return static_cast<std::size_t>(t);
			}
		};

		template<typename Key>
		using HashType = typename std::conditional<std::is_enum<Key>::value, ShaderEnumClassHash, std::hash<Key>>::type;

		template<typename Key, typename T>
		using UnorderedMap = std::unordered_map<Key, T, HashType<Key>>;

		class LUMOS_EXPORT Shader
		{
		public:
			static const Shader* s_CurrentlyBound;

		public:
			virtual void Bind() const = 0;
			virtual void Unbind() const = 0;

			virtual ~Shader() = default;

			virtual const std::vector<ShaderType> GetShaderTypes() const = 0;
			virtual const std::string& GetName() const = 0;
			virtual const std::string& GetFilePath() const = 0;

			virtual void* GetHandle() const = 0;

		public:
			static Shader* CreateFromFile(const std::string& name, const std::string& filepath);
			static bool TryCompile(const std::string& source, std::string& error, const std::string& name);
			static bool TryCompileFromFile(const std::string& filepath, std::string& error);

		protected:
			static Shader* (*CreateFunc)(const std::string&, const std::string&);
		};
	}
}
