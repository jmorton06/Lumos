local M = {}

function M.Clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

function M.Lerp(a, b, t)
    return a + (b - a) * t
end

function M.LerpVec3(a, b, t)
    return Vec3.new(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t)
end

-- Frame-rate-independent damping. lambda is "speed"; larger = faster snap.
function M.Damp(a, b, lambda, dt)
    return M.Lerp(a, b, 1.0 - math.exp(-lambda * dt))
end

function M.DampVec3(a, b, lambda, dt)
    local t = 1.0 - math.exp(-lambda * dt)
    return M.LerpVec3(a, b, t)
end

function M.SmoothStep(t)
    if t < 0 then return 0 end
    if t > 1 then return 1 end
    return t * t * (3 - 2 * t)
end

function M.Sign(v)
    if v > 0 then return 1 end
    if v < 0 then return -1 end
    return 0
end

function M.DistXZ(a, b)
    local dx = a.x - b.x
    local dz = a.z - b.z
    return math.sqrt(dx * dx + dz * dz)
end

function M.Length(v)
    return math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
end

function M.Normalize(v)
    local len = M.Length(v)
    if len < 0.0001 then return Vec3.new(0, 0, 0) end
    return Vec3.new(v.x / len, v.y / len, v.z / len)
end

function M.DegToRad(d) return d * (math.pi / 180.0) end
function M.RadToDeg(r) return r * (180.0 / math.pi) end

return M
