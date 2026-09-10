# For every cross-addon reference the graph reports, show the line it is on and
# whether that line is a comment. A name mentioned in a comment is not a
# dependency; counting it as one is how a clean graph gets reported as dirty.
$root = 'G:\MCF\addons'
$addons = Get-ChildItem $root -Directory | Select-Object -ExpandProperty Name

$declares = @{}
$files = @{}

foreach ($a in $addons) {
    $names = New-Object 'System.Collections.Generic.HashSet[string]'
    $list = @()
    Get-ChildItem "$root\$a\Scripts" -Recurse -Filter '*.c' -ErrorAction SilentlyContinue | ForEach-Object {
        $t = [IO.File]::ReadAllText($_.FullName)
        $list += , @($_.FullName, $t)
        foreach ($m in [regex]::Matches($t, '(?m)^\s*(?:modded\s+)?(?:class|enum)\s+(MCF_[A-Za-z0-9_]+)')) {
            [void]$names.Add($m.Groups[1].Value)
        }
    }
    $declares[$a] = $names
    $files[$a] = $list
}

foreach ($user in $addons) {
    if ($user -eq 'MCF_Dev') { continue }
    foreach ($owner in $addons) {
        if ($owner -eq $user -or $owner -eq 'MCF_Dev') { continue }
        foreach ($sym in ($declares[$owner] | Sort-Object)) {
            if ($declares[$user].Contains($sym)) { continue }
            foreach ($pair in $files[$user]) {
                $path = $pair[0]; $body = $pair[1]
                if (-not $body.Contains($sym)) { continue }
                $n = 0
                foreach ($line in ($body -split "`n")) {
                    $n++
                    if ($line -notmatch [regex]::Escape($sym)) { continue }
                    $trim = $line.Trim()
                    $kind = 'CODE'
                    if ($trim.StartsWith('//')) { $kind = 'comment' }
                    "{0,-14} -> {1,-14} {2,-8} {3}:{4}  {5}" -f $user, $owner, $kind, (Split-Path $path -Leaf), $n, $trim
                }
            }
        }
    }
}
