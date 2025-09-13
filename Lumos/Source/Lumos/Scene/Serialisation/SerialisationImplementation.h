#pragma once
#include "Scene/Serialisation/Serialisation.h"

#ifndef SERIALISATION_INCLUDE_ONLY
#include "Maths/MathsSerialisation.h"
#include "Maths/Transform.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Graphics/ParticleManager.h"
#include "Core/Asset/AssetManager.h"
#include "Utilities/StringUtilities.h"
#include "Scene/Component/ModelComponent.h"
#include "Scene/Component/Components.h"
#include "Scripting/Lua/LuaScriptComponent.h"
#include "Core/Application.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/B2PhysicsEngine/RigidBody2D.h"
#include "Core/Asset/Asset.h"
#include "Core/Asset/AssetRegistry.h"
#include "Audio/AudioManager.h"
#include "Audio/SoundNode.h"
#include "Scene/Scene.h"
#include "Core/String.h"
#include "Core/Thread.h"
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>

namespace Lumos
{

    template <class Archive>
    std::string save_minimal(Archive& ar, String8 const& str)
    {
        // Save number of chars + the data
        // ar( str.size );

        return str.size > 0 ? ToStdString(str) : std::string();
        // ar( cereal::binary_data((char*)str.str, static_cast<std::size_t>(str.size * sizeof(u8))) );
    }

    template <class Archive>
    void load_minimal(Archive& ar, String8& str, const std::string& value)
    {
        // u64 size;
        // ar( size );

        // std::string stringCopy = ToStdString(str);
        // ar( value);

        if(!value.empty())
            str = PushStr8Copy(Application::Get().GetFrameArena(), value.c_str());
        // PushStr8FillByte(Application::Get().GetFrameArena(), size, 0);
        // ar( cereal::binary_data( (char*)str.str, static_cast<std::size_t>(size * sizeof(u8)) ) );
    }

    template <typename Archive>
    void save(Archive& archive, const Listener& listener)
    {
        archive(cereal::make_nvp("enabled", listener.m_Enabled));
    }

    template <typename Archive>
    void load(Archive& archive, Listener& listener)
    {
        archive(cereal::make_nvp("enabled", listener.m_Enabled));
    }

    template <typename Archive>
    void save(Archive& archive, const SoundNode& node)
    {
        String8 path;
        FileSystem::Get().AbsolutePathToFileSystem(Application::Get().GetFrameArena(), node.m_Sound ? Str8StdS(node.m_Sound->GetFilePath()) : Str8Lit(""), path);

        archive(cereal::make_nvp("Position", node.m_Position), cereal::make_nvp("Radius", node.m_Radius), cereal::make_nvp("Pitch", node.m_Pitch), cereal::make_nvp("Volume", node.m_Volume), cereal::make_nvp("Velocity", node.m_Velocity), cereal::make_nvp("Looping", node.m_IsLooping), cereal::make_nvp("Paused", node.m_Paused), cereal::make_nvp("ReferenceDistance", node.m_ReferenceDistance), cereal::make_nvp("Global", node.m_IsGlobal), cereal::make_nvp("TimeLeft", node.m_TimeLeft), cereal::make_nvp("Stationary", node.m_Stationary),
                cereal::make_nvp("SoundNodePath", path), cereal::make_nvp("RollOffFactor", node.m_RollOffFactor));
    }

    template <typename Archive>
    void load(Archive& archive, SoundNode& node)
    {
        std::string soundFilePath;
        archive(cereal::make_nvp("Position", node.m_Position), cereal::make_nvp("Radius", node.m_Radius), cereal::make_nvp("Pitch", node.m_Pitch), cereal::make_nvp("Volume", node.m_Volume), cereal::make_nvp("Velocity", node.m_Velocity), cereal::make_nvp("Looping", node.m_IsLooping), cereal::make_nvp("Paused", node.m_Paused), cereal::make_nvp("ReferenceDistance", node.m_ReferenceDistance), cereal::make_nvp("Global", node.m_IsGlobal), cereal::make_nvp("TimeLeft", 0.0f), cereal::make_nvp("Stationary", node.m_Stationary),
                cereal::make_nvp("SoundNodePath", soundFilePath), cereal::make_nvp("RollOffFactor", node.m_RollOffFactor));

        if(!soundFilePath.empty())
        {
            node.SetSound(Sound::Create(soundFilePath, StringUtilities::GetFilePathExtension(soundFilePath)));
        }
    }

    template <typename Archive>
    void save(Archive& archive, const Scene& scene)
    {
        archive(cereal::make_nvp("Version", SceneSerialisationVersion));
        archive(cereal::make_nvp("Scene Name", scene.m_SceneName));

        archive(cereal::make_nvp("PhysicsEnabled2D", scene.m_Settings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", scene.m_Settings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", scene.m_Settings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", scene.m_Settings.RenderSettings.Renderer2DEnabled),
                cereal::make_nvp("Renderer3DEnabled", scene.m_Settings.RenderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", scene.m_Settings.RenderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", scene.m_Settings.RenderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", scene.m_Settings.RenderSettings.ShadowsEnabled));
        archive(cereal::make_nvp("Exposure", scene.m_Settings.RenderSettings.m_Exposure), cereal::make_nvp("ToneMap", scene.m_Settings.RenderSettings.m_ToneMapIndex));

        archive(cereal::make_nvp("BloomIntensity", scene.m_Settings.RenderSettings.m_BloomIntensity), cereal::make_nvp("BloomKnee", scene.m_Settings.RenderSettings.BloomKnee), cereal::make_nvp("BloomThreshold", scene.m_Settings.RenderSettings.BloomThreshold),
                cereal::make_nvp("BloomUpsampleScale", scene.m_Settings.RenderSettings.BloomUpsampleScale));

        archive(cereal::make_nvp("FXAAEnabled", scene.m_Settings.RenderSettings.FXAAEnabled), cereal::make_nvp("DebandingEnabled", scene.m_Settings.RenderSettings.DebandingEnabled), cereal::make_nvp("ChromaticAberationEnabled", scene.m_Settings.RenderSettings.ChromaticAberationEnabled), cereal::make_nvp("EyeAdaptation", scene.m_Settings.RenderSettings.EyeAdaptation), cereal::make_nvp("SSAOEnabled", scene.m_Settings.RenderSettings.SSAOEnabled), cereal::make_nvp("BloomEnabled", scene.m_Settings.RenderSettings.BloomEnabled), cereal::make_nvp("FilmicGrainEnabled", scene.m_Settings.RenderSettings.FilmicGrainEnabled), cereal::make_nvp("DepthOfFieldEnabled", scene.m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("MotionBlurEnabled", scene.m_Settings.RenderSettings.MotionBlurEnabled));

        archive(cereal::make_nvp("DepthOFFieldEnabled", scene.m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("DepthOfFieldStrength", scene.m_Settings.RenderSettings.DepthOfFieldStrength), cereal::make_nvp("DepthOfFieldDistance", scene.m_Settings.RenderSettings.DepthOfFieldDistance));

        archive(scene.m_Settings.RenderSettings.Brightness, scene.m_Settings.RenderSettings.Saturation, scene.m_Settings.RenderSettings.Contrast);

        archive(scene.m_Settings.RenderSettings.SharpenEnabled);
    }

    template <typename Archive>
    void load(Archive& archive, Scene& scene)
    {
        archive(cereal::make_nvp("Version", scene.m_SceneSerialisationVersion));
        archive(cereal::make_nvp("Scene Name", scene.m_SceneName));

        Serialisation::CurrentSceneVersion = scene.m_SceneSerialisationVersion;

        if(scene.m_SceneSerialisationVersion > 7)
        {
            archive(cereal::make_nvp("PhysicsEnabled2D", scene.m_Settings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", scene.m_Settings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", scene.m_Settings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", scene.m_Settings.RenderSettings.Renderer2DEnabled),
                    cereal::make_nvp("Renderer3DEnabled", scene.m_Settings.RenderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", scene.m_Settings.RenderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", scene.m_Settings.RenderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", scene.m_Settings.RenderSettings.ShadowsEnabled));
        }
        if(scene.m_SceneSerialisationVersion > 9)
        {
            archive(cereal::make_nvp("Exposure", scene.m_Settings.RenderSettings.m_Exposure), cereal::make_nvp("ToneMap", scene.m_Settings.RenderSettings.m_ToneMapIndex));
        }

        if(Serialisation::CurrentSceneVersion > 11)
        {
            archive(cereal::make_nvp("BloomIntensity", scene.m_Settings.RenderSettings.m_BloomIntensity), cereal::make_nvp("BloomKnee", scene.m_Settings.RenderSettings.BloomKnee), cereal::make_nvp("BloomThreshold", scene.m_Settings.RenderSettings.BloomThreshold),
                    cereal::make_nvp("BloomUpsampleScale", scene.m_Settings.RenderSettings.BloomUpsampleScale));
        }
        if(Serialisation::CurrentSceneVersion > 12)
        {
            archive(cereal::make_nvp("FXAAEnabled", scene.m_Settings.RenderSettings.FXAAEnabled), cereal::make_nvp("DebandingEnabled", scene.m_Settings.RenderSettings.DebandingEnabled), cereal::make_nvp("ChromaticAberationEnabled", scene.m_Settings.RenderSettings.ChromaticAberationEnabled), cereal::make_nvp("EyeAdaptation", scene.m_Settings.RenderSettings.EyeAdaptation), cereal::make_nvp("SSAOEnabled", scene.m_Settings.RenderSettings.SSAOEnabled), cereal::make_nvp("BloomEnabled", scene.m_Settings.RenderSettings.BloomEnabled), cereal::make_nvp("FilmicGrainEnabled", scene.m_Settings.RenderSettings.FilmicGrainEnabled), cereal::make_nvp("DepthOfFieldEnabled", scene.m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("MotionBlurEnabled", scene.m_Settings.RenderSettings.MotionBlurEnabled));
        }

        if(Serialisation::CurrentSceneVersion > 15)
        {
            archive(cereal::make_nvp("DepthOfFieldEnabled", scene.m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("DepthOfFieldStrength", scene.m_Settings.RenderSettings.DepthOfFieldStrength), cereal::make_nvp("DepthOfFieldDistance", scene.m_Settings.RenderSettings.DepthOfFieldDistance));
        }

        if(Serialisation::CurrentSceneVersion > 16)
            archive(scene.m_Settings.RenderSettings.Brightness, scene.m_Settings.RenderSettings.Saturation, scene.m_Settings.RenderSettings.Contrast);

        if(Serialisation::CurrentSceneVersion > 18)
            archive(scene.m_Settings.RenderSettings.SharpenEnabled);
    }

    template <typename Archive>
    void save(Archive& archive, const UUID& ID)
    {
        archive(ID.m_UUID);
    }

    template <typename Archive>
    void load(Archive& archive, UUID& ID)
    {
        archive(ID.m_UUID);
    }

    template <typename Archive>
    void save(Archive& archive, const RigidBody3D& rigidBody)
    {
        auto shape = std::unique_ptr<CollisionShape>(rigidBody.m_CollisionShape.get());

        const int Version = 2;

        archive(cereal::make_nvp("Version", Version));
        archive(cereal::make_nvp("Position", rigidBody.m_Position), cereal::make_nvp("Orientation", rigidBody.m_Orientation), cereal::make_nvp("LinearVelocity", rigidBody.m_LinearVelocity), cereal::make_nvp("Force", rigidBody.m_Force), cereal::make_nvp("Mass", 1.0f / rigidBody.m_InvMass), cereal::make_nvp("AngularVelocity", rigidBody.m_AngularVelocity), cereal::make_nvp("Torque", rigidBody.m_Torque), cereal::make_nvp("Static", rigidBody.m_Static), cereal::make_nvp("Friction", rigidBody.m_Friction), cereal::make_nvp("Elasticity", rigidBody.m_Elasticity), cereal::make_nvp("CollisionShape", shape), cereal::make_nvp("Trigger", rigidBody.m_Trigger), cereal::make_nvp("AngularFactor", rigidBody.m_AngularFactor));
        archive(cereal::make_nvp("UUID", (uint64_t)rigidBody.m_UUID));
        shape.release();
    }

    template <typename Archive>
    void load(Archive& archive, RigidBody3D& rigidBody)
    {
        auto shape = std::unique_ptr<CollisionShape>(rigidBody.m_CollisionShape.get());

        int Version;
        archive(cereal::make_nvp("Version", Version));
        archive(cereal::make_nvp("Position", rigidBody.m_Position), cereal::make_nvp("Orientation", rigidBody.m_Orientation), cereal::make_nvp("LinearVelocity", rigidBody.m_LinearVelocity), cereal::make_nvp("Force", rigidBody.m_Force), cereal::make_nvp("Mass", 1.0f / rigidBody.m_InvMass), cereal::make_nvp("AngularVelocity", rigidBody.m_AngularVelocity), cereal::make_nvp("Torque", rigidBody.m_Torque), cereal::make_nvp("Static", rigidBody.m_Static), cereal::make_nvp("Friction", rigidBody.m_Friction), cereal::make_nvp("Elasticity", rigidBody.m_Elasticity), cereal::make_nvp("CollisionShape", shape), cereal::make_nvp("Trigger", rigidBody.m_Trigger), cereal::make_nvp("AngularFactor", rigidBody.m_AngularFactor));

        rigidBody.m_CollisionShape = nullptr;
        rigidBody.m_CollisionShape = SharedPtr<CollisionShape>(shape.get());
        rigidBody.CollisionShapeUpdated();
        shape.release();

        if(Version > 1)
            archive(cereal::make_nvp("UUID", (uint64_t)rigidBody.m_UUID));
    }

    template <typename Archive>
    void save(Archive& archive, const RigidBody2D& rigidBody)
    {
        archive(cereal::make_nvp("Position", rigidBody.GetPosition()), cereal::make_nvp("Friction", rigidBody.m_Friction), cereal::make_nvp("Angle", rigidBody.GetAngle()), cereal::make_nvp("Static", rigidBody.GetIsStatic()), cereal::make_nvp("Mass", rigidBody.m_Mass), cereal::make_nvp("Scale", rigidBody.m_Scale),
                cereal::make_nvp("Shape", rigidBody.m_ShapeType), cereal::make_nvp("CustomShapePos", rigidBody.m_CustomShapePositions));

        archive(rigidBody.m_Damping);
    }

    template <typename Archive>
    void load(Archive& archive, RigidBody2D& rigidBody)
    {
        RigidBodyParameters params;
        float angle;
        Vec2 pos;
        archive(cereal::make_nvp("Position", pos), cereal::make_nvp("Friction", rigidBody.m_Friction), cereal::make_nvp("Angle", angle), cereal::make_nvp("Static", rigidBody.m_Static), cereal::make_nvp("Mass", rigidBody.m_Mass), cereal::make_nvp("Scale", params.scale), cereal::make_nvp("Shape", rigidBody.m_ShapeType), cereal::make_nvp("CustomShapePos", params.customShapePositions));

        if(Serialisation::CurrentSceneVersion > 26)
            archive(params.damping);

        params.shape    = rigidBody.m_ShapeType;
        params.position = Vec3(pos, 1.0f);
        params.isStatic = rigidBody.m_Static;
        params.mass     = rigidBody.m_Mass;
        params.friction = rigidBody.m_Friction;

        rigidBody.Init(params);
        rigidBody.SetOrientation(angle);
    }

    template <typename Archive>
    void save(Archive& archive, const TextComponent& textComponent)
    {
        String8 path;
        FileSystem::Get().AbsolutePathToFileSystem(Application::Get().GetFrameArena(), textComponent.FontHandle ? Str8StdS(textComponent.FontHandle->GetFilePath()) : Str8Lit(""), path);

        archive(cereal::make_nvp("TextString", textComponent.TextString), cereal::make_nvp("Path", path), cereal::make_nvp("Colour", textComponent.Colour), cereal::make_nvp("LineSpacing", textComponent.LineSpacing), cereal::make_nvp("Kerning", textComponent.Kerning),
                cereal::make_nvp("MaxWidth", textComponent.MaxWidth), cereal::make_nvp("OutlineColour", textComponent.OutlineColour), cereal::make_nvp("OutlineWidth", textComponent.OutlineWidth));
    }

    template <typename Archive>
    void load(Archive& archive, TextComponent& textComponent)
    {
        String8 fontFilePath;

        if(Serialisation::CurrentSceneVersion >= 15)
        {
            archive(cereal::make_nvp("TextString", textComponent.TextString), cereal::make_nvp("Path", fontFilePath), cereal::make_nvp("Colour", textComponent.Colour), cereal::make_nvp("LineSpacing", textComponent.LineSpacing), cereal::make_nvp("Kerning", textComponent.Kerning),
                    cereal::make_nvp("MaxWidth", textComponent.MaxWidth), cereal::make_nvp("OutlineColour", textComponent.OutlineColour), cereal::make_nvp("OutlineWidth", textComponent.OutlineWidth));
        }
        else
        {
            archive(cereal::make_nvp("TextString", textComponent.TextString), cereal::make_nvp("Path", fontFilePath), cereal::make_nvp("Colour", textComponent.Colour), cereal::make_nvp("LineSpacing", textComponent.LineSpacing), cereal::make_nvp("Kerning", textComponent.Kerning),
                    cereal::make_nvp("MaxWidth", textComponent.MaxWidth));
        }

        if(!fontFilePath.size > 0 && FileSystem::FileExists(fontFilePath))
        {
            Application::Get().GetAssetManager()->AddAsset(fontFilePath, textComponent.FontHandle);
        }
        else
        {
            textComponent.FontHandle = Graphics::Font::GetDefaultFont();
        }
    }

    template <typename Archive>
    void serialize(Archive& archive, PrefabComponent& prefabComponent)
    {
        archive(prefabComponent.Path);
    }

    static const int AssetRegistrySerialisationVersion = 2;
    template <typename Archive>
    void save(Archive& archive, const AssetRegistry& registry)
    {
        std::vector<std::pair<std::string, UUID>> elems;

        // Fill elems from m_NameMap
        ForHashMapEach(u64, UUID, &registry.m_NameMap, it)
        {
            u64 nameHash = *it.key;
            UUID uuid    = *it.value;

            String8 name;
            if(!registry.GetName(uuid, name))
                continue;

            if(name.size == 0 || !name.str)
                continue;

            std::string nameStr = ToStdString(name);

            if(!StringUtilities::StringContains(nameStr, "Cache/"))
            {
                elems.emplace_back(nameStr, uuid);
            }
        }

        std::sort(elems.begin(), elems.end(), [](const auto& a, const auto& b)
                  { return a.second < b.second; });

        archive(cereal::make_nvp("Version", AssetRegistrySerialisationVersion));
        archive(cereal::make_nvp("Count", (int)elems.size()));

        for(auto& entry : elems)
        {
            uint16_t assetType = 0;
            AssetMetaData meta;
            if(HashMapFind(&registry.m_AssetRegistry, entry.second, &meta))
            {
                assetType = (uint16_t)meta.Type;
            }

            archive(
                cereal::make_nvp("Name", entry.first),
                cereal::make_nvp("UUID", (uint64_t)entry.second),
                cereal::make_nvp("AssetType", assetType));
        }
    }

    template <typename Archive>
    void load(Archive& archive, AssetRegistry& registry)
    {
        int version;
        int count;
        archive(cereal::make_nvp("Version", version));
        archive(cereal::make_nvp("Count", count));

        for(int i = 0; i < count; i++)
        {
            String8 key;
            uint64_t value;
            uint16_t type = 0;

            if(version <= 1)
                archive(cereal::make_nvp("Name", key), cereal::make_nvp("UUID", value));
            else
                archive(cereal::make_nvp("Name", key), cereal::make_nvp("UUID", value), cereal::make_nvp("AssetType", type));

            UUID uuid = (UUID)value;
            registry.AddName(key, uuid);

            if(type)
            {
                UUID currentID;
                if(registry.GetID(key, currentID))
                {
                    if(uuid != currentID)
                    {
                        registry.ReplaceID(currentID, uuid);
                    }
                }

                AssetMetaData* existingMeta = (AssetMetaData*)HashMapFindPtr(&registry.m_AssetRegistry, uuid);
                if(!existingMeta)
                {
                    AssetMetaData metaData;
                    metaData.IsDataLoaded = false;
                    metaData.Type         = (AssetType)type;
                    HashMapInsert(&registry.m_AssetRegistry, uuid, metaData);
                }
            }
        }
    }

    template <typename Archive>
    void save(Archive& archive, const ParticleEmitter& particleEmitter)
    {
        archive(particleEmitter.m_ParticleCount);
        archive(particleEmitter.m_ParticleLife);
        archive(particleEmitter.m_ParticleSize);
        archive(particleEmitter.m_InitialVelocity);
        archive(particleEmitter.m_InitialColour);
        archive(particleEmitter.m_Spread);
        archive(particleEmitter.m_VelocitySpread);
        archive(particleEmitter.m_FadeIn);
        archive(particleEmitter.m_FadeOut);
        archive(particleEmitter.m_NextParticleTime);
        archive(particleEmitter.m_ParticleRate);
        archive(particleEmitter.m_NumLaunchParticles);
        archive(particleEmitter.m_IsAnimated);
        archive(particleEmitter.m_Gravity);
        archive(particleEmitter.m_AnimatedTextureRows);

        ArenaTemp temp = ScratchBegin(nullptr, 0);
        String8 newPath;
        if(particleEmitter.m_Texture)
        {
            FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(particleEmitter.m_Texture->GetFilepath()), newPath);
        }
        archive(std::string((const char*)newPath.str));

        archive(particleEmitter.m_LifeSpread);
        archive(particleEmitter.m_SortParticles);
        archive(particleEmitter.m_DepthWrite);
        archive((uint8_t)particleEmitter.m_BlendType);
        archive((uint8_t)particleEmitter.m_AlignedType);
        ScratchEnd(temp);
    }

    template <typename Archive>
    void load(Archive& archive, ParticleEmitter& particleEmitter)
    {
        archive(particleEmitter.m_ParticleCount);
        archive(particleEmitter.m_ParticleLife);
        archive(particleEmitter.m_ParticleSize);
        archive(particleEmitter.m_InitialVelocity);
        archive(particleEmitter.m_InitialColour);
        archive(particleEmitter.m_Spread);
        archive(particleEmitter.m_VelocitySpread);
        archive(particleEmitter.m_FadeIn);
        archive(particleEmitter.m_FadeOut);
        archive(particleEmitter.m_NextParticleTime);
        archive(particleEmitter.m_ParticleRate);
        archive(particleEmitter.m_NumLaunchParticles);
        archive(particleEmitter.m_IsAnimated);
        archive(particleEmitter.m_Gravity);
        archive(particleEmitter.m_AnimatedTextureRows);

        std::string textureFilePath;
        archive(textureFilePath);

        if(!textureFilePath.empty())
        {
            Graphics::TextureDesc desc;
            desc.minFilter            = Graphics::TextureFilter::LINEAR;
            desc.magFilter            = Graphics::TextureFilter::LINEAR;
            desc.wrap                 = Graphics::TextureWrap::CLAMP;
            particleEmitter.m_Texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("particle", textureFilePath, desc));
        }
        else
            particleEmitter.m_Texture = nullptr;

        if(SceneSerialisationVersion > 23)
        {
            archive(particleEmitter.m_LifeSpread);
            archive(particleEmitter.m_SortParticles);
            archive(particleEmitter.m_DepthWrite);
            uint8_t value;
            archive(value);
            particleEmitter.m_BlendType = (ParticleEmitter::BlendType)value;
            archive(value);
            particleEmitter.m_AlignedType = (ParticleEmitter::AlignedType)value;
        }

        particleEmitter.Init();
    }

    template <typename Archive>
    void save(Archive& archive, const LuaScriptComponent& luaComponent)
    {
        ArenaTemp temp = ScratchBegin(nullptr, 0);
        String8 newPath;
        FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(luaComponent.m_FileName), newPath);
        archive(cereal::make_nvp("FilePath", ToStdString(newPath)));
        ScratchEnd(temp);
    }

    template <typename Archive>
    void load(Archive& archive, LuaScriptComponent& luaComponent)
    {
        luaComponent.m_Scene = Application::Get().GetCurrentScene();
        archive(cereal::make_nvp("FilePath", luaComponent.m_FileName));

        ArenaTemp temp = ScratchBegin(nullptr, 0);
        String8 newPath;
        FileSystem::Get().ResolvePhysicalPath(temp.arena, Str8StdS(luaComponent.m_FileName), &newPath);
        luaComponent.m_FileName = ToStdString(newPath);
        luaComponent.Init();

        ScratchEnd(temp);
    }

    namespace Graphics
    {
        template <typename Archive>
        void save(Archive& archive, const Graphics::Material& material)
        {
            ArenaTemp temp = ScratchBegin(nullptr, 0);

            String8 shaderPath;

            String8 albedoFilePath    = material.m_PBRMaterialTextures.albedo ? FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(material.m_PBRMaterialTextures.albedo->GetFilepath())) : String8();
            String8 normalFilePath    = material.m_PBRMaterialTextures.normal ? FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(material.m_PBRMaterialTextures.normal->GetFilepath())) : String8();
            String8 metallicFilePath  = material.m_PBRMaterialTextures.metallic ? FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(material.m_PBRMaterialTextures.metallic->GetFilepath())) : String8();
            String8 roughnessFilePath = material.m_PBRMaterialTextures.roughness ? FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(material.m_PBRMaterialTextures.roughness->GetFilepath())) : String8();
            String8 emissiveFilePath  = material.m_PBRMaterialTextures.emissive ? FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(material.m_PBRMaterialTextures.emissive->GetFilepath())) : String8();
            String8 aoFilePath        = material.m_PBRMaterialTextures.ao ? FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(material.m_PBRMaterialTextures.ao->GetFilepath())) : String8();

            if(material.m_Shader)
            {
                // std::string path = std::string(material.m_Shader->GetFilePath()) + std::string(material.m_Shader->GetName());
                // FileSystem::Get().AbsolutePathToFileSystem(temo.arena, Str8StdS(path), *shaderPath);
            }

            archive(cereal::make_nvp("Albedo", albedoFilePath),
                    cereal::make_nvp("Normal", normalFilePath),
                    cereal::make_nvp("Metallic", metallicFilePath),
                    cereal::make_nvp("Roughness", roughnessFilePath),
                    cereal::make_nvp("Ao", aoFilePath),
                    cereal::make_nvp("Emissive", emissiveFilePath),
                    cereal::make_nvp("albedoColour", material.m_MaterialProperties->albedoColour),
                    cereal::make_nvp("roughnessValue", material.m_MaterialProperties->roughness),
                    cereal::make_nvp("metallicValue", material.m_MaterialProperties->metallic),
                    cereal::make_nvp("emissiveValue", material.m_MaterialProperties->emissive),
                    cereal::make_nvp("albedoMapFactor", material.m_MaterialProperties->albedoMapFactor),
                    cereal::make_nvp("metallicMapFactor", material.m_MaterialProperties->metallicMapFactor),
                    cereal::make_nvp("roughnessMapFactor", material.m_MaterialProperties->roughnessMapFactor),
                    cereal::make_nvp("normalMapFactor", material.m_MaterialProperties->normalMapFactor),
                    cereal::make_nvp("aoMapFactor", material.m_MaterialProperties->occlusionMapFactor),
                    cereal::make_nvp("emissiveMapFactor", material.m_MaterialProperties->emissiveMapFactor),
                    cereal::make_nvp("alphaCutOff", material.m_MaterialProperties->alphaCutoff),
                    cereal::make_nvp("workflow", material.m_MaterialProperties->workflow),
                    cereal::make_nvp("shader", shaderPath));

            archive(cereal::make_nvp("Reflectance", material.m_MaterialProperties->reflectance));
            ScratchEnd(temp);
        }

        template <typename Archive>
        void load(Archive& archive, Graphics::Material& material)
        {
            std::string albedoFilePath;
            std::string normalFilePath;
            std::string roughnessFilePath;
            std::string metallicFilePath;
            std::string emissiveFilePath;
            std::string aoFilePath;
            String8 shaderFilePath;

            constexpr bool loadOldMaterial = false;

            if constexpr(loadOldMaterial)
            {
                Vec4 roughness, metallic, emissive;
                archive(cereal::make_nvp("Albedo", albedoFilePath),
                        cereal::make_nvp("Normal", normalFilePath),
                        cereal::make_nvp("Metallic", metallicFilePath),
                        cereal::make_nvp("Roughness", roughnessFilePath),
                        cereal::make_nvp("Ao", aoFilePath),
                        cereal::make_nvp("Emissive", emissiveFilePath),
                        cereal::make_nvp("albedoColour", material.m_MaterialProperties->albedoColour),
                        cereal::make_nvp("roughnessColour", roughness),
                        cereal::make_nvp("metallicColour", metallic),
                        cereal::make_nvp("emissiveColour", emissive),
                        cereal::make_nvp("usingAlbedoMap", material.m_MaterialProperties->albedoMapFactor),
                        cereal::make_nvp("usingMetallicMap", material.m_MaterialProperties->metallicMapFactor),
                        cereal::make_nvp("usingRoughnessMap", material.m_MaterialProperties->roughnessMapFactor),
                        cereal::make_nvp("usingNormalMap", material.m_MaterialProperties->normalMapFactor),
                        cereal::make_nvp("usingAOMap", material.m_MaterialProperties->occlusionMapFactor),
                        cereal::make_nvp("usingEmissiveMap", material.m_MaterialProperties->emissiveMapFactor),
                        cereal::make_nvp("workflow", material.m_MaterialProperties->workflow),
                        cereal::make_nvp("shader", shaderFilePath));

                material.m_MaterialProperties->emissive  = emissive.x;
                material.m_MaterialProperties->metallic  = metallic.x;
                material.m_MaterialProperties->roughness = roughness.x;
            }
            else
            {
                archive(cereal::make_nvp("Albedo", albedoFilePath),
                        cereal::make_nvp("Normal", normalFilePath),
                        cereal::make_nvp("Metallic", metallicFilePath),
                        cereal::make_nvp("Roughness", roughnessFilePath),
                        cereal::make_nvp("Ao", aoFilePath),
                        cereal::make_nvp("Emissive", emissiveFilePath),
                        cereal::make_nvp("albedoColour", material.m_MaterialProperties->albedoColour),
                        cereal::make_nvp("roughnessValue", material.m_MaterialProperties->roughness),
                        cereal::make_nvp("metallicValue", material.m_MaterialProperties->metallic),
                        cereal::make_nvp("emissiveValue", material.m_MaterialProperties->emissive),
                        cereal::make_nvp("albedoMapFactor", material.m_MaterialProperties->albedoMapFactor),
                        cereal::make_nvp("metallicMapFactor", material.m_MaterialProperties->metallicMapFactor),
                        cereal::make_nvp("roughnessMapFactor", material.m_MaterialProperties->roughnessMapFactor),
                        cereal::make_nvp("normalMapFactor", material.m_MaterialProperties->normalMapFactor),
                        cereal::make_nvp("aoMapFactor", material.m_MaterialProperties->occlusionMapFactor),
                        cereal::make_nvp("emissiveMapFactor", material.m_MaterialProperties->emissiveMapFactor),
                        cereal::make_nvp("alphaCutOff", material.m_MaterialProperties->alphaCutoff),
                        cereal::make_nvp("workflow", material.m_MaterialProperties->workflow),
                        cereal::make_nvp("shader", shaderFilePath));

                if(Serialisation::CurrentSceneVersion > 19)
                    archive(cereal::make_nvp("Reflectance", material.m_MaterialProperties->reflectance));
            }

            // if(!shaderFilePath.empty())
            // SetShader(shaderFilePath);
            // TODO: Support Custom Shaders;
            material.m_Shader = nullptr;

            if(!albedoFilePath.empty())
                material.m_PBRMaterialTextures.albedo = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("albedo", albedoFilePath));
            if(!normalFilePath.empty())
                material.m_PBRMaterialTextures.normal = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("roughness", normalFilePath));
            if(!metallicFilePath.empty())
                material.m_PBRMaterialTextures.metallic = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("metallic", metallicFilePath));
            if(!roughnessFilePath.empty())
                material.m_PBRMaterialTextures.roughness = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("roughness", roughnessFilePath));
            if(!emissiveFilePath.empty())
                material.m_PBRMaterialTextures.emissive = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("emissive", emissiveFilePath));
            if(!aoFilePath.empty())
                material.m_PBRMaterialTextures.ao = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("ao", aoFilePath));
        }

        template <typename Archive>
        void save(Archive& archive, const Graphics::Sprite& sprite)
        {
            ArenaTemp temp = ScratchBegin(nullptr, 0);

            String8 newPath;
            if(sprite.m_Texture)
            {
                FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(sprite.m_Texture->GetFilepath()), newPath);
            }

            archive(cereal::make_nvp("TexturePath", ToStdString(newPath)),
                    cereal::make_nvp("Position", sprite.m_Position),
                    cereal::make_nvp("Scale", sprite.m_Scale),
                    cereal::make_nvp("Colour", sprite.m_Colour));

            archive(sprite.UsingSpriteSheet, sprite.SpriteSheetTileSizeX);
            archive(sprite.SpriteSheetTileSizeY);
            ScratchEnd(temp);
        }

        template <typename Archive>
        void load(Archive& archive, Graphics::Sprite& sprite)
        {
            std::string textureFilePath;
            archive(cereal::make_nvp("TexturePath", textureFilePath),
                    cereal::make_nvp("Position", sprite.m_Position),
                    cereal::make_nvp("Scale", sprite.m_Scale),
                    cereal::make_nvp("Colour", sprite.m_Colour));

            if(textureFilePath.size() != 0)
                sprite.m_Texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("sprite", textureFilePath));

            if(Serialisation::CurrentSceneVersion > 21 && Serialisation::CurrentSceneVersion < 26)
            {
                archive(sprite.UsingSpriteSheet, sprite.SpriteSheetTileSizeX);
                sprite.SpriteSheetTileSizeY = sprite.SpriteSheetTileSizeX;
            }

            if(Serialisation::CurrentSceneVersion > 25)
            {
                archive(sprite.UsingSpriteSheet, sprite.SpriteSheetTileSizeX);
                archive(sprite.SpriteSheetTileSizeY);
            }
        }

        template <typename Archive>
        void serialize(Archive& archive, AnimatedSprite::AnimationState& state)
        {
            archive(cereal::make_nvp("PlayMode", state.Mode),
                    cereal::make_nvp("Frames", state.Frames),
                    cereal::make_nvp("FrameDuration", state.FrameDuration));
        }

        template <typename Archive>
        void save(Archive& archive, const Graphics::AnimatedSprite& sprite)
        {
            archive(cereal::make_nvp("TexturePath", sprite.m_Texture ? sprite.m_Texture->GetFilepath() : ""),
                    cereal::make_nvp("Position", sprite.m_Position),
                    cereal::make_nvp("Scale", sprite.m_Scale),
                    cereal::make_nvp("Colour", sprite.m_Colour),
                    cereal::make_nvp("AnimationFrames", sprite.m_AnimationStates),
                    cereal::make_nvp("State", sprite.m_State));
            archive(sprite.SpriteSheetTileSizeX);
            archive(sprite.SpriteSheetTileSizeY);
        }

        template <typename Archive>
        void load(Archive& archive, Graphics::AnimatedSprite& sprite)
        {
            std::string textureFilePath;
            archive(cereal::make_nvp("TexturePath", textureFilePath),
                    cereal::make_nvp("Position", sprite.m_Position),
                    cereal::make_nvp("Scale", sprite.m_Scale),
                    cereal::make_nvp("Colour", sprite.m_Colour),
                    cereal::make_nvp("AnimationFrames", sprite.m_AnimationStates),
                    cereal::make_nvp("State", sprite.m_State));

            if(!textureFilePath.empty())
                sprite.m_Texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("sprite", textureFilePath));

            if(Serialisation::CurrentSceneVersion > 21 && Serialisation::CurrentSceneVersion < 26)
            {
                archive(sprite.SpriteSheetTileSizeX);
                sprite.SpriteSheetTileSizeY = sprite.SpriteSheetTileSizeX;
            }

            if(Serialisation::CurrentSceneVersion > 25)
            {
                archive(sprite.SpriteSheetTileSizeX);
                archive(sprite.SpriteSheetTileSizeY);
            }

            sprite.SetState(sprite.m_State);
        }

        template <typename Archive>
        void save(Archive& archive, const Graphics::Model& model)
        {
            if(model.m_Meshes.Size() > 0)
            {
                ArenaTemp temp = ScratchBegin(nullptr, 0);

                String8 newPath;
                FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(model.m_FilePath), newPath);

                auto material = std::unique_ptr<Material>(model.m_Meshes.Front()->GetMaterial().get());
                archive(cereal::make_nvp("PrimitiveType", model.m_PrimitiveType), cereal::make_nvp("FilePath", newPath), cereal::make_nvp("Material", material));
                material.release();
                ScratchEnd(temp);
            }
        }

        template <typename Archive>
        void load(Archive& archive, Graphics::Model& model)
        {
            auto material = std::unique_ptr<Graphics::Material>();

            archive(cereal::make_nvp("PrimitiveType", model.m_PrimitiveType), cereal::make_nvp("FilePath", model.m_FilePath), cereal::make_nvp("Material", material));

            model.m_Meshes.Clear();

            if(model.m_PrimitiveType != PrimitiveType::File)
            {
                model.m_Meshes.PushBack(SharedPtr<Mesh>(CreatePrimative(model.m_PrimitiveType)));
                model.m_Meshes.Back()->SetMaterial(SharedPtr<Material>(material.get()));
                material.release();
            }
            else
            {
                model.LoadModel(model.m_FilePath);
                // TODO: This should load material changes from editor
                // m_Meshes.back()->SetMaterial(SharedPtr<Material>(material.get()));
                // material.release();
            }
        }

        template <typename Archive>
        void save(Archive& archive, const ModelComponent& component)
        {
            if(!component.ModelRef || component.ModelRef->GetMeshes().Size() == 0)
                return;

            ArenaTemp temp = ScratchBegin(nullptr, 0);
            {
                String8 newPath;

                if(component.ModelRef->GetPrimitiveType() == PrimitiveType::File)
                    FileSystem::Get().AbsolutePathToFileSystem(temp.arena, Str8StdS(component.ModelRef->GetFilePath()), newPath);
                else
                    newPath = Str8Lit("Primitive");

                // For now this saved material will be overriden by materials in the model file
                auto material = std::unique_ptr<Material>(component.ModelRef->GetMeshes().Front()->GetMaterial().get());
                archive(cereal::make_nvp("PrimitiveType", component.ModelRef->GetPrimitiveType()), cereal::make_nvp("FilePath", newPath), cereal::make_nvp("Material", material));
                material.release();
            }
            ScratchEnd(temp);
        }

        template <typename Archive>
        void load(Archive& archive, ModelComponent& component)
        {
            auto material = std::unique_ptr<Graphics::Material>();

            std::string filePath;
            PrimitiveType primitiveType;

            archive(cereal::make_nvp("PrimitiveType", primitiveType), cereal::make_nvp("FilePath", filePath), cereal::make_nvp("Material", material));

            if(primitiveType != PrimitiveType::File)
            {
                component.ModelRef = CreateSharedPtr<Model>(primitiveType);
                component.ModelRef->GetMeshes().Back()->SetMaterial(SharedPtr<Material>(material.get()));
                material.release();
            }
            else
            {
                component.LoadFromLibrary(filePath);
            }
        }
    }

    namespace Maths
    {
        template <typename Archive>
        void save(Archive& archive, const Maths::Transform& transform)
        {
            archive(cereal::make_nvp("Position", transform.m_LocalPosition), cereal::make_nvp("Rotation", transform.m_LocalOrientation), cereal::make_nvp("Scale", transform.m_LocalScale));
        }

        template <typename Archive>
        void load(Archive& archive, Maths::Transform& transform)
        {
            archive(cereal::make_nvp("Position", transform.m_LocalPosition), cereal::make_nvp("Rotation", transform.m_LocalOrientation), cereal::make_nvp("Scale", transform.m_LocalScale));
        }

    }
}

#endif

namespace Lumos
{
    void SerialiseAssetRegistry(const String8& path, const AssetRegistry& registry);
    void DeserialiseAssetRegistry(const String8& path, AssetRegistry& registry);
}
