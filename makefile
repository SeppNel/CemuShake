CemuShake:
	mkdir -p build
	g++ *.cpp -o build/CemuShake -Iinclude -lpthread -Llib -lSDL2 -lSDL2main

run:
	./build/CemuShake

clean:
	rm -R build/