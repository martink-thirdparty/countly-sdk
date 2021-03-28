robocopy "%MkBuildPath%\output\Release Lib\x86\openssl\include\openssl" vanilla\src\openssl /MIR

cmake -A Win32 -S vanilla -B %MkBuildPath%\intermediate\countly_sdk
cmake -DBUILD_SHARED_LIBS:BOOL="0"  -DCMAKE_INSTALL_PREFIX:PATH="%MkBuildPath%\output\Release Lib\x86\countly_sdk" -DOPENSSL_ROOT_DIR:PATH="%MkBuildPath%\output\Release Lib\x86\openssl" -DOPENSSL_INCLUDE_DIR:PATH="%MkBuildPath%\output\Release Lib\x86\openssl\include" %MkBuildPath%\intermediate\countly_sdk
cmake --build %MkBuildPath%\intermediate\countly_sdk --config Release
rem does nothing: cmake --install %MkBuildPath%\intermediate\countly_sdk --config Release

robocopy %~dp0\vanilla\include "%MkBuildPath%\output\Release Lib\x86\countly_sdk\include" /MIR
mkdir "%MkBuildPath%\output\Release Lib\x86\countly_sdk\lib"
copy %MkBuildPath%\intermediate\countly_sdk\Release\countly.lib "%MkBuildPath%\output\Release Lib\x86\countly_sdk\lib"