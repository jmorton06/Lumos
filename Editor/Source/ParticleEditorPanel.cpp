#include "ParticleEditorPanel.h"
#include "Editor.h"

#include <Lumos/Graphics/ParticleManager.h>
#include <Lumos/Scene/Entity.h>

#include <imgui/imgui.h>

namespace Lumos
{
    ParticleEditorPanel::ParticleEditorPanel()
    {
        m_Name       = "Particle Editor###ParticleEditor";
        m_SimpleName = "Particle Editor";
    }

    void ParticleEditorPanel::OnImGui()
    {
        ImGuiUtilities::BeginPanel(m_Name.c_str(), &m_Active, ImGuiWindowFlags_NoCollapse);

        if(!m_Editor)
        {
            ImGui::Text("Editor not bound.");
            ImGui::End();
            return;
        }

        const auto& sel = m_Editor->GetSelected();
        if(sel.empty())
        {
            ImGui::TextDisabled("Select an entity with a ParticleEmitter component.");
            ImGui::End();
            return;
        }

        Entity entity        = sel[0];
        ParticleEmitter* emi = entity.TryGetComponent<ParticleEmitter>();
        if(!emi)
        {
            ImGui::TextDisabled("Selected entity has no ParticleEmitter.");
            if(ImGui::Button("Add ParticleEmitter"))
                entity.AddComponent<ParticleEmitter>();
            ImGui::End();
            return;
        }

        ImGui::TextDisabled("Live tuning - changes apply immediately.");
        ImGui::Separator();

        int count = (int)emi->GetParticleCount();
        if(ImGui::SliderInt("Particle Count", &count, 1, 8192))
            emi->SetParticleCount((uint32_t)count);

        float life = emi->GetParticleLife();
        if(ImGui::SliderFloat("Particle Life (s)", &life, 0.1f, 30.0f))
            emi->SetParticleLife(life);

        float lifeSpread = emi->GetLifeSpread();
        if(ImGui::SliderFloat("Life Spread", &lifeSpread, 0.0f, 5.0f))
            emi->SetLifeSpread(lifeSpread);

        float size = emi->GetParticleSize();
        if(ImGui::SliderFloat("Particle Size", &size, 0.001f, 10.0f))
            emi->SetParticleSize(size);

        float rate = emi->GetParticleRate();
        if(ImGui::SliderFloat("Emit Rate (s)", &rate, 0.0f, 5.0f))
            emi->SetParticleRate(rate);

        int launch = (int)emi->GetNumLaunchParticles();
        if(ImGui::SliderInt("Particles per Emit", &launch, 1, 256))
            emi->SetNumLaunchParticles((uint32_t)launch);

        ImGui::Separator();

        Vec3 vel = emi->GetInitialVelocity();
        if(ImGui::DragFloat3("Initial Velocity", &vel.x, 0.05f))
            emi->SetInitialVelocity(vel);

        Vec3 velSpread = emi->GetVelocitySpread();
        if(ImGui::DragFloat3("Velocity Spread", &velSpread.x, 0.05f))
            emi->SetVelocitySpread(velSpread);

        Vec3 spread = emi->GetSpread();
        if(ImGui::DragFloat3("Position Spread", &spread.x, 0.05f))
            emi->SetSpread(spread);

        Vec3 gravity = emi->GetGravity();
        if(ImGui::DragFloat3("Gravity", &gravity.x, 0.05f))
            emi->SetGravity(gravity);

        ImGui::Separator();

        Vec4 col = emi->GetInitialColour();
        if(ImGui::ColorEdit4("Initial Colour", &col.x))
            emi->SetInitialColour(col);

        float fadeIn = emi->GetFadeIn();
        if(ImGui::DragFloat("Fade In", &fadeIn, 0.01f, -1.0f, 30.0f))
            emi->SetFadeIn(fadeIn);

        float fadeOut = emi->GetFadeOut();
        if(ImGui::DragFloat("Fade Out", &fadeOut, 0.01f, -1.0f, 30.0f))
            emi->SetFadeOut(fadeOut);

        ImGui::Separator();

        const char* blendItems[] = { "Additive", "Alpha", "Off" };
        int blendIdx             = (int)emi->GetBlendType();
        if(ImGui::Combo("Blend", &blendIdx, blendItems, IM_ARRAYSIZE(blendItems)))
            emi->SetBlendType((ParticleEmitter::BlendType)blendIdx);

        const char* alignItems[] = { "Aligned2D", "Aligned3D", "None" };
        int alignIdx             = (int)emi->GetAlignedType();
        if(ImGui::Combo("Align", &alignIdx, alignItems, IM_ARRAYSIZE(alignItems)))
            emi->SetAlignedType((ParticleEmitter::AlignedType)alignIdx);

        bool depthWrite = emi->GetDepthWrite();
        if(ImGui::Checkbox("Depth Write", &depthWrite))
            emi->SetDepthWrite(depthWrite);

        bool sort = emi->GetSortParticles();
        if(ImGui::Checkbox("Sort Particles", &sort))
            emi->SetSortParticles(sort);

        ImGui::Separator();

        bool animated = emi->GetIsAnimated();
        if(ImGui::Checkbox("Animated Texture", &animated))
            emi->SetIsAnimated(animated);

        if(animated)
        {
            int rows = (int)emi->GetAnimatedTextureRows();
            if(ImGui::SliderInt("Rows", &rows, 1, 16))
                emi->SetAnimatedTextureRows((uint32_t)rows);
        }

        ImGui::Separator();
        static char pathBuf[512] = "";
        ImGui::InputText("Texture Path", pathBuf, sizeof(pathBuf));
        ImGui::SameLine();
        if(ImGui::Button("Load"))
            emi->SetTextureFromFile(pathBuf);

        ImGui::End();
    }
}
