# Voodoo Vince Remastered - Free Camera installer / uninstaller
# Usage: run Install.bat or Uninstall.bat (they call this script).
param([switch]$Uninstall, [switch]$Elevated, [string]$GameDir = "")

$ErrorActionPreference = "Stop"
$Self = $PSCommandPath
$here = Split-Path -Parent $Self
$KnownHash = "F7E08E9BAE9656DE98B75FFFC74BFC5960AF8648F0A947AC65343934E5A35489"  # Steam v1.14.2.0
$ModFiles = @("XINPUT9_1_0.dll", "freecam.ini", "freecam_README.txt", "freecam.log")

function Done($code) { if ($Elevated) { Read-Host "`nPress Enter to close" | Out-Null }; exit $code }
function Say($msg, $color = "Gray") { Write-Host $msg -ForegroundColor $color }

function Test-GameDir($d) { return ($d -and (Test-Path (Join-Path $d "Vince.exe"))) }

function Find-GameDir {
    # 1) next to this package (user extracted it into the game folder)
    foreach ($c in @((Split-Path $here -Parent), (Split-Path (Split-Path $here -Parent) -Parent))) {
        if (Test-GameDir $c) { return $c }
    }
    # 2) every Steam library listed in libraryfolders.vdf
    $roots = @()
    foreach ($k in @("HKCU:\Software\Valve\Steam", "HKLM:\SOFTWARE\WOW6432Node\Valve\Steam", "HKLM:\SOFTWARE\Valve\Steam")) {
        try {
            $p = Get-ItemProperty $k -ErrorAction Stop
            if ($p.SteamPath)   { $roots += $p.SteamPath }
            if ($p.InstallPath) { $roots += $p.InstallPath }
        } catch {}
    }
    $libs = @()
    foreach ($r in ($roots | Select-Object -Unique)) {
        $r = $r -replace "/", "\"
        $libs += $r
        $vdf = Join-Path $r "steamapps\libraryfolders.vdf"
        if (Test-Path $vdf) {
            foreach ($m in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) {
                $libs += ($m.Groups[1].Value -replace "\\\\", "\")
            }
        }
    }
    foreach ($l in ($libs | Select-Object -Unique)) {
        $c = Join-Path $l "steamapps\common\Voodoo Vince Remastered"
        if (Test-GameDir $c) { return $c }
    }
    # 3) ask
    Add-Type -AssemblyName System.Windows.Forms
    $dlg = New-Object System.Windows.Forms.FolderBrowserDialog
    $dlg.Description = "Couldn't find Voodoo Vince Remastered automatically. Select the folder that contains Vince.exe"
    if ($dlg.ShowDialog() -eq "OK" -and (Test-GameDir $dlg.SelectedPath)) { return $dlg.SelectedPath }
    return $null
}

function Test-Admin { ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator) }

function Relaunch-Elevated($dir) {
    Say "Windows needs administrator permission to write to the game folder. Asking now..." Yellow
    $a = "-NoProfile -ExecutionPolicy Bypass -File `"$Self`" -Elevated -GameDir `"$dir`""
    if ($Uninstall) { $a += " -Uninstall" }
    try { Start-Process powershell -Verb RunAs -ArgumentList $a -Wait } catch { Say "Administrator permission was declined. Nothing was changed." Red; exit 1 }
    Say "Finished in the administrator window."
    exit 0
}

Say ""
Say "  Voodoo Vince Remastered - Free Camera" Cyan
Say "  -------------------------------------" Cyan

if (-not (Test-GameDir $GameDir)) { $GameDir = Find-GameDir }
if (-not $GameDir) { Say "Game folder not found. Nothing was changed." Red; Done 1 }
Say "Game folder: $GameDir"

if (Get-Process -Name "Vince" -ErrorAction SilentlyContinue) {
    Say "Voodoo Vince is running. Close the game and run this again." Red; Done 1
}

try {
    if ($Uninstall) {
        foreach ($f in $ModFiles) {
            $p = Join-Path $GameDir $f
            if (Test-Path $p) {
                if ($f -eq "XINPUT9_1_0.dll" -and -not (Select-String -Path $p -Pattern "freecam.ini" -SimpleMatch -Quiet)) {
                    Say "Skipping $f - it isn't this mod's file." Yellow; continue
                }
                Remove-Item $p -Force; Say "Removed $f"
            }
        }
        Say "`nUninstalled. The game is back to normal." Green
        Done 0
    }

    $hash = (Get-FileHash (Join-Path $GameDir "Vince.exe") -Algorithm SHA256).Hash
    if ($hash -ne $KnownHash) {
        Say "Warning: your Vince.exe is a different version than the one this mod was built for (Steam v1.14.2.0)." Yellow
        Say "It will still install, but the mod will switch itself off safely if the game code doesn't match." Yellow
    }

    Copy-Item (Join-Path $here "XINPUT9_1_0.dll") $GameDir -Force
    Copy-Item (Join-Path $here "README.txt") (Join-Path $GameDir "freecam_README.txt") -Force
    $ini = Join-Path $GameDir "freecam.ini"
    if (Test-Path $ini) { Say "Kept your existing freecam.ini settings." }
    else { Copy-Item (Join-Path $here "freecam.ini") $GameDir }
    Say "`nInstalled! Start the game, load a level and press F5 (or click both sticks)." Green
    Say "Settings: $ini"
    Done 0
} catch [System.UnauthorizedAccessException] {
    if (-not (Test-Admin)) { Relaunch-Elevated $GameDir }
    Say "Could not write to the game folder: $($_.Exception.Message)" Red; Done 1
} catch {
    if (($_.Exception.Message -match "denied") -and -not (Test-Admin)) { Relaunch-Elevated $GameDir }
    Say "Something went wrong: $($_.Exception.Message)" Red; Done 1
}
