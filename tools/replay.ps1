# Rejoue une séquence de touches image par image, compilée dans la ROM (build de diagnostic),
# puis capture l'état figé en fin de script.
#
#   .\tools\replay.ps1 -Name virage -Steps "LEFT+C:54", "LEFT:26", "-:30"
#
# Étape : <touches séparées par + ou - pour aucune>:<nombre d'images (60 par seconde)>
# Touches : LEFT RIGHT UP DOWN A B C START
# Capture : tools\shots\<Name>.png — le HUD de diagnostic (angle, vitesses, position) est en bas.
param(
    [Parameter(Mandatory)] [string] $Name,
    [Parameter(Mandatory)] [string[]] $Steps
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')

# 1. Script C : { frames, boutons }.
$entries = foreach ($step in $Steps) {
    $keys, $frames = $step -split ':'
    $mask = if ($keys -eq '-') { '0' } else { ($keys -split '\+' | ForEach-Object { "BUTTON_$_" }) -join ' | ' }
    "    { $frames, $mask },"
}
$header = "// Généré par tools/replay.ps1 — ne pas modifier.`n#include <genesis.h>`nstatic const u16 SCRIPT[][2] =`n{`n$($entries -join "`n")`n};`n"
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'script.h'), $header)

# 2. Build de diagnostic (script + HUD de debug), dans un dossier de sortie séparé.
$env:GDK = 'D:/code/tools/sgdk'
$env:Path = "D:\code\tools\jre\bin;D:\code\tools\sgdk\bin;$env:Path"
Push-Location $root
try {
    Remove-Item -Recurse -Force out -ErrorAction SilentlyContinue
    # make écrit sur stderr même quand tout va bien : ne pas en faire une erreur PowerShell.
    $ErrorActionPreference = 'Continue'
    $log = & make -f D:/code/tools/sgdk/makefile.gen release "EXTRA_FLAGS=-DDEBUG_HUD -DDEBUG_SCRIPT -Itools" 2>&1
    $ErrorActionPreference = 'Stop'
    if (-not (Test-Path out\rom.bin)) { $log | Select-String 'error' | Select-Object -First 10; throw 'Build échoué' }
} finally { Pop-Location }

# 3. Exécution : durée du script + marge, puis capture.
$totalFrames = ($Steps | ForEach-Object { [int](($_ -split ':')[1]) } | Measure-Object -Sum).Sum
# The emulator can run behind real time at start-up: wait twice the script length.
# Waiting too long is harmless, the game freezes when the script ends.
& (Join-Path $PSScriptRoot 'play.ps1') -Rom (Join-Path $root 'out\rom.bin') -Steps "wait:$([int]($totalFrames * 2000 / 60) + 3000)", "shot:$Name"
