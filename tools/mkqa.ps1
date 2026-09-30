# ============================================================
#  YUAOS mkqa —— .asm -> .qa
#  文件 : tools/mkqa.ps1
#  用法 : powershell -File tools\mkqa.ps1 -Asm <in.asm> -Out <out.qa>
#  依赖 : nasm 已在 PATH
# ============================================================
param(
    [Parameter(Mandatory=$true)][string]$Asm,
    [Parameter(Mandatory=$true)][string]$Out
)

$ErrorActionPreference = "Stop"

# 项目根 = tools/ 的父目录，作为 nasm -I 的搜索路径
$root = Split-Path -Parent $PSScriptRoot

# ---- 1. 汇编 ----
$tmp = [System.IO.Path]::GetTempFileName()
& nasm -f bin -I "$root/" $Asm -o $tmp
if ($LASTEXITCODE -ne 0) {
    Remove-Item $tmp -ErrorAction SilentlyContinue
    throw "nasm failed on $Asm"
}

$code = [IO.File]::ReadAllBytes($tmp)
Remove-Item $tmp -ErrorAction SilentlyContinue

# ---- 2. 生成 24 字节头 ----
#   [0..3]   'Q' 'A' 'E' 'X'
#   [4..7]   version    = 1
#   [8..11]  entry      = 0
#   [12..15] code_size
#   [16..19] data_size  = 0
#   [20..23] reserved   = 0
$hdr = New-Object byte[] 24
$hdr[0] = [byte][char]'Q'
$hdr[1] = [byte][char]'A'
$hdr[2] = [byte][char]'E'
$hdr[3] = [byte][char]'X'

# PowerShell 的整数类型是 [uint32]，不是 [u32]
$verBytes    = [BitConverter]::GetBytes([uint32]1)
$entryBytes  = [BitConverter]::GetBytes([uint32]0)
$codeBytes   = [BitConverter]::GetBytes([uint32]$code.Length)
$dataBytes   = [BitConverter]::GetBytes([uint32]0)
$rsvBytes    = [BitConverter]::GetBytes([uint32]0)

[Array]::Copy($verBytes,   0, $hdr, 4,  4)
[Array]::Copy($entryBytes, 0, $hdr, 8,  4)
[Array]::Copy($codeBytes,  0, $hdr, 12, 4)
[Array]::Copy($dataBytes,  0, $hdr, 16, 4)
[Array]::Copy($rsvBytes,   0, $hdr, 20, 4)

# ---- 3. 拼接输出 ----
$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory $outDir | Out-Null
}

$qa = New-Object byte[] (24 + $code.Length)
[Array]::Copy($hdr,  0, $qa, 0,  24)
[Array]::Copy($code, 0, $qa, 24, $code.Length)
[IO.File]::WriteAllBytes($Out, $qa)

Write-Host ("  mkqa: {0}  ->  {1}  ({2} bytes)" -f `
            (Split-Path -Leaf $Asm), (Split-Path -Leaf $Out), $qa.Length)
