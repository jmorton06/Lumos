-- ============================================================================
-- 03_Entities — create / find / destroy / component access
-- ============================================================================

-- ===== Snippet: Create a named entity =======================================
local function NewNamed(name)
    local e = scene:GetEntityManager():Create(name)
    e:AddTransform()
    return e
end
-- ============================================================================


-- ===== Snippet: Find an entity by name ======================================
local target
function OnInit()
    target = GetEntityByName(scene, "Target")
    if target and target:Valid() then
        Log.Info("found target")
    end
end
-- ============================================================================


-- ===== Snippet: Iterate every entity ========================================
function OnInit()
    for e in EachEntity() do
        -- e is an Entity; query components
        if e:HasModelComponent() then
            -- ...
        end
    end
end
-- ============================================================================


-- ===== Snippet: Safe component access =======================================
local function GetBody(e)
    if not (e and e:Valid()) then return nil end
    if not e:HasRigidBody3DComponent() then return nil end
    return e:GetRigidBody3DComponent():GetRigidBody()
end
-- ============================================================================


-- ===== Snippet: Add common components =======================================
local function MakeBox(scene, pos)
    local e = scene:GetEntityManager():Create("Box")
    local t = e:AddTransform()
    t:SetLocalPosition(pos)
    t:SetLocalScale(Vec3.new(1, 1, 1))

    local mc = e:AddModelComponent()
    mc:LoadPrimitive(PrimitiveType.Cube)

    local params = RigidBodyParameters.new()
    params.position = pos
    params.scale    = Vec3.new(1, 1, 1)
    params.shape    = Shape.Custom        -- 3D path
    params.mass     = 1.0
    params.isStatic = false
    e:AddRigidBody3DComponent(params)
    return e
end
-- ============================================================================


-- ===== Snippet: Parent / child hierarchy ====================================
local parent = scene:GetEntityManager():Create("Rig")
parent:AddTransform()
local child = scene:GetEntityManager():Create("Hand")
child:AddTransform()
child:SetParent(parent)
-- ============================================================================


-- ===== Snippet: Get all entities (snapshot) =================================
local all = GetAllEntities()
for i = 1, #all do
    local e = all[i]
    if e:HasNameComponent() then
        local n = e:GetNameComponent().name
        Log.Info("entity: " .. n)
    end
end
-- ============================================================================


-- ===== Snippet: Defer destroy until end of frame ============================
local _toKill = {}
local function KillLater(e)
    table.insert(_toKill, e)
end
function OnUpdate(dt)
    for i = #_toKill, 1, -1 do
        local e = _toKill[i]
        if e and e:Valid() then e:Destroy() end
        table.remove(_toKill, i)
    end
end
-- ============================================================================


-- ===== Snippet: Toggle visibility (active state) ============================
local function Show(e, on)
    if e and e:Valid() then e:SetActive(on) end
end
-- ============================================================================
