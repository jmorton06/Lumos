#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"
#include "Renderable2D.h"

#include <cereal/cereal.hpp>

namespace Lumos
{
	namespace Maths 
	{
		class Vector4;
	}

	namespace Graphics
	{
		class Texture2D;

		class LUMOS_EXPORT Sprite : public Renderable2D
		{
		public:
			Sprite(const Maths::Vector2& position = Maths::Vector2(0.0f,0.0f), const Maths::Vector2& scale = Maths::Vector2(1.0f,1.0f), const Maths::Vector4& colour = Maths::Vector4(1.0f));
			Sprite(const Ref<Texture2D>& texture, const Maths::Vector2& position, const Maths::Vector2& scale, const Maths::Vector4& colour);
			virtual ~Sprite();
			void SetPosition(const Maths::Vector2& vector2) { m_Position = vector2; };
        
            void SetSpriteSheet(const Ref<Texture2D>& texture, const Maths::Vector2& index, const Maths::Vector2& cellSize, const Maths::Vector2& spriteSize);
            void SetTexture(const Ref<Texture2D>& texture) { m_Texture = texture; }
			void OnImGui();
        
            void SetTextureFromFile(const std::string& filePath);
		
			template<typename Archive>
			void save(Archive& archive) const
			{
				archive(cereal::make_nvp("TexturePath", m_Texture ? m_Texture->GetFilepath() : ""),
						cereal::make_nvp("Position", m_Position),
						cereal::make_nvp("Scale", m_Scale),
						cereal::make_nvp("Colour", m_Colour));
			}

			template<typename Archive>
			void load(Archive& archive)
			{
				std::string textureFilePath;
				archive(cereal::make_nvp("TexturePath", textureFilePath),
					cereal::make_nvp("Position", m_Position),
					cereal::make_nvp("Scale", m_Scale),
					cereal::make_nvp("Colour", m_Colour));

                if(!textureFilePath.empty())
                    m_Texture = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("sprite", textureFilePath));
			}
		};
	}
}
