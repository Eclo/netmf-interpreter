﻿<#
.SYNOPSIS

Retrieves and extracts STM32CubeMX support pack for F4 familly from ST website.

.EXAMPLE

#>

Import-Module .\tools\scripts\Build-netmf.psm1

# current officially supported version
$packVersion = "1.11.0"


# pack file name
$packFileName = "stm32cube_fw_f4_v" + ($packVersion -replace "[.]","") + ".zip"

# zip folder name
$zipPackFileName = "STM32Cube_FW_F4_V" + $packVersion

# directory for the SMT32Cube 
$stmCubePath = [System.IO.Path]::Combine( $SPOCLIENT, "STM32Cube" )

# directory for the series under STM32Cube folder
$seriesPath = [System.IO.Path]::Combine( $stmCubePath, "F4" )

# make sure the destination directory is empty (if it exists at all)
if([System.IO.Directory]::Exists( $seriesPath ) )
{
    [System.IO.Directory]::Delete( $seriesPath, $true)}
else
{
    # need to create destination directory first
    [System.IO.Directory]::CreateDirectory( $seriesPath )
}

# base URL to download the pack file from
$packSourceURLBase = "http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/firmware/" + $packFileName 

# $packSourceURLBase = "http://www.colorado.edu/conflict/peace/download/peace_essay.ZIP"

# download the pack...  
Write-Host "Downloading SMT32F4 pack from ST web site. This will take a while, be patient..."

# check is pack file is already there
if(-not [System.IO.File]::Exists( [System.IO.Path]::Combine( $stmCubePath , $packFileName ) )) 
{
    # must use WebClient because the pack file is huge
    $webclient = New-Object System.Net.WebClient
    #$webclient.DownloadFile( $packSourceURLBase , [System.IO.Path]::Combine( $stmCubePath , $packFileName ) )
}

# ... and extract the files into the series directory
Write-Host "Extracting SMT32F4 pack"

$zipArchive = [IO.Compression.ZipFile]::OpenRead( [System.IO.Path]::Combine( $stmCubePath , $packFileName ) )

try
{
    # source folders and files that are to be copied
    $cmsisDeviceFolder = $zipPackFileName + "/Drivers/CMSIS/Device/ST/"
    $halDriverIncFolder = $zipPackFileName + "/Drivers/STM32F4xx_HAL_Driver/Inc/"
    $halDriverSrcFolder = $zipPackFileName + "/Drivers/STM32F4xx_HAL_Driver/Src/"
    $halDriverReleaseNotes = $zipPackFileName + "/Drivers/STM32F4xx_HAL_Driver/Release_Notes.html"

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

# copy CMSIS folder from STM32Cube folder to DeviceCode repository folder 
# Copy-Item [System.IO.Path]::Combine( $seriesPath, "")

Copy-Item ( $seriesPath + "\Drivers\CMSIS\Device\ST" ) -Destination  ( $SPOCLIENT + "\DeviceCode\Targets\Native\ST\" ) -recurse -Force
