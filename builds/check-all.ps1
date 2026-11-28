# Builds every configuration and runs every automated test, then prints what passed.
#
#   powershell -ExecutionPolicy Bypass -File builds\check-all.ps1          (desktop builds and tests)
#   powershell -ExecutionPolicy Bypass -File builds\check-all.ps1 -Web     (also the web builds; needs EMSDK)
#
# The games are separate repositories. Those found beside this one (or named with -Games) are built against this engine,
# in build-games\ here, so an engine change is checked against them before it's pushed. -Games @() skips them.
#
# Each step writes its output to <build folder>\check.log. Exit code 0 when everything passes.
# The behaviour checks that start the game are separate: see tools\checks.
param(
    [switch]$Web,
    [string[]]$Games
)

$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent $PSScriptRoot
$results = New-Object System.Collections.ArrayList
$generator = 'Visual Studio 17 2022'

# Runs one command, appending its output to the log. True when it exits with 0.
function Invoke-Logged($log, $program, $arguments) {
    "> $program $($arguments -join ' ')" | Out-File -FilePath $log -Append -Encoding utf8
    & $program @arguments 2>&1 | ForEach-Object { "$_" } | Out-File -FilePath $log -Append -Encoding utf8
    return $LASTEXITCODE -eq 0
}

function Add-Result($name, $passed, $detail) {
    [void]$results.Add([pscustomobject]@{ Step = $name; Passed = $passed; Detail = $detail })
    $word = if ($passed) { 'pass' } else { 'FAIL' }
    Write-Host ("{0,-4}  {1}  {2}" -f $word, $name, $detail)
}

if (-not $PSBoundParameters.ContainsKey('Games')) {
    $beside = Split-Path -Parent $root
    $Games = @('SevenGods', 'IndoorCyclingSoftware' | ForEach-Object { Join-Path $beside $_ } |
        Where-Object { Test-Path (Join-Path $_ 'CMakeLists.txt') })
}
$Games = @($Games | Where-Object { $_ } | ForEach-Object { (Resolve-Path $_).Path })

# A path as given, or inside this repository.
function Get-FullPath($path) {
    if ([System.IO.Path]::IsPathRooted($path)) { return $path }
    return Join-Path $root $path
}

# Configures and builds one CMake folder. $tests names the test programs to run after it, relative to the folder; 'all'
# runs every *Tests.exe the build made.
function Test-Build($name, $source, $folder, $config, $configureArguments, $tests) {
    $build = Get-FullPath $folder
    New-Item -ItemType Directory -Force $build | Out-Null
    $log = Join-Path $build 'check.log'
    Set-Content -Path $log -Value "$name" -Encoding utf8

    $configure = @('-S', (Get-FullPath $source), '-B', $build, '-G', $generator) + $configureArguments
    if (-not (Invoke-Logged $log 'cmake' $configure)) {
        Add-Result $name $false "configure failed, see $log"
        return
    }
    if (-not (Invoke-Logged $log 'cmake' @('--build', $build, '--config', $config, '--parallel'))) {
        Add-Result $name $false "build failed, see $log"
        return
    }
    if (-not $tests) {
        Add-Result $name $true 'built'
        return
    }
    if ($tests -eq 'all') {
        $tests = @(Get-ChildItem (Join-Path $build "$config\*Tests.exe") | ForEach-Object { "$config\$($_.Name)" })
    }
    foreach ($test in $tests) {
        $passed = Invoke-Logged $log (Join-Path $build $test) @()
        $summary = Select-String -Path $log -Pattern 'test cases:' | Select-Object -Last 1
        $detail = if ($summary) { ($summary.Line -replace '\x1b\[[0-9;]*m', '').Trim() } else { "see $log" }
        Add-Result $name $passed $detail
    }
}

Push-Location $root
if ($Games.Count -gt 0) {
    python tools\check_asset_case.py @Games
    Add-Result 'asset path case' ($LASTEXITCODE -eq 0) ''
}
Pop-Location

Test-Build 'jobs tests' 'src\Engine\Jobs' 'build-jobs' 'Debug' @('-DSTRUKTUR_THREADS=ON') @('Debug\StrukturJobsTests.exe')
Test-Build 'jobs tests, no threads' 'src\Engine\Jobs' 'build-jobs-nothreads' 'Debug' @('-DSTRUKTUR_THREADS=OFF') @('Debug\StrukturJobsTests.exe')
Test-Build 'fitness tests' 'src\Engine\Fitness' 'build-fitness' 'Debug' @() @('Debug\StrukturFitnessTests.exe')
# The engine compiled for each library and the combinations games use, so nothing depends on a library left out.
Test-Build 'engine libraries' '.' 'build-checks' 'Debug' @('-DCMAKE_BUILD_TYPE=Debug') $null

# Configures and builds one game for the web. Build it for desktop first: the web build embeds those shaders.
function Test-WebBuild($name, $source, $folder, $engineArgument) {
    $build = Get-FullPath $folder
    New-Item -ItemType Directory -Force $build | Out-Null
    $log = Join-Path $build 'check.log'
    Set-Content -Path $log -Value $name -Encoding utf8
    $environment = Join-Path $env:EMSDK 'emsdk_env.bat'
    $toolchain = Join-Path $env:EMSDK 'upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake'
    # A batch file, because emsdk_env only sets up the shell it runs in.
    $batch = Join-Path $build 'check-web.bat'
    Set-Content -Path $batch -Encoding ascii -Value @(
        '@echo off',
        "call `"$environment`" >nul 2>&1",
        "call emcmake cmake -S `"$source`" -B `"$build`" -G Ninja -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release `"$engineArgument`" -DCMAKE_TOOLCHAIN_FILE=`"$toolchain`" || exit /b 1",
        "cmake --build `"$build`" || exit /b 1")
    $passed = Invoke-Logged $log $batch @()
    $detail = if ($passed) { 'built' } else { "see $log" }
    Add-Result $name $passed $detail
}

# Forward slashes: CMake reads backslashes in a path as escapes.
$engine = '-DSTRUKTUR_DIR=' + ($root -replace '\\', '/')
foreach ($game in $Games) {
    $name = Split-Path -Leaf $game
    $builds = "build-games\$name"
    if (Test-Path (Join-Path $game 'tests\CMakeLists.txt')) {
        Test-Build "$name tests" "$game\tests" "$builds\tests" 'Debug' @($engine) 'all'
    }
    Test-Build "$name, debug" $game "$builds\debug" 'Debug' @($engine, '-DCMAKE_BUILD_TYPE=Debug', '-DEDITOR=FALSE') $null
    Test-Build "$name editor, debug" $game "$builds\editor" 'Debug' @($engine, '-DCMAKE_BUILD_TYPE=Debug') $null
    Test-Build "$name, release" $game "$builds\release" 'Release' @($engine, '-DCMAKE_BUILD_TYPE=Release') $null
    if ($Web) {
        Test-WebBuild "$name, web" $game "$builds\web" $engine
    }
}

$failed = @($results | Where-Object { -not $_.Passed })
Write-Host ''
Write-Host ("{0} of {1} passed" -f ($results.Count - $failed.Count), $results.Count)
if ($failed.Count -gt 0) { exit 1 }
exit 0
