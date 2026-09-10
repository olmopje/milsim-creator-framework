# Generate the Game Master property reference from the source.
#
# These are what a mission maker actually sees: the property rows on a placed
# entity, and the rows in Game Master's attribute panel. Writing that reference
# by hand guarantees it drifts away from the code within a release, so it is
# read out of the [Attribute(...)] declarations instead.
#
# Writes:
#   docs/guides/GM_PROPERTIES.md   the reference, committed
#   tools/gm_attributes.csv        the same data, for anything else that wants it
#
# Re-run it after adding or changing any [Attribute].
$root = 'G:\MCF\addons'
$out = @()

foreach ($a in (Get-ChildItem $root -Directory)) {
    Get-ChildItem "$($a.FullName)\Scripts" -Recurse -Filter '*.c' -ErrorAction SilentlyContinue | ForEach-Object {
        $t = [IO.File]::ReadAllText($_.FullName)
        $lines = $t -split "`n"
        $class = ''
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $l = $lines[$i]
            $cm = [regex]::Match($l, '^\s*(?:modded\s+)?class\s+([A-Za-z0-9_]+)')
            if ($cm.Success) { $class = $cm.Groups[1].Value }
            if ($l -notmatch '\[Attribute') { continue }

            # An attribute block can wrap over several lines; collect until the
            # bracket balances, then take the declaration line after it.
            $block = ''
            $j = $i
            $depth = 0
            do {
                $block += $lines[$j]
                $depth += ([regex]::Matches($lines[$j], '\(')).Count - ([regex]::Matches($lines[$j], '\)')).Count
                $j++
            } while ($depth -gt 0 -and $j -lt $lines.Count -and $j -lt $i + 8)

            $decl = ''
            if ($j -lt $lines.Count) { $decl = $lines[$j].Trim() }
            if ($decl -eq '') { $decl = $lines[$j - 1].Trim() }

            $name = [regex]::Match($decl, '([A-Za-z0-9_]+)\s*;').Groups[1].Value
            $type = [regex]::Match($decl, '(?:protected|private|ref)?\s*([A-Za-z0-9_<>\s]+?)\s+[A-Za-z0-9_]+\s*;').Groups[1].Value.Trim()
            $desc = [regex]::Match($block, 'desc\s*:\s*"([^"]*)"').Groups[1].Value
            if ($desc -eq '') {
                $desc = [regex]::Match($block, '\[Attribute\(\s*"[^"]*"\s*,\s*[^,]+,\s*"([^"]*)"').Groups[1].Value
            }
            $def = [regex]::Match($block, 'defvalue\s*:\s*"([^"]*)"').Groups[1].Value
            if ($def -eq '') { $def = [regex]::Match($block, '\[Attribute\(\s*"([^"]*)"').Groups[1].Value }
            $ui = [regex]::Match($block, 'UIWidgets\.([A-Za-z0-9_]+)').Groups[1].Value
            $enum = [regex]::Match($block, 'ParamEnumArray\.FromEnum\(([A-Za-z0-9_]+)\)').Groups[1].Value

            $out += [PSCustomObject]@{
                Addon = $a.Name; File = $_.Name; Class = $class; Name = $name
                Type = $type; Default = $def; Widget = $ui; Enum = $enum; Desc = $desc
            }
            $i = $j - 1
        }
    }
}

$out | Export-Csv 'G:\MCF\tools\gm_attributes.csv' -NoTypeInformation -Encoding UTF8

$order = 'MCF', 'MCF_Ops', 'MCF_Dialogue', 'MCF_AI', 'MCF_Objectives', 'MCF_Dev'
$label = @{
    'MCF' = 'MCF (Core)'; 'MCF_Ops' = 'MCF_Ops'; 'MCF_Dialogue' = 'MCF_Dialogue'
    'MCF_AI' = 'MCF_AI'; 'MCF_Objectives' = 'MCF_Objectives'; 'MCF_Dev' = 'MCF_Dev'
}

$sb = New-Object System.Text.StringBuilder
function W($s) { [void]$sb.AppendLine($s) }

W '# Game Master property reference'
W ''
W '**Generated from the source by `tools/generate_gm_reference.ps1`. Do not edit by hand** — re-run the script instead. Every row below is a real `[Attribute(...)]` declaration, so this file cannot describe a property that does not exist, and it goes stale the moment somebody adds one without re-running it.'
W ''
W ('Last generated: {0}. **{1} properties across {2} classes.**' -f (Get-Date -Format 'yyyy-MM-dd'), $out.Count, (($out | Select-Object -ExpandProperty Class -Unique).Count))
W ''
W 'These are the rows you see in the **entity properties panel** when you select a placed object, and in **Game Master''s attribute panel** for the handful of things MCF exposes there. They are set per placed entity, before or during a session.'
W ''
W 'They are not the whole story. MCF also has **four screens of its own**, opened by right-clicking an entity in Game Master, which edit content rather than settings — the intel editor, the device editor, the conversation editor and the mission data screen. Those are described on the wiki''s [Game Master](https://github.com/olmopje/milsim-creator-framework/wiki/Game-Master) page, which is also where the operations board is explained.'
W ''
W '## Contents'
W ''
foreach ($a in $order) {
    $n = ($out | Where-Object Addon -eq $a).Count
    if ($n -eq 0) { continue }
    $anchor = ($label[$a].ToLower() -replace '[^a-z0-9]+', '-').Trim('-')
    W ('- [{0}](#{1}) — {2} properties' -f $label[$a], $anchor, $n)
}
W ''

foreach ($a in $order) {
    $rows = $out | Where-Object Addon -eq $a
    if (-not $rows) { continue }
    W ('## {0}' -f $label[$a])
    W ''
    foreach ($g in ($rows | Group-Object Class | Sort-Object Name)) {
        W ('### `{0}`' -f $g.Name)
        W ''
        W ('<sub>{0}</sub>' -f $g.Group[0].File)
        W ''
        W '| Property | Type | Default | Shown as | What it does |'
        W '|---|---|---|---|---|'
        foreach ($r in $g.Group) {
            $d = $r.Desc -replace '\|', '\|'
            $t = $r.Type
            if ($r.Enum) { $t = $r.Enum }
            $def = $r.Default
            if ($def -eq '') { $def = '—' } else { $def = '`' + $def + '`' }
            $w = $r.Widget
            if ($w -eq '') { $w = '—' }
            W ('| `{0}` | {1} | {2} | {3} | {4} |' -f $r.Name, $t, $def, $w, $d)
        }
        W ''
    }
}

[IO.File]::WriteAllText('G:\MCF\docs\guides\GM_PROPERTIES.md', $sb.ToString())
"wrote docs/guides/GM_PROPERTIES.md  ($($out.Count) properties, $((($out | Select-Object -ExpandProperty Class -Unique)).Count) classes)"
"wrote tools/gm_attributes.csv"
