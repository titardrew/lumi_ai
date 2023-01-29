rem @echo off

set current_dir=%cd%
set build_folder=build_win
set install_folder=Windows\x86_64
set openvino_bin_folder="C:\Program Files (x86)\Intel\openvino_2022\runtime\bin\intel64\Release\*.*"
set openvino_3rd_party_bin_folder="C:\Program Files (x86)\Intel\openvino_2022\runtime\3rdparty\tbb\bin"

if exist %build_folder% rmdir /s /q %build_folder% || echo ERROR && exit /b
if exist %install_folder% rmdir /s /q %install_folder% || echo ERROR && exit /b

mkdir %build_folder% || echo ERROR && exit /b
mkdir %install_folder% || echo ERROR && exit /b

cmake -DCMAKE_BUILD_TYPE=Release -B %build_folder% -S %current_dir% || echo ERROR && exit /b
cmake --build %build_folder% --config Release || echo ERROR && exit /b

xcopy %openvino_bin_folder% %install_folder% || echo ERROR && exit /b
xcopy %openvino_3rd_party_bin_folder% %install_folder% || echo ERROR && exit /b
xcopy "%build_folder%\Release\*.dll" %install_folder% || echo ERROR && exit /b
