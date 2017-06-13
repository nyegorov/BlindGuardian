param([string]$workingDirectory = $PSScriptRoot)
Write-Host 'Executing Powershell script IncrementVersion.ps1 with working directory set to: ' $workingDirectory
Set-Location $workingDirectory

$inputFileName = 'Package.appxmanifest'
$outputFileName = $PSScriptRoot + '/Package.appxmanifest';

$now = Get-Date
$versionMinor = $now.Year - 2000
$versionBuild = $now.DayOfYear
$versionRevision = ($now.Hour * 60) + $now.Minute

$content = (gc $inputFileName) -join "`r`n"

$callback = {
  param($match)
    [string]$versionMajor = $match.Groups[2].Value
    $match.Groups[1].Value + 'Version="' + $versionMajor + '.' + $versionMinor + '.' + $versionBuild + '.' + $versionRevision + '"'
}

$identityRegex = [regex]'(\<Identity[^\>]*)Version=\"([0-9])+\.([0-9]+)\.([0-9]+)\.([0-9]+)\.*\"'
$newContent = $identityRegex.Replace($content, $callback)
#Write-Host $newContent
[io.file]::WriteAllText($outputFileName, $newContent)