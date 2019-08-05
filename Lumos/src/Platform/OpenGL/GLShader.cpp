#include "LM.h"
#include "GLShader.h"

#include "Platform/OpenGL/GL.h"
#include "System/VFS.h"

namespace Lumos
{
	namespace Graphics
	{
		bool IGNORE_LINES = false;
		static ShaderType type = ShaderType::UNKNOWN;

		bool GLShader::TryCompile(const String& source, String& error)
		{
			std::map<ShaderType, String>* sources = lmnew std::map<ShaderType, String>();
			GLShader::PreProcess(source, sources);

			GLShaderErrorInfo info;
			if (!GLShader::Compile(sources, info))
			{
				error = info.message[info.shader];
				LUMOS_CORE_ERROR(error);
				return false;
			}
			return true;
		}

		bool GLShader::TryCompileFromFile(const String& filepath, String& error)
		{
			const String source = VFS::Get()->ReadTextFile(filepath + ".glsl");
			return TryCompile(source, error);
		}

		GLShader::GLShader(const String& name, const String& source)
			: m_Name(name), m_Source(source)
		{
			Init();
		}

		GLShader::~GLShader()
		{
			Shutdown();

			for (auto& resource : m_Resources)
			{
				delete resource;
			}

			for (auto& structs : m_Structs)
			{
				delete structs;
			}

			for (auto& shader : m_UniformBuffers)
			{
				for (auto j : shader.second)
				{
					delete j;
				}
			}

			for (auto& shader : m_UserUniformBuffers)
			{
				delete shader.second;
			}
		}

		void GLShader::Init()
		{
			std::map<ShaderType, String>* sources = lmnew std::map<ShaderType, String>();
			PreProcess(m_Source, sources);
			Parse(sources);

			for (auto& source : *sources)
			{
				m_ShaderTypes.push_back(source.first);
			}

			GLShaderErrorInfo error;
			m_Handle = Compile(sources, error);

			if (!m_Handle)
				LUMOS_CORE_ERROR("{0} - {1}", error.message[error.shader], m_Name);

			LUMOS_CORE_ASSERT(m_Handle, "");

			ResolveUniforms();
			ValidateUniforms();

			LUMOS_CORE_WARN("Successfully compiled shader: {0}", m_Name);

			delete sources;
		}

		void GLShader::Shutdown() const
		{
			GLCall(glDeleteProgram(m_Handle));
		}

		void GLShader::PreProcess(const String& source, std::map<ShaderType, String>* sources)
		{
			type = ShaderType::UNKNOWN;
			std::vector<String> lines = GetLines(source);
			ReadShaderFile(lines, sources);
		}

		void GLShader::ReadShaderFile(std::vector<String> lines, std::map<ShaderType, String>* shaders)
		{
			for (u32 i = 0; i < lines.size(); i++)
			{
				String str = String(lines[i]);
				str = StringReplace(str, '\t');

				if (IGNORE_LINES)
				{
					if (StartsWith(str, "#end"))
					{
						IGNORE_LINES = false;
					}
				}
				else if (StartsWith(str, "#shader"))
				{
					if (StringContains(str, "vertex"))
					{
						type = ShaderType::VERTEX;
						std::map<ShaderType, String>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, String>(type, ""));
					}
					else if (StringContains(str, "geometry"))
					{
						type = ShaderType::GEOMETRY;
						std::map<ShaderType, String>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, String>(type, ""));
					}
					else if (StringContains(str, "fragment"))
					{
						type = ShaderType::FRAGMENT;
						std::map<ShaderType, String>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, String>(type, ""));
					}
					else if (StringContains(str, "tess_cont"))
					{
						type = ShaderType::TESSELLATION_CONTROL;
						std::map<ShaderType, String>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, String>(type, ""));
					}
					else if (StringContains(str, "tess_eval"))
					{
						type = ShaderType::TESSELLATION_EVALUATION;
						std::map<ShaderType, String>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, String>(type, ""));
					}
					else if (StringContains(str, "compute"))
					{
						type = ShaderType::COMPUTE;
						std::map<ShaderType, String>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, String>(type, ""));
					}
					else if (StringContains(str, "end"))
					{
						type = ShaderType::UNKNOWN;
					}
				}
				else if (StartsWith(str, "#include"))
				{
					String rem = "#include ";
					String file = String(str);
					if (strstr(file.c_str(), rem.c_str()))
					{
						std::string::size_type j = file.find(rem);
						if (j != std::string::npos)
							file.erase(j, rem.length());
						file = StringReplace(file, '\"');
						LUMOS_CORE_WARN("Including file \'{0}\' into shader.", file);
						VFS::Get()->ReadTextFile(file);
						ReadShaderFile(GetLines(VFS::Get()->ReadTextFile(file)), shaders);
					}
				}
				else if (StartsWith(str, "#if"))
				{
					String rem = "#if ";
					String def = String(str);
					if (strstr(def.c_str(), rem.c_str()))
					{
						std::string::size_type j = def.find(rem);
						if (j != std::string::npos)
							def.erase(j, rem.length());
						def = StringReplace(def, '\"');

						if (def == "0")
						{
							IGNORE_LINES = true;
						}
					}
				}
				else if (type != ShaderType::UNKNOWN)
				{
					shaders->at(type).append(lines[i]);
					shaders->at(type).append("\n");
				}
			}
		}

		u32 GLShader::Compile(std::map<ShaderType, String>* sources, GLShaderErrorInfo& info)
		{
			GLCall(u32 program = glCreateProgram());

			std::vector<GLuint> shaders;

			String glVersion;

#ifndef LUMOS_PLATFORM_MOBILE
			glVersion = "#version 330 core \n";
#else
			glVersion = "#version 300 es \n precision highp float; \n precision highp int; \n";
#endif

			for (auto source : *sources)
			{
				source.second.insert(0, glVersion);
				shaders.push_back(CompileShader(source.first, source.second, program, info));
			}

			for (unsigned int shader : shaders)
				glAttachShader(program, shader);

			GLCall(glLinkProgram(program));

			GLint result;
			GLCall(glGetProgramiv(program, GL_LINK_STATUS, &result));
			if (result == GL_FALSE)
			{
				GLint length;
				GLCall(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
				std::vector<char> error(length);
				GLCall(glGetProgramInfoLog(program, length, &length, error.data()));
				String errorMessage(error.data(), length);
				i32 lineNumber = -1;
				sscanf(error.data(), "%*s %*d:%d", &lineNumber);
				info.shader = 3;
				info.message[info.shader] += "Failed to link shader!\n";
				info.line[info.shader] = 0;
				info.message[info.shader] += errorMessage;

				LUMOS_CORE_ERROR(info.message[info.shader]);
				return 0;
			}

			GLCall(glValidateProgram(program));

			for (int z = 0; z < shaders.size(); z++)
				glDetachShader(program, shaders[z]);

			for (int z = 0; z < shaders.size(); z++)
				glDeleteShader(shaders[z]);

			return program;
		}

		GLenum TypeToGL(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VERTEX:
				return GL_VERTEX_SHADER;
			case ShaderType::FRAGMENT:
				return GL_FRAGMENT_SHADER;
#ifndef LUMOS_PLATFORM_MOBILE
			case ShaderType::GEOMETRY:
				return GL_GEOMETRY_SHADER;
			case ShaderType::TESSELLATION_CONTROL:
				return GL_TESS_CONTROL_SHADER;
			case ShaderType::TESSELLATION_EVALUATION:
				return GL_TESS_EVALUATION_SHADER;
			case ShaderType::COMPUTE:
				return GL_COMPUTE_SHADER;
#endif
			default: LUMOS_CORE_ERROR("Unsupported Shader Type"); return -1;
			}
			return -1;
		}

		String TypeToString(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VERTEX:
				return "GL_VERTEX_SHADER";
			case ShaderType::FRAGMENT:
				return "GL_FRAGMENT_SHADER";
			case ShaderType::GEOMETRY:
				return "GL_GEOMETRY_SHADER";
			case ShaderType::TESSELLATION_CONTROL:
				return "GL_TESS_CONTROL_SHADER";
			case ShaderType::TESSELLATION_EVALUATION:
				return "GL_TESS_EVALUATION_SHADER";
			case ShaderType::COMPUTE:
				return "GL_COMPUTE_SHADER";
			case ShaderType::UNKNOWN:
				return "UNKOWN_SHADER";
			}
			return "N/A";
		}

		GLuint GLShader::CompileShader(ShaderType type, String source, u32 program, GLShaderErrorInfo& info)
		{
			const char* cstr = source.c_str();

			GLCall(GLuint shader = glCreateShader(TypeToGL(type)));
			GLCall(glShaderSource(shader, 1, &cstr, NULL));
			GLCall(glCompileShader(shader));

			GLint result;
			GLCall(glGetShaderiv(shader, GL_COMPILE_STATUS, &result));
			if (result == GL_FALSE)
			{
				GLint length;
				GLCall(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
				std::vector<char> error(length);
				GLCall(glGetShaderInfoLog(shader, length, &length, error.data()));
				String errorMessage(error.data(), length);
				i32 lineNumber;
				sscanf(error.data(), "%*s %*d:%d", &lineNumber);
				info.shader = static_cast<u32>(type);
				info.message[info.shader] += "Failed to compile " + TypeToString(type) + " shader!\n";

				info.line[info.shader] = lineNumber;
				info.message[info.shader] += errorMessage;
				GLCall(glDeleteShader(shader));

				LUMOS_CORE_ERROR(info.message[info.shader]);
				return -1;
			}
			return shader;
		}

		void GLShader::Bind() const
		{
			if (s_CurrentlyBound != this)
			{
				GLCall(glUseProgram(m_Handle));
				s_CurrentlyBound = this;
			}
		}

		void GLShader::Unbind() const
		{
			GLCall(glUseProgram(0));
			s_CurrentlyBound = nullptr;
		}

		void GLShader::Parse(std::map<ShaderType, String>* sources)
		{
			for (auto& source : *sources)
			{
				m_UniformBuffers[source.first].push_back(lmnew GLShaderUniformBufferDeclaration("Global", static_cast<u32>(source.first)));

				const char* token;
				const char* str;

				str = source.second.c_str();
				while ((token = FindToken(str, "struct")))
					ParseUniformStruct(GetBlock(token, &str), source.first);

				str = source.second.c_str();
				while ((token = FindToken(str, "uniform")))
					ParseUniform(GetStatement(token, &str), source.first);
			}
		}

		void GLShader::ParseUniform(const String& statement, ShaderType type)
		{
			std::vector<String> tokens = Tokenize(statement);
			u32 index = 0;

			index++; // "uniform"
			const String typeString = tokens[index++];
			String name = tokens[index++];
			// Strip ; from name if present
			if (const char* s = strstr(name.c_str(), ";"))
				name = String(name.c_str(), s - name.c_str());

			String n(name);
			i32 count = 1;
			const char* namestr = n.c_str();
			if (const char* s = strstr(namestr, "["))
			{
				name = String(namestr, s - namestr);

				const char* end = strstr(namestr, "]");
				String c(s + 1, end - s);
				count = atoi(c.c_str());
			}

			if (IsTypeStringResource(typeString))
			{
				ShaderResourceDeclaration* declaration = lmnew GLShaderResourceDeclaration(GLShaderResourceDeclaration::StringToType(typeString), name, count);
				m_Resources.push_back(declaration);
			}
			else
			{

				if (count > 1)
				{
					for (int countID = 0; countID < count; countID++)
					{
						const GLShaderUniformDeclaration::Type t = GLShaderUniformDeclaration::StringToType(typeString, count);
						GLShaderUniformDeclaration* declaration;

						name += "[" + StringFormat::ToString(countID) + "]";
						bool ISStruct = false;

						if (t == GLShaderUniformDeclaration::Type::NONE)
						{
							ShaderStruct* s = FindStruct(typeString);
							LUMOS_CORE_ASSERT(s, "");
							declaration = lmnew GLShaderUniformDeclaration(s, name, count);

							ISStruct = true;
						}
						else
						{
							declaration = lmnew GLShaderUniformDeclaration(t, name, count);
						}

						if (StartsWith(name, "sys_"))
						{
							static_cast<GLShaderUniformBufferDeclaration*>(m_UniformBuffers[type].front())->PushUniform(declaration);
						}
						else
						{

							if (m_UserUniformBuffers[type] == nullptr)
								m_UserUniformBuffers[type] = lmnew GLShaderUniformBufferDeclaration("", 0);

							if (ISStruct)
							{
								for (u32 id = 0; id < static_cast<u32>(count); id++)
								{
									GLShaderUniformDeclaration*	test = lmnew GLShaderUniformDeclaration(*declaration);
									test->SetName(name + "[" + StringFormat::ToString(id) + "]");
									test->m_Struct = lmnew ShaderStruct(*declaration->m_Struct);
									m_UserUniformBuffers[type]->PushUniform(test);
								}

							}
							else
							{
								m_UserUniformBuffers[type]->PushUniform(declaration);
							}
						}

						if (countID > 9)
							name = name.substr(0, name.size() - 4);
						else
							name = name.substr(0, name.size() - 3);
					}
				}
				else
				{
					const GLShaderUniformDeclaration::Type t = GLShaderUniformDeclaration::StringToType(typeString, count);
					GLShaderUniformDeclaration* declaration;

					if (t == GLShaderUniformDeclaration::Type::NONE)
					{
						//ShaderStruct* s = FindStruct(typeString);
						//LUMOS_CORE_ASSERT(s, "");
						//declaration = lmnew GLShaderUniformDeclaration(s, name, count);
						return;
					}
					else
					{
						declaration = lmnew GLShaderUniformDeclaration(t, name, count);
					}

					if (StartsWith(name, "sys_"))
					{
						static_cast<GLShaderUniformBufferDeclaration*>(m_UniformBuffers[type].front())->PushUniform(declaration);
					}
					else
					{
						if (m_UserUniformBuffers[type] == nullptr)
							m_UserUniformBuffers[type] = new GLShaderUniformBufferDeclaration("", 0);
						m_UserUniformBuffers[type]->PushUniform(declaration);
					}
				}
			}
		}

		void GLShader::ParseUniformStruct(const String& block, ShaderType shaderType)
		{
			std::vector<String> tokens = Tokenize(block);

			u32 index = 0;
			index++; // struct
			String name = tokens[index++];
			ShaderStruct* uniformStruct = lmnew ShaderStruct(name);
			index++; // {
			while (index < tokens.size())
			{
				if (tokens[index] == "}")
					break;

				const String type = tokens[index++];
				name = tokens[index++];

				// Strip ; from name if present
				if (const char* s = strstr(name.c_str(), ";"))
					name = String(name.c_str(), s - name.c_str());

				u32 count = 1;
				const char* namestr = name.c_str();
				if (const char* s = strstr(namestr, "["))
				{
					name = String(namestr, s - namestr);

					const char* end = strstr(namestr, "]");
					String c(s + 1, end - s);
					count = atoi(c.c_str());
				}

				ShaderUniformDeclaration* field = lmnew GLShaderUniformDeclaration(GLShaderUniformDeclaration::StringToType(type, count), name, count);
				uniformStruct->AddField(field);
			}
			m_Structs.push_back(uniformStruct);
		}

		bool GLShader::IsTypeStringResource(const String& type)
		{
			if (type == "sampler2D")		return true;
			if (type == "samplerCube")		return true;
			if (type == "sampler2DShadow")	return true;
			if (type == "sampler2DArrayShadow")	return true;
			return false;
		}

		void GLShader::ResolveUniforms()
		{
			Bind();

			for (auto shader : m_UniformBuffers)
			{
				for (auto j : shader.second)
				{
					GLShaderUniformBufferDeclaration* decl = static_cast<GLShaderUniformBufferDeclaration*>(j);
					const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
					for (u32 k = 0; k < uniforms.size(); k++)
					{
						GLShaderUniformDeclaration* uniform = static_cast<GLShaderUniformDeclaration*>(uniforms[k]);
						if (uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
						{
							const ShaderStruct& s = uniform->GetShaderUniformStruct();
							const auto& fields = s.GetFields();
							if (uniform->GetCount() == 1)
							{
								for (u32 l = 0; l < fields.size(); l++)
								{
									GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[l]);
									field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
								}
							}
							else {
								for (u32 i = 0; i < uniform->GetCount(); i++)
								{
									for (u32 l = 0; l < fields.size(); l++)
									{
										GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[l]);
										field->m_Location = GetUniformLocation(uniform->m_Name + "[" + std::to_string(i) + "]" + "." + field->m_Name);
									}
								}
							}
						}
						else
						{
							uniform->m_Location = GetUniformLocation(uniform->m_Name);
						}
					}
				}
			}


			for (const auto& shader : m_UserUniformBuffers)
			{
				GLShaderUniformBufferDeclaration* decl = shader.second;
				if (decl)
				{
					const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
					for (u32 j = 0; j < uniforms.size(); j++)
					{
						GLShaderUniformDeclaration* uniform = static_cast<GLShaderUniformDeclaration*>(uniforms[j]);
						if (uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
						{
							const ShaderStruct& s = uniform->GetShaderUniformStruct();
							const auto& fields = s.GetFields();

							if (uniform->GetCount() == 1)
							{
								for (u32 k = 0; k < fields.size(); k++)
								{
									GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[k]);
									field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
								}
							}
							else
							{
								for (u32 i = 0; i < uniform->GetCount(); i++)
								{
									for (u32 k = 0; k < fields.size(); k++)
									{
										GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[k]);
										field->m_Location = GetUniformLocation(uniform->m_Name + "[" + std::to_string(i) + "]" + "." + field->m_Name);
									}
								}
							}
						}
						else
						{
							uniform->m_Location = GetUniformLocation(uniform->m_Name);
						}
					}
				}
			}

			u32 sampler = 0;
			for (u32 i = 0; i < m_Resources.size(); i++)
			{
				GLShaderResourceDeclaration* resource = static_cast<GLShaderResourceDeclaration*>(m_Resources[i]);
				u32 location = GetUniformLocation(resource->m_Name);
				if (resource->GetCount() == 1)
				{
					resource->m_Register = sampler;
					SetUniform1i(location, sampler++);
				}
				else if (resource->GetCount() > 1)
				{
					resource->m_Register = 0;
					u32 count = resource->GetCount();
					i32* samplers = lmnew i32[count];
					for (u32 s = 0; s < count; s++)
						samplers[s] = s;
					SetUniform1iv(resource->GetName(), samplers, count);
					delete[] samplers;
				}
			}
			Unbind();
		}

		void GLShader::ValidateUniforms()
		{

		}

		bool GLShader::IsSystemUniform(ShaderUniformDeclaration* uniform)
		{
			return StartsWith(uniform->GetName(), "sys_");
		}

		GLint GLShader::GetUniformLocation(const String& name) const
		{
			GLCall(const GLint result = glGetUniformLocation(m_Handle, name.c_str()));
			//if (result == -1)
			//	LUMOS_CORE_WARN("{0} : could not find uniform {1} in shader!",m_Name,name);

			return result;
		}

		void GLShader::SetUniformStruct(GLShaderUniformDeclaration* uniform, u8* data, i32 offset) const
		{
			const ShaderStruct& s = uniform->GetShaderUniformStruct();
			const auto& fields = s.GetFields();
			for (u32 k = 0; k < fields.size(); k++)
			{
				GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[k]);
				ResolveAndSetUniformField(*field, data, offset, uniform->GetCount());
				offset += field->m_Size;
			}
		}

		ShaderUniformDeclaration* GLShader::FindUniformDeclaration(const String& name, const ShaderUniformBufferDeclaration* buffer)
		{
			const ShaderUniformList& uniforms = buffer->GetUniformDeclarations();
			for (u32 i = 0; i < uniforms.size(); i++)
			{
				if (uniforms[i]->GetName() == name)
					return uniforms[i];
			}
			return nullptr;
		}

		ShaderUniformDeclaration* GLShader::FindUniformDeclaration(const String& name)
		{
			ShaderUniformDeclaration* result = nullptr;

			for (auto shader : m_UniformBuffers)
			{
				for (u32 i = 0; i < shader.second.size(); i++)
				{
					result = FindUniformDeclaration(name, shader.second[i]);
					if (result)
						return result;
				}
			}

			for (const auto& shader : m_UserUniformBuffers)
			{
				result = FindUniformDeclaration(name, shader.second);
				if (result)
					return result;
			}

			return result;
		}

		void GLShader::SetSystemUniformBuffer(ShaderType type, u8* data, u32 size, u32 slot)
		{
			Bind();
			LUMOS_CORE_ASSERT(m_UniformBuffers[type].size() > slot, "");
			if (!m_UniformBuffers[type].empty())
			{
				ShaderUniformBufferDeclaration* declaration = m_UniformBuffers[type][slot];
				if (declaration != nullptr)
					ResolveAndSetUniforms(declaration, data, size);
			}
		}

		void GLShader::SetUserUniformBuffer(ShaderType type, u8* data, u32 size)
		{
			ResolveAndSetUniforms(m_UserUniformBuffers[type], data, size);
		}

		ShaderStruct* GLShader::FindStruct(const String& name)
		{
			for (ShaderStruct* s : m_Structs)
			{
				if (s->GetName() == name)
					return s;
			}
			return nullptr;
		}

		void GLShader::ResolveAndSetUniforms(ShaderUniformBufferDeclaration* buffer, u8* data, u32 size) const
		{
			const ShaderUniformList& uniforms = buffer->GetUniformDeclarations();
			for (u32 i = 0; i < uniforms.size(); i++)
			{
				GLShaderUniformDeclaration* uniform = static_cast<GLShaderUniformDeclaration*>(uniforms[i]);

				ResolveAndSetUniform(uniform, data, size, uniform->GetCount());
			}
		}

		void GLShader::ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, u8* data, u32 size, u32 count) const
		{
			if (uniform->GetLocation() == -1)
			{
				//LUMOS_CORE_ERROR( "Couldnt Find Uniform In Shader: " + uniform->GetName());
				return;
			}

			const u32 offset = uniform->GetOffset();
			switch (uniform->GetType())
			{
			case GLShaderUniformDeclaration::Type::FLOAT32:
				SetUniform1f(uniform->GetLocation(), *reinterpret_cast<float*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::INT32:
				SetUniform1i(uniform->GetLocation(), *reinterpret_cast<i32*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::INT:
				SetUniform1i(uniform->GetLocation(), *reinterpret_cast<int*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::VEC2:
				SetUniform2f(uniform->GetLocation(), *reinterpret_cast<Maths::Vector2*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::VEC3:
				SetUniform3f(uniform->GetLocation(), *reinterpret_cast<Maths::Vector3*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::VEC4:
				SetUniform4f(uniform->GetLocation(), *reinterpret_cast<Maths::Vector4*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::MAT3:
				SetUniformMat3(uniform->GetLocation(), *reinterpret_cast<Maths::Matrix3*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::MAT4:
				SetUniformMat4(uniform->GetLocation(), *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::MAT4ARRAY:
				SetUniformMat4Array(uniform->GetLocation(), count, *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::STRUCT:
				SetUniformStruct(uniform, data, offset);
				break;
			default:
				LUMOS_CORE_ASSERT(false, "Unknown type!");
			}
		}

		void GLShader::SetUniform(const String& name, u8* data)
		{
			ShaderUniformDeclaration* uniform = FindUniformDeclaration(name);
			if (!uniform)
			{
				LUMOS_CORE_ASSERT("Cannot find uniform in {0} shader with name '{1}'", m_Name, name);
				return;
			}
			ResolveAndSetUniform(static_cast<GLShaderUniformDeclaration*>(uniform), data, 0, uniform->GetCount());
		}

		void GLShader::ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, u8* data, i32 offset, u32 count) const
		{
			//LUMOS_CORE_ASSERT(field.GetLocation() < 0, "Couldnt Find Uniform In Shader: " + field.GetName());

			switch (field.GetType())
			{
			case GLShaderUniformDeclaration::Type::FLOAT32:
				SetUniform1f(field.GetLocation(), *reinterpret_cast<float*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::INT32:
				SetUniform1i(field.GetLocation(), *reinterpret_cast<i32*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::VEC2:
				SetUniform2f(field.GetLocation(), *reinterpret_cast<Maths::Vector2*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::VEC3:
				SetUniform3f(field.GetLocation(), *reinterpret_cast<Maths::Vector3*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::VEC4:
				SetUniform4f(field.GetLocation(), *reinterpret_cast<Maths::Vector4*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::MAT3:
				SetUniformMat3(field.GetLocation(), *reinterpret_cast<Maths::Matrix3*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::MAT4:
				SetUniformMat4(field.GetLocation(), *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
				break;
			case GLShaderUniformDeclaration::Type::MAT4ARRAY:
				SetUniformMat4Array(field.GetLocation(), count, *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
				break;
			default:
				LUMOS_CORE_ASSERT(false, "Unknown type!");
			}
		}

		void GLShader::SetUniform1f(const String& name, float value) const
		{
			SetUniform1f(GetUniformLocation(name), value);
		}

		void GLShader::SetUniform1fv(const String& name, float* value, i32 count) const
		{
			SetUniform1fv(GetUniformLocation(name), value, count);
		}

		void GLShader::SetUniform1i(const String& name, i32 value) const
		{
			SetUniform1i(GetUniformLocation(name), value);
		}

		void GLShader::SetUniform1iv(const String& name, i32* value, i32 count) const
		{
			SetUniform1iv(GetUniformLocation(name), value, count);
		}

		void GLShader::SetUniform2f(const String& name, const Maths::Vector2& vector) const
		{
			SetUniform2f(GetUniformLocation(name), vector);
		}

		void GLShader::SetUniform3f(const String& name, const Maths::Vector3& vector) const
		{
			SetUniform3f(GetUniformLocation(name), vector);
		}

		void GLShader::SetUniform4f(const String& name, const Maths::Vector4& vector) const
		{
			SetUniform4f(GetUniformLocation(name), vector);
		}

		void GLShader::SetUniformMat4(const String& name, const Maths::Matrix4& matrix) const
		{
			SetUniformMat4(GetUniformLocation(name), matrix);
		}

		void GLShader::SetUniform1f(u32 location, float value)
		{
			GLCall(glUniform1f(location, value));
		}

		void GLShader::SetUniform1fv(u32 location, float* value, i32 count)
		{
			GLCall(glUniform1fv(location, count, value));
		}

		void GLShader::SetUniform1i(u32 location, i32 value)
		{
			GLCall(glUniform1i(location, value));
		}

		void GLShader::SetUniform1iv(u32 location, i32* value, i32 count)
		{
			GLCall(glUniform1iv(location, count, value));
		}

		void GLShader::SetUniform2f(u32 location, const Maths::Vector2& vector)
		{
			GLCall(glUniform2f(location, vector.GetX(), vector.GetY()));
		}

		void GLShader::SetUniform3f(u32 location, const Maths::Vector3& vector)
		{
			GLCall(glUniform3f(location, vector.GetX(), vector.GetY(), vector.GetZ()));
		}

		void GLShader::SetUniform4f(u32 location, const Maths::Vector4& vector)
		{
			GLCall(glUniform4f(location, vector.GetX(), vector.GetY(), vector.GetZ(), vector.GetW()));
		}

		void GLShader::SetUniformMat3(u32 location, const Maths::Matrix3& matrix)
		{
			GLCall(glUniformMatrix3fv(location, 1, GL_FALSE /*GLTRUE*/, &matrix.values[0]));
		}

		void GLShader::SetUniformMat4(u32 location, const Maths::Matrix4& matrix)
		{
			GLCall(glUniformMatrix4fv(location, 1, GL_FALSE /*GLTRUE*/, &matrix.values[0]));
		}

		void GLShader::SetUniformMat4Array(u32 location, u32 count, const Maths::Matrix4& matrix)
		{
			GLCall(glUniformMatrix4fv(location, count, GL_FALSE /*GLTRUE*/, &matrix.values[0]));
		}
	}
}
