$adapter = Get-NetAdapter -Name "vEthernet (WSL)"

if (!$adapter) {
    Write-Host "vEthernet adapter not found."
    exit
}
$ipAddress = Get-NetIPAddress -InterfaceIndex $adapter.ifIndex -AddressFamily IPv4 | Select-Object -ExpandProperty IPAddress
if (!$ipAddress) {
        Write-Host "No IPv4 address found for the vEthernet adapter."
        exit
}
Write-Host "IPv4 address of vEthernet adapter: $ipAddress"

# Define the path to the JSON file
$filePath = "E:\DWP\github\LinuxAudioTest\.vs\launch.vs.json"


# Check if the file exists
if (-Not (Test-Path $filePath)) {
    Write-Host "File not found: $filePath"
    exit
}

# Read the JSON file
$jsonContent = Get-Content -Path $filePath -Raw | ConvertFrom-Json

# Check if "env": {} exists and replace it
$fileHasBeenChanged = $false
if ($jsonContent.configurations) {
    $config = $jsonContent.configurations[0]
    foreach ($config in $jsonContent.configurations) {
        if ($config.PSObject.Properties['env']) {
            # Remove the "env" property
            $config.PSObject.Properties.Remove('env')
            Write-Host "''env'' found, removed"
        }
        elseif ($config.PSObject.Properties['environment']) {
            # Remove the "env" property
            $config.PSObject.Properties.Remove('environment')
            Write-Host "''environment'' found, removed"
        }
    }
    Write-Host "Adding.."

    # Add the new "environment" property
    $config | Add-Member -MemberType NoteProperty -Name 'environment' -Value @(
        @{
            name  = "PULSE_SERVER"
            value = "tcp:$ipAddress"
        }
    )
    $fileHasBeenChanged = $true
}

if (!$fileHasBeenChanged) {
    Write-Host "No need to update: $filePath"
    exit
}

Write-Host "Updating: $filePath"

# Convert the updated object back to JSON
$updatedJsonContent = $jsonContent | ConvertTo-Json -Depth 10

# Save the updated JSON back to the file
Set-Content -Path $filePath -Value $updatedJsonContent

Write-Host "File updated successfully: $filePath"