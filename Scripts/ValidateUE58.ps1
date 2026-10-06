param(
    [Parameter(Mandatory = $true)][string]$EnginePath,
    [string]$ValidationRoot = (Join-Path $env:TEMP ('UnrealImGui5-UE58-' + [guid]::NewGuid().ToString('N'))),
    [switch]$SkipGPU,
    [switch]$VerifyGameModules
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$engineRoot = (Resolve-Path -LiteralPath $EnginePath).Path
$buildVersion = Get-Content -LiteralPath (Join-Path $engineRoot 'Engine\Build\Build.version') -Raw | ConvertFrom-Json
if ($buildVersion.MajorVersion -ne 5 -or $buildVersion.MinorVersion -ne 8) { throw 'This validation fixture targets UE 5.8.' }
$validationPath = [System.IO.Path]::GetFullPath($ValidationRoot)
if (Test-Path -LiteralPath $validationPath) { throw 'Use a new validation directory to preserve earlier evidence.' }
if ($validationPath.StartsWith($repoRoot + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Validation output must be outside the source checkout.'
}
New-Item -ItemType Directory -Path $validationPath | Out-Null
Copy-Item -Path (Join-Path $repoRoot 'Tests\UE58\*') -Destination $validationPath -Recurse
# Keep fixture rules as templates in the plugin so UBT cannot discover duplicate rules.
Get-ChildItem -LiteralPath (Join-Path $validationPath 'Source') -Filter '*.cs.in' -Recurse -File | ForEach-Object {
    Rename-Item -LiteralPath $_.FullName -NewName ([System.IO.Path]::GetFileNameWithoutExtension($_.Name))
}
New-Item -ItemType Directory -Path (Join-Path $validationPath 'Plugins') | Out-Null
New-Item -ItemType Junction -Path (Join-Path $validationPath 'Plugins\ImGui') -Target $repoRoot | Out-Null
$evidencePath = Join-Path $validationPath 'Saved\Validation'
New-Item -ItemType Directory -Path $evidencePath -Force | Out-Null
$projectPath = Join-Path $validationPath 'ImGuiValidation.uproject'
$buildTool = Join-Path $engineRoot 'Engine\Build\BatchFiles\Build.bat'
$editor = Join-Path $engineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$testSource = Get-Content -LiteralPath (Join-Path $repoRoot 'Source\ImGui\Private\Tests\ImGuiIntegrationTests.cpp') -Raw
$expectedTests = @([regex]::Matches($testSource, '"(ImGui\.Integration\.[^"]+)"') | ForEach-Object { $_.Groups[1].Value })
if ($expectedTests.Count -eq 0) { throw 'No ImGui integration tests were found.' }

& $buildTool ImGuiValidationEditor Win64 Development "-Project=$projectPath" -UsePrecompiled -NoHotReloadFromIDE *> (Join-Path $evidencePath 'editor-build.log')
if ($LASTEXITCODE -ne 0) { throw "Editor build failed. See $evidencePath\editor-build.log" }

if ($VerifyGameModules) {
    # Module filters compile the non-editor paths without linking a complete engine/game executable.
    foreach ($configuration in @('Development', 'Shipping')) {
        $moduleArguments = @('-Module=ImGuiValidation')
        if ($configuration -eq 'Development') { $moduleArguments = @('-Module=ImGui', '-Module=ImGuiValidation') }
        $moduleLog = Join-Path $evidencePath ('game-modules-' + $configuration + '.log')
        & $buildTool ImGuiValidation Win64 $configuration "-Project=$projectPath" @moduleArguments -NoHotReloadFromIDE *> $moduleLog
        if ($LASTEXITCODE -ne 0) { throw "Game module compilation failed in $configuration. See $moduleLog" }
        Write-Output "$configuration Game modules compiled (no executable link or packaging)."
    }
}

$renderModes = @('NullRHI')
if (!$SkipGPU) { $renderModes += 'D3D12' }
foreach ($renderMode in $renderModes) {
    $reportPath = Join-Path $evidencePath ('Automation-' + $renderMode)
    $logPath = Join-Path $evidencePath ('automation-' + $renderMode + '.log')
    $renderArguments = if ($renderMode -eq 'NullRHI') { @('-NullRHI') } else { @('-d3d12', '-RenderOffscreen') }
    & $editor $projectPath -unattended -nosplash -nosound @renderArguments '-ExecCmds=Automation RunTests ImGui.Integration' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$reportPath" "-abslog=$logPath" -stdout -FullStdOutLogOutput *> (Join-Path $evidencePath ('console-' + $renderMode + '.log'))
    if ($LASTEXITCODE -ne 0) { throw "Unreal exited with an error in $renderMode. See $logPath" }
    # TestExit can return zero even when tests fail; inspect the exported report as well.
    $report = Get-Content -LiteralPath (Join-Path $reportPath 'index.json') -Raw | ConvertFrom-Json
    $missingTests = @($expectedTests | Where-Object { $_ -notin $report.tests.fullTestPath })
    if ($report.failed -ne 0 -or $report.notRun -ne 0 -or $report.succeededWithWarnings -ne 0 -or $report.succeeded -ne $expectedTests.Count -or $missingTests.Count -ne 0) {
        throw "Expected all $($expectedTests.Count) ImGui tests to pass without warnings in $renderMode. See $reportPath"
    }
    Write-Output "$renderMode : $($report.succeeded)/$($expectedTests.Count) tests passed."
}

if (!$SkipGPU) {
    $smokeLog = Join-Path $evidencePath 'slate-smoke.log'
    & $editor $projectPath /Engine/Maps/Entry -game -d3d12 -RenderOffscreen -Windowed -ResX=1280 -ResY=720 -unattended -nosplash -nosound -ImGuiValidationSmoke "-abslog=$smokeLog" -stdout -FullStdOutLogOutput *> (Join-Path $evidencePath 'slate-smoke-console.log')
    if ($LASTEXITCODE -ne 0) { throw "Slate smoke run failed. See $smokeLog" }
    $screenshot = Join-Path $evidencePath 'UE58-ImGui-Slate.png'
    if (!(Test-Path -LiteralPath $screenshot)) { throw 'The Slate smoke run did not produce its screenshot.' }
    Write-Output "Inspect the Slate screenshot: $screenshot"
}
Write-Output "Validation evidence: $evidencePath"
