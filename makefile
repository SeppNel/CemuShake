appimageDir:="./package/AppImage"

CemuShake:
	mkdir -p build
	g++ *.cpp -o build/CemuShake -Iinclude -lpthread -Llib -lSDL2 -lSDL2main -lyaml-cpp

run:
	./build/CemuShake

clean:
	rm -R build/
	rm -R "${appimageDir}/AppDir"
	rm "${appimageDir}/icon.svg"
	rm "${appimageDir}/linuxdeploy-x86_64.AppImage"

appimage: CemuShake
	touch ${appimageDir}/icon.svg
	curl -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage -o "${appimageDir}/linuxdeploy-x86_64.AppImage"
	chmod +x "${appimageDir}/linuxdeploy-x86_64.AppImage"
	NO_STRIP=1 ${appimageDir}/linuxdeploy-x86_64.AppImage --appdir ${appimageDir}/AppDir --executable build/CemuShake --desktop-file ${appimageDir}/CemuShake.desktop -i ${appimageDir}/icon.svg --output appimage
	mv CemuShake-x86_64.AppImage build/