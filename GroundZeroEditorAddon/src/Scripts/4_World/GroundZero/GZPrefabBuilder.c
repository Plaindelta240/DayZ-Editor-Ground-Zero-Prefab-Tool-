/*
	Ground Zero DayZ Editor Addon - prefab building and placement maths.

	SAVING   world -> prefab : every object is stored relative to one anchor point.
	PLACING  prefab -> world : world = anchor(position, yaw) * relative

	Only the anchor moves and turns, every object keeps its exact offset and its exact
	rotation relative to the group, so furniture stays exactly where it was in the house.
*/

class GZPrefabBuilder
{
	// Anything smaller than this counts as "no rotation" and takes the exact, matrix free path
	protected static const float YAW_EPSILON = 0.0001;

	/*
		Builds a prefab from editor objects.
		skipped = objects that could not be stored (editor-only helpers, invalid objects)
	*/
	static GZPrefabFile CreateFromObjects(string name, string description, notnull array<EditorObject> editor_objects, out int skipped)
	{
		skipped = 0;

		array<ref EditorObjectData> object_data_list = {};
		foreach (EditorObject editor_object: editor_objects) {
			if (!editor_object || !editor_object.GetWorldObject()) {
				skipped++;
				continue;
			}

			// Helpers such as the editor's own player marker are never part of a build
			if (editor_object.GetFlags() & EditorObjectFlags.NOSAVE) {
				skipped++;
				continue;
			}

			EditorObjectData object_data = editor_object.GetData();
			if (!object_data || object_data.Type == string.Empty) {
				skipped++;
				continue;
			}

			object_data_list.Insert(object_data);
		}

		if (object_data_list.Count() == 0) {
			return null;
		}

		// Bounds of the object origins
		vector bounds_min = object_data_list[0].Position;
		vector bounds_max = object_data_list[0].Position;
		foreach (EditorObjectData bounds_data: object_data_list) {
			for (int axis = 0; axis < 3; axis++) {
				if (bounds_data.Position[axis] < bounds_min[axis]) {
					bounds_min[axis] = bounds_data.Position[axis];
				}

				if (bounds_data.Position[axis] > bounds_max[axis]) {
					bounds_max[axis] = bounds_data.Position[axis];
				}
			}
		}

		// Anchor: centre of the footprint, at the height of the lowest object
		vector anchor = Vector((bounds_min[0] + bounds_max[0]) * 0.5, bounds_min[1], (bounds_min[2] + bounds_max[2]) * 0.5);

		GZPrefabFile prefab = new GZPrefabFile();
		prefab.Name = name;
		prefab.Description = description;
		prefab.MapName = GetGame().GetWorldName();
		prefab.Created = FormatTimestamp();
		prefab.Size = bounds_max - bounds_min;

		foreach (EditorObjectData source: object_data_list) {
			GZPrefabObject prefab_object = new GZPrefabObject();
			prefab_object.Type = source.Type;
			prefab_object.DisplayName = source.DisplayName;
			prefab_object.Position = source.Position - anchor;
			prefab_object.Orientation = source.Orientation;
			prefab_object.Scale = source.Scale;
			prefab_object.Flags = source.Flags;
			prefab_object.Locked = source.Locked;
			prefab_object.AllowDamage = source.AllowDamage;
			prefab_object.Simulate = source.Simulate;
			prefab.Objects.Insert(prefab_object);
		}

		return prefab;
	}

	/*
		World transform of a single prefab object for an anchor at anchor_position turned by anchor_yaw
		(degrees). Output is written into world_transform (rows 0-2 = axes * scale, row 3 = position).
	*/
	static void GetWorldTransform(GZPrefabObject prefab_object, vector anchor_position, float anchor_yaw, out vector world_transform[4])
	{
		vector anchor_matrix[4];
		Math3D.YawPitchRollMatrix(Vector(anchor_yaw, 0, 0), anchor_matrix);
		anchor_matrix[3] = anchor_position;

		vector local_matrix[4];
		Math3D.YawPitchRollMatrix(prefab_object.Orientation, local_matrix);
		local_matrix[0] = local_matrix[0] * prefab_object.Scale;
		local_matrix[1] = local_matrix[1] * prefab_object.Scale;
		local_matrix[2] = local_matrix[2] * prefab_object.Scale;
		local_matrix[3] = prefab_object.Position;

		Math3D.MatrixMultiply4(anchor_matrix, local_matrix, world_transform);
	}

	/*
		Editor object data for one prefab object at the given anchor.
		With no rotation the stored values are used untouched, so nothing can ever pick up rounding error.
	*/
	static EditorObjectData CreateObjectData(GZPrefabObject prefab_object, vector anchor_position, float anchor_yaw)
	{
		EditorObjectData object_data;
		if (Math.AbsFloat(anchor_yaw) < YAW_EPSILON) {
			object_data = EditorObjectData.Create(prefab_object.Type, anchor_position + prefab_object.Position, prefab_object.Orientation, prefab_object.Scale, prefab_object.Flags);
		} else {
			vector world_transform[4];
			GetWorldTransform(prefab_object, anchor_position, anchor_yaw, world_transform);
			object_data = EditorObjectData.Create(prefab_object.Type, world_transform, prefab_object.Flags);
		}

		if (!object_data) {
			return null;
		}

		if (prefab_object.DisplayName != string.Empty) {
			object_data.DisplayName = prefab_object.DisplayName;
		}

		object_data.Locked = prefab_object.Locked;
		object_data.AllowDamage = prefab_object.AllowDamage;
		object_data.Simulate = prefab_object.Simulate;
		return object_data;
	}

	static string FormatTimestamp()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);
		return string.Format("%1-%2-%3 %4:%5", year, PadTwo(month), PadTwo(day), PadTwo(hour), PadTwo(minute));
	}

	protected static string PadTwo(int value)
	{
		if (value < 10) {
			return "0" + value.ToString();
		}

		return value.ToString();
	}
}
