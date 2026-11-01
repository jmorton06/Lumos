#include "Precompiled.h"
#include "SpriteEditor.h"
#include "Core/ToolPanels.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/UI.h"
#include "Graphics/UIInternal.h"
#include "Graphics/UIDock.h"
#include "Graphics/UITheme.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Physics/B2PhysicsEngine/RigidBody2D.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Entity.h"
#include "Scene/SceneUndo.h"
#include "Scene/EntityManager.h"
#include "Maths/Transform.h"
#include "Maths/MathsUtilities.h"
#include "Utilities/LoadImage.h"
#include "Utilities/ImageExport.h"
#include "ImGui/IconsMaterialDesignIcons.h"

#include <filesystem>

namespace Lumos
{
    namespace SpriteEditor
    {
        namespace
        {
            enum Tool : u8
            {
                Tool_Pencil = 0,
                Tool_Eraser,
                Tool_Fill,
                Tool_Line,
                Tool_Rect,
                Tool_Picker,
                Tool_Hand,
                Tool_Count
            };

            struct ToolInfo
            {
                const char* Icon;
                const char* Name;
                InputCode::Key Key;
            };

            const ToolInfo kTools[Tool_Count] = {
                { ICON_MDI_PENCIL, "Pencil (B)", InputCode::Key::B },
                { ICON_MDI_ERASER, "Eraser (E)", InputCode::Key::E },
                { ICON_MDI_FORMAT_COLOR_FILL, "Fill (G)", InputCode::Key::G },
                { ICON_MDI_VECTOR_LINE, "Line (L)", InputCode::Key::L },
                { ICON_MDI_SQUARE_OUTLINE, "Rectangle (U)", InputCode::Key::U },
                { ICON_MDI_EYEDROPPER, "Pick colour (I)", InputCode::Key::I },
                { ICON_MDI_CURSOR_MOVE, "Pan (H, or hold Space)", InputCode::Key::H },
            };

            // PICO-8's sixteen, then sixteen of DawnBringer's 32 that it lacks
            const u32 kPalette[32] = {
                0x000000ff, 0x1d2b53ff, 0x7e2553ff, 0x008751ff, 0xab5236ff, 0x5f574fff, 0xc2c3c7ff, 0xfff1e8ff,
                0xff004dff, 0xffa300ff, 0xffec27ff, 0x00e436ff, 0x29adffff, 0x83769cff, 0xff77a8ff, 0xffccaaff,
                0x222034ff, 0x45283cff, 0x663931ff, 0x8f563bff, 0xdf7126ff, 0xd9a066ff, 0xeec39aff, 0xfbf236ff,
                0x99e550ff, 0x6abe30ff, 0x37946eff, 0x4b692fff, 0x524b24ff, 0x323c39ff, 0x3f3f74ff, 0x306082ff,
            };

            const u32 kMaxSide      = 2048;
            const u32 kUndoSteps    = 48;
            const u32 kMaxShapeDots = 64;

            struct Animation
            {
                char Name[32] = "idle";
                i32 First     = 0;
                i32 Count     = 1;
                float Fps     = 8.0f;
                bool PingPong = false;
            };

            struct State
            {
                UIDockSpace* Dock = nullptr;

                // the picture
                u32 Width  = 0;
                u32 Height = 0;
                TDArray<u8> Pixels;
                Graphics::Texture2D* Texture = nullptr;
                u32 TextureWidth             = 0;
                u32 TextureHeight            = 0;
                bool Upload                  = false;
                bool Dirty                   = false;
                char Path[512]               = { 0 }; // VFS
                char Physical[768]           = { 0 }; // where it is written
                char Status[160]             = { 0 };
                float StatusTime             = 0.0f;

                // view
                float Zoom   = 8.0f;
                Vec2 Pan     = Vec2(0.0f); // picture's top-left inside the canvas, px
                bool Fit     = true;       // centre and size to the canvas next frame
                bool ShowGrid = true;

                // tools
                Tool Current   = Tool_Pencil;
                Tool BeforePan = Tool_Pencil;
                u32 Brush      = 1;
                bool MirrorX   = false;
                Vec4 Fore      = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
                Vec4 Back      = Vec4(1.0f, 1.0f, 1.0f, 0.0f);

                // a stroke in progress
                bool Stroking    = false;
                bool StrokeBack  = false; // drawing with the second colour (right button)
                bool Panning     = false;
                Vec2 PanFrom     = Vec2(0.0f);
                Vec2 PanStart    = Vec2(0.0f);
                i32 StartX       = 0;
                i32 StartY       = 0;
                i32 LastX        = 0;
                i32 LastY        = 0;
                TDArray<u8> StrokeBase; // the picture when the stroke began (line / rect previews redraw onto it)

                // two fingers
                bool Pinching    = false;
                float PinchDist  = 0.0f;
                Vec2 PinchMid    = Vec2(0.0f);

                // undo / redo: whole pictures, oldest first
                TDArray<TDArray<u8>> Undo;
                TDArray<TDArray<u8>> Redo;

                // collision shape, in picture pixels (y down)
                bool ShapeMode = false;
                TDArray<Vec2> Shape;
                i32 DragDot    = -1;

                // sprite sheet: the picture cut into cells (0 = the whole picture), frames numbered
                // along the rows; animations are runs of frames
                i32 CellW = 0;
                i32 CellH = 0;
                TDArray<Animation> Anims;
                i32 CurrentAnim = -1;
                bool Playing    = true;
                float AnimTime  = 0.0f;
                Graphics::Texture2D* FrameTexture = nullptr;
                u32 FrameTexW   = 0;
                u32 FrameTexH   = 0;
                i32 FrameShown  = -1;
                bool PickFrames  = false; // presses on the canvas choose the animation's cells
                i32 PickFrom     = -1;    // cell the drag began on
                bool BigPreview  = false;
            };

            State* s = nullptr;

            template <typename T>
            void RemoveAt(TDArray<T>& array, u32 index)
            {
                if(index >= (u32)array.Size())
                    return;
                for(u32 i = index; i + 1 < (u32)array.Size(); i++)
                    array[i] = std::move(array[i + 1]);
                array.PopBack();
            }

            State& St()
            {
                if(!s)
                    s = new State();
                return *s;
            }

            void SetStatus(const char* fmt, ...)
            {
                va_list args;
                va_start(args, fmt);
                vsnprintf(St().Status, sizeof(St().Status), fmt, args);
                va_end(args);
                St().StatusTime = 4.0f;
            }

            u32 Pack(const Vec4& c)
            {
                auto b = [](float v) { return (u32)Maths::Clamp(v * 255.0f + 0.5f, 0.0f, 255.0f); };
                return (b(c.x) << 24) | (b(c.y) << 16) | (b(c.z) << 8) | b(c.w);
            }

            Vec4 Unpack(u32 rgba)
            {
                return Vec4((float)((rgba >> 24) & 0xff) / 255.0f, (float)((rgba >> 16) & 0xff) / 255.0f, (float)((rgba >> 8) & 0xff) / 255.0f, (float)(rgba & 0xff) / 255.0f);
            }

            u32 Get(i32 x, i32 y)
            {
                State& st   = St();
                const u8* p = &st.Pixels[((u32)y * st.Width + (u32)x) * 4];
                return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
            }

            void Put(i32 x, i32 y, u32 rgba)
            {
                State& st = St();
                if(x < 0 || y < 0 || x >= (i32)st.Width || y >= (i32)st.Height)
                    return;
                u8* p = &st.Pixels[((u32)y * st.Width + (u32)x) * 4];
                p[0]  = (u8)(rgba >> 24);
                p[1]  = (u8)(rgba >> 16);
                p[2]  = (u8)(rgba >> 8);
                p[3]  = (u8)rgba;
                st.Upload = true;
                st.Dirty  = true;
            }

            // a brush dab, and its mirror image
            void Dab(i32 x, i32 y, u32 rgba)
            {
                State& st      = St();
                const i32 size = (i32)st.Brush;
                const i32 from = -(size - 1) / 2;
                for(i32 dy = 0; dy < size; dy++)
                    for(i32 dx = 0; dx < size; dx++)
                    {
                        Put(x + from + dx, y + from + dy, rgba);
                        if(st.MirrorX)
                            Put((i32)st.Width - 1 - (x + from + dx), y + from + dy, rgba);
                    }
            }

            void Line(i32 x0, i32 y0, i32 x1, i32 y1, u32 rgba)
            {
                const i32 dx = Maths::Abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
                const i32 dy = -Maths::Abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
                i32 err      = dx + dy;
                for(;;)
                {
                    Dab(x0, y0, rgba);
                    if(x0 == x1 && y0 == y1)
                        break;
                    const i32 e2 = 2 * err;
                    if(e2 >= dy)
                    {
                        err += dy;
                        x0 += sx;
                    }
                    if(e2 <= dx)
                    {
                        err += dx;
                        y0 += sy;
                    }
                }
            }

            void RectOutline(i32 x0, i32 y0, i32 x1, i32 y1, u32 rgba)
            {
                Line(x0, y0, x1, y0, rgba);
                Line(x1, y0, x1, y1, rgba);
                Line(x1, y1, x0, y1, rgba);
                Line(x0, y1, x0, y0, rgba);
            }

            void Fill(i32 x, i32 y, u32 rgba)
            {
                State& st = St();
                if(x < 0 || y < 0 || x >= (i32)st.Width || y >= (i32)st.Height)
                    return;
                const u32 target = Get(x, y);
                if(target == rgba)
                    return;
                TDArray<u32> stack;
                stack.PushBack((u32)y * st.Width + (u32)x);
                while(!stack.Empty())
                {
                    const u32 at = stack.Back();
                    stack.PopBack();
                    const i32 px = (i32)(at % st.Width), py = (i32)(at / st.Width);
                    if(Get(px, py) != target)
                        continue;
                    Put(px, py, rgba);
                    if(px > 0) stack.PushBack(at - 1);
                    if(px + 1 < (i32)st.Width) stack.PushBack(at + 1);
                    if(py > 0) stack.PushBack(at - st.Width);
                    if(py + 1 < (i32)st.Height) stack.PushBack(at + st.Width);
                }
            }

            void PushUndo()
            {
                State& st = St();
                if(st.Undo.Size() >= kUndoSteps)
                    RemoveAt(st.Undo, 0);
                st.Undo.PushBack(st.Pixels);
                st.Redo.Clear();
            }

            void StepHistory(bool redo)
            {
                State& st                 = St();
                TDArray<TDArray<u8>>& from = redo ? st.Redo : st.Undo;
                TDArray<TDArray<u8>>& to   = redo ? st.Undo : st.Redo;
                if(from.Empty())
                    return;
                to.PushBack(st.Pixels);
                st.Pixels = from.Back();
                from.PopBack();
                st.Upload = true;
                st.Dirty  = true;
            }

            void Resize(u32 width, u32 height)
            {
                State& st = St();
                st.Width  = width;
                st.Height = height;
                st.Pixels.Resize(width * height * 4);
                MemorySet(st.Pixels.Data(), 0, st.Pixels.Size());
                st.Undo.Clear();
                st.Redo.Clear();
                st.Shape.Clear();
                st.Upload = true;
                st.Fit    = true;
            }

            void EnsureTexture()
            {
                State& st = St();
                if(st.Width == 0)
                    return;
                if(!st.Texture || st.TextureWidth != st.Width || st.TextureHeight != st.Height)
                {
                    delete st.Texture;
                    Graphics::TextureDesc desc;
                    desc.format          = Graphics::RHIFormat::R8G8B8A8_Unorm;
                    desc.minFilter       = Graphics::TextureFilter::NEAREST;
                    desc.magFilter       = Graphics::TextureFilter::NEAREST;
                    desc.wrap            = Graphics::TextureWrap::CLAMP_TO_EDGE;
                    desc.generateMipMaps = false;
                    desc.srgb            = false;
                    st.Texture           = Graphics::Texture2D::CreateFromSource(st.Width, st.Height, st.Pixels.Data(), desc);
                    st.TextureWidth      = st.Width;
                    st.TextureHeight     = st.Height;
                    st.Upload            = false;
                }
                else if(st.Upload)
                {
                    st.Texture->SetData(st.Pixels.Data());
                    st.Upload = false;
                }
            }

            // ---- collision shape ----

            float Cross(const Vec2& o, const Vec2& a, const Vec2& b)
            {
                return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
            }

            // Box2D wants a convex polygon of at most 8 corners: the hull, then the corners that
            // matter least taken away
            TDArray<Vec2> PhysicsPolygon(const TDArray<Vec2>& points)
            {
                TDArray<Vec2> sorted = points;
                TDArray<Vec2> hull;
                if(sorted.Size() < 3)
                    return hull;
                std::sort(&sorted[0], &sorted[0] + sorted.Size(), [](const Vec2& a, const Vec2& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
                for(u32 pass = 0; pass < 2; pass++)
                {
                    const u32 start = (u32)hull.Size();
                    for(u32 i = 0; i < (u32)sorted.Size(); i++)
                    {
                        const Vec2& p = pass == 0 ? sorted[i] : sorted[(u32)sorted.Size() - 1 - i];
                        while(hull.Size() >= start + 2 && Cross(hull[hull.Size() - 2], hull[hull.Size() - 1], p) <= 0.0f)
                            hull.PopBack();
                        hull.PushBack(p);
                    }
                    hull.PopBack();
                }
                while(hull.Size() > 8)
                {
                    u32 weakest = 0;
                    float least = 1.0e30f;
                    for(u32 i = 0; i < (u32)hull.Size(); i++)
                    {
                        const Vec2& a    = hull[(i + hull.Size() - 1) % hull.Size()];
                        const Vec2& b    = hull[i];
                        const Vec2& c    = hull[(i + 1) % hull.Size()];
                        const float area = Maths::Abs(Cross(a, b, c));
                        if(area < least)
                        {
                            least   = area;
                            weakest = i;
                        }
                    }
                    RemoveAt(hull, weakest);
                }
                return hull;
            }

            // the hull of everything not see-through
            void AutoShape()
            {
                State& st = St();
                TDArray<Vec2> corners;
                for(u32 y = 0; y < st.Height; y++)
                    for(u32 x = 0; x < st.Width; x++)
                    {
                        if((Get((i32)x, (i32)y) & 0xff) == 0)
                            continue;
                        // outer pixels only: the hull never needs the inside ones
                        const bool edge = x == 0 || y == 0 || x + 1 == st.Width || y + 1 == st.Height || (Get((i32)x - 1, (i32)y) & 0xff) == 0 || (Get((i32)x + 1, (i32)y) & 0xff) == 0
                            || (Get((i32)x, (i32)y - 1) & 0xff) == 0 || (Get((i32)x, (i32)y + 1) & 0xff) == 0;
                        if(!edge)
                            continue;
                        corners.PushBack(Vec2((float)x, (float)y));
                        corners.PushBack(Vec2((float)x + 1.0f, (float)y));
                        corners.PushBack(Vec2((float)x, (float)y + 1.0f));
                        corners.PushBack(Vec2((float)x + 1.0f, (float)y + 1.0f));
                    }
                st.Shape = PhysicsPolygon(corners);
                st.Dirty = true;
                if(st.Shape.Empty())
                    SetStatus("Nothing to outline: the picture is empty");
            }

            u32 CellWidth() { return St().CellW > 0 ? (u32)St().CellW : St().Width; }
            u32 CellHeight() { return St().CellH > 0 ? (u32)St().CellH : St().Height; }
            u32 CellColumns() { return Maths::Max(1u, St().Width / Maths::Max(1u, CellWidth())); }
            u32 FrameCount() { return CellColumns() * Maths::Max(1u, St().Height / Maths::Max(1u, CellHeight())); }

            // What the editor knows about a picture beyond its pixels - cell size, animations, the
            // collision shape - kept beside it as "<file>.sprite" (plain lines, one thing each).
            void SaveMeta()
            {
                State& st = St();
                if(!st.Physical[0])
                    return;
                std::string text = "# Sprite Editor: cell w h / anim name first count fps loop|pingpong / shape x y\n";
                char line[160];
                snprintf(line, sizeof(line), "cell %d %d\n", st.CellW, st.CellH);
                text += line;
                for(const Animation& anim : st.Anims)
                {
                    snprintf(line, sizeof(line), "anim %s %d %d %g %s\n", anim.Name, anim.First, anim.Count, anim.Fps, anim.PingPong ? "pingpong" : "loop");
                    text += line;
                }
                for(const Vec2& p : st.Shape)
                {
                    snprintf(line, sizeof(line), "shape %g %g\n", p.x, p.y);
                    text += line;
                }
                char meta[800];
                snprintf(meta, sizeof(meta), "%s.sprite", st.Physical);
                FileSystem::WriteTextFile(Str8C(meta), Str8((u8*)text.c_str(), text.size()));
            }

            void LoadMeta()
            {
                State& st = St();
                st.Shape.Clear();
                st.Anims.Clear();
                st.CellW = st.CellH = 0;
                st.CurrentAnim      = -1;
                if(!st.Physical[0])
                    return;
                ArenaTemp scratch = ScratchBegin(nullptr, 0);
                char meta[800];
                snprintf(meta, sizeof(meta), "%s.sprite", st.Physical);
                bool legacy = false;
                if(!FileSystem::FileExists(Str8C(meta)))
                {
                    // the first version kept only the shape, as bare "x y" lines
                    snprintf(meta, sizeof(meta), "%s.shape", st.Physical);
                    legacy = true;
                }
                if(FileSystem::FileExists(Str8C(meta)))
                {
                    String8 text  = FileSystem::ReadTextFile(scratch.arena, Str8C(meta));
                    const char* c = (const char*)text.str;
                    const char* e = c + text.size;
                    while(c < e)
                    {
                        const char* nl = (const char*)memchr(c, '\n', (size_t)(e - c));
                        const u64 len  = nl ? (u64)(nl - c) : (u64)(e - c);
                        char line[200];
                        snprintf(line, sizeof(line), "%.*s", (int)Maths::Min<u64>(len, 199), c);
                        c += len + 1;
                        float x, y;
                        Animation anim;
                        char mode[16] = { 0 };
                        if(line[0] == '#')
                            continue;
                        if(legacy && sscanf(line, "%f %f", &x, &y) == 2)
                            st.Shape.PushBack(Vec2(x, y));
                        else if(sscanf(line, "shape %f %f", &x, &y) == 2 && st.Shape.Size() < kMaxShapeDots)
                            st.Shape.PushBack(Vec2(x, y));
                        else if(sscanf(line, "cell %d %d", &st.CellW, &st.CellH) == 2)
                            ;
                        else if(sscanf(line, "anim %31s %d %d %f %15s", anim.Name, &anim.First, &anim.Count, &anim.Fps, mode) >= 4)
                        {
                            anim.PingPong = strcmp(mode, "pingpong") == 0;
                            st.Anims.PushBack(anim);
                        }
                    }
                }
                if(!st.Anims.Empty())
                    st.CurrentAnim = 0;
                ScratchEnd(scratch);
            }

            void PutTextureCopies();

            // this picture's cells as frames on the selected entity's Animated Sprite
            void ApplyAnimations()
            {
                State& st    = St();
                Scene* scene = Application::Get().GetSceneManager()->GetCurrentScene();
                if(!scene)
                    return;
                if(!st.Path[0])
                {
                    SetStatus("Save the picture first");
                    return;
                }
                const u32 id = ToolPanels::GetSelected();
                Entity entity((entt::entity)id, scene);
                if(id == 0xFFFFFFFF || !entity.Valid())
                {
                    // nothing selected: a new entity for it, named after the picture, and selected
                    const char* slash = strrchr(st.Path, '/');
                    char name[64];
                    snprintf(name, sizeof(name), "%s", slash ? slash + 1 : st.Path);
                    if(char* dot = strrchr(name, '.'))
                        *dot = 0;
                    entity = scene->GetEntityManager()->Create(name);
                    entity.AddComponent<Maths::Transform>();
                    SceneUndo::EntityCreated(entity);
                    ToolPanels::Select((u32)entity.GetHandle());
                }
                Graphics::AnimatedSprite* sprite = entity.TryGetComponent<Graphics::AnimatedSprite>();
                if(!sprite)
                    sprite = &entity.AddComponent<Graphics::AnimatedSprite>();
                sprite->SetTextureFromFile(st.Path);
                sprite->FrameSize = Vec2((float)CellWidth(), (float)CellHeight());
                sprite->GetAnimationStates().clear();
                const u32 columns = CellColumns();
                for(const Animation& anim : st.Anims)
                {
                    std::vector<Vec2> frames;
                    for(i32 f = 0; f < anim.Count; f++)
                    {
                        const u32 frame = (u32)Maths::Max(0, anim.First + f);
                        frames.push_back(Vec2((float)((frame % columns) * CellWidth()), (float)((frame / columns) * CellHeight())));
                    }
                    sprite->AddState(frames, 1.0f / Maths::Max(0.1f, anim.Fps), anim.Name);
                    sprite->SetPlayMode(anim.Name, anim.PingPong ? Graphics::AnimatedSprite::PlayMode::PingPong : Graphics::AnimatedSprite::PlayMode::Loop);
                }
                if(!st.Anims.Empty())
                    sprite->SetState(st.Anims[(u32)Maths::Max(0, st.CurrentAnim)].Name);
                SetStatus("%u animation%s put on %s", (u32)st.Anims.Size(), st.Anims.Size() == 1 ? "" : "s", entity.GetName().c_str());
            }

            // a sprite sheet laid out one animation a row: a state per row, as long as its opaque cells
            void AnimationsFromRows()
            {
                State& st         = St();
                const u32 cw      = CellWidth(), ch = CellHeight(), columns = CellColumns();
                const u32 rows    = Maths::Max(1u, st.Height / Maths::Max(1u, ch));
                auto cellEmpty    = [&](u32 cell)
                {
                    const u32 fx = (cell % columns) * cw, fy = (cell / columns) * ch;
                    for(u32 y = fy; y < fy + ch && y < st.Height; y++)
                        for(u32 x = fx; x < fx + cw && x < st.Width; x++)
                            if(st.Pixels[(y * st.Width + x) * 4 + 3] != 0)
                                return false;
                    return true;
                };
                st.Anims.Clear();
                for(u32 r = 0; r < rows && st.Anims.Size() < 32; r++)
                {
                    i32 count = 0;
                    for(u32 c = 0; c < columns; c++)
                        if(!cellEmpty(r * columns + c))
                            count = (i32)c + 1;
                    if(count == 0)
                        continue;
                    Animation anim;
                    snprintf(anim.Name, sizeof(anim.Name), "row%u", r);
                    anim.First = (i32)(r * columns);
                    anim.Count = count;
                    st.Anims.PushBack(anim);
                }
                st.CurrentAnim = st.Anims.Empty() ? -1 : 0;
                st.AnimTime    = 0.0f;
                st.Dirty       = true;
                SetStatus(st.Anims.Empty() ? "No frames found: set the cell size first" : "%u animations, one a row", (u32)st.Anims.Size());
            }

            // onto the selected entity's 2D body, through its sprite's rectangle
            void ApplyShape()
            {
                State& st    = St();
                Scene* scene = Application::Get().GetSceneManager()->GetCurrentScene();
                const u32 id = ToolPanels::GetSelected();
                if(!scene || id == 0xFFFFFFFF)
                {
                    SetStatus("Select the entity to give the shape to");
                    return;
                }
                Entity entity((entt::entity)id, scene);
                if(!entity.Valid() || !entity.HasComponent<RigidBody2DComponent>())
                {
                    SetStatus("The selected entity has no RigidBody 2D");
                    return;
                }
                TDArray<Vec2> polygon = PhysicsPolygon(st.Shape);
                if(polygon.Size() < 3)
                {
                    SetStatus("A shape needs at least three corners");
                    return;
                }

                // where the picture sits in the entity: its sprite, else a unit square round the middle
                Vec2 origin = Vec2(-0.5f), size = Vec2(1.0f);
                if(Graphics::Sprite* sprite = entity.TryGetComponent<Graphics::Sprite>())
                {
                    origin = sprite->GetPosition();
                    size   = sprite->GetScale();
                }
                else if(Graphics::AnimatedSprite* animated = entity.TryGetComponent<Graphics::AnimatedSprite>())
                {
                    origin = animated->GetPosition();
                    size   = animated->GetScale();
                }
                Vec3 scale = Vec3(1.0f);
                if(Maths::Transform* transform = entity.TryGetComponent<Maths::Transform>())
                    scale = transform->GetLocalScale();

                std::vector<Vec2> local;
                for(const Vec2& p : polygon)
                    local.push_back(Vec2((origin.x + p.x / (float)st.Width * size.x) * scale.x, (origin.y + (1.0f - p.y / (float)st.Height) * size.y) * scale.y));
                entity.GetComponent<RigidBody2DComponent>().GetRigidBody()->SetShape(Shape::Custom, local);
                SetStatus("Shape put on %s (%u corners)", entity.GetName().c_str(), (u32)local.size());
            }

            // ---- files ----

            void PutTextureCopies()
            {
                State& st    = St();
                Scene* scene = Application::Get().GetSceneManager()->GetCurrentScene();
                if(!scene || !st.Path[0])
                    return;
                auto update = [&](Graphics::Sprite& sprite)
                {
                    const SharedPtr<Graphics::Texture2D>& texture = sprite.GetTexture();
                    if(!texture || (texture->GetFilepath() != st.Path && texture->GetFilepath() != st.Physical))
                        return;
                    if(texture->GetWidth() == st.Width && texture->GetHeight() == st.Height)
                        texture->SetData(st.Pixels.Data());
                    else
                        sprite.SetTextureFromFile(st.Path);
                };
                auto& registry = scene->GetRegistry();
                for(auto entity : registry.view<Graphics::Sprite>())
                    update(registry.get<Graphics::Sprite>(entity));
                for(auto entity : registry.view<Graphics::AnimatedSprite>())
                    update(registry.get<Graphics::AnimatedSprite>(entity));
            }

            bool Save()
            {
                State& st = St();
                if(st.Width == 0)
                    return false;
                if(!st.Physical[0])
                {
                    // first save: a new file under Assets/Sprites
                    const String8& assets = FileSystem::Get().GetAssetPath();
                    std::string folder    = std::string((const char*)assets.str, assets.size);
                    if(!folder.empty() && folder.back() != '/')
                        folder += "/";
                    folder += "Sprites";
                    std::error_code error;
                    std::filesystem::create_directories(folder, error);
                    for(u32 i = 1; i < 1000; i++)
                    {
                        char physical[768];
                        snprintf(physical, sizeof(physical), "%s/Sprite%u.png", folder.c_str(), i);
                        if(!std::filesystem::exists(physical, error))
                        {
                            snprintf(st.Physical, sizeof(st.Physical), "%s", physical);
                            snprintf(st.Path, sizeof(st.Path), "//Assets/Sprites/Sprite%u.png", i);
                            break;
                        }
                    }
                }
                const bool saved = st.Physical[0] && ImageExport::SavePNG(st.Physical, st.Width, st.Height, st.Pixels.Data());
                if(!saved)
                {
                    SetStatus("Could not save to %s", st.Physical[0] ? st.Physical : "(no folder for Assets)");
                    return false;
                }
                SaveMeta();
                st.Dirty = false;
                PutTextureCopies();
                ToolPanels::RefreshAssetIndex();
                SetStatus("Saved %s", st.Path);
                return true;
            }

            // ---- small widgets ----

            UI_Widget* Quad(u64 hash, const Vec2& position, const Vec2& size, const Vec4& colour, f32 rounding = 0.0f)
            {
                UI_Widget* w = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""), hash,
                                          { SizeKind_Pixels, Maths::Max(1.0f, size.x) }, { SizeKind_Pixels, Maths::Max(1.0f, size.y) });
                w->relative_position                    = position;
                w->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                w->style_vars[StyleVar_Border]          = Vec4(0.0f);
                w->style_vars[StyleVar_CornerRadius]    = Vec4(rounding, 0.0f, 0.0f, 0.0f);
                w->style_vars[StyleVar_BackgroundColor] = colour;
                return w;
            }

            // a straight line made of one thin turned quad
            void Segment(u64 hash, const Vec2& a, const Vec2& b, float thickness, const Vec4& colour)
            {
                const Vec2 d       = b - a;
                const float length = sqrtf(d.x * d.x + d.y * d.y);
                if(length < 0.5f)
                    return;
                const Vec2 mid = (a + b) * 0.5f;
                UI_Widget* w   = Quad(hash, Vec2(mid.x - length * 0.5f, mid.y - thickness * 0.5f), Vec2(length, thickness), colour);
                w->Rotation    = atan2f(d.y, d.x);
            }

            bool Swatch(const char* id, const Vec4& colour, float size, bool selected)
            {
                const f32 dpi = GetUIState()->DPIScale;
                u64 hash;
                HandleUIString(id, &hash);
                UI_Widget* w = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder, Str8Lit(""), hash,
                                          { SizeKind_Pixels, size * dpi }, { SizeKind_Pixels, size * dpi });
                const UIToolTheme& theme = UIGetToolTheme();
                const Vec4 shown         = Vec4(colour.x, colour.y, colour.z, Maths::Max(colour.w, 0.0f));
                UISetBackground(w, shown, shown, shown);
                w->style_vars[StyleVar_Padding]      = Vec4(0.0f);
                w->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
                w->style_vars[StyleVar_Border]       = Vec4(selected ? 2.0f * dpi : 1.0f, 0.0f, 0.0f, 0.0f);
                const Vec4 edge                      = selected ? theme.Accent : theme.Border;
                w->style_vars[StyleVar_BorderColor]       = edge;
                w->style_vars[StyleVar_HotBorderColor]    = theme.Text;
                w->style_vars[StyleVar_ActiveBorderColor] = theme.Text;
                UI_Interaction in = HandleWidgetInteraction(w);
                return in.clicked || in.right_clicked;
            }
        }

        // ---------------------------------------------------------------------

        void SetDock(UIDockSpace* space)
        {
            St().Dock = space;
        }

        void New(u32 width, u32 height)
        {
            State& st = St();
            Resize(Maths::Clamp(width, 1u, kMaxSide), Maths::Clamp(height, 1u, kMaxSide));
            st.Path[0]     = 0;
            st.Physical[0] = 0;
            st.Dirty       = false;
            st.Anims.Clear();
            st.CurrentAnim = -1;
            st.CellW = st.CellH = 0;
            if(st.Dock)
                UIDockFocusPanel(st.Dock, "Sprite Editor");
        }

        bool Open(const char* path)
        {
            State& st = St();
            if(!path || !path[0])
                return false;

            // both forms of the path: VFS to match the textures that use it, physical to write to
            ArenaTemp scratch = ScratchBegin(nullptr, 0);
            String8 vfs       = Str8C((char*)path);
            String8 physical  = Str8C((char*)path);
            if(path[0] == '/' && path[1] == '/')
            {
                if(!FileSystem::Get().ResolvePhysicalPath(scratch.arena, vfs, &physical))
                    physical = {};
            }
            else
            {
                String8 converted;
                if(FileSystem::Get().AbsolutePathToFileSystem(scratch.arena, physical, converted) && converted.size > 0)
                    vfs = converted;
            }

            u32 width = 0, height = 0, bits = 0;
            bool hdr     = false;
            u8* pixels   = Lumos::LoadImageFromFile((const char*)(physical.size ? physical.str : vfs.str), &width, &height, &bits, &hdr, false, false);
            const bool ok = pixels && !hdr && width > 0 && height > 0 && width <= kMaxSide && height <= kMaxSide;
            if(ok)
            {
                Resize(width, height);
                MemoryCopy(st.Pixels.Data(), pixels, (u64)width * height * 4);
                snprintf(st.Path, sizeof(st.Path), "%.*s", (int)vfs.size, (const char*)vfs.str);
                snprintf(st.Physical, sizeof(st.Physical), "%.*s", (int)physical.size, physical.size ? (const char*)physical.str : "");
                st.Dirty = false;
                LoadMeta();
                st.FrameShown = -1;
                SetStatus("%s  %u x %u", st.Path, width, height);
            }
            else
                SetStatus("Could not open %s as an 8 bit picture", path);
            delete[] pixels;
            ScratchEnd(scratch);

            if(ok && st.Dock)
                UIDockFocusPanel(st.Dock, "Sprite Editor");
            return ok;
        }

        void SetShapeMode(bool shape)
        {
            St().ShapeMode = shape;
        }

        bool GetShapeMode() { return St().ShapeMode; }
        u32 GetToolCount() { return Tool_Count; }
        u32 GetTool() { return (u32)St().Current; }
        void SetTool(u32 tool)
        {
            if(tool < Tool_Count)
                St().Current = (Tool)tool;
        }
        const char* GetToolIcon(u32 tool) { return tool < Tool_Count ? kTools[tool].Icon : ""; }
        const char* GetToolName(u32 tool) { return tool < Tool_Count ? kTools[tool].Name : ""; }

        void Release()
        {
            if(!s)
                return;
            delete s->Texture;
            s->Texture = nullptr;
            delete s->FrameTexture;
            s->FrameTexture = nullptr;
        }

        void DrawPanel(const UIDockPanelContext& context)
        {
            State& st                = St();
            UI_State* ui             = GetUIState();
            const UIToolTheme& theme = UIGetToolTheme();
            const f32 dpi            = context.DPIScale;
            Input& input             = Input::Get();
            const float dt           = 1.0f / 60.0f;
            st.StatusTime            = Maths::Max(0.0f, st.StatusTime - dt);

            if(st.Width == 0)
                Resize(32, 32);
            EnsureTexture();

            const bool typing = ui->FocusedTextInput != 0;
            const bool keys   = (context.Hovered || context.Focused) && !typing;
            const bool ctrl   = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl)
                || input.GetKeyHeldRaw(InputCode::Key::LeftSuper) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);
            const bool shift = input.GetKeyHeldRaw(InputCode::Key::LeftShift) || input.GetKeyHeldRaw(InputCode::Key::RightShift);

            // ---- keys (Aseprite's where there is one) ----
            if(keys)
            {
                if(ctrl)
                {
                    if(input.GetKeyPressedRaw(InputCode::Key::Z))
                        StepHistory(shift);
                    if(input.GetKeyPressedRaw(InputCode::Key::Y))
                        StepHistory(true);
                    if(input.GetKeyPressedRaw(InputCode::Key::S))
                        Save();
                }
                else
                {
                    for(u32 t = 0; t < Tool_Count; t++)
                        if(input.GetKeyPressedRaw(kTools[t].Key))
                        {
                            st.Current   = (Tool)t;
                            st.ShapeMode = false;
                        }
                    if(input.GetKeyPressedRaw(InputCode::Key::C))
                        st.ShapeMode = !st.ShapeMode;
                    if(input.GetKeyPressedRaw(InputCode::Key::X))
                        std::swap(st.Fore, st.Back);
                    if(input.GetKeyPressedRaw(InputCode::Key::LeftBracket))
                        st.Brush = Maths::Max(1u, st.Brush - 1);
                    if(input.GetKeyPressedRaw(InputCode::Key::RightBracket))
                        st.Brush = Maths::Min(32u, st.Brush + 1);
                    if(input.GetKeyPressedRaw(InputCode::Key::D0))
                        st.Fit = true;
                    if(input.GetKeyPressedRaw(InputCode::Key::Equal))
                        st.Zoom = Maths::Min(st.Zoom * 1.5f, 128.0f);
                    if(input.GetKeyPressedRaw(InputCode::Key::Minus))
                        st.Zoom = Maths::Max(st.Zoom / 1.5f, 0.5f);
                    if(st.ShapeMode && input.GetKeyPressedRaw(InputCode::Key::Backspace) && !st.Shape.Empty())
                    {
                        st.Shape.PopBack();
                        st.Dirty = true;
                    }
                }
            }

            // ---- tool bar ----
            UIBeginRowFullWidth();
            {
                UI_Widget* row                        = ui->parents.Back();
                row->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->flags |= WidgetFlags_CentreChildrenY;
            }
            for(u32 t = 0; t < Tool_Count; t++)
            {
                char id[48];
                snprintf(id, sizeof(id), "%s###spritetool%u", kTools[t].Icon, t);
                if(UIIconButton(id, 28.0f, !st.ShapeMode && st.Current == t).clicked)
                {
                    st.Current   = (Tool)t;
                    st.ShapeMode = false;
                }
                UITooltip(kTools[t].Name);
            }
            UISpacer(8.0f);
            if(UIIconButton(ICON_MDI_REFLECT_HORIZONTAL "###spritemirror", 28.0f, st.MirrorX).clicked)
                st.MirrorX = !st.MirrorX;
            UITooltip("Mirror left / right");
            if(UIIconButton(ICON_MDI_GRID "###spritegrid", 28.0f, st.ShowGrid).clicked)
                st.ShowGrid = !st.ShowGrid;
            UITooltip("Pixel grid");
            UISpacer(8.0f);
            if(UIIconButton(ICON_MDI_VECTOR_POLYGON "###spriteshape", 28.0f, st.ShapeMode).clicked)
                st.ShapeMode = !st.ShapeMode;
            UITooltip("Collision shape (C)");
            UISpacer(8.0f);
            {
                char brush[32];
                snprintf(brush, sizeof(brush), "%upx", st.Brush);
                if(UIIconButton(ICON_MDI_MINUS "###spritebrushless", 24.0f).clicked)
                    st.Brush = Maths::Max(1u, st.Brush - 1);
                UITooltip("Smaller brush ([)");
                UILabelCStr("##spritebrush", brush);
                if(UIIconButton(ICON_MDI_PLUS "###spritebrushmore", 24.0f).clicked)
                    st.Brush = Maths::Min(32u, st.Brush + 1);
                UITooltip("Bigger brush (])");
            }
            UIFlexSpacer();
            if(UIIconButton(ICON_MDI_UNDO "###spriteundo", 28.0f).clicked)
                StepHistory(false);
            UITooltip("Undo (Ctrl Z)");
            if(UIIconButton(ICON_MDI_REDO "###spriteredo", 28.0f).clicked)
                StepHistory(true);
            UITooltip("Redo (Ctrl Shift Z)");
            if(UIIconButton(ICON_MDI_FILE_OUTLINE "###spritenew", 28.0f).clicked)
                UIOpenPopup("##spritenewmenu");
            UITooltip("New picture");
            if(UIBeginPopup("##spritenewmenu"))
            {
                for(u32 side : { 8u, 16u, 24u, 32u, 48u, 64u, 128u, 256u })
                {
                    char item[32];
                    snprintf(item, sizeof(item), "%u x %u###spritenew%u", side, side, side);
                    if(UIMenuItem(item))
                        New(side, side);
                }
                UIEndPopup();
            }
            if(UIIconButton(ICON_MDI_CONTENT_SAVE "###spritesave", 28.0f, st.Dirty).clicked)
                Save();
            UITooltip("Save (Ctrl S)");
            UIEndRow();

            // ---- colours + palette ----
            UIBeginRowFullWidth();
            {
                UI_Widget* row                        = ui->parents.Back();
                row->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->flags |= WidgetFlags_CentreChildrenY;
            }
            if(Swatch("##spritefore", st.Fore, 26.0f, true))
                UIOpenPopup("##spriteforepick");
            UITooltip("Colour (left button) - X swaps");
            if(Swatch("##spriteback", st.Back, 20.0f, false))
                UIOpenPopup("##spritebackpick");
            UITooltip("Second colour (right button)");
            if(UIBeginPopup("##spriteforepick"))
            {
                UIBeginPropertyGrid("##spriteforegrid", 0.3f);
                UIPropertyColour("Colour", &st.Fore.x, true);
                UIEndPropertyGrid();
                UIEndPopup();
            }
            if(UIBeginPopup("##spritebackpick"))
            {
                UIBeginPropertyGrid("##spritebackgrid", 0.3f);
                UIPropertyColour("Second", &st.Back.x, true);
                UIEndPropertyGrid();
                UIEndPopup();
            }
            UISpacer(6.0f);
            for(u32 i = 0; i < 32; i++)
            {
                char id[32];
                snprintf(id, sizeof(id), "##spritepal%u", i);
                const Vec4 colour = Unpack(kPalette[i]);
                const bool chosen = Pack(st.Fore) == kPalette[i];
                if(Swatch(id, colour, 18.0f, chosen))
                {
                    if(input.GetMouseHeld(InputCode::MouseKey::ButtonRight) || UILongPressed())
                        st.Back = colour;
                    else
                        st.Fore = colour;
                }
            }
            UIEndRow();

            // ---- canvas ----
            // the bars above and below take about this much
            const f32 statusHeight = floorf(26.0f * dpi) + floorf(36.0f * dpi); // status + animation bar
            const f32 top          = floorf(76.0f * dpi);
            const Vec2 canvasSize  = Vec2(Maths::Max(32.0f, context.Size.x), Maths::Max(32.0f, context.Size.y - top - statusHeight));

            u64 canvasHash;
            HandleUIString("##spritecanvas", &canvasHash);
            UI_Widget* canvas = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground, Str8Lit(""), canvasHash,
                                           { SizeKind_Pixels, canvasSize.x }, { SizeKind_Pixels, canvasSize.y });
            canvas->Clip = true;
            UIClearBorders(canvas);
            canvas->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            canvas->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
            const Vec4 back                           = theme.Dark ? Vec4(0.10f, 0.10f, 0.11f, 1.0f) : Vec4(0.82f, 0.82f, 0.84f, 1.0f);
            UISetBackground(canvas, back, back, back);
            const UI_Interaction in = HandleWidgetInteraction(canvas);

            if(st.Fit && canvasSize.x > 40.0f)
            {
                st.Fit  = false;
                st.Zoom = Maths::Max(1.0f, floorf(Maths::Min((canvasSize.x - 40.0f * dpi) / (float)st.Width, (canvasSize.y - 40.0f * dpi) / (float)st.Height)));
                st.Pan  = Vec2(floorf((canvasSize.x - st.Zoom * (float)st.Width) * 0.5f), floorf((canvasSize.y - st.Zoom * (float)st.Height) * 0.5f));
            }

            const Vec2 pointer = UIMousePosition() - canvas->position;
            // On a touch screen the finger jumps: the move and the press arrive in the same frame,
            // and the UI's hover target is still whatever was under the last touch. So on touch the
            // canvas goes by where the press is, not by being the hover target.
            const bool touch    = ui->ClickOnRelease;
            auto overCanvas     = [&](const Vec2& local) { return local.x >= 0.0f && local.y >= 0.0f && local.x < canvas->size.x && local.y < canvas->size.y; };
            const bool inside   = in.hovering && (touch ? overCanvas(pointer) : ui->hot_widget == canvas->hash);
            const bool pressed  = in.pressed || (touch && inside && input.GetMouseClicked(InputCode::MouseKey::ButtonLeft));
            auto pixelAt       = [&](const Vec2& at, i32* x, i32* y)
            {
                *x = (i32)floorf((at.x - st.Pan.x) / st.Zoom);
                *y = (i32)floorf((at.y - st.Pan.y) / st.Zoom);
            };
            auto zoomAround = [&](const Vec2& at, float zoom)
            {
                zoom          = Maths::Clamp(zoom, 0.5f, 128.0f);
                const Vec2 px = (at - st.Pan) / st.Zoom;
                st.Zoom       = zoom;
                st.Pan        = at - px * zoom;
            };

            // scrolling moves the picture (a trackpad's two fingers); with Ctrl / Cmd it zooms where the pointer is
            if(inside)
            {
                const float wheelY = input.GetScrollOffset();
                const float wheelX = input.GetScrollOffsetX();
                if(ctrl && wheelY != 0.0f)
                    zoomAround(pointer, st.Zoom * powf(1.15f, Maths::Clamp(wheelY, -4.0f, 4.0f)));
                else if(wheelX != 0.0f || wheelY != 0.0f)
                    st.Pan += Vec2(wheelX, wheelY) * 24.0f * dpi;
            }

            // two fingers = pan + pinch; a stroke the first finger began is taken back
            const u32 fingers = input.GetTouchPointCount();
            const Vec2 touchA = fingers >= 2 ? input.GetTouchPoint(0).Position * ui->InputScale - ui->InputOffset - canvas->position : Vec2(0.0f);
            const Vec2 touchB = fingers >= 2 ? input.GetTouchPoint(1).Position * ui->InputScale - ui->InputOffset - canvas->position : Vec2(0.0f);
            if(fingers >= 2 && (overCanvas((touchA + touchB) * 0.5f) || st.Pinching || st.Stroking))
            {
                const Vec2 a     = touchA;
                const Vec2 b     = touchB;
                const Vec2 mid   = (a + b) * 0.5f;
                const float dist = Maths::Max(1.0f, (a - b).Length());
                if(st.Stroking)
                {
                    st.Pixels   = st.StrokeBase;
                    st.Upload   = true;
                    st.Stroking = false;
                    if(!st.Undo.Empty())
                        st.Undo.PopBack();
                }
                if(st.Pinching)
                {
                    st.Pan += mid - st.PinchMid;
                    zoomAround(mid, st.Zoom * dist / st.PinchDist);
                }
                st.Pinching  = true;
                st.PinchDist = dist;
                st.PinchMid  = mid;
            }
            else if(st.Pinching && fingers < 2 && !input.GetMouseHeld(InputCode::MouseKey::ButtonLeft))
                st.Pinching = false;

            // pan: hand tool, space held, or the middle button
            const bool spaceHeld = keys && input.GetKeyHeldRaw(InputCode::Key::Space);
            const bool panInput  = (pressed && (st.Current == Tool_Hand || spaceHeld)) || (inside && input.GetMouseClicked(InputCode::MouseKey::ButtonMiddle));
            if(panInput && !st.Pinching)
            {
                st.Panning  = true;
                st.PanFrom  = pointer;
                st.PanStart = st.Pan;
            }
            if(st.Panning)
            {
                if(input.GetMouseHeld(InputCode::MouseKey::ButtonLeft) || input.GetMouseHeld(InputCode::MouseKey::ButtonMiddle))
                    st.Pan = st.PanStart + (pointer - st.PanFrom);
                else
                    st.Panning = false;
            }

            i32 px = 0, py = 0;
            pixelAt(pointer, &px, &py);
            const bool onPicture = px >= 0 && py >= 0 && px < (i32)st.Width && py < (i32)st.Height;

            // ---- drawing / shape ----
            const bool rightDown = inside && input.GetMouseClicked(InputCode::MouseKey::ButtonRight);
            if(!st.Panning && !st.Pinching && fingers < 2)
            {
                if(st.PickFrames && !st.ShapeMode)
                {
                    // drag over the cells: the animation in hand becomes that run of frames
                    const u32 cw = CellWidth(), ch = CellHeight(), columns = CellColumns();
                    const i32 cx = Maths::Clamp(px, 0, (i32)st.Width - 1) / (i32)cw;
                    const i32 cy = Maths::Clamp(py, 0, (i32)st.Height - 1) / (i32)ch;
                    const i32 cell = Maths::Min(cy * (i32)columns + cx, (i32)FrameCount() - 1);
                    if(pressed && !spaceHeld && onPicture)
                    {
                        if(st.CurrentAnim < 0 && st.Anims.Size() < 32)
                        {
                            st.Anims.PushBack(Animation());
                            snprintf(st.Anims.Back().Name, sizeof(st.Anims.Back().Name), "anim%u", (u32)st.Anims.Size());
                            st.CurrentAnim = (i32)st.Anims.Size() - 1;
                        }
                        st.PickFrom = cell;
                    }
                    if(st.PickFrom >= 0 && st.CurrentAnim >= 0)
                    {
                        Animation& anim = st.Anims[(u32)st.CurrentAnim];
                        anim.First      = Maths::Min(st.PickFrom, cell);
                        anim.Count      = Maths::Abs(cell - st.PickFrom) + 1;
                        st.AnimTime     = 0.0f;
                        st.Dirty        = true;
                        if(!input.GetMouseHeld(InputCode::MouseKey::ButtonLeft))
                            st.PickFrom = -1;
                    }
                }
                else if(st.ShapeMode)
                {
                    // corners: press one to drag it, press elsewhere to add one, right button takes the nearest away
                    const Vec2 at = Vec2(Maths::Clamp(roundf((pointer.x - st.Pan.x) / st.Zoom * 2.0f) * 0.5f, 0.0f, (float)st.Width),
                                         Maths::Clamp(roundf((pointer.y - st.Pan.y) / st.Zoom * 2.0f) * 0.5f, 0.0f, (float)st.Height));
                    i32 nearest   = -1;
                    float best    = 12.0f * dpi;
                    for(u32 i = 0; i < (u32)st.Shape.Size(); i++)
                    {
                        const Vec2 screen = st.Pan + st.Shape[i] * st.Zoom;
                        const float d     = (screen - pointer).Length();
                        if(d < best)
                        {
                            best    = d;
                            nearest = (i32)i;
                        }
                    }
                    if(pressed && !spaceHeld)
                    {
                        if(nearest >= 0)
                            st.DragDot = nearest;
                        else if(st.Shape.Size() < kMaxShapeDots)
                        {
                            st.Shape.PushBack(at);
                            st.DragDot = (i32)st.Shape.Size() - 1;
                        }
                        st.Dirty = true;
                    }
                    if(st.DragDot >= 0)
                    {
                        if(input.GetMouseHeld(InputCode::MouseKey::ButtonLeft) && st.DragDot < (i32)st.Shape.Size())
                            st.Shape[(u32)st.DragDot] = at;
                        else
                            st.DragDot = -1;
                    }
                    if((rightDown || (in.right_clicked && UILongPressed())) && nearest >= 0)
                    {
                        RemoveAt(st.Shape, (u32)nearest);
                        st.Dirty = true;
                    }
                }
                else if(st.Current != Tool_Hand)
                {
                    const bool begin = (pressed && !spaceHeld) || rightDown;
                    if(begin)
                    {
                        st.StrokeBack = rightDown;
                        if(st.Current == Tool_Picker)
                        {
                            if(onPicture)
                                (st.StrokeBack ? st.Back : st.Fore) = Unpack(Get(px, py));
                        }
                        else
                        {
                            PushUndo();
                            st.StrokeBase = st.Pixels;
                            st.Stroking   = true;
                            st.StartX = st.LastX = px;
                            st.StartY = st.LastY = py;
                            const u32 colour     = st.Current == Tool_Eraser ? 0u : Pack(st.StrokeBack ? st.Back : st.Fore);
                            if(st.Current == Tool_Fill)
                            {
                                Fill(px, py, colour);
                                st.Stroking = false;
                            }
                            else
                                Dab(px, py, colour);
                        }
                    }
                    else if(st.Stroking)
                    {
                        const bool held = input.GetMouseHeld(st.StrokeBack ? InputCode::MouseKey::ButtonRight : InputCode::MouseKey::ButtonLeft)
                            || (touch && !st.StrokeBack && input.GetTouchCount() == 1);
                        const u32 colour = st.Current == Tool_Eraser ? 0u : Pack(st.StrokeBack ? st.Back : st.Fore);
                        if(held)
                        {
                            if(st.Current == Tool_Pencil || st.Current == Tool_Eraser)
                            {
                                if(px != st.LastX || py != st.LastY)
                                    Line(st.LastX, st.LastY, px, py, colour);
                            }
                            else if(st.Current == Tool_Line || st.Current == Tool_Rect)
                            {
                                // drawn again from the start each frame: the picture as it was, plus the shape
                                st.Pixels = st.StrokeBase;
                                i32 ex = px, ey = py;
                                if(shift && st.Current == Tool_Line)
                                {
                                    // straight, or at 45 degrees
                                    const i32 dx = ex - st.StartX, dy = ey - st.StartY;
                                    if(Maths::Abs(dx) > Maths::Abs(dy) * 2)
                                        ey = st.StartY;
                                    else if(Maths::Abs(dy) > Maths::Abs(dx) * 2)
                                        ex = st.StartX;
                                    else
                                    {
                                        const i32 n = Maths::Max(Maths::Abs(dx), Maths::Abs(dy));
                                        ex          = st.StartX + (dx < 0 ? -n : n);
                                        ey          = st.StartY + (dy < 0 ? -n : n);
                                    }
                                }
                                if(st.Current == Tool_Line)
                                    Line(st.StartX, st.StartY, ex, ey, colour);
                                else
                                    RectOutline(st.StartX, st.StartY, ex, ey, colour);
                                st.Upload = true;
                            }
                            st.LastX = px;
                            st.LastY = py;
                        }
                        else
                            st.Stroking = false;
                    }
                }
            }
            EnsureTexture();

            // ---- what the canvas shows ----
            PushParent(canvas);
            {
                const Vec2 pictureSize = Vec2((float)st.Width, (float)st.Height) * st.Zoom;
                // see-through shows as a two-tone check behind the picture: two flat tones, one per half
                const Vec4 checkA = theme.Dark ? Vec4(0.24f, 0.24f, 0.26f, 1.0f) : Vec4(0.96f, 0.96f, 0.96f, 1.0f);
                Quad(UIHashCombine(canvasHash, 0x434B31ull), st.Pan, pictureSize, checkA);

                // the picture: not clickable, so a press on it is a press on the canvas
                if(st.Texture)
                {
                    UI_Widget* picture = Quad(UIHashCombine(canvasHash, 0x50494354ull), st.Pan, pictureSize, Vec4(1.0f));
                    picture->texture   = st.Texture;
                }

                // sprite sheet cells, and the frames of the animation in hand
                if(CellWidth() < st.Width || CellHeight() < st.Height)
                {
                    const Vec4 cellLine = Vec4(theme.Accent.x, theme.Accent.y, theme.Accent.z, 0.55f);
                    const u32 cw = CellWidth(), ch = CellHeight();
                    for(u32 x = cw, n = 0; x < st.Width && n < 256; x += cw, n++)
                        Quad(UIHashCombine(canvasHash, 0x43580000ull + x), Vec2(floorf(st.Pan.x + (float)x * st.Zoom), st.Pan.y), Vec2(Maths::Max(1.0f, dpi), pictureSize.y), cellLine);
                    for(u32 y = ch, n = 0; y < st.Height && n < 256; y += ch, n++)
                        Quad(UIHashCombine(canvasHash, 0x43590000ull + y), Vec2(st.Pan.x, floorf(st.Pan.y + (float)y * st.Zoom)), Vec2(pictureSize.x, Maths::Max(1.0f, dpi)), cellLine);
                    if(st.CurrentAnim >= 0 && st.CurrentAnim < (i32)st.Anims.Size())
                    {
                        const Animation& anim = st.Anims[(u32)st.CurrentAnim];
                        const u32 columns     = CellColumns();
                        for(i32 f = 0; f < anim.Count && f < 128; f++)
                        {
                            const u32 frame = (u32)Maths::Max(0, anim.First + f);
                            const Vec2 at   = st.Pan + Vec2((float)((frame % columns) * cw), (float)((frame / columns) * ch)) * st.Zoom;
                            const bool now  = (i32)frame == st.FrameShown;
                            Quad(UIHashCombine(canvasHash, 0x46520000ull + (u64)f), at, Vec2((float)cw * st.Zoom, Maths::Max(2.0f, 3.0f * dpi)),
                                 now ? theme.Accent : Vec4(theme.Accent.x, theme.Accent.y, theme.Accent.z, 0.35f));
                        }
                    }
                }

                // pixel grid, only where it is worth seeing
                if(st.ShowGrid && st.Zoom >= 6.0f)
                {
                    const Vec4 line = theme.Dark ? Vec4(1.0f, 1.0f, 1.0f, 0.08f) : Vec4(0.0f, 0.0f, 0.0f, 0.10f);
                    const i32 x0    = Maths::Max(0, (i32)floorf(-st.Pan.x / st.Zoom));
                    const i32 x1    = Maths::Min((i32)st.Width, (i32)ceilf((canvasSize.x - st.Pan.x) / st.Zoom));
                    const i32 y0    = Maths::Max(0, (i32)floorf(-st.Pan.y / st.Zoom));
                    const i32 y1    = Maths::Min((i32)st.Height, (i32)ceilf((canvasSize.y - st.Pan.y) / st.Zoom));
                    if(x1 - x0 < 200 && y1 - y0 < 200)
                    {
                        for(i32 x = x0; x <= x1; x++)
                            Quad(UIHashCombine(canvasHash, 0x47580000ull + (u64)x), Vec2(floorf(st.Pan.x + (float)x * st.Zoom), Maths::Max(0.0f, st.Pan.y)),
                                 Vec2(1.0f, Maths::Min(pictureSize.y, canvasSize.y)), line);
                        for(i32 y = y0; y <= y1; y++)
                            Quad(UIHashCombine(canvasHash, 0x47590000ull + (u64)y), Vec2(Maths::Max(0.0f, st.Pan.x), floorf(st.Pan.y + (float)y * st.Zoom)),
                                 Vec2(Maths::Min(pictureSize.x, canvasSize.x), 1.0f), line);
                    }
                }

                // the brush under the pointer
                if(inside && onPicture && !st.ShapeMode && st.Current != Tool_Hand && st.Current != Tool_Fill)
                {
                    const i32 size  = (st.Current == Tool_Picker) ? 1 : (i32)st.Brush;
                    const i32 from  = -(size - 1) / 2;
                    const Vec4 ring = Vec4(theme.Accent.x, theme.Accent.y, theme.Accent.z, 0.55f);
                    const Vec2 at   = st.Pan + Vec2((float)(px + from), (float)(py + from)) * st.Zoom;
                    const float s2  = (float)size * st.Zoom;
                    const float t   = Maths::Max(1.0f, dpi);
                    Quad(UIHashCombine(canvasHash, 0x42523100ull), at, Vec2(s2, t), ring);
                    Quad(UIHashCombine(canvasHash, 0x42523200ull), at + Vec2(0.0f, s2 - t), Vec2(s2, t), ring);
                    Quad(UIHashCombine(canvasHash, 0x42523300ull), at, Vec2(t, s2), ring);
                    Quad(UIHashCombine(canvasHash, 0x42523400ull), at + Vec2(s2 - t, 0.0f), Vec2(t, s2), ring);
                }

                // the collision shape: its outline as it will be made (hull, 8 corners), and the corners placed
                if(st.ShapeMode || !st.Shape.Empty())
                {
                    const float alpha      = st.ShapeMode ? 1.0f : 0.45f;
                    const Vec4 lineColour  = Vec4(0.25f, 0.95f, 0.45f, 0.9f * alpha);
                    TDArray<Vec2> polygon  = PhysicsPolygon(st.Shape);
                    for(u32 i = 0; i < (u32)polygon.Size(); i++)
                        Segment(UIHashCombine(canvasHash, 0x53480000ull + i), st.Pan + polygon[i] * st.Zoom, st.Pan + polygon[(i + 1) % polygon.Size()] * st.Zoom,
                                Maths::Max(1.5f, 2.0f * dpi), lineColour);
                    if(st.ShapeMode)
                        for(u32 i = 0; i < (u32)st.Shape.Size(); i++)
                        {
                            const float r = 5.0f * dpi;
                            const Vec2 c  = st.Pan + st.Shape[i] * st.Zoom;
                            Quad(UIHashCombine(canvasHash, 0x53440000ull + i), c - Vec2(r), Vec2(r * 2.0f), (i32)i == st.DragDot ? theme.Accent : Vec4(1.0f, 1.0f, 1.0f, 0.95f), r);
                        }
                }

                // the animation playing, in the corner
                if(st.CurrentAnim >= 0 && st.CurrentAnim < (i32)st.Anims.Size() && st.FrameTexture)
                {
                    const float side   = (st.BigPreview ? Maths::Min(canvasSize.x, canvasSize.y) * 0.45f : 120.0f * dpi);
                    const float aspect = (float)CellWidth() / (float)Maths::Max(1u, CellHeight());
                    const Vec2 size    = aspect >= 1.0f ? Vec2(side, side / aspect) : Vec2(side * aspect, side);
                    const Vec2 at      = Vec2(canvasSize.x - size.x - 12.0f * dpi, 12.0f * dpi);
                    Quad(UIHashCombine(canvasHash, 0x50564247ull), at - Vec2(4.0f * dpi), size + Vec2(8.0f * dpi), theme.Popup, 6.0f * dpi);
                    Quad(UIHashCombine(canvasHash, 0x50564348ull), at, size, Vec4(theme.Dark ? 0.24f : 0.96f, theme.Dark ? 0.24f : 0.96f, theme.Dark ? 0.26f : 0.96f, 1.0f));
                    UI_Widget* preview = Quad(UIHashCombine(canvasHash, 0x50564957ull), at, size, Vec4(1.0f));
                    preview->texture   = st.FrameTexture;
                }
            }
            PopParent(canvas);

            // ---- sprite sheet + animations ----
            {
                // play: the frame to show now
                if(st.CurrentAnim >= (i32)st.Anims.Size())
                    st.CurrentAnim = st.Anims.Empty() ? -1 : 0;
                i32 frame = -1;
                if(st.CurrentAnim >= 0)
                {
                    const Animation& anim = st.Anims[(u32)st.CurrentAnim];
                    if(st.Playing)
                        st.AnimTime += dt;
                    const i32 count = Maths::Max(1, anim.Count);
                    i32 step        = (i32)floorf(st.AnimTime * Maths::Max(0.1f, anim.Fps));
                    if(anim.PingPong && count > 1)
                    {
                        const i32 period = (count - 1) * 2;
                        step %= period;
                        if(step >= count)
                            step = period - step;
                    }
                    else
                        step %= count;
                    frame = Maths::Clamp(anim.First + step, 0, (i32)FrameCount() - 1);
                }
                // the corner preview's own little texture: the frame's pixels, copied when the frame or the picture changes
                if(frame >= 0 && (frame != st.FrameShown || st.Dirty))
                {
                    const u32 cw = CellWidth(), ch = CellHeight(), columns = CellColumns();
                    const u32 fx = ((u32)frame % columns) * cw, fy = ((u32)frame / columns) * ch;
                    TDArray<u8> cell;
                    cell.Resize(cw * ch * 4);
                    for(u32 y = 0; y < ch; y++)
                        for(u32 x = 0; x < cw; x++)
                        {
                            const u32 sx = Maths::Min(fx + x, st.Width - 1), sy = Maths::Min(fy + y, st.Height - 1);
                            MemoryCopy(&cell[(y * cw + x) * 4], &st.Pixels[(sy * st.Width + sx) * 4], 4);
                        }
                    if(!st.FrameTexture || st.FrameTexW != cw || st.FrameTexH != ch)
                    {
                        delete st.FrameTexture;
                        Graphics::TextureDesc desc;
                        desc.format          = Graphics::RHIFormat::R8G8B8A8_Unorm;
                        desc.minFilter       = Graphics::TextureFilter::NEAREST;
                        desc.magFilter       = Graphics::TextureFilter::NEAREST;
                        desc.wrap            = Graphics::TextureWrap::CLAMP_TO_EDGE;
                        desc.generateMipMaps = false;
                        desc.srgb            = false;
                        st.FrameTexture      = Graphics::Texture2D::CreateFromSource(cw, ch, cell.Data(), desc);
                        st.FrameTexW         = cw;
                        st.FrameTexH         = ch;
                    }
                    else
                        st.FrameTexture->SetData(cell.Data());
                }
                st.FrameShown = frame;

                UIBeginRowFullWidth();
                {
                    UI_Widget* row                        = ui->parents.Back();
                    row->style_vars[StyleVar_Padding]     = Vec4(8.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
                    row->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                    row->flags |= WidgetFlags_CentreChildrenY;
                }
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("##spritecelllabel", "Cell");
                UIPopStyle(StyleVar_TextColor);
                int cw = (int)CellWidth(), ch = (int)CellHeight();
                if(UIDragIntField("###spritecellw", &cw, 0.2f, 1, (int)st.Width, 44.0f))
                {
                    st.CellW = cw;
                    st.Dirty = true;
                }
                UILabelCStr("##spritecellx", "x");
                if(UIDragIntField("###spritecellh", &ch, 0.2f, 1, (int)st.Height, 44.0f))
                {
                    st.CellH = ch;
                    st.Dirty = true;
                }
                UITooltip("Sprite sheet cell size: frames are the cells, numbered along the rows");
                UISpacer(8.0f);

                for(u32 i = 0; i < (u32)st.Anims.Size() && i < 12; i++)
                {
                    char id[64];
                    snprintf(id, sizeof(id), "%s###spriteanim%u", st.Anims[i].Name, i);
                    if(UIButton(id).clicked)
                    {
                        st.CurrentAnim = (i32)i;
                        st.AnimTime    = 0.0f;
                    }
                    if(UI_Widget* w = GetCurrentParent()->last)
                        if((i32)i == st.CurrentAnim)
                            UISetTextColour(w, theme.Accent);
                }
                if(UIIconButton(ICON_MDI_SELECTION "###spritepickframes", 24.0f, st.PickFrames).clicked)
                    st.PickFrames = !st.PickFrames;
                UITooltip("Pick frames: drag over the cells to set the animation's frames");
                if(UIIconButton(ICON_MDI_VIEW_LIST "###spriteanimrows", 24.0f).clicked)
                    AnimationsFromRows();
                UITooltip("One animation per row of the sheet (replaces the list)");
                if(UIIconButton(ICON_MDI_PLUS "###spriteanimadd", 24.0f).clicked && st.Anims.Size() < 32)
                {
                    Animation anim;
                    snprintf(anim.Name, sizeof(anim.Name), "anim%u", (u32)st.Anims.Size() + 1);
                    // the row after the last one, or the whole first row
                    anim.First = st.Anims.Empty() ? 0 : Maths::Min((i32)FrameCount() - 1, st.Anims.Back().First + st.Anims.Back().Count);
                    anim.Count = Maths::Max(1, Maths::Min((i32)CellColumns(), (i32)FrameCount() - anim.First));
                    st.Anims.PushBack(anim);
                    st.CurrentAnim = (i32)st.Anims.Size() - 1;
                    st.Dirty       = true;
                }
                UITooltip("New animation");

                if(st.CurrentAnim >= 0)
                {
                    Animation& anim = st.Anims[(u32)st.CurrentAnim];
                    UISpacer(6.0f);
                    if(UIIconButton(st.Playing ? ICON_MDI_PAUSE "###spriteanimplay" : ICON_MDI_PLAY "###spriteanimplay", 24.0f, st.Playing).clicked)
                        st.Playing = !st.Playing;
                    UITooltip("Play the animation in the corner");
                    if(!st.Playing)
                    {
                        // a frame at a time while paused
                        const float step = 1.0f / Maths::Max(0.1f, anim.Fps);
                        if(UIIconButton(ICON_MDI_SKIP_PREVIOUS "###spriteanimprev", 24.0f).clicked)
                            st.AnimTime = Maths::Max(0.0f, st.AnimTime - step);
                        if(UIIconButton(ICON_MDI_SKIP_NEXT "###spriteanimnext", 24.0f).clicked)
                            st.AnimTime += step;
                    }
                    if(UIIconButton(st.BigPreview ? ICON_MDI_FULLSCREEN_EXIT "###spriteanimbig" : ICON_MDI_FULLSCREEN "###spriteanimbig", 24.0f, st.BigPreview).clicked)
                        st.BigPreview = !st.BigPreview;
                    UITooltip("Bigger preview");
                    UIPushStyle(StyleVar_TextColor, theme.TextDim);
                    UILabelCStr("##spriteanimfrom", "frames");
                    UIPopStyle(StyleVar_TextColor);
                    int first = anim.First, count = anim.Count;
                    if(UIDragIntField("###spriteanimfirst", &first, 0.1f, 0, (int)FrameCount() - 1, 36.0f))
                    {
                        anim.First = first;
                        st.Dirty   = true;
                    }
                    UITooltip("First frame (cells numbered from 0 along the rows)");
                    if(UIDragIntField("###spriteanimcount", &count, 0.1f, 1, (int)FrameCount(), 36.0f))
                    {
                        anim.Count = count;
                        st.Dirty   = true;
                    }
                    UITooltip("How many frames");
                    int fps = (int)roundf(anim.Fps);
                    if(UIDragIntField("###spriteanimfps", &fps, 0.1f, 1, 60, 36.0f))
                    {
                        anim.Fps = (float)fps;
                        st.Dirty = true;
                    }
                    UITooltip("Frames a second");
                    if(UIButton(anim.PingPong ? "ping-pong###spriteanimmode" : "loop###spriteanimmode").clicked)
                    {
                        anim.PingPong = !anim.PingPong;
                        st.Dirty      = true;
                    }
                    static char s_Rename[32];
                    static i32 s_RenameFor = -2;
                    if(s_RenameFor != st.CurrentAnim)
                    {
                        s_RenameFor = st.CurrentAnim;
                        snprintf(s_Rename, sizeof(s_Rename), "%s", anim.Name);
                    }
                    if(UITextField("###spriteanimname", s_Rename, sizeof(s_Rename), "name", 90.0f).widget && strcmp(s_Rename, anim.Name) != 0 && s_Rename[0])
                    {
                        // one word: it is the state's name in the Animated Sprite
                        for(char* c = s_Rename; *c; c++)
                            if(*c == ' ')
                                *c = '_';
                        snprintf(anim.Name, sizeof(anim.Name), "%s", s_Rename);
                        st.Dirty = true;
                    }
                    if(UIIconButton(ICON_MDI_DELETE "###spriteanimdel", 24.0f).clicked)
                    {
                        RemoveAt(st.Anims, (u32)st.CurrentAnim);
                        st.CurrentAnim = st.Anims.Empty() ? -1 : 0;
                        st.Dirty       = true;
                    }
                    UITooltip("Delete this animation");
                }
                UIFlexSpacer();
                if(!st.Anims.Empty())
                {
                    const bool selected = ToolPanels::GetSelected() != 0xFFFFFFFF;
                    if(UIButton(selected ? ICON_MDI_CHECK "  Animate selected###spriteanimapply" : ICON_MDI_PLUS "  Create animated entity###spriteanimapply").clicked)
                        ApplyAnimations();
                    UITooltip(selected ? "Give the selected entity an Animated Sprite with these animations (its states)"
                                       : "A new entity with an Animated Sprite: one state per animation");
                }
                UIEndRow();
            }

            // ---- status line ----
            UIBeginRowFullWidth();
            {
                UI_Widget* row                        = ui->parents.Back();
                row->style_vars[StyleVar_Padding]     = Vec4(8.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                row->flags |= WidgetFlags_CentreChildrenY;
            }
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            char line[256];
            if(st.StatusTime > 0.0f)
                snprintf(line, sizeof(line), "%s", st.Status);
            else
            {
                const char* name = strrchr(st.Path, '/');
                snprintf(line, sizeof(line), "%s%s   %u x %u   %.0f%%", st.Path[0] ? (name ? name + 1 : st.Path) : "unsaved", st.Dirty ? " *" : "", st.Width, st.Height, st.Zoom * 100.0f);
                if(onPicture && inside)
                {
                    const size_t used = strlen(line);
                    snprintf(line + used, sizeof(line) - used, "   %d, %d", px, py);
                }
            }
            UILabelCStr("##spritestatus", line);
            UIPopStyle(StyleVar_TextColor);
            UIFlexSpacer();
            if(st.ShapeMode)
            {
                if(UIButton("Outline###spriteshapeauto").clicked)
                    AutoShape();
                UITooltip("A shape round everything not see-through");
                if(UIButton("Clear###spriteshapeclear").clicked)
                {
                    st.Shape.Clear();
                    st.Dirty = true;
                }
                if(UIButton(ICON_MDI_CHECK "  Apply to selected###spriteshapeapply").clicked)
                    ApplyShape();
                UITooltip("Put the shape on the selected entity's RigidBody 2D");
            }
            UIEndRow();
        }
    }
}
