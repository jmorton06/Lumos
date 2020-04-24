regy = scene:GetRegistry()
entity = regy:Create()
regy:assign_NameComponent(entity)
nameComp = regy:get_NameComponent(entity)
nameComp.name = "LUAEntity"

regy:assign_Transform(entity)
tran = regy:get_Transform(entity)
tran:SetLocalPosition(Vector3.new(10.0,0.0,-2.0))

function OnUpdate(dt)
    --Log.Info("Scene Update")
end