param([string]$workingDirectory = $PSScriptRoot)
Write-Host 'Executing Powershell script IncrementVersion.ps1 with working directory set to: ' $workingDirectory
Set-Location $workingDirectory

$inputFileName = 'Package.appxmanifest'
$outputFileName = $PSScriptRoot + '/Package.appxmanifest';

$now = Get-Date
$versionBuild = $now.Year - 2000
$versionRevision = $now.DayOfYear

$content = (gc $inputFileName) -join "`r`n"

$callback = {
  param($match)
    [string]$versionMajor = $match.Groups[2].Value
    [string]$versionMinor = $match.Groups[3].Value
    #[int]$intNum = [convert]::ToInt32($versionMajor, 10)
    $match.Groups[1].Value + 'Version="' + $versionMajor + '.' + $versionMinor + '.' + $versionBuild + '.' + $versionRevision + '"'
}

$identityRegex = [regex]'(\<Identity[^\>]*)Version=\"([0-9])+\.([0-9]+)\.([0-9]+)\.([0-9]+)\.*\"'
$newContent = $identityRegex.Replace($content, $callback)
#Write-Host $newContent
[io.file]::WriteAllText($outputFileName, $newContent)