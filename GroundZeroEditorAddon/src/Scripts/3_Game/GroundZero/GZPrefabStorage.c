/*
	Ground Zero DayZ Editor Addon - prefab library storage.

	Prefabs live in   <profile>/Editor/Prefabs/<any folders>/<name>.gzprefab
	Folders are real folders on disk, so the library can also be organised in Explorer.
*/

class GZPrefabStorage
{
	static const string EXTENSION = ".gzprefab";
	static const string ROOT = "$saves:Editor/Prefabs";

	static void EnsureRoot()
	{
		MakeDirectory("$saves:Editor");
		MakeDirectory(ROOT);
	}

	static string Join(string directory, string name)
	{
		if (directory == string.Empty) {
			return name;
		}

		return directory + "/" + name;
	}

	// Removes anything that cannot live in a file / folder name
	static string SanitizeName(string name)
	{
		string clean = name;
		clean.Replace("\\", string.Empty);
		clean.Replace("/", string.Empty);
		clean.Replace(":", string.Empty);
		clean.Replace("*", string.Empty);
		clean.Replace("?", string.Empty);
		clean.Replace("\"", string.Empty);
		clean.Replace("<", string.Empty);
		clean.Replace(">", string.Empty);
		clean.Replace("|", string.Empty);
		clean.Replace(".", string.Empty);
		clean.Replace("\t", string.Empty);
		clean.Replace("\r", string.Empty);
		clean.Replace("\n", string.Empty);
		return clean.Trim();
	}

	static bool IsPrefabFileName(string file_name)
	{
		int ext_len = EXTENSION.Length();
		int name_len = file_name.Length();
		if (name_len <= ext_len) {
			return false;
		}

		string tail = file_name.Substring(name_len - ext_len, ext_len);
		tail.ToLower();
		return tail == EXTENSION;
	}

	// "$saves:Editor/Prefabs/Houses/Big Barn.gzprefab" -> "Big Barn"
	static string GetPrefabDisplayName(string path)
	{
		string file_name = GetLeafName(path);
		if (IsPrefabFileName(file_name)) {
			return file_name.Substring(0, file_name.Length() - EXTENSION.Length());
		}

		return file_name;
	}

	static string GetLeafName(string path)
	{
		int slash = path.LastIndexOf("/");
		if (slash == -1) {
			return path;
		}

		return path.Substring(slash + 1, path.Length() - slash - 1);
	}

	static string GetParentDirectory(string path)
	{
		int slash = path.LastIndexOf("/");
		if (slash == -1) {
			return string.Empty;
		}

		return path.Substring(0, slash);
	}

	// Lists the sub folders and the prefab files that are directly inside a directory
	static void ListDirectory(string directory, out array<string> folders, out array<string> prefabs)
	{
		folders = new array<string>();
		prefabs = new array<string>();

		string entry_name;
		FileAttr entry_attr;
		FindFileHandle handle = FindFile(directory + "/*", entry_name, entry_attr, FindFileFlags.DIRECTORIES);
		if (!handle) {
			return;
		}

		ClassifyEntry(entry_name, folders, prefabs);
		while (FindNextFile(handle, entry_name, entry_attr)) {
			ClassifyEntry(entry_name, folders, prefabs);
		}

		CloseFindFile(handle);

		folders.Sort();
		prefabs.Sort();
	}

	protected static void ClassifyEntry(string entry_name, array<string> folders, array<string> prefabs)
	{
		if (entry_name == string.Empty || entry_name == "." || entry_name == "..") {
			return;
		}

		if (IsPrefabFileName(entry_name)) {
			prefabs.Insert(entry_name);
			return;
		}

		// Our own folders can never contain a dot, anything with a dot is some other kind of file
		if (entry_name.IndexOf(".") == -1) {
			folders.Insert(entry_name);
		}
	}

	static bool CreateFolder(string parent_directory, string folder_name)
	{
		string clean = SanitizeName(folder_name);
		if (clean == string.Empty) {
			return false;
		}

		EnsureRoot();
		string path = Join(parent_directory, clean);
		if (FileExist(path)) {
			return true;
		}

		return MakeDirectory(path);
	}

	// Picks "Name", "Name (2)", "Name (3)" ... so nothing is ever silently overwritten
	static string FindFreeFilePath(string directory, string base_name)
	{
		string path = Join(directory, base_name + EXTENSION);
		int counter = 2;
		while (FileExist(path) && counter < 1000) {
			path = Join(directory, string.Format("%1 (%2)%3", base_name, counter, EXTENSION));
			counter++;
		}

		return path;
	}

	static bool Save(string path, GZPrefabFile prefab, out string error)
	{
		if (!prefab) {
			error = "Nothing to save";
			return false;
		}

		EnsureRoot();

		if (!JsonFileLoader<GZPrefabFile>.SaveFile(path, prefab, error)) {
			return false;
		}

		return FileExist(path);
	}

	static GZPrefabFile Load(string path, out string error)
	{
		GZPrefabFile prefab = new GZPrefabFile();
		if (!JsonFileLoader<GZPrefabFile>.LoadFile(path, prefab, error)) {
			return null;
		}

		return prefab;
	}

	static bool DeletePrefab(string path)
	{
		return DeleteFile(path);
	}

	// Only empty folders can be removed, this never deletes prefabs by accident
	static bool DeleteFolder(string path)
	{
		array<string> sub_folders, sub_prefabs;
		ListDirectory(path, sub_folders, sub_prefabs);
		if (sub_folders.Count() > 0 || sub_prefabs.Count() > 0) {
			return false;
		}

		return DeleteFile(path);
	}

	// Copies a prefab to a new path and only then removes the old one
	static bool Move(string from_path, string to_path)
	{
		if (from_path == to_path) {
			return true;
		}

		if (FileExist(to_path)) {
			return false;
		}

		if (!CopyFile(from_path, to_path)) {
			return false;
		}

		if (!FileExist(to_path)) {
			return false;
		}

		DeleteFile(from_path);
		return true;
	}
}
