[CmdletBinding()]
param(
    [string]$Configuration = "Release",
    [switch]$DownloadPrerequisites
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($Configuration -ne "Release") {
    throw "Only the canonical Release UTT artefacts may be packaged."
}

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$projectFile = Join-Path $repoRoot "CMakeLists.txt"
$projectText = Get-Content -LiteralPath $projectFile -Raw
$versionMatch = [regex]::Match(
    $projectText,
    'project\s*\(\s*UTT\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)',
    [Text.RegularExpressions.RegexOptions]::IgnoreCase)
if (-not $versionMatch.Success) {
    throw "Could not read the UTT product version from $projectFile."
}
$version = $versionMatch.Groups[1].Value

$releaseRoot = Join-Path $repoRoot "build\UTT_artefacts\Release"
$vst3Source = Join-Path $releaseRoot "VST3\UTT.vst3"
$standaloneSource = Join-Path $releaseRoot "Standalone\UTT.exe"
$payloadRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "payload"))
$cacheRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "cache"))
$distRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot "dist\UTT-$version-windows-x64"))
$releaseArchive = "$distRoot.zip"
$releaseArchiveHashPath = "$releaseArchive.sha256"
$workspacePrefix = $repoRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar

foreach ($path in @($payloadRoot, $distRoot, $releaseArchive, $releaseArchiveHashPath)) {
    if (-not $path.StartsWith($workspacePrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a path outside the workspace: $path"
    }
}

if (-not (Test-Path -LiteralPath $vst3Source)) {
    throw "Missing canonical VST3 bundle: $vst3Source. Build UTT_All first."
}
if (-not (Test-Path -LiteralPath $standaloneSource)) {
    throw "Missing canonical Standalone executable: $standaloneSource. Build UTT_All first."
}

function Assert-VerifiedMicrosoftSignature {
    param([Parameter(Mandatory = $true)][string]$Path)

    $signature = Get-AuthenticodeSignature -FilePath $Path
    if ($signature.Status -ne "Valid" -or $signature.SignerCertificate.Subject -notmatch "Microsoft Corporation") {
        throw "Microsoft signature verification failed for ${Path}: $($signature.Status)"
    }
}

function Get-VerifiedMicrosoftDownload {
    param(
        [Parameter(Mandatory = $true)][string]$Uri,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    try {
        Invoke-WebRequest -Uri $Uri -OutFile $Destination
    }
    catch {
        $firstFailure = $_
        $downloadUri = [Uri]$Uri
        if ($downloadUri.Scheme -ne "https") {
            throw $firstFailure
        }

        $addresses = Resolve-DnsName -Name $downloadUri.Host -Server "8.8.8.8" -DnsOnly -ErrorAction Stop |
            Where-Object { $_.Type -eq "A" } |
            Select-Object -ExpandProperty IPAddress -Unique
        $downloaded = $false
        foreach ($address in $addresses) {
            Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
            & curl.exe --fail --location --silent --show-error --retry 2 --connect-timeout 15 `
                --speed-time 30 --speed-limit 1 `
                --resolve "$($downloadUri.Host):443:$address" --output $Destination $Uri
            $downloadedFile = Get-Item -LiteralPath $Destination -ErrorAction SilentlyContinue
            if ($LASTEXITCODE -eq 0 -and $null -ne $downloadedFile -and $downloadedFile.Length -gt 0) {
                $downloaded = $true
                break
            }
        }
        if (-not $downloaded) {
            throw $firstFailure
        }
    }

    Assert-VerifiedMicrosoftSignature -Path $Destination
}

if (Test-Path -LiteralPath $payloadRoot) {
    Remove-Item -LiteralPath $payloadRoot -Recurse -Force
}
if (Test-Path -LiteralPath $distRoot) {
    Remove-Item -LiteralPath $distRoot -Recurse -Force
}
foreach ($path in @($releaseArchive, $releaseArchiveHashPath)) {
    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Force
    }
}
New-Item -ItemType Directory -Path $payloadRoot, $cacheRoot, $distRoot -Force | Out-Null

$payloadZip = Join-Path $payloadRoot "utt_data.zip"
$licensePayload = Join-Path $payloadRoot "LICENSE.txt"
Compress-Archive -LiteralPath $vst3Source -DestinationPath $payloadZip -CompressionLevel Optimal
Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSE") -Destination $licensePayload

$webViewCache = Join-Path $cacheRoot "MicrosoftEdgeWebView2RuntimeInstallerX64.exe"
$vcRedistCache = Join-Path $cacheRoot "VC_redist.x64.exe"
if ($DownloadPrerequisites -or -not (Test-Path -LiteralPath $webViewCache)) {
    Get-VerifiedMicrosoftDownload -Uri "https://go.microsoft.com/fwlink/p/?LinkId=2124701" -Destination $webViewCache
}
if ($DownloadPrerequisites -or -not (Test-Path -LiteralPath $vcRedistCache)) {
    Get-VerifiedMicrosoftDownload -Uri "https://aka.ms/vs/17/release/vc_redist.x64.exe" -Destination $vcRedistCache
}
Assert-VerifiedMicrosoftSignature -Path $webViewCache
Assert-VerifiedMicrosoftSignature -Path $vcRedistCache
$webViewPayload = Join-Path $payloadRoot "MicrosoftEdgeWebView2RuntimeInstallerX64.exe"
$vcRedistPayload = Join-Path $payloadRoot "VC_redist.x64.exe"
Copy-Item -LiteralPath $webViewCache -Destination $webViewPayload
Copy-Item -LiteralPath $vcRedistCache -Destination $vcRedistPayload

function Find-CSharpCompiler {
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vsWhere) {
        $candidate = @(& $vsWhere -latest -products "*" -requires Microsoft.Component.MSBuild `
            -find "MSBuild\Current\Bin\Roslyn\csc.exe") |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Select-Object -First 1
        if ($null -ne $candidate -and (Test-Path -LiteralPath $candidate)) {
            return $candidate
        }
    }

    $frameworkCompiler = Join-Path $env:WINDIR "Microsoft.NET\Framework64\v4.0.30319\csc.exe"
    if (Test-Path -LiteralPath $frameworkCompiler) {
        return $frameworkCompiler
    }

    throw "No C# compiler was found. Install Visual Studio Build Tools or .NET Framework 4.x."
}

$frameworkRoot = Join-Path $env:WINDIR "Microsoft.NET\Framework64\v4.0.30319"
$csc = Find-CSharpCompiler
$setupPath = Join-Path $distRoot "UTT_Setup.exe"
$compilerArgs = @(
    "/nologo",
    "/target:winexe",
    "/platform:x64",
    "/optimize+",
    "/debug-",
    "/out:$setupPath",
    "/win32manifest:$PSScriptRoot\app.manifest",
    "/reference:$frameworkRoot\System.IO.Compression.dll",
    "/reference:$frameworkRoot\System.IO.Compression.FileSystem.dll",
    "/reference:$frameworkRoot\System.Windows.Forms.dll",
    "/resource:$payloadZip,UTTInstaller.Payload.utt_data.zip",
    "/resource:$webViewPayload,UTTInstaller.Payload.MicrosoftEdgeWebView2RuntimeInstallerX64.exe",
    "/resource:$vcRedistPayload,UTTInstaller.Payload.VC_redist.x64.exe",
    "/resource:$licensePayload,UTTInstaller.Payload.LICENSE.txt",
    "$PSScriptRoot\Installer.cs"
)

& $csc @compilerArgs
if ($LASTEXITCODE -ne 0) {
    throw "Installer compilation failed with exit code $LASTEXITCODE."
}

Copy-Item -LiteralPath $vst3Source -Destination (Join-Path $distRoot "UTT.vst3") -Recurse
New-Item -ItemType Directory -Path (Join-Path $distRoot "Standalone") | Out-Null
Copy-Item -LiteralPath $standaloneSource -Destination (Join-Path $distRoot "Standalone\UTT.exe")
Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSE") -Destination (Join-Path $distRoot "LICENSE.txt")

$hashPath = Join-Path $distRoot "SHA256SUMS.txt"
$hashes = Get-ChildItem -LiteralPath $distRoot -File -Recurse |
    Sort-Object FullName |
    ForEach-Object {
        $relativePath = $_.FullName.Substring($distRoot.Length).TrimStart([IO.Path]::DirectorySeparatorChar)
        "{0}  {1}" -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $relativePath
    }
Set-Content -LiteralPath $hashPath -Value $hashes -Encoding Ascii

Compress-Archive -LiteralPath $distRoot -DestinationPath $releaseArchive -CompressionLevel Optimal
$archiveHash = (Get-FileHash -LiteralPath $releaseArchive -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath $releaseArchiveHashPath -Value (
    "$archiveHash  $([IO.Path]::GetFileName($releaseArchive))") -Encoding Ascii

Write-Host "Built installer and release artefacts: $distRoot"
Write-Host "Built release archive: $releaseArchive"
