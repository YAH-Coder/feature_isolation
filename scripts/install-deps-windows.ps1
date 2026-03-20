$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Require-Winget {
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        throw "winget is required for the Windows bootstrap script."
    }
}

function Install-WingetPackage {
    param(
        [Parameter(Mandatory = $true)][string]$Id
    )

    winget install --id $Id --exact --accept-package-agreements --accept-source-agreements --silent
}

Require-Winget

Install-WingetPackage -Id "MSYS2.MSYS2"
Install-WingetPackage -Id "DimitriVanHeesch.Doxygen"
Install-WingetPackage -Id "Graphviz.Graphviz"

$msysRoot = Join-Path $env:SystemDrive "msys64"
$msysBash = Join-Path $msysRoot "usr\bin\bash.exe"

if (-not (Test-Path $msysBash)) {
    throw "MSYS2 bash was not found at $msysBash. Start MSYS2 once to finish its setup, then rerun this script."
}

$pacmanCommand = @'
pacman -Sy --noconfirm --needed \
  base-devel \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-make \
  mingw-w64-ucrt-x86_64-doxygen \
  mingw-w64-ucrt-x86_64-graphviz
'@

& $msysBash -lc $pacmanCommand

Write-Host "Windows dependencies installed."
Write-Host "Use an MSYS2 UCRT64 shell to run 'make test' and 'make docs'."
