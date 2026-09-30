/*
	Ground Zero DayZ Editor Addon - prefab file format.

	A prefab is stored as plain JSON so it can be inspected, copied and shared.

	Every object is stored RELATIVE to a single anchor point (the prefab origin):
		Position    = object world position - anchor position
		Orientation = the yaw / pitch / roll (degrees) exactly as the Editor reports it
		Scale       = the object scale

	Because nothing is baked into the world's coordinate system, placing the prefab
	anywhere is a rigid transform of the whole group. Objects can never drift apart.
*/

class GZPrefabObject
{
	string Type;
	string DisplayName;
	vector Position;
	vector Orientation;
	float Scale = 1;
	int Flags;
	bool Locked;
	bool AllowDamage;
	bool Simulate;
}

class GZPrefabFile
{
	static const int CURRENT_VERSION = 1;

	int Version = CURRENT_VERSION;
	string Name;
	string Description;
	string MapName;
	string Created;

	// Extent of the object origins (x = width, y = height, z = depth), in metres
	vector Size;

	ref array<ref GZPrefabObject> Objects = {};

	int Count()
	{
		return Objects.Count();
	}
}
