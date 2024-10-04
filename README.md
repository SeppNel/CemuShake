# CemuShake
Linux app to simulate a shake motion input with a standar controller using the cemuhook protocol.

Anything that uses the cemuhook (dsu) protocol should be compatible with this, if it lets you map a standard controller alongside this.  

The main purpose for the development for this tool was to do motion exclusive throws in Mario Odyssey (Ryujinx).

# Usage
Configure your client (Ryujinx, Dolphin, etc...) like any other dsu server, with your ip and port 26760.  
Turn on controller first, open CemuShake and open your client. It should work.

For now shoulder buttons result in a shake (upwards or downwards) and cannot be remaped, but I plan to add configuration in the future.

# Building
You need SDL2 for the controller, so install that through your package manager.  
Then you just need to run make

# Acknowledgements
[Valeri](https://github.com/v1993) for his documentation [Cemuhook-Protocol](https://github.com/v1993/cemuhook-protocol)  
[kmicki](https://github.com/kmicki) for his [SteamDeckGyroDSU](https://github.com/kmicki/SteamDeckGyroDSU) wich i borrowed a lot of the server code
