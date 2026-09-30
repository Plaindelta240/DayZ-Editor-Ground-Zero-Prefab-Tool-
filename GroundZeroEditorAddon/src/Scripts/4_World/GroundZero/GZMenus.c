/*
	Ground Zero DayZ Editor Addon - menu entries.
*/

modded class EditorMenu
{
	// Right click menu of a placed object: prefab entries go just above "Properties"
	override void AddMenuButton(EditorCommand editor_command)
	{
		if (editor_command && IsInherited(EditorPlacedContextMenu) && editor_command.IsInherited(EditorObjectPropertiesCommand)) {
			AddMenuDivider();
			super.AddMenuButton(m_Editor.CommandManager[GZMakePrefabCommand]);
			super.AddMenuButton(m_Editor.CommandManager[GZOpenPrefabLibraryCommand]);
			AddMenuDivider();
		}

		super.AddMenuButton(editor_command);
	}
}

modded class EditorViewMenu
{
	void EditorViewMenu()
	{
		AddMenuDivider();
		AddMenuButton(m_Editor.CommandManager[GZOpenPrefabLibraryCommand]);
	}
}

modded class EditorEditMenu
{
	void EditorEditMenu()
	{
		AddMenuDivider();
		AddMenuButton(m_Editor.CommandManager[GZMakePrefabCommand]);
	}
}
