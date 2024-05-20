VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["entt"] = "%{wks.location}/Lumos/External/entt/src/"
IncludeDir["GLFW"] = "%{wks.location}/Lumos/External/glfw/include/"
IncludeDir["Glad"] = "%{wks.location}/Lumos/External/glad/include/"
IncludeDir["lua"] = "%{wks.location}/Lumos/External/lua/src/"
IncludeDir["stb"] = "%{wks.location}/Lumos/External/stb/"
IncludeDir["OpenAL"] = "%{wks.location}/Lumos/External/OpenAL/include/"
IncludeDir["Box2D"] = "%{wks.location}/Lumos/External/box2d/include/"
IncludeDir["vulkan"] = "%{wks.location}/Lumos/External/vulkan/"
IncludeDir["Lumos"] = "%{wks.location}/Lumos/Source"
IncludeDir["External"] = "%{wks.location}/Lumos/External/"
IncludeDir["ImGui"] = "%{wks.location}/Lumos/External/imgui/"
IncludeDir["freetype"] = "%{wks.location}/Lumos/External/freetype/include"
IncludeDir["SpirvCross"] = "%{wks.location}/Lumos/External/vulkan/SPIRV-Cross"
IncludeDir["cereal"] = "%{wks.location}/Lumos/External/cereal/include"
IncludeDir["spdlog"] = "%{wks.location}/Lumos/External/spdlog/include"
IncludeDir["glm"] = "%{wks.location}/Lumos/External/glm"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Lumos/External/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["msdfgen"] = "%{wks.location}/Lumos/External/msdf-atlas-gen/msdfgen"
IncludeDir["ozz"] = "%{wks.location}/Lumos/External/ozz-animation/include"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
