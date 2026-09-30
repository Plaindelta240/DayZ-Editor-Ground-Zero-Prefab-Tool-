/*
	Ground Zero DayZ Editor Addon - Editor integration.

	A prefab in hand plugs into the Editor's own "placing" state, so it behaves like any other thing
	you are placing: left click drops it, Shift keeps placing copies, Escape cancels, the normal
	rotate controls turn it. The only difference is that the group is moved as one rigid piece.
*/

modded class Editor
{
	protected ref GZPrefabPlacer m_GZPlacer;

	GZPrefabPlacer GetPrefabPlacer()
	{
		if (!m_GZPlacer) {
			m_GZPlacer = new GZPrefabPlacer(this);
		}

		return m_GZPlacer;
	}

	bool IsPlacingPrefab()
	{
		return m_GZPlacer && m_GZPlacer.IsActive();
	}

	override bool IsPlacing()
	{
		if (IsPlacingPrefab()) {
			return true;
		}

		return super.IsPlacing();
	}

	override void HandleHands(float dt)
	{
		if (IsPlacingPrefab()) {
			// The editor collects the rotate controls in here, we consume them for the prefab
			float yaw_input = m_HandsInputOrientation[0];
			m_HandsInputOrientation = vector.Zero;
			m_GZPlacer.Update(dt, yaw_input);
			return;
		}

		super.HandleHands(dt);
	}

	override array<EditorObject> PlaceObject()
	{
		if (IsPlacingPrefab()) {
			array<EditorObject> placed_objects = m_GZPlacer.Commit();
			if (placed_objects) {
				m_JustPlacedObject = true;
			}

			return placed_objects;
		}

		return super.PlaceObject();
	}

	override void CancelPlacing()
	{
		if (m_GZPlacer) {
			m_GZPlacer.Cancel();
		}

		super.CancelPlacing();
	}

	override void ClearHand()
	{
		if (m_GZPlacer) {
			m_GZPlacer.Cancel();
		}

		super.ClearHand();
	}

	override void SetActive(bool active)
	{
		if (!active && m_GZPlacer) {
			m_GZPlacer.Cancel();
		}

		super.SetActive(active);
	}
}
