# Runs one behaviour check in the real game and collects its report, screenshots and warnings. The game's scripts and
# the save folder are put back afterwards. See tools\checks\README.md.
#
#   powershell -ExecutionPolicy Bypass -File tools\checks\run_check.ps1 -Scenario ride -OutDir baseline\ride -GameArguments --demo
param(
    [Parameter(Mandatory = $true)][string]$Scenario,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [string]$Game = '',                   # folder holding assets\ (default: the game the scenario is for)
    [string]$Exe = '',                    # default: <Game>\build-nogame\Debug\<the game folder's name>.exe
    [string]$Saves = '',                  # save files to install for the run, so runs can be compared
    [string]$GameArguments = '',
    [string]$SaveName = '',               # %APPDATA%\StrukturGames\<SaveName> (default: the name the game sets)
    [int]$TimeoutSeconds = 360
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class CheckCapture {
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
[CheckCapture]::SetProcessDPIAware() | Out-Null

# The window is captured even when other windows cover it, so a check never takes focus.
function Save-Shot([IntPtr]$handle, [string]$path) {
    $rect = New-Object CheckCapture+RECT
    [CheckCapture]::GetClientRect($handle, [ref]$rect) | Out-Null
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) { Write-Host "no window to capture"; return }
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $context = $graphics.GetHdc()
    [CheckCapture]::PrintWindow($handle, $context, 3) | Out-Null
    $graphics.ReleaseHdc($context)
    $graphics.Dispose()
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

# Relative file name -> content hash for every file under a folder.
function Get-FileHashes([string]$folder) {
    $hashes = @{}
    if (Test-Path $folder) {
        $full = (Resolve-Path $folder).Path
        foreach ($file in Get-ChildItem -Recurse -File $full) {
            $hashes[$file.FullName.Substring($full.Length + 1)] = (Get-FileHash $file.FullName).Hash
        }
    }
    return $hashes
}

# Makes $target hold exactly the files in $source (only used on the save folder, from its own backup).
function Restore-Folder([string]$source, [string]$target) {
    $wanted = Get-FileHashes $source
    foreach ($name in (Get-FileHashes $target).Keys) {
        if (-not $wanted.ContainsKey($name)) { Remove-Item -LiteralPath (Join-Path $target $name) }
    }
    foreach ($name in $wanted.Keys) {
        $destination = Join-Path $target $name
        New-Item -ItemType Directory -Force (Split-Path -Parent $destination) | Out-Null
        Copy-Item -LiteralPath (Join-Path $source $name) -Destination $destination -Force
    }
}

# Not a parameter default: Windows PowerShell leaves $PSScriptRoot empty there.
$repository = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not $Game) {
    # As a game's engine\ submodule, that game; otherwise the scenario's game, in its repository beside this one.
    $beside = Split-Path -Parent $repository
    $name = if ($Scenario.StartsWith('ride')) { 'IndoorCyclingSoftware' } else { 'SevenGods' }
    $Game = if (Test-Path (Join-Path $beside 'assets')) { $beside } else { Join-Path $beside $name }
}
$Game = (Resolve-Path $Game).Path
if (-not $Exe) {
    $Exe = Join-Path $Game "build-nogame\Debug\$(Split-Path -Leaf $Game).exe"
    # A copy of the engine from before the split builds one program, Struktur.exe.
    if (-not (Test-Path $Exe)) { $Exe = Join-Path $Game 'build-nogame\Debug\Struktur.exe' }
}
$Exe = (Resolve-Path $Exe).Path
$program = [System.IO.Path]::GetFileNameWithoutExtension($Exe)
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
# Any of the games: they may share a save folder.
$running = Get-Process -Name $program, Struktur, SevenGods, IndoorCyclingSoftware -ErrorAction SilentlyContinue
if ($running) {
    throw "$($running[0].ProcessName) is already running. Close it first: a check swaps the save files."
}

# The game saves under the name its Main.wren sets, and a check must back up and restore that folder.
if (-not $SaveName) {
    $main = Join-Path $Game 'assets\Scripts\Main.wren'
    $named = Select-String -Path $main -Pattern 'Application\.setApplicationName\("([^"]+)"\)' | Select-Object -First 1
    if (-not $named) { throw "no Application.setApplicationName in $main - give -SaveName" }
    $SaveName = $named.Matches[0].Groups[1].Value
}
$saveFolder = Join-Path $env:APPDATA "StrukturGames\$SaveName"
New-Item -ItemType Directory -Force $saveFolder | Out-Null
# One backup per run, never overwritten, so an interrupted run can't replace the real saves with test ones.
$backup = Join-Path $OutDir ('saves-before-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
Copy-Item -Recurse $saveFolder $backup
$scripts = Join-Path $OutDir 'scripts-before'
$report = Join-Path $saveFolder 'check_report.txt'
$marker = Join-Path $saveFolder 'check_shot.txt'

try {
    if ($Saves) {
        # Only the given game saves, so a file the set lacks can't leak in from the player's own. All restored below.
        Get-ChildItem -File $saveFolder -Filter '*.sav' | ForEach-Object { Remove-Item -LiteralPath $_.FullName }
        Copy-Item -Recurse -Force (Join-Path (Resolve-Path $Saves).Path '*') $saveFolder
    }
    Remove-Item $report, $marker -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $OutDir '*.png'), (Join-Path $OutDir 'report.txt') -ErrorAction SilentlyContinue
    python (Join-Path $PSScriptRoot 'scenarios.py') apply $Scenario $Game $scripts
    if ($LASTEXITCODE -ne 0) { throw "could not add the check to the scripts" }

    $start = Get-Date
    $options = @{
        FilePath = $Exe; WorkingDirectory = $Game; PassThru = $true
        RedirectStandardOutput = (Join-Path $OutDir 'game.out'); RedirectStandardError = (Join-Path $OutDir 'game.err')
    }
    if ($GameArguments) { $options['ArgumentList'] = $GameArguments }
    $process = Start-Process @options
    $null = $process.Handle  # keeps the exit code readable
    $taken = @{}
    $finished = $false
    $deadline = $start.AddSeconds($TimeoutSeconds)
    while (-not $finished -and (Get-Date) -lt $deadline -and -not $process.HasExited) {
        Start-Sleep -Milliseconds 200
        $name = $null
        if (Test-Path $marker) { $name = Get-Content $marker -Raw -ErrorAction SilentlyContinue }
        if ($name -and -not $taken.ContainsKey($name.Trim())) {
            $name = $name.Trim()
            $taken[$name] = $true
            Start-Sleep -Milliseconds 300
            $process.Refresh()
            Save-Shot $process.MainWindowHandle (Join-Path $OutDir "$name.png")
            if ($name -eq 'done') { $finished = $true; Start-Sleep -Seconds 3 }
        }
    }
    $exit = 'closed on request'
    if ($process.HasExited) {
        $exit = "exited early with code $($process.ExitCode)"
    } else {
        $process.CloseMainWindow() | Out-Null
        if ($process.WaitForExit(15000)) { $exit = "closed, code $($process.ExitCode)" }
        else { $process.Kill(); $exit = 'had to be killed' }
    }
    if (Test-Path $report) { Copy-Item $report (Join-Path $OutDir 'report.txt') }
    $crashes = @(Get-WinEvent -FilterHashtable @{ LogName = 'Application'; Id = 1000; StartTime = $start } -ErrorAction SilentlyContinue |
        Where-Object { $_.Message -match $program }).Count
    # Warnings and errors with their timestamps and numbers removed, so two runs can be compared.
    $problems = Get-Content (Join-Path $OutDir 'game.out'), (Join-Path $OutDir 'game.err') -ErrorAction SilentlyContinue |
        Where-Object { $_ -match '^(WARN|WARNING|ERROR|FATAL)|Wren (Runtime|Compile)' } |
        ForEach-Object { ($_ -replace '\[[0-9:. -]+\]', '' -replace '0x[0-9a-fA-F]+', 'N' -replace '\d+(\.\d+)?', 'N').Trim() } |
        Sort-Object -Unique
    Set-Content -Path (Join-Path $OutDir 'problems.txt') -Value $problems -Encoding utf8

    Write-Host "scenario: $Scenario"
    Write-Host "finished: $finished"
    Write-Host "process:  $exit"
    Write-Host "crashes:  $crashes"
    Write-Host "distinct warnings and errors: $(@($problems).Count)"
    if (Test-Path (Join-Path $OutDir 'report.txt')) { Get-Content (Join-Path $OutDir 'report.txt') }
} finally {
    python (Join-Path $PSScriptRoot 'scenarios.py') restore $Game $scripts
    Restore-Folder $backup $saveFolder
    $before = Get-FileHashes $backup
    $after = Get-FileHashes $saveFolder
    $different = @($before.Keys | Where-Object { $after[$_] -ne $before[$_] }).Count + @($after.Keys | Where-Object { -not $before.ContainsKey($_) }).Count
    Write-Host "save files differing from before the run: $different"
}
if (-not $finished) { exit 1 }
exit 0
