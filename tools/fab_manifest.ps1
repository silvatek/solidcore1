# Read or update one string field on an asset in tools/fab-assets.json.
# The field replace matches SolidFabManifest::WriteJsonStringField.
param(
    [Parameter(Mandatory = $true)][ValidateSet('get', 'set')][string]$Action,
    [Parameter(Mandatory = $true)][string]$Id,
    [Parameter(Mandatory = $true)][string]$Field,
    [string]$Value = '',
    [string]$OutFile = ''
)

$ErrorActionPreference = 'Stop'
$Manifest = Join-Path $PSScriptRoot 'fab-assets.json'
$Utf8 = New-Object System.Text.UTF8Encoding $false

function Find-AssetSpan([string]$Text, [string]$AssetId) {
    $Quoted = '"' + $AssetId + '"'
    $Search = 0
    while ($Search -lt $Text.Length) {
        $IdKey = $Text.IndexOf('"id"', $Search)
        if ($IdKey -lt 0) { return $null }
        $Index = $IdKey + 4
        while ($Index -lt $Text.Length -and [char]::IsWhiteSpace($Text[$Index])) { $Index++ }
        if ($Index -ge $Text.Length -or $Text[$Index] -ne ':') { $Search = $IdKey + 4; continue }
        $Index++
        while ($Index -lt $Text.Length -and [char]::IsWhiteSpace($Text[$Index])) { $Index++ }
        if (-not $Text.Substring($Index).StartsWith($Quoted)) { $Search = $IdKey + 4; continue }

        $Brace = -1
        for ($Cursor = $IdKey; $Cursor -ge 0; $Cursor--) {
            if ($Text[$Cursor] -eq '{') { $Brace = $Cursor; break }
        }
        if ($Brace -lt 0) { return $null }

        $Depth = 0
        $InString = $false
        $Escape = $false
        for ($Cursor = $Brace; $Cursor -lt $Text.Length; $Cursor++) {
            $Char = $Text[$Cursor]
            if ($InString) {
                if ($Escape) { $Escape = $false; continue }
                if ($Char -eq '\') { $Escape = $true; continue }
                if ($Char -eq '"') { $InString = $false }
                continue
            }
            if ($Char -eq '"') { $InString = $true; continue }
            if ($Char -eq '{') { $Depth++ }
            elseif ($Char -eq '}') {
                $Depth--
                if ($Depth -eq 0) { return @{ Start = $Brace; End = $Cursor } }
            }
        }
        return $null
    }
    return $null
}

function Read-StringAt([string]$Text, [int]$KeyAt, [int]$Limit) {
    $KeyEnd = $Text.IndexOf('"', $KeyAt + 1)
    if ($KeyEnd -lt 0 -or $KeyEnd -ge $Limit) { return $null }
    $Index = $KeyEnd + 1
    while ($Index -lt $Limit -and [char]::IsWhiteSpace($Text[$Index])) { $Index++ }
    if ($Index -ge $Limit -or $Text[$Index] -ne ':') { return $null }
    $Index++
    while ($Index -lt $Limit -and [char]::IsWhiteSpace($Text[$Index])) { $Index++ }
    if ($Index -ge $Limit -or $Text[$Index] -ne '"') { return $null }
    $Index++
    $Start = $Index
    $Builder = New-Object System.Text.StringBuilder
    while ($Index -lt $Limit) {
        $Char = $Text[$Index]
        if ($Char -eq '"') {
            return @{ Value = $Builder.ToString(); Start = $Start; End = $Index }
        }
        if ($Char -eq '\' -and ($Index + 1) -lt $Limit) {
            [void]$Builder.Append($Text[$Index + 1])
            $Index += 2
            continue
        }
        [void]$Builder.Append($Char)
        $Index++
    }
    return $null
}

function Read-Field([string]$Text, [int]$Start, [int]$End, [string]$Name) {
    $Key = '"' + $Name + '"'
    $From = $Start
    while ($From -lt $End) {
        $At = $Text.IndexOf($Key, $From)
        if ($At -lt 0 -or $At -ge $End) { return $null }
        $Read = Read-StringAt $Text $At $End
        if ($null -ne $Read) { return $Read }
        $From = $At + $Key.Length
    }
    return $null
}

function Normalize-ContentPath([string]$Path) {
    $Normal = $Path.Replace('\', '/')
    while ($Normal.StartsWith('/')) { $Normal = $Normal.Substring(1) }
    while ($Normal.EndsWith('/')) { $Normal = $Normal.Substring(0, $Normal.Length - 1) }
    return $Normal
}

$Raw = [System.IO.File]::ReadAllText($Manifest)
$Span = Find-AssetSpan $Raw $Id
if ($null -eq $Span) {
    Write-Error "Fab manifest has no asset $Id"
    exit 2
}
$Current = Read-Field $Raw $Span.Start $Span.End $Field
if ($null -eq $Current) {
    Write-Error "Fab manifest asset $Id has no field $Field"
    exit 2
}

if ($Action -eq 'get') {
    if ($OutFile) {
        [System.IO.File]::WriteAllText($OutFile, $Current.Value, $Utf8)
    } else {
        Write-Output $Current.Value
    }
    exit 0
}

$Normal = Normalize-ContentPath $Value
if ($Current.Value -eq $Normal) { exit 0 }

$Updated = $Raw.Substring(0, $Current.Start) + $Normal + $Raw.Substring($Current.End)
[System.IO.File]::WriteAllText($Manifest, $Updated, $Utf8)
exit 0
