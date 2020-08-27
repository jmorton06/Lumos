#pragma once

namespace Lumos
{
	namespace Graphics
	{
		class LUMOS_EXPORT ShaderUniformDeclaration
		{
		private:
			friend class Shader;
			friend class GLShader;
			friend class DXShader;
			friend class ShaderStruct;

		public:
			virtual ~ShaderUniformDeclaration() = default;
			virtual const std::string& GetName() const = 0;
			virtual void SetName(const std::string&) = 0;
			virtual u32 GetSize() const = 0;
			virtual u32 GetCount() const = 0;
			virtual u32 GetOffset() const = 0;

		protected:
			virtual void SetOffset(u32 offset) = 0;

		protected:
			static ShaderUniformDeclaration* (*CreateFunc)();
		};

		typedef std::vector<ShaderUniformDeclaration*> ShaderUniformList;

		class ShaderUniformBufferDeclaration
		{
		public:
			virtual ~ShaderUniformBufferDeclaration() = default;
			virtual const std::string& GetName() const = 0;
			virtual u32 GetRegister() const = 0;
			virtual u32 GetShaderType() const = 0;
			virtual u32 GetSize() const = 0;
			virtual const ShaderUniformList& GetUniformDeclarations() const = 0;

			virtual ShaderUniformDeclaration* FindUniform(const std::string& name) = 0;

		protected:
			static ShaderUniformBufferDeclaration* (*CreateFunc)();
		};

		typedef std::vector<ShaderUniformBufferDeclaration*> ShaderUniformBufferList;

		class ShaderStruct
		{
		private:
			friend class Shader;

		private:
			std::string m_Name;
			std::vector<ShaderUniformDeclaration*> m_Fields;
			u32 m_Size;
			u32 m_Offset;

		public:
			explicit ShaderStruct(const std::string& name)
				: m_Name(name)
				, m_Size(0)
				, m_Offset(0)
			{
			}

			ShaderStruct(const ShaderStruct& s)
				: m_Name(s.GetName())
				, m_Size(s.GetSize())
				, m_Offset(s.GetOffset())
			{
				for(auto field : s.GetFields())
				{
					m_Fields.push_back(field);
				}
			}

			~ShaderStruct()
			{
				for(auto field : m_Fields)
				{
					delete field;
				}
			}

			void AddField(ShaderUniformDeclaration* field)
			{
				m_Size += field->GetSize();
				u32 offset = 0;
				if(m_Fields.size())
				{
					ShaderUniformDeclaration* previous = m_Fields.back();
					offset = previous->GetOffset() + previous->GetSize();
				}
				field->SetOffset(offset);
				m_Fields.push_back(field);
			}

			_FORCE_INLINE_ void SetOffset(u32 offset)
			{
				m_Offset = offset;
			}

			_FORCE_INLINE_ const std::string& GetName() const
			{
				return m_Name;
			}
			_FORCE_INLINE_ u32 GetSize() const
			{
				return m_Size;
			}
			_FORCE_INLINE_ u32 GetOffset() const
			{
				return m_Offset;
			}
			_FORCE_INLINE_ const std::vector<ShaderUniformDeclaration*>& GetFields() const
			{
				return m_Fields;
			}
		};

		typedef std::vector<ShaderStruct*> ShaderStructList;
	}
}
