registry = scene:GetRegistry()
entity = registry:Create()
registry:assign_NameComponent(entity)
nameComp = registry:get_NameComponent(entity)
nameComp.name = "LUAEntity"

registry:assign_Transform(entity)
tran = registry:get_Transform(entity)
tran:SetLocalPosition(Vector3.new(10.0,0.0,-2.0))
registry:assign_MeshComponent(entity)
registry:get_MeshComponent(entity):SetMesh(LoadMesh(PrimitiveType.Pyramid))


--damagedHelmet = LoadMesh("/Meshes/DamagedHelmet/glTF/DamagedHelmet.gltf", regy)
--meshTrans = registry:get_Transform(damagedHelmet)
--meshTrans:SetLocalPosition(Vector3.new(10.0, 10.0, 10.0))
--meshTrans:SetLocalScale(Vector3.new(100.0,100.0,100.0))

function OnUpdate(dt)
    --Log.Info("Scene Update")
end

function CreatPillar(index, offset)
    return registry:Create();
end

pillars = {}
for i=1,10 do
    pillars[i] = CreatPillar(i, (i + 3) * 10.0)
end