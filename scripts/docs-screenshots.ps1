# Launch debug standalone and overwrite docs/Screenshot*.png.
# Clicks are editor-client coords (520x280), scaled to the real client.
$ErrorActionPreference = 'Stop'

$editorW = 520
$editorH = 280

Add-Type -AssemblyName System.Drawing, System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win {
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(int f, int x, int y, int d, int e);
  [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [StructLayout(LayoutKind.Sequential)] struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)] struct POINT { public int X, Y; }
  public static bool WindowRect(IntPtr h, out int l, out int t, out int r, out int b) {
    RECT rc; bool ok = GetWindowRect(h, out rc);
    l = rc.L; t = rc.T; r = rc.R; b = rc.B; return ok;
  }
  public static bool ClientSize(IntPtr h, out int w, out int ht) {
    RECT rc; bool ok = GetClientRect(h, out rc);
    w = rc.R; ht = rc.B; return ok;
  }
  public static bool ClientOrigin(IntPtr h, out int x, out int y) {
    POINT p; p.X = 0; p.Y = 0;
    bool ok = ClientToScreen(h, ref p);
    x = p.X; y = p.Y; return ok;
  }
}
"@
[Win]::SetProcessDPIAware() | Out-Null

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$exe  = Join-Path $root 'Builds\VisualStudio2022\x64\Debug\Standalone Plugin\fD1ST.exe'
$out  = Join-Path $root 'docs'
if (-not (Test-Path $exe)) { throw "no debug standalone: $exe — run just build-debug" }

Get-Process -Name fD1ST -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 300
$proc = Start-Process $exe -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 50; $i++) {
    Start-Sleep -Milliseconds 200
    $p = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
    if ($p -and $p.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $p.MainWindowHandle; break }
}
if ($hwnd -eq [IntPtr]::Zero) { throw 'fD1ST window did not appear' }

function Get-WinRect {
    $l = 0; $t = 0; $r = 0; $b = 0
    [void][Win]::WindowRect($hwnd, [ref]$l, [ref]$t, [ref]$r, [ref]$b)
    return @{ L = $l; T = $t; R = $r; B = $b }
}

function Save-Shot([string]$name) {
    $r = Get-WinRect
    $w = $r.R - $r.L
    $h = $r.B - $r.T
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
    $bmp.Save((Join-Path $out "$name.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

Start-Sleep -Seconds 1
Save-Shot 'Screenshot1'
Save-Shot 'Screenshot2'

Stop-Process -Id $proc.Id -Force
Write-Host "wrote Screenshot1.png Screenshot2.png in $out"
