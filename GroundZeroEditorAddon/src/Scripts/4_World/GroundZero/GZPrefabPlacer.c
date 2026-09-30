/*
	Ground Zero DayZ Editor Addon - prefab placement.

	While a prefab is "in hand" every object in it is shown as a live preview that follows the cursor.
	Each frame the whole group is positioned with one rigid transform:

		world = anchor(cursor position, yaw) * relative offset

	so all the objects move and turn together and never shift relative to each other.
	Clicking commits the group through the normal Editor path (one undo step, everything selected).
*/

class GZPrefabPlacer
{
	// Height change speed of PageUp / PageDown, metres per second (Shift = fine, Ctrl = coarse)
	protected static const float HEIGHT_SPEED = 2.0;

	protected Editor m_Editor;
	protected ref GZPrefabFile m_Prefab;
	protected string m_PrefabPath;

	// Preview objects and the prefab entry each one belongs to (same index)
	protected ref array<Object> m_Ghosts = {};
	protected ref array<GZPrefabObject> m_GhostSource = {};
	protected ref array<Object> m_RayIgnores = {};

	protected bool m_Active;
	protected int m_MissingCount;
	protected float m_Yaw;
	protected float m_HeightOffset;
	protected vector m_Anchor;
	protected vector m_AppliedAnchor;
	protected float m_AppliedYaw;
	protected bool m_HasAppliedTransform;

	void GZPrefabPlacer(Editor editor)
	{
		m_Editor = editor;
	}

	void ~GZPrefabPlacer()
	{
		DeleteGhosts();
	}

	bool IsActive()
	{
		return m_Active;
	}

	GZPrefabFile GetPrefab()
	{
		return m_Prefab;
	}

	int GetMissingCount()
	{
		return m_MissingCount;
	}

	/*
		Starts placing a prefab. Returns false when none of its objects can be created
		(for example when the mod that provides them is not loaded).
	*/
	bool Begin(GZPrefabFile prefab, string prefab_path = string.Empty)
	{
		Cancel();

		if (!m_Editor || !prefab || prefab.Objects.Count() == 0) {
			return false;
		}

		// Drop whatever the editor itself was placing
		m_Editor.ClearHand();
		m_Editor.ClearSelection();

		m_Prefab = prefab;
		m_PrefabPath = prefab_path;
		m_MissingCount = 0;
		m_Yaw = 0;
		m_HeightOffset = 0;
		m_HasAppliedTransform = false;

		foreach (GZPrefabObject prefab_object: prefab.Objects) {
			Object ghost = EditorWorldObject.CreateObject(prefab_object.Type, vector.Zero, vector.Zero, 1);
			if (!ghost) {
				m_MissingCount++;
				continue;
			}

			m_Ghosts.Insert(ghost);
			m_GhostSource.Insert(prefab_object);
			m_RayIgnores.Insert(ghost);
		}

		if (m_Ghosts.Count() == 0) {
			m_Prefab = null;
			return false;
		}

		if (m_Editor.GetPlayer()) {
			m_RayIgnores.Insert(m_Editor.GetPlayer());
		}

		m_Active = true;

		// Put the preview under the cursor right away instead of flashing at the map origin
		Update(0, 0);
		return true;
	}

	// Stops placing and removes the previews
	void Cancel()
	{
		DeleteGhosts();
		m_Active = false;
		m_Prefab = null;
		m_PrefabPath = string.Empty;
		m_HasAppliedTransform = false;
	}

	protected void DeleteGhosts()
	{
		if (m_Ghosts) {
			foreach (Object ghost: m_Ghosts) {
				if (ghost) {
					GetGame().ObjectDelete(ghost);
				}
			}

			m_Ghosts.Clear();
		}

		if (m_GhostSource) {
			m_GhostSource.Clear();
		}

		if (m_RayIgnores) {
			m_RayIgnores.Clear();
		}
	}

	/*
		Called every frame by the Editor while a prefab is in hand.
		yaw_input = degrees of rotation the user asked for this frame (the Editor's normal rotate controls)
	*/
	void Update(float dt, float yaw_input)
	{
		if (!m_Active) {
			return;
		}

		m_Yaw = m_Yaw + yaw_input;
		if (m_Yaw > 360) {
			m_Yaw = m_Yaw - 360;
		}

		if (m_Yaw < -360) {
			m_Yaw = m_Yaw + 360;
		}

		float height_speed = HEIGHT_SPEED;
		if (m_Editor.IsShiftDown()) {
			height_speed = height_speed * 0.1;
		}

		if (m_Editor.IsCtrlDown()) {
			height_speed = height_speed * 5;
		}

		if (KeyState(KeyCode.KC_PRIOR)) {
			m_HeightOffset = m_HeightOffset + height_speed * dt;
		}

		if (KeyState(KeyCode.KC_NEXT)) {
			m_HeightOffset = m_HeightOffset - height_speed * dt;
		}

		// Where is the cursor pointing at? (Same lookup the Editor uses for its own objects)
		vector cursor_position;
		Raycast cursor_raycast = m_Editor.GetCursorRaycastModeSafeEx(m_RayIgnores, m_Editor.GroundMode);
		if (cursor_raycast && cursor_raycast.Bounce) {
			cursor_position = cursor_raycast.Bounce.Position;
		} else if (m_Editor.GetCursorRay()) {
			cursor_position = m_Editor.GetCursorRay().GetPoint(10.0);
		}

		m_Anchor = cursor_position + Vector(0, m_HeightOffset, 0);
		ApplyTransforms();
	}

	// Moves every preview to its place. Skipped when neither the anchor nor the yaw changed.
	protected void ApplyTransforms()
	{
		if (m_HasAppliedTransform && vector.DistanceSq(m_Anchor, m_AppliedAnchor) < 0.000001 && Math.AbsFloat(m_Yaw - m_AppliedYaw) < 0.0001) {
			return;
		}

		for (int i = 0; i < m_Ghosts.Count(); i++) {
			if (!m_Ghosts[i]) {
				continue;
			}

			vector ghost_transform[4];
			GZPrefabBuilder.GetWorldTransform(m_GhostSource[i], m_Anchor, m_Yaw, ghost_transform);
			m_Ghosts[i].SetTransform(ghost_transform);
		}

		m_AppliedAnchor = m_Anchor;
		m_AppliedYaw = m_Yaw;
		m_HasAppliedTransform = true;
	}

	/*
		Places the prefab where the previews currently are.
		Returns the created objects, or null when nothing was placed.
	*/
	array<EditorObject> Commit()
	{
		if (!m_Active) {
			return null;
		}

		// Same rule as the editor uses for its own objects: clicks on UI are not placements
		Widget widget_under_cursor = GetWidgetUnderCursor();
		if (widget_under_cursor && !widget_under_cursor.IsInherited(MapWidget) && widget_under_cursor.GetName() != "HudPanel" && widget_under_cursor.GetName() != "CursorIcons") {
			return null;
		}

		array<ref EditorObjectData> data_list = {};
		foreach (GZPrefabObject prefab_object: m_GhostSource) {
			EditorObjectData object_data = GZPrefabBuilder.CreateObjectData(prefab_object, m_Anchor, m_Yaw);
			if (object_data) {
				data_list.Insert(object_data);
			}
		}

		string prefab_name = m_Prefab.Name;
		bool keep_placing = m_Editor.IsShiftDown();
		if (!keep_placing) {
			Cancel();
		}

		array<EditorObject> placed_objects = {};
		EditorObjectMap created_objects = m_Editor.CreateObjects(data_list);
		foreach (int created_id, EditorObject created_object: created_objects) {
			if (!created_object) {
				continue;
			}

			EditorEvents.ObjectPlaced(m_Editor, created_object);
			m_Editor.SelectObject(created_object);
			placed_objects.Insert(created_object);
		}

		if (GetGame().IsMultiplayer()) {
			m_Editor.GetEditorHud().CreateNotification(string.Format("Prefab '%1' sent to the server (%2 objects)", prefab_name, data_list.Count()));
		} else {
			m_Editor.GetEditorHud().CreateNotification(string.Format("Placed prefab '%1' (%2 objects)", prefab_name, placed_objects.Count()));
		}

		return placed_objects;
	}
}
