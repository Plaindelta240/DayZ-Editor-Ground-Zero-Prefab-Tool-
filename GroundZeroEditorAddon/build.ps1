<#
	Ground Zero DayZ Editor Addon - build script.

	Packs  src\  into  dist\@Ground Zero DayZ Editor Addon\addons\GroundZeroEditorAddon.pbo
	(no DayZ Tools needed) and verifies the result by unpacking it again.

	  .\build.ps1              build only
	  .\build.ps1 -Install     build and copy the mod into your DayZ game folder
#>
param(
	[switch]$Install,
	[string]$GameFolder = "C:\Program Files (x86)\Steam\steamapps\common\DayZ"
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\tools\Pbo.ps1"

$modFolderName = "@Ground Zero DayZ Editor Addon"
$pboName = "GroundZeroEditorAddon.pbo"
$prefix = "GroundZeroEditorAddon"
$source = Join-Path $PSScriptRoot "src"
$modRoot = Join-Path (Join-Path $PSScriptRoot "dist") $modFolderName
$addonFolder = Join-Path $modRoot "addons"

# Fresh output
if (Test-Path $modRoot) { Remove-Item $modRoot -Recurse -Force }
New-Item -ItemType Directory -Force $addonFolder | Out-Null

# Collect every source file, path inside the pbo uses backslashes
$files = New-Object 'System.Collections.Generic.SortedDictionary[string,string]'
Get-ChildItem $source -Recurse -File | ForEach-Object {
	$relative = $_.FullName.Substring($source.Length + 1)
	$files[$relative] = $_.FullName
}

$pboPath = Join-Path $addonFolder $pboName
[PboTool]::Pack($pboPath, $prefix, $files)
Write-Host ("Packed {0} files -> {1} ({2:N0} bytes)" -f $files.Count, $pboPath, (Get-Item $pboPath).Length)

# Verify by unpacking and comparing every file byte for byte
$check = Join-Path ([IO.Path]::GetTempPath()) ("gz_pbo_check_" + [Guid]::NewGuid().ToString("N"))
$null = [PboTool]::Extract($pboPath, $check)
$bad = 0
foreach ($entry in $files.GetEnumerator()) {
	$unpacked = Join-Path $check $entry.Key
	if (-not (Test-Path $unpacked)) { Write-Host "MISSING in pbo: $($entry.Key)"; $bad++; continue }
	$a = [IO.File]::ReadAllBytes($entry.Value)
	$b = [IO.File]::ReadAllBytes($unpacked)
	if ($a.Length -ne $b.Length -or (Compare-Object $a $b -SyncWindow 0)) { Write-Host "DIFFERENT: $($entry.Key)"; $bad++ }
}
Remove-Item $check -Recurse -Force
if ($bad -gt 0) { throw "PBO verification failed ($bad problems)" }
Write-Host "PBO verified: every file matches its source."

@"
name = "Ground Zero DayZ Editor Addon";
picture = "";
logo = "";
logoSmall = "";
logoOver = "";
tooltip = "Ground Zero DayZ Editor Addon";
overview = "Prefab library for the DayZ Editor: save any selection as a prefab, organise prefabs in folders and drag them back into the world with every object in exactly the same relative position.";
author = "Ground Zero";
version = "1.0";
action = "";
"@ | Set-Content -Path (Join-Path $modRoot "mod.cpp") -Encoding ASCII

Write-Host "Built: $modRoot"

if ($Install) {
	$target = Join-Path $GameFolder $modFolderName
	if (Test-Path $target) { Remove-Item $target -Recurse -Force }
	Copy-Item $modRoot $target -Recurse -Force
	Write-Host "Installed to: $target"
}
