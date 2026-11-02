#include "Precompiled.h"
#include "BusyOverlay.h"

#include "Core/Application.h"
#include "Core/Engine.h"
#include "Graphics/UI.h"
#include "Graphics/UIInternal.h"
#include "Graphics/RHI/Texture.h"
#include "Maths/MathsUtilities.h"
#include "Utilities/Timer.h"

namespace Lumos
{
    namespace Busy
    {
        namespace
        {
            // Fixed buffers: nothing here allocates, so it is safe to draw mid-load.
            char s_Label[96]  = { 0 };
            char s_Step[160]  = { 0 };
            float s_Progress  = -1.0f;
            i32 s_Depth       = 0;
            float s_Time      = 0.0f;
            // wall clock: the engine's frame timer does not tick while a load holds the main loop
            TimeStamp s_LastDraw;
            bool s_HasLastDraw = false;

            constexpr u32 kMaxQueued = 8;
            Function<void()> s_Queued[kMaxQueued];
            u32 s_QueuedCount = 0;

            void Copy(char* dst, size_t size, const char* src)
            {
                size_t n = 0;
                if(src)
                    for(; src[n] && n < size - 1; n++)
                        dst[n] = src[n];
                dst[n] = 0;
            }
        }

        void Begin(const char* label)
        {
            if(s_Depth == 0)
            {
                Copy(s_Label, sizeof(s_Label), label ? label : "Working");
                s_Step[0]  = 0;
                s_Progress = -1.0f;
                s_Time     = 0.0f;
            }
            s_Depth++;
        }

        void End()
        {
            if(s_Depth > 0)
                s_Depth--;
        }

        bool IsBusy() { return s_Depth > 0; }

        void SetLabel(const char* label) { Copy(s_Label, sizeof(s_Label), label); }
        void SetStep(const char* step) { Copy(s_Step, sizeof(s_Step), step); }
        void SetProgress(float progress) { s_Progress = progress; }

        const char* GetLabel() { return s_Label; }
        const char* GetStep() { return s_Step; }
        float GetProgress() { return s_Progress; }

        void Pump()
        {
            if(s_Depth > 0)
                Application::Get().PumpBusyFrame();
        }

        void Run(const char* label, const Function<void()>& work)
        {
            // parked here: a Function is too big to capture in another one
            if(s_QueuedCount >= kMaxQueued)
            {
                work();
                return;
            }
            Begin(label);
            s_Queued[s_QueuedCount++] = work;

            Application::Get().SubmitToMainThread([]()
                                                  {
                                                      if(s_QueuedCount == 0)
                                                          return;
                                                      Function<void()> job = s_Queued[0];
                                                      for(u32 i = 1; i < s_QueuedCount; i++)
                                                          s_Queued[i - 1] = s_Queued[i];
                                                      s_QueuedCount--;

                                                      // one frame up before the work starts
                                                      Application::Get().PumpBusyFrame(true);
                                                      job();
                                                      Busy::End(); });
        }

        namespace
        {
            // A thin rounded bar. progress < 0 = not known: a short piece runs along it instead.
            void Bar(u64 id, float width, float height, float progress, const Vec4& track, const Vec4& fill)
            {
                UI_Widget* back = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), id, { SizeKind_Pixels, width }, { SizeKind_Pixels, height });
                UIClearBorders(back);
                back->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                back->style_vars[StyleVar_CornerRadius]    = Vec4(height * 0.5f, 0.0f, 0.0f, 0.0f);
                back->style_vars[StyleVar_BackgroundColor] = track;

                float from = 0.0f, to = Maths::Clamp(progress, 0.0f, 1.0f);
                if(progress < 0.0f)
                {
                    // eases across and wraps: in at the left as it leaves the right
                    const float piece = 0.32f;
                    float t           = fmodf(s_Time * 0.75f, 1.0f);
                    t                 = t * t * (3.0f - 2.0f * t);
                    const float head  = t * (1.0f + piece);
                    from              = Maths::Clamp(head - piece, 0.0f, 1.0f);
                    to                = Maths::Clamp(head, 0.0f, 1.0f);
                }
                const float length = floorf((to - from) * width);
                if(length >= 1.0f)
                {
                    PushParent(back);
                    UI_Widget* front = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""), UIHashCombine(id, 1),
                                                  { SizeKind_Pixels, Maths::Max(length, height) }, { SizeKind_Pixels, height });
                    UIClearBorders(front);
                    front->relative_position                    = Vec2(floorf(Maths::Min(from * width, width - Maths::Max(length, height))), 0.0f);
                    front->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                    front->style_vars[StyleVar_CornerRadius]    = Vec4(height * 0.5f, 0.0f, 0.0f, 0.0f);
                    front->style_vars[StyleVar_BackgroundColor] = fill;
                    PopParent(back);
                }
            }

            void Label(const char* id, const char* text, float size, const Vec4& colour)
            {
                UIPushStyle(StyleVar_FontSize, size);
                UIPushStyle(StyleVar_TextColor, colour);
                UILabelCStr(id, text);
                UIPopStyle(StyleVar_TextColor);
                UIPopStyle(StyleVar_FontSize);
            }
        }

        void Draw(bool opaque)
        {
            UI_State* ui    = GetUIState();
            const float dpi = ui->DPIScale;

            const TimeStamp now = Timer::Now();
            const float dt      = s_HasLastDraw ? Maths::Clamp(Timer::Duration(s_LastDraw, now, 1.0f), 0.0f, 0.1f) : 0.016f;
            s_LastDraw          = now;
            s_HasLastDraw       = true;
            s_Time += dt;

            const Vec4 text  = Vec4(0.94f, 0.95f, 0.97f, 1.0f);
            const Vec4 dim   = Vec4(0.62f, 0.65f, 0.71f, 1.0f);
            const Vec4 track = Vec4(1.0f, 1.0f, 1.0f, 0.10f);
            const Vec4 fill  = Vec4(0.93f, 0.94f, 0.96f, 0.92f);

            if(opaque)
            {
                // Nothing behind it (startup, a scene switch from nothing): carry on from the splash screen -
                // its colour edge to edge and its picture where it was, with the bar under it. No box.
                Application& app      = Application::Get();
                const float* splash   = app.GetProjectSettings().SplashBGColour;
                const Vec4 background = Vec4(splash[0], splash[1], splash[2], 1.0f);
                const Vec2 view       = ui->root_parent.size;

                UI_Widget* page = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_StackVertically | WidgetFlags_CentreChildrenX
                                                 | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost,
                                             Str8Lit(""), 0x4255535950414745ull, { SizeKind_Pixels, view.x }, { SizeKind_Pixels, view.y });
                UIClearBorders(page);
                page->relative_position                 = Vec2(0.0f, 0.0f);
                page->style_vars[StyleVar_Padding]      = Vec4(0.0f);
                page->style_vars[StyleVar_ItemSpacing]  = Vec4(0.0f);
                page->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
                UISetBackground(page, background, background, background);
                HandleWidgetInteraction(page);
                PushParent(page);

                // the same place and size the splash screen drew it (a quarter of a wide window, half of a tall one)
                Graphics::Texture2D* picture = app.GetSplashTexture();
                float pictureW               = floorf(view.y > view.x ? view.x * 0.5f : view.x * 0.25f);
                float pictureH               = pictureW;
                if(picture && picture->GetWidth() > 0)
                    pictureH = floorf(pictureW * (float)picture->GetHeight() / (float)picture->GetWidth());
                UISpacer(floorf((view.y - pictureH) * 0.5f) / dpi);
                if(picture)
                    UIImageSized("##busysplash", picture, Vec2(pictureW, pictureH));
                else
                    UISpacer(pictureH / dpi);

                // light on a dark splash, dark on a light one
                const bool lightPage = (0.299f * background.x + 0.587f * background.y + 0.114f * background.z) > 0.6f;
                const Vec4 ink       = lightPage ? Vec4(0.10f, 0.11f, 0.13f, 0.85f) : fill;
                const Vec4 faint     = lightPage ? Vec4(0.0f, 0.0f, 0.0f, 0.10f) : track;

                UISpacer(22.0f);
                Bar(0x425553594241ull, floorf(Maths::Min(220.0f * dpi, view.x * 0.5f)), Maths::Max(2.0f, floorf(3.0f * dpi)), s_Progress, faint, ink);
                UISpacer(12.0f);
                Label("##busylabel", s_Step[0] ? s_Step : s_Label, 12.5f * dpi, lightPage ? Vec4(0.25f, 0.27f, 0.30f, 1.0f) : dim);
                PopParent(page);
                return;
            }

            // Over whatever is on screen: dim it, and a small card in the middle.
            UIDimBackdrop("##busybackdrop", 0.5f);

            const float width = floorf(300.0f * dpi);
            const float pad   = floorf(20.0f * dpi);
            UIPushStyle(StyleVar_BackgroundColor, Vec4(0.11f, 0.12f, 0.15f, 0.98f));
            UIPushStyle(StyleVar_Padding, Vec2(pad, floorf(18.0f * dpi)));
            UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 0.0f));
            UIPushStyle(StyleVar_CornerRadius, 14.0f * dpi);
            UIBeginOverlay("##busypanel", SizeKind_Pixels, width, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically | WidgetFlags_TopMost);
            UIWindowAnchor(UIAnchor_MiddleCenter);
            UIPopStyle(StyleVar_CornerRadius);
            UIPopStyle(StyleVar_ItemSpacing);
            UIPopStyle(StyleVar_Padding);
            UIPopStyle(StyleVar_BackgroundColor);

            Label("##busylabel", s_Label, 14.5f * dpi, text);
            UISpacer(12.0f);
            Bar(0x425553594241ull, width - pad * 2.0f, Maths::Max(3.0f, floorf(4.0f * dpi)), s_Progress, track, fill);
            if(s_Step[0])
            {
                UISpacer(10.0f);
                UIPushStyle(StyleVar_FontSize, 12.0f * dpi);
                UIPushStyle(StyleVar_TextColor, dim);
                UILabelEllipsis("##busystep", Str8C(s_Step), (width - pad * 2.0f) / dpi);
                UIPopStyle(StyleVar_TextColor);
                UIPopStyle(StyleVar_FontSize);
            }
            UIEndPanel();
        }
    }
}
