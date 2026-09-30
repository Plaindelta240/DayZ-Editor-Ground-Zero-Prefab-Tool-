Ground Zero DayZ Editor Addon
A prefab library for the DayZ Editor (InclementDab, Workshop 2250764298).

Select any group of objects in the Editor, save them as a prefab, keep them in a folder library, and drag them back into the world. Every object keeps its exact position and rotation relative to the rest of the group, so furniture stays exactly where it was inside a building.

Built and tested against: DayZ 1.29, DayZ Editor 1.35.6, Dabs Framework, Community Framework.

Requirements (load order)
@CF (Community Framework, 1559212036)
@Dabs Framework (2545327648)
@DayZ-Editor (2250764298)
@Ground Zero DayZ Editor Addon (this mod)
Install
Copy the @Ground Zero DayZ Editor Addon folder next to your other mods (for example into the DayZ game folder) and add it to the -mod= list after the three mods above.

The PBO is unsigned, which is fine for offline / singleplayer Editor use. To use it on a server that verifies signatures, sign addons\GroundZeroEditorAddon.pbo with DayZ Tools (DSCreateKey + DSSignFile) and put the .bikey in a keys folder.

Using it
Action	How
Make a prefab	Select objects, right click, Make Prefab... (or Ctrl+Shift+P, or Edit menu)
Open the library	Ctrl+P or View > Prefab Library
Place a prefab	Drag it into the world, double click it, or Place
While placing	Click = place, R / rotate keys = turn, PageUp / PageDown = raise / lower (Shift fine, Ctrl coarse), Shift = keep placing copies, Esc = cancel
Undo a placement	Ctrl+Z (one step for the whole prefab)
Library window: folder tree on the left, folders and prefabs of the selected folder on the right, search box (searches the whole library), New Folder, Rename, Move To..., Delete (empty folders only).

Where prefabs are stored
Documents\DayZ\Editor\Prefabs\<folders>\<name>.gzprefab

Plain JSON, one file per prefab, real folders on disk. Files can be copied between machines and shared. Every object is stored as an offset from a single anchor point, plus its yaw/pitch/roll and scale as reported by the Editor.

How the exact positioning works
Saving: anchor = centre of the objects' footprint at the height of the lowest object. Each object stores position - anchor, its orientation and its scale.

Placing: world = anchor(cursor position, yaw) * relative. Only the anchor moves and turns. With no rotation the stored numbers are used untouched (no maths at all); with rotation the same matrix maths the Editor uses for its own "Import Relative" is applied. The Editor's built-in "in hand" placement is deliberately not used because it aligns each object by its own bounding-box bottom, which would shift objects of different heights.

Source layout
src/config.cpp                                   CfgPatches / CfgMods
src/Scripts/3_Game/GroundZero/GZPrefabData.c     prefab file format
src/Scripts/3_Game/GroundZero/GZPrefabStorage.c  folder + file handling
src/Scripts/4_World/GroundZero/GZPrefabBuilder.c build a prefab / placement maths
src/Scripts/4_World/GroundZero/GZPrefabPlacer.c  live preview + commit
src/Scripts/4_World/GroundZero/GZEditor.c        hooks into the Editor's placing flow
src/Scripts/4_World/GroundZero/GZCommands.c      [RegisterEditorCommand] commands + hotkeys
src/Scripts/4_World/GroundZero/GZMenus.c         menu entries
src/Scripts/4_World/GroundZero/GZLibraryRow.c    list rows
src/Scripts/4_World/GroundZero/GZPrefabLibraryWindow.c   the library window
src/GUI/layouts/*.layout                         window + row layouts
build.ps1                                        packs src\ into the PBO (no DayZ Tools needed)
tools/Pbo.ps1                                    small PBO packer / unpacker used by build.ps1
Building
powershell -ExecutionPolicy Bypass -File .\build.ps1            # build into dist\
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Install   # build and copy into the DayZ game folder
build.ps1 packs src\, unpacks the result again and compares every file byte for byte before finishing.

Script errors only show up in-game: check %LOCALAPPDATA%\DayZ\script_*.log after launching. (A NULL pointer ... EditorVersionCallback exception in that log comes from the Editor itself, not from this addon.)

Known limits
Stores what the Editor itself stores: attachments / inventory contents and hidden (deleted) map objects are not part of a prefab.
Objects from mods that are not loaded are skipped when placing (with a warning).
The Editor only runs on approved servers in multiplayer; this addon is aimed at offline / singleplayer use.
Folders cannot be renamed yet, only created and deleted (when empty).

(Claude Was Used To Code Part Of This Mod)
