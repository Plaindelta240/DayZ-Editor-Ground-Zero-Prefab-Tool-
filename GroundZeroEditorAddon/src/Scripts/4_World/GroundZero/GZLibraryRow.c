/*
	Ground Zero DayZ Editor Addon - one row of the prefab library (a folder or a prefab).
*/

class GZLibraryRow: ScriptView
{
	protected string m_Path;
	protected bool m_IsFolder;
	protected bool m_IsTree;
	protected bool m_Selected;
	protected float m_ClickTick;

	ImageWidget Icon;
	TextWidget Label;

	void GZLibraryRow(string path, string label, bool is_folder, bool is_tree, int indent)
	{
		m_Path = path;
		m_IsFolder = is_folder;
		m_IsTree = is_tree;

		string indent_text;
		for (int indent_index = 0; indent_index < indent; indent_index++) {
			indent_text = indent_text + "    ";
		}

		Label.SetText(indent_text + label);

		if (m_IsFolder) {
			Symbols.FOLDER.Load(Icon, 3);
			Icon.SetColor(0xFFFFCD45);
		} else {
			Symbols.CUBES.Load(Icon, 3);
			Icon.SetColor(0xFF9CD3FF);
		}

		// Only prefabs in the list can be dragged into the world
		if (m_IsFolder || m_IsTree) {
			m_LayoutRoot.ClearFlags(WidgetFlags.DRAGGABLE);
		}

		SetSelected(false);
	}

	string GetPath()
	{
		return m_Path;
	}

	bool IsFolder()
	{
		return m_IsFolder;
	}

	bool IsTreeRow()
	{
		return m_IsTree;
	}

	void SetSelected(bool selected)
	{
		m_Selected = selected;
		if (selected) {
			m_LayoutRoot.SetColor(ARGB(255, 0, 122, 204));
		} else {
			m_LayoutRoot.SetColor(ARGB(255, 36, 40, 46));
		}
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!m_Selected) {
			m_LayoutRoot.SetColor(ARGB(255, 52, 58, 66));
		}

		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_Selected) {
			m_LayoutRoot.SetColor(ARGB(255, 36, 40, 46));
		}

		return super.OnMouseLeave(w, enterW, x, y);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		GZPrefabLibraryWindow window = GZPrefabLibraryWindow.Cast(GetParent());
		if (!window || button != 0) {
			return super.OnMouseButtonDown(w, x, y, button);
		}

		bool double_click = GetGame().GetTickTime() < m_ClickTick + 0.3;
		if (double_click) {
			m_ClickTick = 0;
		} else {
			m_ClickTick = GetGame().GetTickTime();
		}

		if (m_IsTree) {
			window.NavigateDeferred(m_Path);
		} else if (m_IsFolder) {
			if (double_click) {
				window.NavigateDeferred(m_Path);
			} else {
				window.SelectRow(this);
			}
		} else {
			window.SelectRow(this);
			if (double_click) {
				window.PlaceRow(this);
			}
		}

		return super.OnMouseButtonDown(w, x, y, button);
	}

	// Dragging a prefab out of the library: the real objects appear in the world and follow the cursor
	override bool OnDrag(Widget w, int x, int y)
	{
		GZPrefabLibraryWindow window = GZPrefabLibraryWindow.Cast(GetParent());
		if (window && !m_IsFolder && !m_IsTree) {
			if (window.BeginDrag(this)) {
				w.Show(false);
			}
		}

		return super.OnDrag(w, x, y);
	}

	override bool OnDrop(Widget w, int x, int y, Widget reciever)
	{
		GZPrefabLibraryWindow window = GZPrefabLibraryWindow.Cast(GetParent());
		if (window) {
			window.EndDrag();
		}

		w.Show(true);
		return super.OnDrop(w, x, y, reciever);
	}

	override string GetLayoutFile()
	{
		return "GroundZeroEditorAddon/GUI/layouts/GZLibraryRow.layout";
	}
}
