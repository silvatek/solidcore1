# List meshes and animations inside a downloaded Fab pack.
# Read-only: does not edit tools/fab-assets.json.
#
# Usage:
#   tools\fab_mesh_info.bat Viking
#   tools\fab_mesh_info.bat Content\Viking
param(
    [Parameter(Position = 0)]
    [string]$Query
)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = (Resolve-Path (Join-Path $ScriptDir '..')).Path
$Content = Join-Path $Root 'Content'

function Write-Usage {
    Write-Output 'SolidCore1 Fab mesh info'
    Write-Output 'Usage: tools\fab_mesh_info.bat <folder name or path>'
    Write-Output 'Examples:'
    Write-Output '  tools\fab_mesh_info.bat Viking'
    Write-Output '  tools\fab_mesh_info.bat Content\Viking'
    Write-Output 'Prints folders, mesh files (size and SKM / SK / PA), and animation names.'
    Write-Output 'Does not change tools\fab-assets.json.'
}

function Normalize-Query([string]$Text) {
    $sep = [IO.Path]::DirectorySeparatorChar
    return $Text.Replace('/', $sep).Replace('\', $sep)
}

function Get-RelativePath([string]$Full, [string]$Base) {
    $prefix = $Base
    if (-not $prefix.EndsWith([IO.Path]::DirectorySeparatorChar)) {
        $prefix = $prefix + [IO.Path]::DirectorySeparatorChar
    }
    if ($Full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        return $Full.Substring($prefix.Length)
    }
    return $Full
}

function Split-Segments([string]$Relative) {
    return @($Relative -split '[\\/]' | Where-Object { $_ -ne '' })
}

function Test-Segment([string]$Relative, [string[]]$Names) {
    foreach ($part in (Split-Segments $Relative)) {
        foreach ($name in $Names) {
            if ($part.Equals($name, [StringComparison]::OrdinalIgnoreCase)) {
                return $true
            }
        }
    }
    return $false
}

function Get-Kind([string]$Name) {
    if ($Name.StartsWith('SKM_', [StringComparison]::OrdinalIgnoreCase)) { return 'SKM' }
    if ($Name.StartsWith('SK_', [StringComparison]::OrdinalIgnoreCase)) { return 'SK' }
    if ($Name.StartsWith('PA_', [StringComparison]::OrdinalIgnoreCase)) { return 'PA' }
    if ($Name.StartsWith('anim_', [StringComparison]::OrdinalIgnoreCase)) { return 'ANIM' }
    if ($Name.StartsWith('Anim_', [StringComparison]::OrdinalIgnoreCase)) { return 'ANIM' }
    return 'FILE'
}

function Resolve-PackFolders([string]$Text) {
    $result = New-Object System.Collections.Generic.List[string]
    $normalized = Normalize-Query $Text
    $direct = New-Object System.Collections.Generic.List[string]
    if ([IO.Path]::IsPathRooted($normalized)) {
        $direct.Add($normalized)
    }
    else {
        $direct.Add((Join-Path $Root $normalized))
        $direct.Add((Join-Path $Content $normalized))
    }
    foreach ($candidate in $direct) {
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            [void]$result.Add((Resolve-Path -LiteralPath $candidate).Path)
            Write-Output -NoEnumerate $result.ToArray()
            return
        }
    }

    $leaf = Split-Path -Leaf $normalized
    if ($leaf -and (Test-Path -LiteralPath $Content -PathType Container)) {
        foreach ($dir in @(Get-ChildItem -LiteralPath $Content -Recurse -Directory -Filter $leaf -ErrorAction SilentlyContinue)) {
            [void]$result.Add($dir.FullName)
        }
    }
    Write-Output -NoEnumerate $result.ToArray()
}

if (-not $Query) {
    Write-Usage
    exit 2
}

$folders = Resolve-PackFolders $Query
Write-Output 'SolidCore1 Fab mesh info'
Write-Output "Query: $Query"
Write-Output "Project: $Root"

if ($folders.Count -eq 0) {
    $leaf = Split-Path -Leaf (Normalize-Query $Query)
    Write-Output "MISSING  no folder named '$leaf' under Content"
    exit 1
}

if ($folders.Count -gt 1) {
    Write-Output "Matches: $($folders.Count)"
}

foreach ($folder in $folders) {
    $contentRel = Get-RelativePath $folder $Root
    Write-Output ''
    Write-Output "Folder: $contentRel"
    Write-Output "Full: $folder"

    $directories = @(Get-ChildItem -LiteralPath $folder -Recurse -Directory -ErrorAction SilentlyContinue |
        Sort-Object FullName)
    Write-Output ''
    Write-Output "Directories ($($directories.Count)):"
    foreach ($directory in $directories) {
        Write-Output ('  ' + (Get-RelativePath $directory.FullName $folder))
    }

    $assets = @(Get-ChildItem -LiteralPath $folder -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -eq '.uasset' -or $_.Extension -eq '.umap' })

    $meshes = New-Object System.Collections.Generic.List[object]
    $anims = New-Object System.Collections.Generic.List[object]
    $maps = New-Object System.Collections.Generic.List[object]
    $others = New-Object System.Collections.Generic.List[object]

    foreach ($file in $assets) {
        $rel = Get-RelativePath $file.FullName $folder
        $kind = Get-Kind $file.Name
        $row = [pscustomobject]@{
            Rel  = $rel
            Name = $file.Name
            Kind = $kind
            Size = $file.Length
        }
        $inMeshes = (Test-Segment $rel @('Meshes')) -or ($kind -eq 'SKM') -or ($kind -eq 'SK') -or ($kind -eq 'PA')
        $inAnims = ($kind -eq 'ANIM') -or (Test-Segment $rel @('animation', 'animations', 'anim'))
        if ($file.Extension -eq '.umap') {
            $maps.Add($row)
        }
        elseif ($inMeshes -and $kind -ne 'ANIM') {
            $meshes.Add($row)
        }
        elseif ($inAnims) {
            $anims.Add($row)
        }
        else {
            $others.Add($row)
        }
    }

    Write-Output ''
    Write-Output 'Meshes (SKM skeletal mesh, SK skeleton, PA physics):'
    $meshRows = @($meshes | Sort-Object Rel)
    if ($meshRows.Count -eq 0) {
        Write-Output '  NONE'
    }
    else {
        foreach ($row in $meshRows) {
            Write-Output ('  {0,12}  {1,-4}  {2}' -f $row.Size, $row.Kind, $row.Rel)
        }
        $largest = $meshRows | Where-Object { $_.Kind -eq 'SKM' } | Sort-Object Size -Descending | Select-Object -First 1
        if ($largest) {
            Write-Output ("Largest SKM: {0}  {1} bytes" -f $largest.Rel, $largest.Size)
        }
    }

    Write-Output ''
    Write-Output 'Animations:'
    $animRows = @($anims | Sort-Object Rel)
    if ($animRows.Count -eq 0) {
        Write-Output '  NONE'
    }
    else {
        foreach ($row in $animRows) {
            Write-Output ('  {0,12}  {1,-4}  {2}' -f $row.Size, $row.Kind, $row.Rel)
        }
    }

    Write-Output ''
    Write-Output 'Locomotion names (idle, walk, run, jump):'
    $loco = @($animRows | Where-Object { $_.Name -match '(?i)(idle|walk|run|jump)' })
    if ($loco.Count -eq 0) {
        Write-Output '  NONE'
    }
    else {
        foreach ($row in $loco) {
            Write-Output ('  {0}' -f $row.Rel)
        }
    }

    Write-Output ''
    Write-Output 'Maps:'
    $mapRows = @($maps | Sort-Object Rel)
    if ($mapRows.Count -eq 0) {
        Write-Output '  NONE'
    }
    else {
        foreach ($row in $mapRows) {
            Write-Output ('  {0,12}  {1}' -f $row.Size, $row.Rel)
        }
    }

    Write-Output ''
    Write-Output "Other uassets (not listed): $($others.Count)"
    if ($others.Count -gt 0) {
        $groups = $others | ForEach-Object {
            if ($_.Name -match '^([A-Za-z]+)_') { $Matches[1] + '_' } else { 'other' }
        } | Group-Object | Sort-Object Name
        foreach ($group in $groups) {
            Write-Output ("  {0} {1}" -f $group.Name, $group.Count)
        }
    }
}

exit 0
