<#
.SYNOPSIS

Downloads and and extracts STM32CubeMX support pack for STM32 series from ST website.

.EXAMPLE

#>

Import-Module .\tools\scripts\Build-netmf.psm1

# target version for STM32Cube pack
$packVersion = "1.11.0"

# STM32 series 
$seriesName = "F4" 

Write-Host "Installing STM32Cube CMSIS pack for STM32$seriesName series"

# pack file name
$packFileName = "stm32cube_fw_" + $seriesName.ToLower() +  "_v" + ($packVersion -replace "[.]","") + ".zip"

# zip folder name
$zipPackFileName = "STM32Cube_FW_" + $seriesName + "_V" + $packVersion

# directory for the SMT32Cube 
$stmCubePath = [System.IO.Path]::Combine( $SPOCLIENT, "STM32Cube" )

# directory for the series under STM32Cube folder
$seriesPath = [System.IO.Path]::Combine( $stmCubePath, $seriesName )

# make sure the destination directory is empty (if it exists at all)
if([System.IO.Directory]::Exists( $seriesPath ) )
{
    [System.IO.Directory]::Delete( $seriesPath, $true)
}
else
{
    # need to create destination directory first
    [System.IO.Directory]::CreateDirectory( $seriesPath )
}

# check is pack file is already there
if(-not [System.IO.File]::Exists( [System.IO.Path]::Combine( $stmCubePath , $packFileName ) )) 
{
    # base URL to download the pack file from
    $packSourceURL = "http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/firmware/" + $packFileName 

    # download the pack...  
    Write-Host "Downloading SMT32Cube pack from ST web site. This will take a while, be patient..."

    # must use WebClient because the pack file is huge
    $webclient = New-Object System.Net.WebClient
    $webclient.DownloadFile( $packSourceURL , [System.IO.Path]::Combine( $stmCubePath , $packFileName ) )
}
else
{
    # pack is available...  
    Write-Host "SMT32Cube pack is already available, skipping download"
}

# ... and extract the files into the series directory
Write-Host "Extracting SMT32 pack..."

# must load this type to open Zip files
Add-Type -assembly  System.IO.Compression.FileSystem

$zipArchive = [IO.Compression.ZipFile]::OpenRead( [System.IO.Path]::Combine( $stmCubePath , $packFileName ) )

try
{
    # source folders and files that are to be copied
    $cmsisDeviceFolder = $zipPackFileName + "/Drivers/CMSIS/Device/ST/"
    $halDriverIncFolder = $zipPackFileName + "/Drivers/STM32" + $seriesName + "xx_HAL_Driver/Inc/"
    $halDriverSrcFolder = $zipPackFileName + "/Drivers/STM32" + $seriesName + "xx_HAL_Driver/Src/"
    $halDriverReleaseNotes = $zipPackFileName + "/Drivers/STM32" + $seriesName + "xx_HAL_Driver/Release_Notes.html"

    foreach($zipItem in $zipArchive.Entries)
    {

        if($zipItem.FullName.StartsWith( $cmsisDeviceFolder ) -Or
            $zipItem.FullName.StartsWith( $halDriverIncFolder ) -Or
            $zipItem.FullName.StartsWith( $halDriverSrcFolder ) -Or
            $zipItem.FullName.StartsWith( $halDriverReleaseNotes ))
        {
            $destinationFile   = [System.IO.Path]::Combine( $seriesPath, $zipItem.FullName.Replace($zipPackFileName, "").Replace("/", "\").SubString(1))
            $parentDir = [System.IO.Path]::GetDirectoryName( $destinationFile )

            # create directory if it doesn't exist
            if((-not [System.IO.Path]::HasExtension($parentDir)) -And (-not [System.IO.Directory]::Exists( $parentDir )) )
            {
                [System.IO.Directory]::CreateDirectory( $parentDir )
            }
            else
            {
                [IO.Compression.ZipFileExtensions]::ExtractToFile($zipItem, $destinationFile, $true)
            }
        }
    }
}
finally
{
    # free file
    $zipArchive.Dispose()
}

# copy CMSIS folder for this series from STM32Cube folder to the respective Device folder in CMSIS folder
# first make sure the destination directory is empty (if it exists at all) before copying as we don't want to mix versions
Write-Host "Copying CMSIS driver..."

$cmsisPathForSeries = $SPOCLIENT + "\CMSIS\Device\ST"

if([System.IO.Directory]::Exists( $cmsisPathForSeries ) )
{
    [System.IO.Directory]::Delete( $cmsisPathForSeries , $true)
}
  
Copy-Item ( $seriesPath + "\Drivers\CMSIS\Device\ST" ) -Destination $cmsisPathForSeries -recurse -Force

# copy HAL drivers for this series from STM32Cube folder to the respective DeviceCode folder 
# first make sure the destination directories are empty (if they exists at all) before copying as we don't want to mix versions
Write-Host "Copying HAL driver..."

$deviceCodePathForSeries = $SPOCLIENT + "\DeviceCode\Targets\Native\STM32" + $seriesName + "xx\HAL_Driver"

if([System.IO.Directory]::Exists( $deviceCodePathForSeries ) )
{
    [System.IO.Directory]::Delete( $deviceCodePathForSeries , $true)
}


Copy-Item ( $seriesPath + "\Drivers\STM32" + $seriesName + "xx_HAL_Driver" ) -Destination $deviceCodePathForSeries -recurse -Force

Write-Host "Installation of STM32Cube CMSIS pack for STM32$seriesName series completed"
