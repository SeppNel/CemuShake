appimageDir:="./package/AppImage"

CXXFLAGS:=-O2 -Iinclude -MMD -MP
LDFLAGS:=-Llib
LDLIBS:=-lpthread -lSDL2 -lSDL2main -lyaml-cpp

SRCS:=$(wildcard *.cpp)
OBJS:=$(SRCS:%.cpp=build/%.o)
DEPS:=$(OBJS:.o=.d)

.PHONY: CemuShake run clean appimage

CemuShake: build/CemuShake

build/CemuShake: $(OBJS)
	g++ $(OBJS) -o build/CemuShake $(LDFLAGS) $(LDLIBS)

build/%.o: %.cpp | build
	g++ $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

-include $(DEPS)

run: CemuShake
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