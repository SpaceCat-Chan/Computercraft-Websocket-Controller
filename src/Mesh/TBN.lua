function Sub(lhs, rhs)
	if lhs.z and rhs.z then
		return {x = lhs.x - rhs.x, y = lhs.y - rhs.y, z = lhs.z - rhs.z}
	else
		return {x = lhs.x - rhs.x, y = lhs.y - rhs.y}
	end
end

function PrintVector(v, Prefix)
	Prefix = Prefix or ""
	if(v.z) then
		print(Prefix.."{"..tostring(v.x)..", "..tostring(v.y)..", "..tostring(v.z).."}")
	else
		print(Prefix.."{"..tostring(v.x)..", "..tostring(v.y).."}")
	end
end

local pos0 = {x = 0, y = 0, z = 0}
local pos1 = {x = 1, y = 0, z = 1}
local pos2 = {x = 1, y = 0, z = 0}

local uv0 = {x = 0, y = 0}
local uv1 = {x = 1, y = 1}
local uv2 = {x = 1, y = 0}

PrintVector(pos0, "pos0: ")
PrintVector(pos1, "pos1: ")
PrintVector(pos2, "pos2: ")

PrintVector(uv0, "uv0: ")
PrintVector(uv1, "uv1: ")
PrintVector(uv2, "uv2: ")

local edge1 = Sub(pos1, pos0)
local edge2 = Sub(pos2, pos0)
local dUV1 = Sub(uv1, uv0)
local dUV2 = Sub(uv2, uv0)

print("")

PrintVector(edge1, "edge1: ")
PrintVector(edge2, "edge2: ")

PrintVector(dUV1, "dUV1: ")
PrintVector(dUV2, "dUV2: ")

local Result = {}

local f = 1.0 / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
Result.x = f * (edge1.x * dUV2.y - edge2.x * dUV1.y);
Result.y = f * (edge1.y * dUV2.y - edge2.y * dUV1.y);
Result.z = f * (edge1.z * dUV2.y - edge2.z * dUV1.y);

print("")
PrintVector(Result, "Result: ")
