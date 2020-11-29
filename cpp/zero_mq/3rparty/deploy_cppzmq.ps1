$ErrorActionPreference = "Stop"

$jobs_count = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

$libzmq_version = '4.3.3'
$libzmq_zip = 'libzmq.zip'
$libzmq_root = 'libzmq'
$libzmq_config = 'Release'
$libzmq_install_root = 'C:\local\libzmq'

$cppzmq_version = '4.7.1'
$cppzmq_zip = 'cppzmq.zip'
$cppzmq_root = 'cppzmq'
$cppzmq_config = 'Release'
$cppzmq_install_root = 'C:\local\cppzmq'

$build_root = 'build_cppzmq'
if ((Test-Path $build_root))
{
	Remove-Item -Path $build_root -Recurse
}
New-Item -Path $build_root -Name $build_root -ItemType "directory"
Push-Location $build_root

# Install libzmq
if (-not (Test-Path $libzmq_zip))
{
	Invoke-WebRequest https://github.com/zeromq/libzmq/archive/v${libzmq_version}.zip -OutFile $libzmq_zip
}
if ((Test-Path $libzmq_root))
{
	Remove-Item -Path $libzmq_root -Recurse
}
Expand-Archive -Path $libzmq_zip -DestinationPath $libzmq_root

$libzmq_build = "$libzmq_root\build"
cmake `
	-S $libzmq_root\libzmq-$libzmq_version `
	-B $libzmq_build `
	-DCMAKE_INSTALL_PREFIX="$libzmq_install_root"
cmake `
	--build $libzmq_build `
	--parallel $jobs_count `
	--config $libzmq_config `
	--target install

# Install cppzmq
if (-not (Test-Path $cppzmq_zip))
{
	Invoke-WebRequest https://github.com/zeromq/cppzmq/archive/v${cppzmq_version}.zip -OutFile $cppzmq_zip
}
if ((Test-Path $cppzmq_root))
{
	Remove-Item -path $cppzmq_root -Recurse
}
Expand-Archive -Path $cppzmq_zip -DestinationPath $cppzmq_root

$cppzmq_build = "$cppzmq_root\build"
cmake `
	-S $cppzmq_root\cppzmq-$cppzmq_version `
	-B $cppzmq_build `
	-DCPPZMQ_BUILD_TESTS=OFF `
	-DCMAKE_PREFIX_PATH="$libzmq_install_root" `
	-DCMAKE_INSTALL_PREFIX="$cppzmq_install_root"
cmake `
	--build $cppzmq_build `
	--parallel $jobs_count `
	--config $cppzmq_config `
	--target install

Pop-Location
Remove-Item -Path $build_root -Recurse
