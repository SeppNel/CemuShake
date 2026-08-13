appimageDir:="./package/AppImage"

CXXFLAGS:=-O2 -Iinclude -MMD -MP
LDFLAGS:=-Llib

ifeq ($(OS),Windows_NT)
EXE_EXT:=.exe
LDFLAGS+=-static-libgcc -static-libstdc++
LDLIBS:=-Wl,-Bstatic -lwinpthread -Wl,-Bdynamic -lmingw32 -lSDL2main -lSDL2 -lyaml-cpp -lws2_32
else
EXE_EXT:=
LDLIBS:=-lpthread -lSDL2 -lSDL2main -lyaml-cpp
endif

TARGET:=build/CemuShake$(EXE_EXT)

SRCS:=$(wildcard *.cpp)
OBJS:=$(SRCS:%.cpp=build/%.o)
DEPS:=$(OBJS:.o=.d)

.PHONY: CemuShake run clean appimage windist

CemuShake: $(TARGET)

$(TARGET): $(OBJS)
	g++ $(OBJS) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

build/%.o: %.cpp | build
	g++ $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

-include $(DEPS)

run: CemuShake
	./$(TARGET)

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

windist: CemuShake
	rm -Rf build/windist
	mkdir -p build/windist
	cp $(TARGET) build/windist/
	ldd $(TARGET) | grep -i '/mingw64/' | awk '{print $$3}' | xargs -I{} cp {} build/windist/
	cd build/windist && zip -r ../CemuShake-x86_64-windows.zip .