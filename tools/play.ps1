# Banc de test : lance la ROM dans BlastEm, joue une séquence de touches et prend des captures.
#
#   .\tools\play.ps1 -Steps "wait:1500", "hold:LEFT+C:500", "shot:droite", "hold:C:430", "shot:tourne"
#
# Étapes : wait:<ms> | hold:<touches séparées par +>:<ms> | shot:<nom>
# Touches (clavier BlastEm par défaut) : LEFT RIGHT UP DOWN A B C START
# Les captures vont dans tools\shots\<nom>.png (dossier ignoré par git).
param(
    [Parameter(Mandatory)] [string[]] $Steps,
    [string] $Rom = (Join-Path $PSScriptRoot '..\out\rom.bin'),
    [switch] $KeepOpen
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public class TwPlay {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    public static void Key(byte vk, bool ext, bool up) {
        keybd_event(vk, (byte)MapVirtualKey(vk, 0), (ext ? 1u : 0u) | (up ? 2u : 0u), UIntPtr.Zero);
    }
}
'@
[TwPlay]::SetProcessDPIAware() | Out-Null

# Virtual-key codes and whether they are "extended" keys (arrows).
$keys = @{
    LEFT  = @(0x25, $true);  UP    = @(0x26, $true); RIGHT = @(0x27, $true); DOWN = @(0x28, $true)
    A     = @(0x41, $false); B     = @(0x53, $false); C    = @(0x44, $false); START = @(0x0D, $false)
}

$shots = Join-Path $PSScriptRoot 'shots'
New-Item -ItemType Directory -Force $shots | Out-Null

Get-Process blastem -ErrorAction SilentlyContinue | Stop-Process
$emu = Start-Process 'D:\code\tools\blastem\blastem.exe' -ArgumentList "`"$((Resolve-Path $Rom).Path)`"" -PassThru
Start-Sleep -Milliseconds 1500
$emu.Refresh()

function Focus { [TwPlay]::SetForegroundWindow($emu.MainWindowHandle) | Out-Null; Start-Sleep -Milliseconds 50 }

function Shot([string] $name) {
    $r = New-Object TwPlay+RECT
    [TwPlay]::GetWindowRect($emu.MainWindowHandle, [ref] $r) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
    [System.Drawing.Graphics]::FromImage($bmp).CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
    $file = Join-Path $shots "$name.png"
    $bmp.Save($file)
    $file
}

Focus
foreach ($step in $Steps) {
    $parts = $step -split ':'
    switch ($parts[0]) {
        'wait' {
            # BlastEm pauses when it loses focus: keep bringing it back while waiting.
            $until = (Get-Date).AddMilliseconds([int] $parts[1])
            while ((Get-Date) -lt $until) { Focus; Start-Sleep -Milliseconds 200 }
        }
        'shot' { Shot $parts[1] }
        'hold' {
            $names = $parts[1] -split '\+'
            Focus
            foreach ($n in $names) { [TwPlay]::Key($keys[$n][0], $keys[$n][1], $false) }
            Start-Sleep -Milliseconds ([int] $parts[2])
            foreach ($n in $names) { [TwPlay]::Key($keys[$n][0], $keys[$n][1], $true) }
        }
        default { throw "Étape inconnue : $step" }
    }
}

if (-not $KeepOpen) { Stop-Process -Id $emu.Id -ErrorAction SilentlyContinue }
