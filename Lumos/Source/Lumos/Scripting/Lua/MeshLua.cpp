#include "Precompiled.h"
#include "Maths/MathsUtilities.h"
#include "MeshLua.h"
#include "LuaSmartPtrTraits.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/MeshOptimiser.h"
#include "Scene/Component/ModelComponent.h"

namespace Lumos
{
    using namespace Graphics;

    static SimplifySettings ReadSimplifySettings(sol::optional<sol::table> t)
    {
        SimplifySettings s;
        if(!t)
            return s;
        sol::table& tbl    = *t;
        s.TargetRatio      = tbl.get_or("ratio", s.TargetRatio);
        s.TargetError      = tbl.get_or("error", s.TargetError);
        s.LockBorder       = tbl.get_or("lockBorder", s.LockBorder);
        s.Sloppy           = tbl.get_or("sloppy", s.Sloppy);
        s.UseAttributes    = tbl.get_or("attributes", s.UseAttributes);
        s.Prune            = tbl.get_or("prune", s.Prune);
        s.Regularize       = tbl.get_or("regularize", s.Regularize);
        s.RecomputeNormals = tbl.get_or("recomputeNormals", s.RecomputeNormals);
        return s;
    }

    static sol::table MeshListStats(sol::state_view lua, const TDArray<SharedPtr<Mesh>>& meshes)
    {
        u32 verts = 0, tris = 0;
        for(auto& m : meshes)
        {
            if(!m)
                continue;
            verts += m->GetCPUPositions().Size();
            tris += m->GetCPUIndices().Size() / 3;
        }
        sol::table t     = lua.create_table();
        t["vertices"]  = verts;
        t["triangles"] = tris;
        t["meshes"]    = meshes.Size();
        return t;
    }

    void BindMeshLua(sol::state& state)
    {
        auto opt = state["MeshOptimiser"].get_or_create<sol::table>();

        opt.set_function("SetRetainCPUVertices", [](bool v) { Mesh::SetRetainCPUVertices(v); });

        // LOD levels built for every .lmesh loaded from now on (0 = off). Call from the AppScript.
        // optional: ratio per level, first level's error (relative), first level's screen size, sloppy last level
        opt.set_function("SetAutoLOD", [](int levels, sol::optional<float> ratio, sol::optional<float> error, sol::optional<float> firstScreen, sol::optional<bool> sloppyLast)
                         {
            Model::s_AutoLODLevels       = (u32)Maths::Max(0, levels);
            Model::s_AutoLODRatio        = ratio.value_or(0.5f);
            Model::s_AutoLODError        = error.value_or(0.02f);
            Model::s_AutoLODFirstScreen  = firstScreen.value_or(0.3f);
            Model::s_AutoLODSloppyLast   = sloppyLast.value_or(false); });

        // static batching (merge by material per cellSize cube) for every .lmesh loaded from now on (0 = off)
        opt.set_function("SetAutoMerge", [](float cellSize) { Model::s_AutoMergeCell = Maths::Max(0.0f, cellSize); });

        opt.set_function("Simplify", [](Mesh* mesh, sol::optional<sol::table> settings) -> std::tuple<SharedPtr<Mesh>, float>
                         {
                             float err = 0.0f;
                             if(!mesh)
                                 return { nullptr, 0.0f };
                             auto out = MeshOptimiser::Simplify(*mesh, ReadSimplifySettings(settings), &err);
                             return { out, err }; });

        opt.set_function("Optimise", [](Mesh* mesh) -> SharedPtr<Mesh>
                         { return mesh ? MeshOptimiser::Optimise(*mesh) : nullptr; });

        opt.set_function("Analyse", [](Mesh* mesh, sol::this_state s) -> sol::table
                         {
                             sol::state_view lua(s);
                             sol::table t = lua.create_table();
                             if(!mesh)
                                 return t;
                             MeshAnalysis a = MeshOptimiser::Analyse(*mesh);
                             t["vertices"]  = a.VertexCount;
                             t["triangles"] = a.TriangleCount;
                             t["acmr"]      = a.ACMR;
                             t["atvr"]      = a.ATVR;
                             t["overdraw"]  = a.Overdraw;
                             t["overfetch"] = a.Overfetch;
                             return t; });

        // Whole-model helpers. Replace LOD0 in place, return the max error.
        opt.set_function("SimplifyModel", [](Model* model, sol::optional<sol::table> settings) -> float
                         {
                             if(!model)
                                 return 0.0f;
                             SimplifySettings s = ReadSimplifySettings(settings);
                             float maxErr       = 0.0f;
                             for(auto& mesh : model->GetMeshesRef())
                             {
                                 float err = 0.0f;
                                 if(!mesh)
                                     continue;
                                 if(auto out = MeshOptimiser::Simplify(*mesh, s, &err))
                                     mesh = out;
                                 maxErr = Maths::Max(maxErr, err);
                             }
                             model->ClearLODs();
                             return maxErr; });

        opt.set_function("OptimiseModel", [](Model* model)
                         {
                             if(!model)
                                 return;
                             for(auto& mesh : model->GetMeshesRef())
                                 if(mesh)
                                     if(auto out = MeshOptimiser::Optimise(*mesh))
                                         mesh = out;
                             model->ClearLODs(); });

        opt.set_function("GenerateLODs", [](Model* model, int count, sol::optional<float> ratioStep, sol::optional<sol::table> settings) -> int
                         {
                             if(!model || count <= 0)
                                 return 0;
                             return (int)MeshOptimiser::GenerateLODs(*model, (u32)count, ratioStep.value_or(0.5f), ReadSimplifySettings(settings)); });

        opt.set_function("ExportOBJ", [](Model* model, const std::string& path, sol::optional<int> lod) -> bool
                         {
                             if(!model)
                                 return false;
                             return MeshOptimiser::ExportOBJ(model->GetLODMeshes((u32)Maths::Max(0, lod.value_or(0))), path); });

        opt.set_function("HasCPUVertices", [](Model* model) -> bool
                         {
                             if(!model)
                                 return false;
                             for(auto& m : model->GetMeshes())
                                 if(m && !m->HasCPUVertices())
                                     return false;
                             return !model->GetMeshes().Empty(); });

        sol::usertype<Model> modelType = state["Model"];
        modelType["GetLODCount"]  = &Model::GetLODCount;
        modelType["SetForcedLOD"] = &Model::SetForcedLOD;
        modelType["GetForcedLOD"] = &Model::GetForcedLOD;
        modelType["GetCurrentLOD"] = &Model::GetLastSelectedLOD;
        modelType["ClearLODs"]    = &Model::ClearLODs;
        modelType["GetMesh"]      = [](Model& m, int i) -> SharedPtr<Mesh>
        {
            auto& meshes = m.GetMeshes();
            return (i >= 1 && i <= (int)meshes.Size()) ? meshes[i - 1] : nullptr;
        };
        modelType["GetLODStats"] = [](Model& m, int lod, sol::this_state s) -> sol::table
        {
            sol::state_view lua(s);
            sol::table t = MeshListStats(lua, m.GetLODMeshes((u32)Maths::Max(0, lod)));
            if(auto* level = m.GetLODLevel((u32)Maths::Max(0, lod)))
            {
                t["screenSize"] = level->ScreenSize;
                t["error"]      = level->Error;
            }
            else
            {
                t["screenSize"] = 1.0f;
                t["error"]      = 0.0f;
            }
            return t;
        };
        modelType["SetLODScreenSize"] = [](Model& m, int lod, float size)
        {
            if(auto* level = m.GetLODLevel((u32)Maths::Max(0, lod)))
                level->ScreenSize = size;
        };

        sol::usertype<ModelComponent> mcType = state["ModelComponent"];
        // Fresh load that skips the asset cache, e.g. to pick up CPU vertices or a changed file.
        mcType["LoadUncached"] = [](ModelComponent& mc, const std::string& path) -> Model*
        {
            mc.ModelRef = CreateSharedPtr<Model>(path);
            return mc.ModelRef.get();
        };
    }
}
