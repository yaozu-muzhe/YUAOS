# ============================================================
#  YUAOS mkfont ---- .hz text font -> .bin binary
#  File : tools/mkfont.ps1
# ============================================================
param(
    [Parameter(Mandatory=$true)][string]$In,
    [Parameter(Mandatory=$true)][string]$Out
)

$ErrorActionPreference = "Stop"

$lines = Get-Content $In -Encoding UTF8
$glyphs = @()
$i = 0

function Is-GlyphStart([string]$line) {
    return $line -match '^@[0-9A-Fa-f]+\s'
}

function Is-SkipLine([string]$line) {
    $t = $line.Trim()
    if ($t -eq "") { return $true }
    if ($t.StartsWith("#")) { return $true }
    return $false
}

while ($i -lt $lines.Count) {
    if (Is-SkipLine $lines[$i]) { $i++; continue }

    $trimmed = $lines[$i].Trim()
    $i++

    if (-not (Is-GlyphStart $trimmed)) {
        throw "line $($i): expected '@<hex> <name>', got: '$trimmed'"
    }

    $parts = $trimmed.Substring(1).Split(" ", 2)
    $cp = [Convert]::ToUInt32($parts[0], 16)

    $bits = New-Object byte[] 32
    $rowCount = 0

    while ($rowCount -lt 16) {
        # 跳过内部空行和注释
        while ($i -lt $lines.Count -and (Is-SkipLine $lines[$i])) { $i++ }
        if ($i -ge $lines.Count) {
            throw "glyph U+$($cp.ToString('X4')) truncated: only $rowCount rows"
        }

        $rowStr = $lines[$i].Trim()
        $i++

        if (Is-GlyphStart $rowStr) {
            throw "glyph U+$($cp.ToString('X4')) has only $rowCount rows (before next '@')"
        }

        if ($rowStr.Length -lt 16) {
            $rowStr = $rowStr.PadRight(16, '.')
        }

        $word = 0
        for ($col = 0; $col -lt 16; $col++) {
            $ch = $rowStr[$col]
            if ($ch -eq '#' -or $ch -eq 'X') {
                $word = $word -bor (0x8000 -shr $col)
            }
        }
        $bits[$rowCount*2]     = [byte](($word -shr 8) -band 0xFF)
        $bits[$rowCount*2 + 1] = [byte]($word -band 0xFF)
        $rowCount++
    }

    $glyphs += @{ cp = $cp; bits = $bits }
}

# ---- 写二进制 ----
$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)

$bw.Write([byte][char]'Y')
$bw.Write([byte][char]'F')
$bw.Write([byte][char]'N')
$bw.Write([byte][char]'T')
$bw.Write([uint32]1)
$bw.Write([uint32]$glyphs.Count)
$bw.Write([uint32]0)

$off = 0
foreach ($g in $glyphs) {
    $bw.Write([uint32]$g.cp)
    $bw.Write([uint32]$off)
    $off += 32
}

foreach ($g in $glyphs) { $bw.Write($g.bits) }

$bw.Flush()
$bytes = $ms.ToArray()
[IO.File]::WriteAllBytes($Out, $bytes)

Write-Host ("  mkfont: " + $glyphs.Count + " glyphs, " + $bytes.Length + " bytes")
