#include "TipsPanel.h"
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Core/OS/Store.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/UIDock.h>
#include <Lumos/Graphics/UITheme.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

namespace Lumos
{
    namespace
    {
        // consumables in App Store Connect, smallest first
        const char* kTipIds[]  = { "com.jmorton.lumos.tip.small", "com.jmorton.lumos.tip.medium", "com.jmorton.lumos.tip.large" };
        const char* kTipName[] = { "Coffee", "Lunch", "Dinner" };
        constexpr int kTipCount = 3;

        enum class State
        {
            Idle,
            Loading,
            Ready,
            Buying,
            Thanks,
            Failed
        };
        State s_State      = State::Idle;
        char s_Message[160] = { 0 };
        TDArray<Store::Product> s_Products;
    }

    void TipsPanel::Update()
    {
        if(s_State == State::Idle || !Store::IsSupported())
            return;
        TDArray<Store::Event> events;
        Store::Poll(events);
        for(auto& e : events)
        {
            if(e.kind == "products")
            {
                s_Products.Clear();
                Store::GetProducts(s_Products);
                if(s_State == State::Loading)
                    s_State = s_Products.Empty() ? State::Failed : State::Ready;
                if(s_Products.Empty())
                    snprintf(s_Message, sizeof(s_Message), "The App Store has no tips to show right now.");
            }
            else if(e.kind == "purchased")
                s_State = State::Thanks;
            else if(e.kind == "cancelled")
                s_State = State::Ready;
            else if(e.kind == "pending")
            {
                s_State = State::Ready;
                snprintf(s_Message, sizeof(s_Message), "Waiting for approval. Thank you!");
            }
            else if(e.kind == "failed")
            {
                s_State = State::Ready;
                snprintf(s_Message, sizeof(s_Message), "%s", e.message.empty() ? "The purchase did not go through." : e.message.c_str());
            }
        }
    }

    void TipsPanel::Draw(const UIDockPanelContext& context)
    {
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = context.DPIScale;
        const f32 width          = Maths::Max(80.0f, context.Size.x / dpi - 16.0f);

        UISpacer(6.0f);
        UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.3f * dpi);
        UILabel("##tipstitle", Str8Lit(ICON_MDI_HEART "  Support Lumos"));
        UIPopStyle(StyleVar_FontSize);
        UISpacer(4.0f);
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        UILabelWrapped("##tipsbody", Str8Lit("Lumos is free and made by one person in their spare time. If it helps you make something, a tip keeps it going. Tips unlock nothing - everything is already yours."), width);
        UIPopStyle(StyleVar_TextColor);
        UISpacer(8.0f);

        if(!Store::IsSupported())
        {
            if(UIButton(ICON_MDI_GITHUB_CIRCLE "  GitHub Sponsors###tipsgithub").clicked)
                OS::Get().OpenURL("https://github.com/sponsors/jmorton06");
            return;
        }

        if(s_State == State::Idle)
        {
            TDArray<std::string> ids;
            for(int i = 0; i < kTipCount; i++)
                ids.PushBack(kTipIds[i]);
            Store::Request(ids);
            s_State = State::Loading;
        }

        if(s_State == State::Thanks)
        {
            UIPushStyle(StyleVar_TextColor, theme.Accent);
            UILabelWrapped("##tipsthanks", Str8Lit(ICON_MDI_HEART "  Thank you! That really helps."), width);
            UIPopStyle(StyleVar_TextColor);
            UISpacer(6.0f);
            if(UIButton("Tip again###tipsagain").clicked)
                s_State = State::Ready;
            return;
        }
        if(s_State == State::Loading)
        {
            UILabel("##tipsloading", Str8Lit("Asking the App Store..."));
            return;
        }

        // in our order, not the store's
        char label[160];
        for(int i = 0; i < kTipCount; i++)
        {
            const Store::Product* product = nullptr;
            for(auto& p : s_Products)
                if(p.id == kTipIds[i])
                    product = &p;
            if(!product)
                continue;
            snprintf(label, sizeof(label), "%s  %s###tip%d", kTipName[i], product->price.c_str(), i);
            const bool busy = s_State == State::Buying;
            UIPushStyle(StyleVar_TextColor, busy ? theme.TextDim : theme.Text);
            if(UIButton(label).clicked && !busy)
            {
                s_Message[0] = 0;
                s_State      = State::Buying;
                Store::Buy(product->id);
            }
            UIPopStyle(StyleVar_TextColor);
        }
        if(s_State == State::Buying)
            UILabel("##tipsbuying", Str8Lit("Waiting for the App Store..."));
        if(s_Message[0])
        {
            UISpacer(4.0f);
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelWrapped("##tipsmessage", Str8C(s_Message), width);
            UIPopStyle(StyleVar_TextColor);
        }
        if(s_State == State::Failed && UIButton("Try again###tipsretry").clicked)
            s_State = State::Idle;
    }
}
