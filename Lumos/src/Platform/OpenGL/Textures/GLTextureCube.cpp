#include "LM.h"
#include "GLTextureCube.h"
#include "Platform/OpenGL/GL.h"
#include "Platform/OpenGL/GLTools.h"
#include "Platform/OpenGL/GLDebug.h"
#include "Utilities/LoadImage.h"

namespace Lumos
{
	namespace Graphics
	{
		GLTextureCube::GLTextureCube(u32 size)
			: m_Size(size), m_Bits(0), m_NumMips(0), m_Format()
		{
			glGenTextures(1, &m_Handle);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);
#ifndef LUMOS_PLATFORM_MOBILE
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP);
#endif
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// set textures
			for (int i = 0; i < 6; ++i)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);


			GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
			m_Width = size;
			m_Height = size;
		}

		GLTextureCube::GLTextureCube(const String& name, const String& filepath) : m_Width(0), m_Height(0), m_Size(0), m_NumMips(0), m_Bits(0),
			m_Format()
		{
			m_Name = name;
			m_Files[0] = filepath;
			m_Handle = LoadFromSingleFile();
		}

		GLTextureCube::GLTextureCube(const String& name, const String* files)
		{
			m_Name = name;
			for (u32 i = 0; i < 6; i++)
				m_Files[i] = files[i];
			m_Handle = LoadFromMultipleFiles();
		}

		GLTextureCube::GLTextureCube(const String& name, const String* files, u32 mips, const InputFormat format)
		{
			m_Name = name;
			m_NumMips = mips;
			for (u32 i = 0; i < mips; i++)
				m_Files[i] = files[i];
			if (format == InputFormat::VERTICAL_CROSS)
				m_Handle = LoadFromVCross(mips);
		}

		GLTextureCube::~GLTextureCube()
		{
			GLCall(glDeleteTextures(m_NumMips, &m_Handle));
		}

		void GLTextureCube::Bind(u32 slot) const
		{
			GLCall(glActiveTexture(GL_TEXTURE0 + slot));
			GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle));
		}

		void GLTextureCube::Unbind(u32 slot) const
		{
			GLCall(glActiveTexture(GL_TEXTURE0 + slot));
			GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
		}

		u32 GLTextureCube::LoadFromSingleFile()
		{
			// TODO: Implement
			return 0;
		}

		u32 GLTextureCube::LoadFromMultipleFiles()
		{
			const String& xpos = m_Files[0];
			const String& xneg = m_Files[1];
			const String& ypos = m_Files[2];
			const String& yneg = m_Files[3];
			const String& zpos = m_Files[4];
			const String& zneg = m_Files[5];

			m_Parameters.format = TextureFormat::RGBA;

			u32 width, height, bits;
			u8* xp = Lumos::LoadImageFromFile(xpos, &width, &height, &bits, true);
			u8* xn = Lumos::LoadImageFromFile(xneg, &width, &height, &bits, true);
			u8* yp = Lumos::LoadImageFromFile(ypos, &width, &height, &bits, true);
			u8* yn = Lumos::LoadImageFromFile(yneg, &width, &height, &bits, true);
			u8* zp = Lumos::LoadImageFromFile(zpos, &width, &height, &bits, true);
			u8* zn = Lumos::LoadImageFromFile(zneg, &width, &height, &bits, true);

			u32 result;
			GLCall(glGenTextures(1, &result));
			GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, result));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			u32 internalFormat = GLTools::TextureFormatToGL(m_Parameters.format);
			u32 format = internalFormat;

			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, xp));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, xn));

			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, yp));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, yn));

			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, zp));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, zn));

			GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

			delete[] xp;
			delete[] xn;
			delete[] yp;
			delete[] yn;
			delete[] zp;
			delete[] zn;

			return result;
		}

		u32 GLTextureCube::LoadFromVCross(u32 mips)
		{
			u32 srcWidth, srcHeight, bits;
			u8*** cubeTextureData = new u8**[mips];
			for (u32 i = 0; i < mips; i++)
				cubeTextureData[i] = new u8*[6];

			u32* faceWidths = new u32[mips];
			u32* faceHeights = new u32[mips];

			for (u32 m = 0; m < mips; m++)
			{
				u8* data = Lumos::LoadImageFromFile(m_Files[m], &srcWidth, &srcHeight, &bits, !m_LoadOptions.flipY);
				m_Parameters.format = GLTools::BitsToTextureFormat(bits);
				u32 stride = bits / 8;

				u32 face = 0;
				u32 faceWidth = srcWidth / 3;
				u32 faceHeight = srcHeight / 4;
				faceWidths[m] = faceWidth;
				faceHeights[m] = faceHeight;
				for (u32 cy = 0; cy < 4; cy++)
				{
					for (u32 cx = 0; cx < 3; cx++)
					{
						if (cy == 0 || cy == 2 || cy == 3)
						{
							if (cx != 1)
								continue;
						}

						cubeTextureData[m][face] = new u8[faceWidth * faceHeight * stride];

						for (u32 y = 0; y < faceHeight; y++)
						{
							u32 offset = y;
							if (face == 5)
								offset = faceHeight - (y + 1);
							u32 yp = cy * faceHeight + offset;
							for (u32 x = 0; x < faceWidth; x++)
							{
								offset = x;
								if (face == 5)
									offset = faceWidth - (x + 1);
								u32 xp = cx * faceWidth + offset;
								cubeTextureData[m][face][(x + y * faceWidth) * stride + 0] = data[(xp + yp * srcWidth) * stride + 0];
								cubeTextureData[m][face][(x + y * faceWidth) * stride + 1] = data[(xp + yp * srcWidth) * stride + 1];
								cubeTextureData[m][face][(x + y * faceWidth) * stride + 2] = data[(xp + yp * srcWidth) * stride + 2];
								if (stride >= 4)
									cubeTextureData[m][face][(x + y * faceWidth) * stride + 3] = data[(xp + yp * srcWidth) * stride + 3];
							}
						}
						face++;
					}
				}
				delete[] data;
			}

			u32 result;
			GLCall(glGenTextures(1, &result));
			GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, result));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
			//GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD,   mips - 1));
			//GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, mips - 1));

			u32 internalFormat = GLTools::TextureFormatToGL(m_Parameters.format);
			u32 format = GLTools::TextureFormatToInternalFormat(internalFormat);
			for (u32 m = 0; m < mips; m++)
			{
				GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][3]));
				GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][1]));

				GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][0]));
				GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][4]));

				GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][2]));
				GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][5]));
			}
			GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

			for (u32 m = 0; m < mips; m++)
			{
				for (u32 f = 0; f < 6; f++)
				{
					delete[] cubeTextureData[m][f];
				}
				delete[] cubeTextureData[m];
			}
			delete[] cubeTextureData;
			delete[] faceHeights;
			delete[] faceWidths;

			return result;
		}
	}
}
