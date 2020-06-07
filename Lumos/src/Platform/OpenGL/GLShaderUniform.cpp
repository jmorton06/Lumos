#include "lmpch.h"
#include "GLShaderUniform.h"
#include "Graphics/API/Shader.h"

namespace Lumos
{
	namespace Graphics
	{
		GLShaderUniformDeclaration::GLShaderUniformDeclaration(Type type, const String& name, u32 count)
			: m_Offset(0), m_Type(type), m_Struct(nullptr), m_Location(0)
		{
			m_Name = name;
			m_Count = count;
			m_Size = SizeOfUniformType(type) * count;
		}

		GLShaderUniformDeclaration::GLShaderUniformDeclaration(ShaderStruct* uniformStruct, const String& name, u32 count)
			: m_Offset(0), m_Type(GLShaderUniformDeclaration::Type::STRUCT), m_Struct(uniformStruct), m_Location(0)
		{
			m_Name = name;
			m_Count = count;
			m_Size = m_Struct->GetSize() * count;
		}

		void GLShaderUniformDeclaration::SetOffset(u32 offset)
		{
			if (m_Type == GLShaderUniformDeclaration::Type::STRUCT)
				m_Struct->SetOffset(offset);

			m_Offset = offset;
		}

		u32 GLShaderUniformDeclaration::SizeOfUniformType(Type type)
		{
			switch (type)
			{
			case GLShaderUniformDeclaration::Type::INT:				return 4;
			case GLShaderUniformDeclaration::Type::INT32:				return 4;
			case GLShaderUniformDeclaration::Type::FLOAT32:			return 4;
            case GLShaderUniformDeclaration::Type::UINT:               return 4;
			case GLShaderUniformDeclaration::Type::VEC2:				return sizeof(Maths::Vector2);// 4 * 2;
			case GLShaderUniformDeclaration::Type::VEC3:				return sizeof(Maths::Vector3);//4 * 3;
			case GLShaderUniformDeclaration::Type::VEC4:				return sizeof(Maths::Vector4);//4 * 4;
			case GLShaderUniformDeclaration::Type::MAT3:				return sizeof(Maths::Matrix3);//4 * 3 * 3;
			case GLShaderUniformDeclaration::Type::MAT4:				return sizeof(Maths::Matrix4);//4 * 4 * 4;
			case GLShaderUniformDeclaration::Type::MAT4ARRAY:			return sizeof(Maths::Matrix4);//4 * 4 * 4;
			default: return 0;
			}
		}

		GLShaderUniformDeclaration::Type GLShaderUniformDeclaration::StringToType(const String& type, u32 count)
		{
			if (type == "i32")	return Type::INT32;
			if (type == "uint")	return Type::UINT;
			if (type == "int")		return Type::INT32;
			if (type == "float")	return Type::FLOAT32;
			if (type == "vec2")		return Type::VEC2;
			if (type == "vec3")		return Type::VEC3;
			if (type == "vec4")		return Type::VEC4;
			if (type == "mat3")		return Type::MAT3;
			if (type == "mat4")		return Type::MAT4;
			if (type == "mat4" && count > 1) return Type::MAT4ARRAY;

			return Type::NONE;
		}

		String GLShaderUniformDeclaration::TypeToString(Type type)
		{
			switch (type)
			{
			case GLShaderUniformDeclaration::Type::INT32:		return "i32";
			case GLShaderUniformDeclaration::Type::FLOAT32:		return "float";
			case GLShaderUniformDeclaration::Type::VEC2:		return "vec2";
			case GLShaderUniformDeclaration::Type::VEC3:		return "vec3";
			case GLShaderUniformDeclaration::Type::VEC4:		return "vec4";
			case GLShaderUniformDeclaration::Type::MAT3:		return "mat3";
			case GLShaderUniformDeclaration::Type::MAT4:		return "mat4";
			case GLShaderUniformDeclaration::Type::INT:			return "int";
            case GLShaderUniformDeclaration::Type::UINT:            return "uint";
			case GLShaderUniformDeclaration::Type::MAT4ARRAY:	return "mat4Array";
			default: return "Invalid Type";
			}
		}

		GLShaderUniformBufferDeclaration::GLShaderUniformBufferDeclaration(const String& name, u32 shaderType)
			: m_Name(name), m_Register(0), m_Size(0), m_ShaderType(shaderType)
		{
		}

		void GLShaderUniformBufferDeclaration::PushUniform(GLShaderUniformDeclaration* uniform)
		{
			u32 offset = 0;
			if (!m_Uniforms.empty())
			{
				GLShaderUniformDeclaration* previous = static_cast<GLShaderUniformDeclaration*>(m_Uniforms.back());
				offset = previous->m_Offset + previous->m_Size;
			}
			uniform->SetOffset(offset);
			m_Size += uniform->GetSize();
			m_Uniforms.push_back(uniform);
		}

		ShaderUniformDeclaration* GLShaderUniformBufferDeclaration::FindUniform(const String& name)
		{
			for (ShaderUniformDeclaration* uniform : m_Uniforms)
			{
				if (uniform->GetName() == name)
					return uniform;
			}
			return nullptr;
		}

		GLShaderUniformBufferDeclaration::~GLShaderUniformBufferDeclaration()
		{
			for (auto& uniform : m_Uniforms)
				delete uniform;
		}
	}
}
