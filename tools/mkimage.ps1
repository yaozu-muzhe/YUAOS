# ============================================================
#  YUAOS 镜像组装脚本
#  文件 : tools/mkimage.ps1
#  用法 : powershell -File tools\mkimage.ps1 -Boot ... -Stage2 ... -Kernel ... -Output ...
# ============================================================
param(
    [Parameter(Mandatory=$true)][string]$Boot,
    [Parameter(Mandatory=$true)][string]$Stage2,
    [Parameter(Mandatory=$true)][string]$Kernel,
    [Parameter(Mandatory=$true)][string]$Output
)

$ErrorActionPreference = "Stop"

# ---- 布局常量 ----
$IMG_SIZE      = 1474560          # 1.44 MB
$SECTOR        = 512
$BOOT_OFFSET   = 0
$STAGE2_OFFSET = $SECTOR * 1
$KERNEL_OFFSET = $SECTOR * 9
$STAGE2_MAX    = $SECTOR * 8      # 4 KB
$KERNEL_MAX    = $SECTOR * 512    # 256 KB

# ---- 读取输入 ----
if (-not (Test-Path $Boot))   { throw "missing: $Boot" }
if (-not (Test-Path $Stage2)) { throw "missing: $Stage2" }
if (-not (Test-Path $Kernel)) { throw "missing: $Kernel" }

$boot = [IO.File]::ReadAllBytes($Boot)
$s2   = [IO.File]::ReadAllBytes($Stage2)
$k    = [IO.File]::ReadAllBytes($Kernel)

# ---- 大小检查 ----
if ($boot.Length -gt $SECTOR)     { throw "boot.bin too large: $($boot.Length) > $SECTOR" }
if ($s2.Length   -gt $STAGE2_MAX) { throw "stage2.bin too large: $($s2.Length) > $STAGE2_MAX" }
if ($k.Length    -gt $KERNEL_MAX) { throw "kernel.bin too large: $($k.Length) > $KERNEL_MAX" }

Write-Host ("  boot.bin   = {0,8} bytes" -f $boot.Length)
Write-Host ("  stage2.bin = {0,8} bytes" -f $s2.Length)
Write-Host ("  kernel.bin = {0,8} bytes" -f $k.Length)

# ---- 组装 ----
$img = New-Object byte[] $IMG_SIZE
[Array]::Copy($boot, 0, $img, $BOOT_OFFSET,   $boot.Length)
[Array]::Copy($s2,   0, $img, $STAGE2_OFFSET, $s2.Length)
[Array]::Copy($k,    0, $img, $KERNEL_OFFSET, $k.Length)

# ---- 写盘 ----
[IO.File]::WriteAllBytes($Output, $img)
