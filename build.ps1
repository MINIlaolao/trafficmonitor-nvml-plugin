param(
    [Parameter(Mandatory = $true)]
    [string]$TrafficMonitorInclude,
    [string]$NvmlInclude,
    [switch]$Probe
)

$ErrorActionPreference = 'Stop'
if (-not $NvmlInclude -and $env:CUDA_PATH) {
    $NvmlInclude = Join-Path $env:CUDA_PATH 'include'
}
if (-not $NvmlInclude) { throw 'Pass the directory containing nvml.h with -NvmlInclude.' }
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') { throw 'Run the build in an x64 Visual Studio developer shell.' }

$hostInclude = (Resolve-Path -LiteralPath $TrafficMonitorInclude).Path
$gpuInclude = (Resolve-Path -LiteralPath $NvmlInclude).Path
foreach ($header in @((Join-Path $hostInclude 'PluginInterface.h'), (Join-Path $gpuInclude 'nvml.h'))) {
    if (-not (Test-Path -LiteralPath $header -PathType Leaf)) { throw "Missing header: $header" }
}

$out = Join-Path $PSScriptRoot 'build'
New-Item -Path $out -ItemType Directory -Force | Out-Null
$source = Join-Path $PSScriptRoot 'NvidiaTaskbarMetrics.cpp'
$flags = @('/nologo', '/std:c++17', '/O2', '/W3', '/EHsc', '/MT', "/I$hostInclude", "/I$gpuInclude")

Push-Location -LiteralPath $out
try {
    & cl.exe @flags /LD $source /link /OUT:NvidiaTaskbarMetrics.dll
    if ($LASTEXITCODE -ne 0) { throw "Plugin build failed ($LASTEXITCODE)." }
    if ($Probe) {
        & cl.exe @flags /DMONITOR_PROBE /Fe:NvidiaMetricsProbe.exe $source
        if ($LASTEXITCODE -ne 0) { throw "Probe build failed ($LASTEXITCODE)." }
        & .\NvidiaMetricsProbe.exe
        if ($LASTEXITCODE -ne 0) { throw 'The NVIDIA readings did not pass the probe.' }
    }
} finally {
    Pop-Location
}
