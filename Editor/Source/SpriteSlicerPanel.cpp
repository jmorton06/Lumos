#include "SpriteSlicerPanel.h"
#include "Editor.h"

#include <Lumos/Core/Application.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/AnimatedSprite.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/ImGui/ImGuiManager.h>
#include <Lumos/Graphics/RHI/IMGUIRenderer.h>

#include <imgui/imgui.h>
#include <vector>

namespace Lumos
{
    SpriteSlicerPanel::SpriteSlicerPanel()
    {
        m_Name       = "Sprite Slicer###SpriteSlicer";
        m_SimpleName = "Sprite Slicer";
    }

    void SpriteSlicerPanel::LoadTexture(const std::string& path)
    {
        Graphics::TextureDesc desc;
        desc.minFilter = Graphics::TextureFilter::NEAREST;
        desc.magFilter = Graphics::TextureFilter::NEAREST;
        m_Texture      = SharedPtr<Graphics::Texture2D>(
            Graphics::Texture2D::CreateFromFile("SpriteSheet", path, desc));
        m_TexturePath = path;
        m_GeneratedSummary.clear();
    }

    void SpriteSlicerPanel::GenerateFrames()
    {
        int total = m_FrameCount > 0 ? m_FrameCount : (m_Rows * m_Cols);
        if(total > m_Rows * m_Cols)
            total = m_Rows * m_Cols;

        std::string s;
        s.reserve(64 * total + 64);
        s += "AnimationState \"" + m_StateName + "\"\n";
        s += "  Mode: " + std::string(m_PingPong ? "PingPong" : "Loop") + "\n";
        s += "  FrameDuration: " + std::to_string(m_FrameTime) + "\n";
        s += "  Frames (UV cells, normalised 0..1):\n";

        for(int i = 0; i < total; ++i)
        {
            int col   = i % m_Cols;
            int row   = i / m_Cols;
            float u   = (float)col / (float)m_Cols;
            float v   = (float)row / (float)m_Rows;
            char line[96];
            snprintf(line, sizeof(line), "    [%d] = (%.4f, %.4f)\n", i, u, v);
            s += line;
        }
        m_GeneratedSummary = s;
    }

    void SpriteSlicerPanel::OnImGui()
    {
        ImGuiUtilities::BeginPanel(m_Name.c_str(), &m_Active, ImGuiWindowFlags_NoCollapse);

        static char pathBuf[512] = "";
        ImGui::InputText("Texture Path", pathBuf, sizeof(pathBuf));
        ImGui::SameLine();
        if(ImGui::Button("Load"))
            LoadTexture(pathBuf);

        if(m_Texture)
        {
            ImGui::Separator();
            ImGui::Text("%s (%ux%u)", m_TexturePath.c_str(),
                        m_Texture->GetWidth(), m_Texture->GetHeight());

            ImGui::SliderInt("Rows", &m_Rows, 1, 32);
            ImGui::SliderInt("Cols", &m_Cols, 1, 32);
            ImGui::InputInt("Frame Count (0 = all)", &m_FrameCount);
            if(m_FrameCount < 0) m_FrameCount = 0;
            ImGui::InputFloat("Frame Duration", &m_FrameTime, 0.01f, 0.1f, "%.3f s");
            ImGui::Checkbox("Ping-Pong", &m_PingPong);

            static char stateBuf[128];
            strncpy(stateBuf, m_StateName.c_str(), sizeof(stateBuf) - 1);
            stateBuf[sizeof(stateBuf) - 1] = 0;
            if(ImGui::InputText("State Name", stateBuf, sizeof(stateBuf)))
                m_StateName = stateBuf;

            ImGui::Separator();

            // Preview with grid overlay
            ImVec2 imgSize(256.0f, 256.0f);
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::Image(reinterpret_cast<ImTextureID>(
                             Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(m_Texture)),
                         imgSize, ImVec2(0, 0), ImVec2(1, 1));

            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImU32 col      = IM_COL32(255, 0, 0, 200);
            for(int c = 1; c < m_Cols; ++c)
            {
                float x = cursor.x + imgSize.x * ((float)c / m_Cols);
                dl->AddLine(ImVec2(x, cursor.y), ImVec2(x, cursor.y + imgSize.y), col);
            }
            for(int r = 1; r < m_Rows; ++r)
            {
                float y = cursor.y + imgSize.y * ((float)r / m_Rows);
                dl->AddLine(ImVec2(cursor.x, y), ImVec2(cursor.x + imgSize.x, y), col);
            }

            ImGui::Separator();
            if(ImGui::Button("Generate Frames"))
                GenerateFrames();

            ImGui::SameLine();
            bool canApply = false;
            Entity selected;
            if(m_Editor)
            {
                auto& sel = m_Editor->GetSelected();
                if(!sel.empty())
                {
                    selected = sel[0];
                    canApply = selected.Valid();
                }
            }
            ImGui::BeginDisabled(!canApply);
            if(ImGui::Button("Apply to Selected Entity"))
            {
                std::vector<Vec2> frames;
                int total = m_FrameCount > 0 ? m_FrameCount : (m_Rows * m_Cols);
                if(total > m_Rows * m_Cols) total = m_Rows * m_Cols;
                frames.reserve(total);
                for(int i = 0; i < total; ++i)
                {
                    int col_i = i % m_Cols;
                    int row_i = i / m_Cols;
                    frames.emplace_back((float)col_i / (float)m_Cols,
                                        (float)row_i / (float)m_Rows);
                }

                auto* sprite = selected.TryGetComponent<Graphics::AnimatedSprite>();
                if(!sprite)
                    sprite = &selected.AddComponent<Graphics::AnimatedSprite>();
                sprite->SetTexture(m_Texture);
                sprite->AddState(frames, m_FrameTime, m_StateName);
                sprite->SetState(m_StateName);
            }
            ImGui::EndDisabled();

            if(!m_GeneratedSummary.empty())
            {
                ImGui::Separator();
                ImGui::TextWrapped("%s", m_GeneratedSummary.c_str());
            }
        }
        else
        {
            ImGui::TextDisabled("Enter a path to a sprite-sheet texture and click Load.");
        }

        ImGui::End();
    }
}
