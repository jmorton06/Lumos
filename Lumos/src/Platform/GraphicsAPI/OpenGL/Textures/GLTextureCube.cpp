#include "LM.h"
#include "GLTextureCube.h"
#include "Platform/GraphicsAPI/OpenGL/GL.h"
#include "Utilities/LoadImage.h"
#include "GLTexture2D.h"

namespace Lumos
{

	GLTextureCube::GLTextureCube(uint size)
		: m_Size(size),m_Bits(0), m_NumMips(0), m_Format()
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
		for (uint i = 0; i < 6; i++)
			m_Files[i] = files[i];
		m_Handle = LoadFromMultipleFiles();
	}

	GLTextureCube::GLTextureCube(const String& name, const String* files, uint mips, const InputFormat format)
	{
		m_Name = name;
		m_NumMips = mips;
		for (uint i = 0; i < mips; i++)
			m_Files[i] = files[i];
		if (format == InputFormat::VERTICAL_CROSS)
			m_Handle = LoadFromVCross(mips);
	}

	GLTextureCube::~GLTextureCube()
	{
		GLCall(glDeleteTextures(m_NumMips, &m_Handle));
	}

	void GLTextureCube::Bind(uint slot) const
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle));
	}

	void GLTextureCube::Unbind(uint slot) const
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	}

	uint GLTextureCube::LoadFromSingleFile()
	{
		// TODO: Implement
		return 0;
	}

	uint GLTextureCube::LoadFromMultipleFiles()
	{
		const String& xpos = m_Files[0];
		const String& xneg = m_Files[1];
		const String& ypos = m_Files[2];
		const String& yneg = m_Files[3];
		const String& zpos = m_Files[4];
		const String& zneg = m_Files[5];

		m_Parameters.format = TextureFormat::RGBA;

		uint width, height, bits;
		byte* xp = Lumos::LoadImageFromFile(xpos, &width, &height, &bits, true);
		byte* xn = Lumos::LoadImageFromFile(xneg, &width, &height, &bits, true);
		byte* yp = Lumos::LoadImageFromFile(ypos, &width, &height, &bits, true);
		byte* yn = Lumos::LoadImageFromFile(yneg, &width, &height, &bits, true);
		byte* zp = Lumos::LoadImageFromFile(zpos, &width, &height, &bits, true);
		byte* zn = Lumos::LoadImageFromFile(zneg, &width, &height, &bits, true);

		uint result;
		GLCall(glGenTextures(1, &result));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, result));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

		uint internalFormat = GLTexture2D::TextureFormatToGL(m_Parameters.format);
		uint format = internalFormat;

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

	uint GLTextureCube::LoadFromVCross(uint mips)
	{
		uint srcWidth, srcHeight, bits;
		byte*** cubeTextureData = new byte**[mips];
		for (uint i = 0; i < mips; i++)
			cubeTextureData[i] = new byte*[6];

		uint* faceWidths  = new uint[mips];
		uint* faceHeights = new uint[mips];

		for (uint m = 0; m < mips; m++)
		{
			byte* data = Lumos::LoadImageFromFile(m_Files[m], &srcWidth, &srcHeight, &bits, !m_LoadOptions.flipY);
			m_Parameters.format = GLTexture2D::BitsToTextureFormat(bits);
			uint stride = bits / 8;

			uint face = 0;
			uint faceWidth = srcWidth / 3;
			uint faceHeight = srcHeight / 4;
			faceWidths[m] = faceWidth;
			faceHeights[m] = faceHeight;
			for (uint cy = 0; cy < 4; cy++)
			{
				for (uint cx = 0; cx < 3; cx++)
				{
					if (cy == 0 || cy == 2 || cy == 3)
					{
						if (cx != 1)
							continue;
					}

					cubeTextureData[m][face] = new byte[faceWidth * faceHeight * stride];

					for (uint y = 0; y < faceHeight; y++)
					{
						uint offset = y;
						if (face == 5)
							offset = faceHeight - (y + 1);
						uint yp = cy * faceHeight + offset;
						for (uint x = 0; x < faceWidth; x++)
						{
							offset = x;
							if (face == 5)
								offset = faceWidth - (x + 1);
							uint xp = cx * faceWidth + offset;
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

		uint result;
		GLCall(glGenTextures(1, &result));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, result));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
		//GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD,   mips - 1));
		//GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, mips - 1));

		uint internalFormat = GLTexture2D::TextureFormatToGL(m_Parameters.format);
		uint format = GLTexture2D::TextureFormatToInternalFormat(internalFormat);
		for (uint m = 0; m < mips; m++)
		{
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][3]));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][1]));

			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][0]));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][4]));

			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][2]));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][5]));
		}
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

		for (uint m = 0; m < mips; m++)
		{
			for (uint f = 0; f < 6; f++)
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
