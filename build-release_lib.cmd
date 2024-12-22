robocopy "%MkBuildPath%\output\Release Lib\x86\openssl\include\openssl" vanilla\src\openssl /MIR

cmake -S vanilla -B %MkBuildPath%\intermediate\countly_sdk\release_lib
cmake -DBUILD_SHARED_LIBS:BOOL="0" -DCMAKE_BUILD_TYPE:STRING="Release" -DCMAKE_INSTALL_PREFIX:PATH="%MkBuildPath%\output\Release Lib\x86\countly_sdk" -DOPENSSL_ROOT_DIR:PATH="%MkBuildPath%\output\Release Lib\x86\openssl" -DOPENSSL_INCLUDE_DIR:PATH="%MkBuildPath%\output\Release Lib\x86\openssl\include" %MkBuildPath%\intermediate\countly_sdk\release_lib
cmake --build %MkBuildPath%\intermediate\countly_sdk\release_lib --config Release
rem does nothing: cmake --install %MkBuildPath%\intermediate\countly_sdk\release_lib --config Release

robocopy vanilla\include "%MkBuildPath%\output\Release Lib\x86\countly_sdk\include" /MIR
mkdir "%MkBuildPath%\output\Release Lib\x86\countly_sdk\lib"
copy %MkBuildPath%\intermediate\countly_sdk\release_lib\countly.lib "%MkBuildPath%\output\Release Lib\x86\countly_sdk\lib"

robocopy vanilla\vendor\json\include\nlohmann "%MkBuildPath%\output\Release Lib\x86\countly_sdk\include\nlohmann" /MIR