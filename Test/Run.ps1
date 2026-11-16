param(
    [ValidateSet('Win32', 'x64')]
    [string]$Platform = 'x64'
)
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot)
try {
    $env:_CL_ = '/source-charset:.932'
    & msbuild BonCasClient.sln /m /p:Configuration=Release "/p:Platform=$Platform" /p:PlatformToolset=v143 /verbosity:minimal
    if ($LASTEXITCODE -ne 0) { throw 'DLL build failed' }
    New-Item -ItemType Directory -Force artifacts | Out-Null
    & cl /nologo /EHsc /std:c++14 /W4 /I BonCasClient /I Common /DUNICODE /D_UNICODE Test\SafetyTest.cpp Common\SmartSock.cpp BonCasClient\CasProxy.cpp /Fo:artifacts\ /Fe:artifacts\SafetyTest.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
    $dll = if ($Platform -eq 'x64') { 'x64\Release\BonCasClient.dll' } else { 'Release\BonCasClient.dll' }
    & .\artifacts\SafetyTest.exe $dll
    if ($LASTEXITCODE -ne 0) { throw 'Safety tests failed' }
} finally {
    Pop-Location
}
