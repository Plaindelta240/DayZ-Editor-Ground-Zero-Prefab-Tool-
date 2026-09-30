class CfgPatches
{
	class GroundZeroEditorAddon_Scripts
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] =
		{
			"DZ_Scripts",
			"JM_CF_Scripts",
			"DF_Scripts",
			"Editor_Scripts"
		};
	};
};

class CfgMods
{
	class GroundZeroEditorAddon
	{
		dir = "GroundZeroEditorAddon";
		name = "Ground Zero DayZ Editor Addon";
		author = "Ground Zero";
		version = "1.0";
		type = "mod";
		dependencies[] = { "Game", "World" };

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = { "GroundZeroEditorAddon/Scripts/3_Game" };
			};
			class worldScriptModule
			{
				value = "";
				files[] = { "GroundZeroEditorAddon/Scripts/4_World" };
			};
		};
	};
};
