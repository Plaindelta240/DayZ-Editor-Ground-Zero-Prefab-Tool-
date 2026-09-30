/*
	Ground Zero DayZ Editor Addon - the prefab library window.

	Left   : folder tree of the library (real folders under  <profile>/Editor/Prefabs)
	Right  : the folders and prefabs of the selected folder (or search results)
	Bottom : save the objects you have selected in the world as a new prefab in the current folder

	A prefab can be placed by dragging it into the world, double clicking it, or with the Place button.
*/

class GZPrefabLibraryController: ViewController
{
	ref ObservableCollection<ref GZLibraryRow> Tree = new ObservableCollection<ref GZLibraryRow>(this);
	ref ObservableCollection<ref GZLibraryRow> Items = new ObservableCollection<ref GZLibraryRow>(this);
}

enum GZPromptMode
{
	NONE = 0,
	NEW_FOLDER,
	RENAME,
	MOVE,
	DELETE
}

class GZPrefabLibraryWindow: EditorModal
{
	protected static ref GZPrefabLibraryWindow s_Instance;
	protected static string s_LastDirectory;
	protected static float s_LastScreenX = -1;
	protected static float s_LastScreenY = -1;

	protected static const string SEARCH_PLACEHOLDER = "Search...";
	protected static const string DEFAULT_PREFAB_NAME = "New Prefab";
	protected static const int MAX_TREE_DEPTH = 6;

	protected GZPrefabLibraryController m_TemplateController;

	protected string m_CurrentDirectory;
	protected string m_Filter;
	protected GZLibraryRow m_SelectedRow;
	protected GZLibraryRow m_DraggedRow;
	protected ref GZPrefabFile m_SelectedPrefab;
	protected GZPromptMode m_PromptMode;
	protected int m_LastSelectionCount = -1;
	protected bool m_Closing;

	ButtonWidget TitleClose;
	ButtonWidget UpButton, NewFolderButton, RefreshButton;
	TextWidget PathText;
	EditBoxWidget SearchBox;
	ScrollWidget TreeScroller, ItemsScroller;
	TextWidget EmptyText;
	TextWidget DetailsTitle, DetailsText;
	EditBoxWidget PrefabNameBox;
	ButtonWidget SavePrefabButton;
	TextWidget SelectionText;
	ButtonWidget PlaceButton, RenameButton, MoveButton, DeleteButton;
	Widget PromptPanel, PromptBoxWrapper;
	TextWidget PromptLabel;
	EditBoxWidget PromptBox;
	ButtonWidget PromptOK, PromptCancel;

	void GZPrefabLibraryWindow()
	{
		m_TemplateController = GZPrefabLibraryController.Cast(m_Controller);
		TitleText.SetText("Ground Zero Prefab Library");

		GZPrefabStorage.EnsureRoot();

		string start_directory = s_LastDirectory;
		if (start_directory == string.Empty || !FileExist(start_directory)) {
			start_directory = GZPrefabStorage.ROOT;
		}

		NavigateTo(start_directory);
		UpdateDetails();

		int screen_w, screen_h;
		GetScreenSize(screen_w, screen_h);
		float window_w, window_h;
		m_LayoutRoot.GetScreenSize(window_w, window_h);
		if (s_LastScreenX >= 0) {
			m_LayoutRoot.SetScreenPos(s_LastScreenX, s_LastScreenY);
		} else {
			m_LayoutRoot.SetScreenPos(24, Math.Max(10, (screen_h - window_h) * 0.5));
		}

		Editor editor = GetEditor();
		if (editor && editor.GetEditorHud()) {
			editor.GetEditorHud().ShowCursor(true);
		}
	}

	void ~GZPrefabLibraryWindow()
	{
		if (m_LayoutRoot) {
			m_LayoutRoot.GetScreenPos(s_LastScreenX, s_LastScreenY);
		}

		// Never leave a preview behind that belongs to a dragged row that no longer exists
		Editor editor = GetEditor();
		if (editor && m_DraggedRow) {
			editor.GetPrefabPlacer().Cancel();
		}
	}

	/* ---------------------------------------------------------------- static API */

	static bool IsOpen()
	{
		return s_Instance != null;
	}

	static GZPrefabLibraryWindow Open()
	{
		// A window left over from a previous editor session has no widgets any more
		if (s_Instance && !s_Instance.GetLayoutRoot()) {
			delete s_Instance;
		}

		if (!s_Instance) {
			s_Instance = new GZPrefabLibraryWindow();
		}

		return s_Instance;
	}

	static void Toggle()
	{
		if (s_Instance && s_Instance.GetLayoutRoot()) {
			s_Instance.Close();
			return;
		}

		Open();
	}

	// Deferred by ScriptView.Delete, so this is safe to call from inside the window's own events
	void Close()
	{
		if (m_Closing) {
			return;
		}

		m_Closing = true;
		Delete();
	}

	static void OpenForSave()
	{
		GZPrefabLibraryWindow window = Open();
		if (window) {
			window.FocusSave();
		}
	}

	void FocusSave()
	{
		PrefabNameBox.SetText(DEFAULT_PREFAB_NAME);
		SetFocus(PrefabNameBox);
	}

	/* ---------------------------------------------------------------- navigation */

	// Rows call this from their own mouse events, rebuilding the lists in-place would delete the row mid-event
	void NavigateDeferred(string directory)
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(NavigateTo, 0, false, directory);
	}

	void NavigateTo(string directory)
	{
		if (directory == string.Empty || !FileExist(directory)) {
			directory = GZPrefabStorage.ROOT;
		}

		m_CurrentDirectory = directory;
		s_LastDirectory = directory;
		m_Filter = string.Empty;
		if (SearchBox.GetText() != SEARCH_PLACEHOLDER) {
			SearchBox.SetText(SEARCH_PLACEHOLDER);
		}

		ClearSelection();
		RefreshTree();
		RefreshItems();
		PathText.SetText(GetDisplayPath(m_CurrentDirectory));
	}

	void Refresh()
	{
		string keep_path;
		if (m_SelectedRow) {
			keep_path = m_SelectedRow.GetPath();
		}

		RefreshTree();
		RefreshItems();
		SelectPath(keep_path);
	}

	protected void RefreshTree()
	{
		m_TemplateController.Tree.Clear();

		AddTreeRow(GZPrefabStorage.ROOT, "Prefabs", 0);
		AddTreeChildren(GZPrefabStorage.ROOT, 1);
	}

	protected void AddTreeChildren(string directory, int depth)
	{
		array<string> child_folders, child_prefabs;
		GZPrefabStorage.ListDirectory(directory, child_folders, child_prefabs);
		foreach (string child_folder: child_folders) {
			string child_path = GZPrefabStorage.Join(directory, child_folder);
			AddTreeRow(child_path, child_folder, depth);
			if (depth < MAX_TREE_DEPTH) {
				AddTreeChildren(child_path, depth + 1);
			}
		}
	}

	protected void AddTreeRow(string path, string label, int depth)
	{
		GZLibraryRow tree_row = new GZLibraryRow(path, label, true, true, depth);
		tree_row.SetParent(this);
		tree_row.SetSelected(path == m_CurrentDirectory);
		m_TemplateController.Tree.Insert(tree_row);
	}

	protected void RefreshItems()
	{
		m_SelectedRow = null;
		m_TemplateController.Items.Clear();

		if (m_Filter == string.Empty) {
			array<string> folders, prefabs;
			GZPrefabStorage.ListDirectory(m_CurrentDirectory, folders, prefabs);
			foreach (string folder_name: folders) {
				AddItemRow(GZPrefabStorage.Join(m_CurrentDirectory, folder_name), folder_name, true);
			}

			foreach (string prefab_file: prefabs) {
				AddItemRow(GZPrefabStorage.Join(m_CurrentDirectory, prefab_file), GZPrefabStorage.GetPrefabDisplayName(prefab_file), false);
			}
		} else {
			array<string> found_paths = {};
			CollectPrefabs(GZPrefabStorage.ROOT, found_paths, 0);

			string filter_lower = m_Filter;
			filter_lower.ToLower();
			foreach (string found_path: found_paths) {
				string found_label = GetDisplayPath(found_path);
				string found_lower = found_label;
				found_lower.ToLower();
				if (found_lower.Contains(filter_lower)) {
					AddItemRow(found_path, found_label, false);
				}
			}
		}

		EmptyText.Show(m_TemplateController.Items.Count() == 0);
	}

	protected void AddItemRow(string path, string label, bool is_folder)
	{
		GZLibraryRow item_row = new GZLibraryRow(path, label, is_folder, false, 0);
		item_row.SetParent(this);
		m_TemplateController.Items.Insert(item_row);
	}

	protected void CollectPrefabs(string directory, array<string> results, int depth)
	{
		array<string> sub_folders, sub_prefabs;
		GZPrefabStorage.ListDirectory(directory, sub_folders, sub_prefabs);
		foreach (string sub_prefab: sub_prefabs) {
			results.Insert(GZPrefabStorage.Join(directory, sub_prefab));
		}

		if (depth >= MAX_TREE_DEPTH + 1) {
			return;
		}

		foreach (string sub_folder: sub_folders) {
			CollectPrefabs(GZPrefabStorage.Join(directory, sub_folder), results, depth + 1);
		}
	}

	// "$saves:Editor/Prefabs/Houses/Barn.gzprefab" -> "Houses/Barn"
	protected string GetDisplayPath(string path)
	{
		string root_prefix = GZPrefabStorage.ROOT + "/";
		string display = path;
		if (display.Length() >= root_prefix.Length() && display.Substring(0, root_prefix.Length()) == root_prefix) {
			display = display.Substring(root_prefix.Length(), display.Length() - root_prefix.Length());
		} else if (display == GZPrefabStorage.ROOT) {
			return "Prefabs";
		}

		if (GZPrefabStorage.IsPrefabFileName(GZPrefabStorage.GetLeafName(display))) {
			display = display.Substring(0, display.Length() - GZPrefabStorage.EXTENSION.Length());
		}

		return display;
	}

	/* ---------------------------------------------------------------- selection and details */

	protected void ClearSelection()
	{
		if (m_SelectedRow) {
			m_SelectedRow.SetSelected(false);
		}

		m_SelectedRow = null;
		m_SelectedPrefab = null;
		UpdateDetails();
	}

	void SelectRow(GZLibraryRow row)
	{
		if (m_SelectedRow && m_SelectedRow != row) {
			m_SelectedRow.SetSelected(false);
		}

		m_SelectedRow = row;
		m_SelectedPrefab = null;
		if (row) {
			row.SetSelected(true);
			if (!row.IsFolder()) {
				string load_error;
				m_SelectedPrefab = GZPrefabStorage.Load(row.GetPath(), load_error);
			}
		}

		UpdateDetails();
	}

	protected void SelectPath(string path)
	{
		if (path == string.Empty) {
			return;
		}

		foreach (GZLibraryRow item_row: m_TemplateController.Items.GetArray()) {
			if (item_row && item_row.GetPath() == path) {
				SelectRow(item_row);
				return;
			}
		}
	}

	protected void UpdateDetails()
	{
		if (!m_SelectedRow) {
			DetailsTitle.SetText(GetDisplayPath(m_CurrentDirectory));
			DetailsText.SetText(string.Format("%1 items in this folder.   Select a prefab to see its details.", m_TemplateController.Items.Count()));
			return;
		}

		if (m_SelectedRow.IsFolder()) {
			DetailsTitle.SetText(GZPrefabStorage.GetLeafName(m_SelectedRow.GetPath()));
			DetailsText.SetText("Folder.   Double click to open it.");
			return;
		}

		if (!m_SelectedPrefab) {
			DetailsTitle.SetText(GZPrefabStorage.GetPrefabDisplayName(m_SelectedRow.GetPath()));
			DetailsText.SetText("This prefab file could not be read.");
			return;
		}

		DetailsTitle.SetText(m_SelectedPrefab.Name);

		string details = string.Format("%1 objects   |   %2 x %3 x %4 m", m_SelectedPrefab.Count(), FormatMeters(m_SelectedPrefab.Size[0]), FormatMeters(m_SelectedPrefab.Size[2]), FormatMeters(m_SelectedPrefab.Size[1]));
		if (m_SelectedPrefab.MapName != string.Empty) {
			details = details + "   |   made on " + m_SelectedPrefab.MapName;
		}

		if (m_SelectedPrefab.Created != string.Empty) {
			details = details + "   |   " + m_SelectedPrefab.Created;
		}

		DetailsText.SetText(details);
	}

	protected string FormatMeters(float value)
	{
		int tenths = Math.Round(value * 10);
		return string.Format("%1.%2", tenths / 10, tenths % 10);
	}

	/* ---------------------------------------------------------------- saving a prefab */

	void SaveSelection()
	{
		Editor editor = GetEditor();
		if (!editor) {
			return;
		}

		EditorObjectMap selected_objects = editor.GetSelectedObjects();
		if (!selected_objects || selected_objects.Count() == 0) {
			Notify("Select the objects you want to save first (click them in the world or in the placed list).");
			return;
		}

		string prefab_name = GZPrefabStorage.SanitizeName(PrefabNameBox.GetText());
		if (prefab_name == string.Empty) {
			prefab_name = "Prefab";
		}

		int skipped_count;
		GZPrefabFile prefab = GZPrefabBuilder.CreateFromObjects(prefab_name, string.Empty, selected_objects.GetValueArray(), skipped_count);
		if (!prefab) {
			Notify("None of the selected objects can be saved into a prefab.");
			return;
		}

		// Never overwrite: an existing name simply gets a number added
		string prefab_path = GZPrefabStorage.FindFreeFilePath(m_CurrentDirectory, prefab_name);
		prefab.Name = GZPrefabStorage.GetPrefabDisplayName(prefab_path);

		string save_error;
		if (!GZPrefabStorage.Save(prefab_path, prefab, save_error)) {
			Notify("Could not save the prefab: " + save_error);
			return;
		}

		string message = string.Format("Saved prefab '%1' (%2 objects)", prefab.Name, prefab.Count());
		if (skipped_count > 0) {
			message = message + string.Format(", %1 skipped", skipped_count);
		}

		Notify(message);

		m_Filter = string.Empty;
		SearchBox.SetText(SEARCH_PLACEHOLDER);
		RefreshTree();
		RefreshItems();
		SelectPath(prefab_path);
		PrefabNameBox.SetText(DEFAULT_PREFAB_NAME);
	}

	/* ---------------------------------------------------------------- placing */

	void PlaceRow(GZLibraryRow row)
	{
		if (row && !row.IsFolder()) {
			BeginPlacing(row.GetPath());
		}
	}

	protected bool BeginPlacing(string prefab_path)
	{
		Editor editor = GetEditor();
		if (!editor) {
			return false;
		}

		string load_error;
		GZPrefabFile prefab = GZPrefabStorage.Load(prefab_path, load_error);
		if (!prefab) {
			Notify("Could not read that prefab: " + load_error);
			return false;
		}

		GZPrefabPlacer placer = editor.GetPrefabPlacer();
		if (!placer.Begin(prefab, prefab_path)) {
			Notify(string.Format("None of the %1 objects in '%2' exist in this game (missing mod?).", prefab.Count(), prefab.Name));
			return false;
		}

		if (placer.GetMissingCount() > 0) {
			Notify(string.Format("%1 objects are not available in this game and are left out.", placer.GetMissingCount()));
		} else {
			Notify(string.Format("Placing '%1': click to place, R / rotate keys to turn, PageUp / PageDown to raise or lower, Shift keeps placing, Esc cancels", prefab.Name), 8.0);
		}

		return true;
	}

	bool BeginDrag(GZLibraryRow row)
	{
		SelectRow(row);
		if (BeginPlacing(row.GetPath())) {
			m_DraggedRow = row;
			return true;
		}

		return false;
	}

	// Mouse released after dragging a prefab: drop it into the world, or cancel when let go over the UI
	void EndDrag()
	{
		if (!m_DraggedRow) {
			return;
		}

		GZLibraryRow dragged_row = m_DraggedRow;
		m_DraggedRow = null;
		dragged_row.Show(true);

		Editor editor = GetEditor();
		if (!editor || !editor.IsPlacingPrefab()) {
			return;
		}

		if (IsCursorOverWorld()) {
			editor.PlaceObject();
		} else {
			editor.GetPrefabPlacer().Cancel();
		}
	}

	static bool IsCursorOverWorld()
	{
		Widget widget_under_cursor = GetWidgetUnderCursor();
		if (!widget_under_cursor) {
			return true;
		}

		return widget_under_cursor.IsInherited(MapWidget) || widget_under_cursor.GetName() == "HudPanel" || widget_under_cursor.GetName() == "CursorIcons";
	}

	/* ---------------------------------------------------------------- prompt (new folder / rename / move / delete) */

	protected void ShowPrompt(GZPromptMode mode, string label, string default_text, bool has_input)
	{
		m_PromptMode = mode;
		PromptLabel.SetText(label);
		PromptBox.SetText(default_text);
		PromptBoxWrapper.Show(has_input);
		PromptPanel.Show(true);
		if (has_input) {
			SetFocus(PromptBox);
		}
	}

	protected void HidePrompt()
	{
		m_PromptMode = GZPromptMode.NONE;
		PromptPanel.Show(false);
		SetFocus(null);
	}

	protected void ConfirmPrompt()
	{
		GZPromptMode mode = m_PromptMode;
		string input_text = PromptBox.GetText();
		HidePrompt();

		switch (mode) {
			case GZPromptMode.NEW_FOLDER: {
				DoCreateFolder(input_text);
				break;
			}

			case GZPromptMode.RENAME: {
				DoRename(input_text);
				break;
			}

			case GZPromptMode.MOVE: {
				DoMove(input_text);
				break;
			}

			case GZPromptMode.DELETE: {
				DoDelete();
				break;
			}
		}
	}

	protected void DoCreateFolder(string folder_name)
	{
		string clean_name = GZPrefabStorage.SanitizeName(folder_name);
		if (clean_name == string.Empty) {
			Notify("Folder names cannot be empty.");
			return;
		}

		if (!GZPrefabStorage.CreateFolder(m_CurrentDirectory, clean_name)) {
			Notify("Could not create that folder.");
			return;
		}

		string new_path = GZPrefabStorage.Join(m_CurrentDirectory, clean_name);
		RefreshTree();
		RefreshItems();
		SelectPath(new_path);
	}

	protected void DoRename(string new_name)
	{
		if (!m_SelectedRow || m_SelectedRow.IsFolder()) {
			return;
		}

		string clean_name = GZPrefabStorage.SanitizeName(new_name);
		if (clean_name == string.Empty) {
			Notify("Prefab names cannot be empty.");
			return;
		}

		string old_path = m_SelectedRow.GetPath();
		string new_path = GZPrefabStorage.Join(GZPrefabStorage.GetParentDirectory(old_path), clean_name + GZPrefabStorage.EXTENSION);
		if (new_path == old_path) {
			return;
		}

		if (FileExist(new_path)) {
			Notify("A prefab with that name already exists in this folder.");
			return;
		}

		string load_error;
		GZPrefabFile prefab = GZPrefabStorage.Load(old_path, load_error);
		if (!prefab) {
			Notify("Could not read that prefab: " + load_error);
			return;
		}

		prefab.Name = clean_name;
		string save_error;
		if (!GZPrefabStorage.Save(new_path, prefab, save_error)) {
			Notify("Could not rename: " + save_error);
			return;
		}

		GZPrefabStorage.DeletePrefab(old_path);
		Refresh();
		SelectPath(new_path);
	}

	protected void DoMove(string target_folder)
	{
		if (!m_SelectedRow || m_SelectedRow.IsFolder()) {
			return;
		}

		// Any number of nested folders, created on the way. Empty = the library root.
		string target_directory = GZPrefabStorage.ROOT;
		string normalized = target_folder;
		normalized.Replace("\\", "/");
		array<string> segments = {};
		normalized.Split("/", segments);
		foreach (string segment: segments) {
			string clean_segment = GZPrefabStorage.SanitizeName(segment);
			if (clean_segment == string.Empty) {
				continue;
			}

			if (!GZPrefabStorage.CreateFolder(target_directory, clean_segment)) {
				Notify("Could not create the target folder.");
				return;
			}

			target_directory = GZPrefabStorage.Join(target_directory, clean_segment);
		}

		string old_path = m_SelectedRow.GetPath();
		string new_path = GZPrefabStorage.Join(target_directory, GZPrefabStorage.GetLeafName(old_path));
		if (new_path == old_path) {
			return;
		}

		if (!GZPrefabStorage.Move(old_path, new_path)) {
			Notify("Could not move it (is there already a prefab with that name in the target folder?).");
			return;
		}

		Notify("Moved to " + GetDisplayPath(target_directory));
		RefreshTree();
		RefreshItems();
	}

	protected void DoDelete()
	{
		if (!m_SelectedRow) {
			return;
		}

		string path = m_SelectedRow.GetPath();
		if (m_SelectedRow.IsFolder()) {
			if (!GZPrefabStorage.DeleteFolder(path)) {
				Notify("Only empty folders can be deleted. Move or delete what is inside first.");
				return;
			}
		} else {
			if (!GZPrefabStorage.DeletePrefab(path)) {
				Notify("Could not delete that prefab.");
				return;
			}
		}

		RefreshTree();
		RefreshItems();
		UpdateDetails();
	}

	/* ---------------------------------------------------------------- events */

	override void Update(float dt)
	{
		Editor editor = GetEditor();
		if (!editor) {
			return;
		}

		EditorObjectMap selected_objects = editor.GetSelectedObjects();
		int selection_count;
		if (selected_objects) {
			selection_count = selected_objects.Count();
		}

		if (selection_count != m_LastSelectionCount) {
			m_LastSelectionCount = selection_count;
			SelectionText.SetText(string.Format("%1 selected", selection_count));
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0) {
			return super.OnClick(w, x, y, button);
		}

		switch (w) {
			case TitleClose: {
				Close();
				return true;
			}

			case UpButton: {
				if (m_CurrentDirectory != GZPrefabStorage.ROOT) {
					NavigateTo(GZPrefabStorage.GetParentDirectory(m_CurrentDirectory));
				}

				return true;
			}

			case NewFolderButton: {
				ShowPrompt(GZPromptMode.NEW_FOLDER, "New folder inside '" + GetDisplayPath(m_CurrentDirectory) + "':", string.Empty, true);
				return true;
			}

			case RefreshButton: {
				Refresh();
				return true;
			}

			case SavePrefabButton: {
				SaveSelection();
				return true;
			}

			case PlaceButton: {
				if (m_SelectedRow && !m_SelectedRow.IsFolder()) {
					PlaceRow(m_SelectedRow);
				} else {
					Notify("Select a prefab in the list first.");
				}

				return true;
			}

			case RenameButton: {
				if (m_SelectedRow && !m_SelectedRow.IsFolder()) {
					ShowPrompt(GZPromptMode.RENAME, "Rename prefab to:", GZPrefabStorage.GetPrefabDisplayName(m_SelectedRow.GetPath()), true);
				} else {
					Notify("Select a prefab in the list first.");
				}

				return true;
			}

			case MoveButton: {
				if (m_SelectedRow && !m_SelectedRow.IsFolder()) {
					string current_folder = GetDisplayPath(GZPrefabStorage.GetParentDirectory(m_SelectedRow.GetPath()));
					if (current_folder == "Prefabs") {
						current_folder = string.Empty;
					}

					ShowPrompt(GZPromptMode.MOVE, "Move to folder (path inside the library, empty = top level):", current_folder, true);
				} else {
					Notify("Select a prefab in the list first.");
				}

				return true;
			}

			case DeleteButton: {
				if (m_SelectedRow) {
					string what = "prefab";
					if (m_SelectedRow.IsFolder()) {
						what = "empty folder";
					}

					ShowPrompt(GZPromptMode.DELETE, "Delete the " + what + " '" + GZPrefabStorage.GetLeafName(m_SelectedRow.GetPath()) + "'? This cannot be undone.", string.Empty, false);
				} else {
					Notify("Select a prefab or folder in the list first.");
				}

				return true;
			}

			case PromptOK: {
				ConfirmPrompt();
				return true;
			}

			case PromptCancel: {
				HidePrompt();
				return true;
			}
		}

		return super.OnClick(w, x, y, button);
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == SearchBox) {
			string search_text = SearchBox.GetText();
			if (search_text == SEARCH_PLACEHOLDER) {
				search_text = string.Empty;
			}

			search_text = search_text.Trim();
			if (search_text != m_Filter) {
				m_Filter = search_text;
				ClearSelection();
				RefreshItems();
			}

			return true;
		}

		if (finished) {
			if (w == PromptBox) {
				ConfirmPrompt();
				return true;
			}

			if (w == PrefabNameBox) {
				SaveSelection();
				return true;
			}
		}

		return super.OnChange(w, x, y, finished);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (w == SearchBox && SearchBox.GetText() == SEARCH_PLACEHOLDER) {
			SearchBox.SetText(string.Empty);
		}

		if (w == PrefabNameBox && PrefabNameBox.GetText() == DEFAULT_PREFAB_NAME) {
			PrefabNameBox.SetText(string.Empty);
		}

		return super.OnMouseButtonDown(w, x, y, button);
	}

	protected void Notify(string text, float duration = 4.0)
	{
		Editor editor = GetEditor();
		if (editor && editor.GetEditorHud()) {
			editor.GetEditorHud().CreateNotification(text, duration);
		}
	}

	override string GetTitle()
	{
		return "Ground Zero Prefab Library";
	}

	override typename GetControllerType()
	{
		return GZPrefabLibraryController;
	}

	override string GetLayoutFile()
	{
		return "GroundZeroEditorAddon/GUI/layouts/GZPrefabLibrary.layout";
	}
}
