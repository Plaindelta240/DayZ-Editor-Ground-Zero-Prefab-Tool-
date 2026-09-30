/*
	Ground Zero DayZ Editor Addon - editor commands.
	Registered through the Editor's own [RegisterEditorCommand] hook, so they show up in menus and get hotkeys.
*/

[RegisterEditorCommand(GZOpenPrefabLibraryCommand)]
class GZOpenPrefabLibraryCommand: EditorCommand
{
	protected override bool Execute(Class sender, CommandArgs args)
	{
		super.Execute(sender, args);
		GZPrefabLibraryWindow.Toggle();
		return true;
	}

	override string GetName()
	{
		return "Prefab Library";
	}

	override Symbols GetSymbol()
	{
		return Symbols.CUBES;
	}

	override ShortcutKeys GetShortcut()
	{
		return { KeyCode.KC_LCONTROL, KeyCode.KC_P };
	}
}

[RegisterEditorCommand(GZMakePrefabCommand)]
class GZMakePrefabCommand: EditorCommand
{
	protected override bool Execute(Class sender, CommandArgs args)
	{
		super.Execute(sender, args);

		Editor editor = GetEditor();
		if (!editor) {
			return true;
		}

		if (editor.GetSelectedObjects().Count() == 0) {
			editor.GetEditorHud().CreateNotification("Select the objects you want to turn into a prefab first");
			return true;
		}

		GZPrefabLibraryWindow.OpenForSave();
		return true;
	}

	override bool CanExecute()
	{
		return GetEditor() && GetEditor().GetSelectedObjects().Count() > 0;
	}

	override string GetName()
	{
		return "Make Prefab...";
	}

	override Symbols GetSymbol()
	{
		return Symbols.CUBES;
	}

	override ShortcutKeys GetShortcut()
	{
		return { KeyCode.KC_LCONTROL, KeyCode.KC_LSHIFT, KeyCode.KC_P };
	}
}
