param(
    [string]$Root = '.\note',
    [switch]$Apply
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-SectionContent {
    param(
        [string]$Block,
        [string[]]$Titles
    )

    $alts = ($Titles | ForEach-Object { [regex]::Escape($_) }) -join '|'
    $pattern = "(?ms)^###\s*(?:$alts)\s*\r?\n(.*?)(?=^###\s+|^##\s*题目：|^---\s*$|\z)"
    $m = [regex]::Match($Block, $pattern)
    if ($m.Success) {
        return $m.Groups[1].Value.Trim()
    }
    return ''
}

function Get-CalloutContent {
    param(
        [string]$Block,
        [string]$Type,
        [string]$Title
    )

    $escapedType = [regex]::Escape($Type)
    $escapedTitle = [regex]::Escape($Title)
    $pattern = "(?ms)^>\[!$escapedType\]-\s*$escapedTitle\s*\r?\n(.*?)(?=^>\[![^\]]+\]-|^###\s+|^##\s*题目：|^---\s*$|\z)"
    $m = [regex]::Match($Block, $pattern)
    if (-not $m.Success) {
        return ''
    }

    $raw = $m.Groups[1].Value.Trim()
    $lines = $raw -split '\r?\n' | ForEach-Object {
        if ($_ -match '^>\s?(.*)$') {
            $matches[1]
        } else {
            $_
        }
    }
    return (($lines -join "`n").Trim())
}

function Normalize-SampleText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ''
    }

    $lines = $Text -split '\r?\n' | ForEach-Object { $_.TrimEnd() }
    $lines = $lines | Where-Object {
        $t = $_.Trim()
        $t -ne '---' -and $t -ne '>---'
    }
    return (($lines -join "`n").Trim())
}

function To-CalloutBody {
    param(
        [string]$Text,
        [string]$Fallback = '- （待补充）'
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        $Text = $Fallback
    }

    $lines = $Text -split '\r?\n'
    return ($lines | ForEach-Object {
        if ($_ -eq '') { '>' } else { ">$($_.TrimEnd())" }
    }) -join "`n"
}

function Convert-QuestionBlock {
    param([string]$Block)

    $titleMatch = [regex]::Match($Block, '(?m)^##\s*题目：.*$')
    if (-not $titleMatch.Success) {
        return $Block
    }

    $already = ($Block -match '(?m)^>\[!NOTE\]-\s*标准准确的说法') -and
               ($Block -match '(?m)^>\[!tip\]-\s*代码示例') -and
               ($Block -match '(?m)^>\[!quote\]-\s*通俗易懂的理解') -and
               ($Block -match '(?m)^>\[!example\]-\s*面试回答简版模板') -and
               ($Block -match '(?m)^>\[!info\]-\s*对应示例') -and
               (-not ($Block -match '(?m)^>\s*---\s*$'))

    if ($already) {
        return ($Block.TrimEnd() + "`n")
    }

    $title = $titleMatch.Value.Trim()

    $std = Get-CalloutContent -Block $Block -Type 'NOTE' -Title '标准准确的说法'
    if ([string]::IsNullOrWhiteSpace($std)) {
        $std = Get-SectionContent -Block $Block -Titles @('标准准确的说法')
    }

    $code = Get-CalloutContent -Block $Block -Type 'tip' -Title '代码示例'
    if ([string]::IsNullOrWhiteSpace($code)) {
        $code = Get-SectionContent -Block $Block -Titles @('代码示例', '示例代码')
    }

    $plain = Get-CalloutContent -Block $Block -Type 'quote' -Title '通俗易懂的理解'
    if ([string]::IsNullOrWhiteSpace($plain)) {
        $plain = Get-SectionContent -Block $Block -Titles @('通俗易懂的理解')
    }

    $tpl = Get-CalloutContent -Block $Block -Type 'example' -Title '面试回答简版模板'
    if ([string]::IsNullOrWhiteSpace($tpl)) {
        $tpl = Get-SectionContent -Block $Block -Titles @('面试回答简版模板')
    }

    $sample = Get-CalloutContent -Block $Block -Type 'info' -Title '对应示例'
    if ([string]::IsNullOrWhiteSpace($sample)) {
        $sample = Get-SectionContent -Block $Block -Titles @('对应示例')
    }
    $sample = Normalize-SampleText -Text $sample

    $codeFallback = [string]::Join("`n", @('```cpp', '// （待补充）', '```'))

    if ([string]::IsNullOrWhiteSpace($code)) {
        $firstCode = [regex]::Match($Block, '(?ms)```[\s\S]*?```')
        if ($firstCode.Success) {
            $code = $firstCode.Value.Trim()
        }
    }

    if ([string]::IsNullOrWhiteSpace($code)) {
        $code = $codeFallback
    }

    $out = New-Object System.Collections.Generic.List[string]
    $out.Add($title)
    $out.Add('>[!NOTE]- 标准准确的说法')
    $out.Add((To-CalloutBody -Text $std))
    $out.Add('')
    $out.Add('>[!tip]- 代码示例')
    $out.Add((To-CalloutBody -Text $code -Fallback $codeFallback))
    $out.Add('')
    $out.Add('>[!quote]- 通俗易懂的理解')
    $out.Add((To-CalloutBody -Text $plain))
    $out.Add('')
    $out.Add('>[!example]- 面试回答简版模板')
    $out.Add((To-CalloutBody -Text $tpl -Fallback '- （待补充）'))
    $out.Add('')
    $out.Add('>[!info]- 对应示例')
    $out.Add((To-CalloutBody -Text $sample -Fallback '- （待补充）'))
    $out.Add('')
    $out.Add('---')
    $out.Add('')

    return ($out -join "`n")
}

$rxQuestion = [regex]'(?ms)^##\s*题目：.*?(?=^##\s*题目：|\z)'

$files = Get-ChildItem -Path $Root -Recurse -File -Filter *.md |
    Where-Object { $_.FullName -notmatch '[\\/]\.obsidian[\\/]' }

$scanned = 0
$changed = 0

foreach ($f in $files) {
    $scanned++
    $old = [System.IO.File]::ReadAllText($f.FullName)
    $new = $rxQuestion.Replace($old, { param($m) Convert-QuestionBlock -Block $m.Value })

    if ($new -ne $old) {
        $changed++
        if ($Apply) {
            $bak = "$($f.FullName).bak"
            if (-not (Test-Path -LiteralPath $bak)) {
                Copy-Item -LiteralPath $f.FullName -Destination $bak
            }
            $utf8Bom = New-Object System.Text.UTF8Encoding($true)
            [System.IO.File]::WriteAllText($f.FullName, $new, $utf8Bom)
            Write-Host "[APPLY] $($f.FullName)"
        } else {
            Write-Host "[DRY ] $($f.FullName)"
        }
    }
}

Write-Host ''
Write-Host "Scanned: $scanned"
Write-Host "Changed: $changed"
if (-not $Apply) {
    Write-Host '当前为预览模式；确认后加 -Apply 执行写入。'
}