# ============================================================
#  YUAOS Build Script (PowerShell) - v16-clean  (ASCII only)
#  File : tools/build.ps1
# ============================================================
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

Write-Host "============================================================"
Write-Host " YUAOS Build"
Write-Host " root : $root"
Write-Host (" time : " + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'))
Write-Host "============================================================"
Write-Host ""

# ---- toolchain check ----
Write-Host "[PRE] Checking toolchain..."
$toolNames = @("nasm","i686-elf-gcc","i686-elf-ld","i686-elf-objcopy","gcc")
foreach ($name in $toolNames) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if (-not $cmd) {
        Write-Host ("[FAIL] " + $name + " not found") -ForegroundColor Red
        exit 2
    }
    Write-Host ("  {0,-20} -> {1}" -f $name, $cmd.Source)
}
$nasm    = "nasm"
$cc      = "i686-elf-gcc"
$ld      = "i686-elf-ld"
$objcopy = "i686-elf-objcopy"
$gcc     = "gcc"
Write-Host "[PRE] OK"
Write-Host ""

# ---- paths ----
$buildDir  = "build"
$objDir    = "build\obj"
$ulDistDir = "build\userland_dist"
$ulBinDir  = "build\userland_dist\bin"
$ulEtcDir  = "build\userland_dist\etc"
$ulHomeDir = "build\userland_dist\home\yuaos"
$ulTmpDir  = "build\userland_dist\tmp"

$bootDir   = "boot"
$kernelDir = "kernel"
$toolsDir  = "tools"

$bootBin   = "build\boot.bin"
$stage2Bin = "build\stage2.bin"
$kernelElf = "build\kernel.elf"
$kernelBin = "build\kernel.bin"
$imgPath   = "build\yuaos.img"
$pcfsImg   = "build\pcfs.img"

$mkqaSrc   = Join-Path $root "tools\mkqa.ps1"
$mkfsSrc   = Join-Path $root "tools\mkfs_pcfs.c"
$mkfsExe   = "build\mkfs_pcfs.exe"

$cflags = @("-m32","-std=c11","-ffreestanding","-fno-stack-protector",
            "-fno-pic","-fno-pie","-Wall","-Wextra","-O2","-Ikernel")

if (-not (Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }
if (-not (Test-Path $objDir))   { New-Item -ItemType Directory -Path $objDir   | Out-Null }

# ---- helper ----
function Run([string]$exe, [string[]]$argList, [string]$label) {
    Write-Host ("  > " + $exe + " " + ($argList -join " "))
    & $exe @argList
    if ($LASTEXITCODE -ne 0) {
        Write-Host ("[FAIL] " + $label + " (exit " + $LASTEXITCODE + ")") -ForegroundColor Red
        exit 1
    }
}

# ============================================================
# kernel
# ============================================================
Write-Host "[1/11] boot sector..."
Run $nasm @("-f","bin","-I","boot/","boot\boot.asm","-o",$bootBin) "boot.asm"

Write-Host "[2/11] stage2..."
Run $nasm @("-f","bin","-I","boot/","boot\stage2.asm","-o",$stage2Bin) "stage2.asm"

Write-Host "[3/11] kernel entry + arch stubs..."
Run $nasm @("-f","elf32","kernel\entry.asm","-o","build\obj\entry.o") "entry.asm"
Run $nasm @("-f","elf32","kernel\arch\gdt_flush.asm","-o","build\obj\gdt_flush.o") "gdt_flush.asm"
Run $nasm @("-f","elf32","kernel\arch\isr_stub.asm","-o","build\obj\isr_stub.o") "isr_stub.asm"

Write-Host "[4/11] core C..."
Run $cc ($cflags + @("-c","kernel\main.c",            "-o","build\obj\main.o"))     "main.c"
Run $cc ($cflags + @("-c","kernel\log.c",             "-o","build\obj\log.o"))      "log.c"
Run $cc ($cflags + @("-c","kernel\lib\string.c",      "-o","build\obj\string.o"))   "string.c"
Run $cc ($cflags + @("-c","kernel\lib\printf.c",      "-o","build\obj\printf.o"))   "printf.c"
Run $cc ($cflags + @("-c","kernel\drivers\vga.c",     "-o","build\obj\vga.o"))      "vga.c"
Run $cc ($cflags + @("-c","kernel\drivers\fb.c",      "-o","build\obj\fb.o"))       "fb.c"
Run $cc ($cflags + @("-c","kernel\drivers\keyboard.c","-o","build\obj\keyboard.o")) "keyboard.c"
Run $cc ($cflags + @("-c","kernel\drivers\ata.c",     "-o","build\obj\ata.o"))      "ata.c"

Write-Host "[5/11] arch C..."
Run $cc ($cflags + @("-c","kernel\arch\gdt.c",    "-o","build\obj\gdt.o"))    "gdt.c"
Run $cc ($cflags + @("-c","kernel\arch\idt.c",    "-o","build\obj\idt.o"))    "idt.c"
Run $cc ($cflags + @("-c","kernel\arch\isr.c",    "-o","build\obj\isr.o"))    "isr.c"
Run $cc ($cflags + @("-c","kernel\arch\irq.c",    "-o","build\obj\irq.o"))    "irq.c"
Run $cc ($cflags + @("-c","kernel\arch\pic.c",    "-o","build\obj\pic.o"))    "pic.c"
Run $cc ($cflags + @("-c","kernel\arch\pit.c",    "-o","build\obj\pit.o"))    "pit.c"
Run $cc ($cflags + @("-c","kernel\arch\paging.c", "-o","build\obj\paging.o")) "paging.c"
Run $cc ($cflags + @("-c","kernel\arch\rtc.c",    "-o","build\obj\rtc.o"))    "rtc.c"

Write-Host "[6/11] mm C..."
Run $cc ($cflags + @("-c","kernel\mm\e820.c","-o","build\obj\e820.o")) "e820.c"
Run $cc ($cflags + @("-c","kernel\mm\pmm.c", "-o","build\obj\pmm.o"))  "pmm.c"
Run $cc ($cflags + @("-c","kernel\mm\heap.c","-o","build\obj\heap.o")) "heap.c"

Write-Host "[6.5/11] fs C..."
Run $cc ($cflags + @("-c","kernel\fs\pcfs.c",      "-o","build\obj\pcfs.o"))      "pcfs.c"
Run $cc ($cflags + @("-c","kernel\fs\pcfs_init.c", "-o","build\obj\pcfs_init.o")) "pcfs_init.c"
Run $cc ($cflags + @("-c","kernel\fs\pcfs_disk.c", "-o","build\obj\pcfs_disk.o")) "pcfs_disk.c"

Write-Host "[6.7/11] loader C..."
Run $cc ($cflags + @("-c","kernel\loader\qa.c","-o","build\obj\qa.o")) "qa.c"

Write-Host "[6.8/11] task C..."
Run $nasm @("-f","elf32","kernel\task\task_switch.asm","-o","build\obj\task_switch.o") "task_switch.asm"
Run $cc ($cflags + @("-c","kernel\task\task.c","-o","build\obj\task.o")) "task.c"

Write-Host "[7/11] shell + i18n + cmds..."
Run $cc ($cflags + @("-c","kernel\shell\shell.c",        "-o","build\obj\shell.o"))    "shell.c"
Run $cc ($cflags + @("-c","kernel\shell\i18n\i18n.c",    "-o","build\obj\i18n.o"))     "i18n.c"
Run $cc ($cflags + @("-c","kernel\shell\i18n\langs.c",   "-o","build\obj\langs.o"))    "langs.c"
Run $cc ($cflags + @("-c","kernel\shell\cmds\cmd_fs.c",  "-o","build\obj\cmd_fs.o"))   "cmd_fs.c"
Run $cc ($cflags + @("-c","kernel\shell\cmds\cmd_sys.c", "-o","build\obj\cmd_sys.o"))  "cmd_sys.c"

Write-Host "[8/11] Linking kernel ELF..."
$objects = @(
    "build\obj\entry.o","build\obj\main.o","build\obj\log.o",
    "build\obj\string.o","build\obj\printf.o",
    "build\obj\vga.o","build\obj\fb.o",
    "build\obj\keyboard.o","build\obj\ata.o",
    "build\obj\gdt_flush.o","build\obj\isr_stub.o",
    "build\obj\gdt.o","build\obj\idt.o","build\obj\isr.o","build\obj\irq.o",
    "build\obj\pic.o","build\obj\pit.o","build\obj\paging.o","build\obj\rtc.o",
    "build\obj\e820.o","build\obj\pmm.o","build\obj\heap.o",
    "build\obj\pcfs.o","build\obj\pcfs_init.o","build\obj\pcfs_disk.o",
    "build\obj\qa.o",
    "build\obj\task_switch.o","build\obj\task.o",
    "build\obj\shell.o","build\obj\i18n.o","build\obj\langs.o",
    "build\obj\cmd_fs.o","build\obj\cmd_sys.o"
)

$libgcc = & $cc -m32 -print-libgcc-file-name
if (-not (Test-Path $libgcc)) {
    Write-Host "[FAIL] libgcc not found" -ForegroundColor Red
    exit 1
}
Write-Host ("  libgcc : " + $libgcc)

$linkArgs = @("-T","kernel\kernel.ld","-o",$kernelElf) + $objects + @($libgcc)
Run $ld $linkArgs "linking kernel.elf"

Write-Host "[9/11] objcopy..."
Run $objcopy @("-O","binary",$kernelElf,$kernelBin) "objcopy"

# ============================================================
# userland
# ============================================================
Write-Host "[10/11] userland: build .qa files..."

if (Test-Path $ulDistDir) { Remove-Item -Recurse -Force $ulDistDir }
New-Item -ItemType Directory -Path $ulBinDir  | Out-Null
New-Item -ItemType Directory -Path $ulEtcDir  | Out-Null
New-Item -ItemType Directory -Path $ulHomeDir | Out-Null
New-Item -ItemType Directory -Path $ulTmpDir  | Out-Null

# apps/*/*.asm -> .qa
$appsDir = "userland\apps"
if (Test-Path $appsDir) {
    $asmFiles = Get-ChildItem -Path $appsDir -Filter "*.asm" -Recurse
    foreach ($f in $asmFiles) {
        $name = $f.BaseName
        $out  = Join-Path $ulBinDir ($name + ".qa")
        & powershell -NoProfile -ExecutionPolicy Bypass -File $mkqaSrc -Asm $f.FullName -Out $out
        if ($LASTEXITCODE -ne 0) {
            Write-Host ("[FAIL] mkqa on " + $f.Name) -ForegroundColor Red
            exit 1
        }
    }
} else {
    Write-Host "  (no userland\apps dir, skipping)"
}

# scripts / config / home
$scriptsDir = "userland\scripts"
if (Test-Path $scriptsDir) {
    Copy-Item "$scriptsDir\*.set" $ulBinDir -ErrorAction SilentlyContinue
}
$configDir = "userland\config"
if (Test-Path $configDir) {
    Copy-Item "$configDir\*.ou" $ulEtcDir -ErrorAction SilentlyContinue
}
$homeDir = "userland\home"
if (Test-Path $homeDir) {
    Copy-Item "$homeDir\*" $ulHomeDir -Recurse -ErrorAction SilentlyContinue
}

# ============================================================
# PCFS image
# ============================================================
Write-Host "[10.5/11] mkfs_pcfs..."

if (-not (Test-Path $mkfsSrc)) {
    Write-Host ("[FAIL] missing: " + $mkfsSrc) -ForegroundColor Red
    exit 1
}
Write-Host ("  src : " + $mkfsSrc)

Run $gcc @("-O2","-o",$mkfsExe,$mkfsSrc) "mkfs_pcfs.c"

# 每次重建 pcfs.img
if (Test-Path $pcfsImg) {
    Remove-Item $pcfsImg -Force
}
Run $mkfsExe @($ulDistDir, $pcfsImg) "mkfs_pcfs"

# ============================================================
# floppy image
# ============================================================
Write-Host "[11/11] assembling floppy image..."

foreach ($f in @($imgPath, $pcfsImg)) {
    if (Test-Path $f) {
        try {
            $stream = [IO.File]::Open($f, 'Open', 'ReadWrite', 'None')
            $stream.Close()
        } catch {
            Write-Host ("[FAIL] " + $f + " is locked (QEMU still running?)") -ForegroundColor Red
            exit 1
        }
    }
}

$IMG_SIZE = 1474560
$SECTOR = 512
$STAGE2_MAX = $SECTOR * 8
$KERNEL_MAX = $SECTOR * 512

$b = [IO.File]::ReadAllBytes($bootBin)
$s = [IO.File]::ReadAllBytes($stage2Bin)
$k = [IO.File]::ReadAllBytes($kernelBin)

if ($b.Length -gt $SECTOR)     { Write-Host "[FAIL] boot.bin too large" -ForegroundColor Red; exit 1 }
if ($s.Length -gt $STAGE2_MAX) { Write-Host "[FAIL] stage2.bin too large" -ForegroundColor Red; exit 1 }
if ($k.Length -gt $KERNEL_MAX) { Write-Host "[FAIL] kernel.bin too large" -ForegroundColor Red; exit 1 }

Write-Host ("  boot.bin   = {0,8} bytes" -f $b.Length)
Write-Host ("  stage2.bin = {0,8} bytes" -f $s.Length)
Write-Host ("  kernel.bin = {0,8} bytes" -f $k.Length)

$img = New-Object byte[] $IMG_SIZE
[Array]::Copy($b, 0, $img, 0,         $b.Length)
[Array]::Copy($s, 0, $img, $SECTOR,   $s.Length)
[Array]::Copy($k, 0, $img, $SECTOR*9, $k.Length)
[IO.File]::WriteAllBytes($imgPath, $img)

Write-Host ""
Write-Host "============================================================"
Write-Host " Build OK"
Write-Host ("   floppy: " + $imgPath)
Write-Host ("   disk  : " + $pcfsImg)
Write-Host "============================================================"
exit 0
