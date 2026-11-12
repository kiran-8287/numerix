[CmdletBinding()]
param(
    [ValidateSet('MinGW', 'MSVC')]
    [string]$Compiler = 'MinGW',

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [switch]$RunExamples
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)][string]$Program,
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)][string[]]$ArgumentList
    )

    Write-Host "> $Program $($ArgumentList -join ' ')"
    & $Program @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Program"
    }
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'CMake was not found on PATH. Install CMake and reopen this PowerShell session.'
}

if ($Compiler -eq 'MinGW') {
    foreach ($tool in @('gcc', 'g++', 'mingw32-make')) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "$tool was not found. Open PowerShell with the MSYS2 UCRT64 bin directory on PATH."
        }
    }
    $buildDir = Join-Path $repoRoot "build/windows-$($Configuration.ToLowerInvariant())"
    Invoke-CheckedNative 'cmake' @(
        '-S', $repoRoot, '-B', $buildDir,
        '-G', 'MinGW Makefiles',
        "-DCMAKE_BUILD_TYPE=$Configuration",
        '-DCMAKE_C_COMPILER=gcc'
    )
    $binaryDir = $buildDir
}
else {
    if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
        throw 'MSVC cl.exe was not found. Start this script from a Visual Studio 2022 Developer PowerShell.'
    }
    $buildDir = Join-Path $repoRoot 'build/windows-msvc'
    Invoke-CheckedNative 'cmake' @(
        '-S', $repoRoot, '-B', $buildDir,
        '-G', 'Visual Studio 17 2022', '-A', 'x64'
    )
    $binaryDir = Join-Path $buildDir $Configuration
}

Invoke-CheckedNative 'cmake' @('--build', $buildDir, '--config', $Configuration, '--parallel')
Invoke-CheckedNative 'cmake' @('-E', 'chdir', $buildDir, 'ctest', '-C', $Configuration, '--output-on-failure')

$installDir = Join-Path $buildDir 'installed'
$consumerDir = Join-Path $buildDir 'installed-consumer'
Invoke-CheckedNative 'cmake' @('--install', $buildDir, '--config', $Configuration, '--prefix', $installDir)
if ($Compiler -eq 'MinGW') {
    Invoke-CheckedNative 'cmake' @(
        '-S', (Join-Path $repoRoot 'tests/installed_consumer'), '-B', $consumerDir,
        '-G', 'MinGW Makefiles', "-DNUMERIX_INSTALL_PREFIX=$installDir",
        "-DCMAKE_BUILD_TYPE=$Configuration",
        '-DCMAKE_C_COMPILER=gcc', '-DCMAKE_CXX_COMPILER=g++'
    )
}
else {
    Invoke-CheckedNative 'cmake' @(
        '-S', (Join-Path $repoRoot 'tests/installed_consumer'), '-B', $consumerDir,
        '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DNUMERIX_INSTALL_PREFIX=$installDir"
    )
}
Invoke-CheckedNative 'cmake' @('--build', $consumerDir, '--config', $Configuration, '--parallel')
Invoke-CheckedNative 'cmake' @('-E', 'chdir', $consumerDir, 'ctest', '-C', $Configuration, '--output-on-failure')

if ($RunExamples) {
    $cli = Join-Path $binaryDir 'numerix_cli.exe'
    Invoke-CheckedNative $cli @('--help')
    Invoke-CheckedNative $cli @('--eval', 'sin(pi/2)+x^2', '2')
    Invoke-CheckedNative $cli @('--demo')
    Invoke-CheckedNative $cli @('--root', 'x^2-2', '0', '2')
    Invoke-CheckedNative $cli @('--roots', '(x-1)*(x-2)', '0', '3')
    Invoke-CheckedNative $cli @('--piecewise-demo')

    foreach ($demo in @(
        'numerix_expression_demo.exe',
        'numerix_piecewise_demo.exe',
        'numerix_intelligence_demo.exe',
        'numerix_managed_execution_demo.exe',
        'numerix_root_discovery_demo.exe'
    )) {
        Invoke-CheckedNative (Join-Path $binaryDir $demo) @()
    }
}

Write-Host "NUMERIX $Configuration build and CTest suite succeeded ($Compiler)."
