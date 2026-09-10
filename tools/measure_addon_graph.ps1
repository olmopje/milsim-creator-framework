# Measure the real cross-addon dependency graph.
#
# Not by reading anyone's word for it: extract every class and enum each addon
# declares, then count how many of those names are USED in every other addon's
# scripts.
#
# COMMENT LINES ARE STRIPPED FIRST, and that is not a nicety. Counting them
# reported five illegal edges that do not exist -- Core "depending on" Ops
# because MCF_Core_Roles.c has the words MCF_ETaskState in a sentence explaining
# why an enum is append-only. A cross-reference is code or it is nothing.
#
# Run this before believing any claim about the structure. This document's
# predecessor was wrong for a week because nobody did.
$root = 'G:\MCF\addons'
$addons = Get-ChildItem $root -Directory | Select-Object -ExpandProperty Name

$declares = @{}
$code = @{}

foreach ($a in $addons) {
    $names = New-Object 'System.Collections.Generic.HashSet[string]'
    $sb = New-Object System.Text.StringBuilder
    $bytes = 0
    Get-ChildItem "$root\$a\Scripts" -Recurse -Filter '*.c' -ErrorAction SilentlyContinue | ForEach-Object {
        $t = [IO.File]::ReadAllText($_.FullName)
        $bytes += $t.Length
        foreach ($m in [regex]::Matches($t, '(?m)^\s*(?:modded\s+)?(?:class|enum)\s+(MCF_[A-Za-z0-9_]+)')) {
            [void]$names.Add($m.Groups[1].Value)
        }
        foreach ($line in ($t -split "`n")) {
            if ($line.Trim().StartsWith('//')) { continue }
            [void]$sb.AppendLine($line)
        }
    }
    $declares[$a] = $names
    $code[$a] = @{ text = $sb.ToString(); bytes = $bytes }
}

"addon            declares   script"
foreach ($a in $addons) {
    "{0,-16} {1,8}   {2} KB" -f $a, $declares[$a].Count, [int]($code[$a].bytes / 1024)
}
""
"cross-addon references in CODE (user -> owner: distinct symbols used):"
foreach ($user in $addons) {
    $line = @()
    foreach ($owner in $addons) {
        if ($owner -eq $user) { continue }
        $n = 0
        foreach ($sym in $declares[$owner]) {
            if ($declares[$user].Contains($sym)) { continue }
            if ($code[$user].text.Contains($sym)) { $n++ }
        }
        if ($n -gt 0) { $line += "$owner $n" }
    }
    if ($line.Count -eq 0) { $line += '(none)' }
    "{0,-16} -> {1}" -f $user, ($line -join ', ')
}
""
"namespaces declared per addon:"
foreach ($a in $addons) {
    $ns = @{}
    foreach ($sym in $declares[$a]) {
        $m = [regex]::Match($sym, '^(MCF_[A-Za-z0-9]+)_')
        if ($m.Success) { $ns[$m.Groups[1].Value] = 1 }
    }
    "{0,-16} {1}" -f $a, (($ns.Keys | Sort-Object) -join ', ')
}
