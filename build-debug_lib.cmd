robocopy "%MkBuildPath%\output\Debug Lib\x86\openssl\include\openssl" vanilla\src\openssl /MIR

cmake -A Win32 -S vanilla -B %MkBuildPath%\intermediate\countly_sdk\debug_lib
cmake -DBUILD_SHARED_LIBS:BOOL="0"  -DCMAKE_INSTALL_PREFIX:PATH="%MkBuildPath%\output\Debug Lib\x86\countly_sdk" -DOPENSSL_ROOT_DIR:PATH="%MkBuildPath%\output\Debug Lib\x86\openssl" -DOPENSSL_INCLUDE_DIR:PATH="%MkBuildPath%\output\Debug Lib\x86\openssl\include" %MkBuildPath%\intermediate\countly_sdk\debug_lib
cmake --build %MkBuildPath%\intermediate\countly_sdk\debug_lib --config Debug
rem does nothing: cmake --install %MkBuildPath%\intermediate\countly_sdk\debug_lib --config Debug

robocopy vanilla\include "%MkBuildPath%\output\Debug Lib\x86\countly_sdk\include" /MIR
mkdir "%MkBuildPath%\output\Debug Lib\x86\countly_sdk\lib"
copy %MkBuildPath%\intermediate\countly_sdk\debug_lib\Debug\countly.lib "%MkBuildPath%\output\Debug Lib\x86\countly_sdk\lib"

robocopy vanilla\vendor\json\include\nlohmann "%MkBuildPath%\output\Debug Lib\x86\countly_sdk\include\nlohmann" /MIR