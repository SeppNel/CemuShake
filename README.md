# CemuShake
Linux and Windows app to simulate a shake motion input with a standard controller using the cemuhook protocol.

Anything that uses the cemuhook (dsu) protocol should be compatible with this, if it lets you map a standard controller alongside this.  

The main purpose for the development for this tool was to do motion exclusive throws in Mario Odyssey (Ryujinx).

# Usage
Configure your client (Ryujinx, Dolphin, etc...) like any other dsu client, with your ip and port 26760, or set up a custom port in the config file.  
Turn on controller first, open CemuShake and open your client. It should work.

By default RB (R1) is a shake with no gyro, to change this see the configuration section below.

## Configuration
You can configure the actions and some stuff creating a yaml config file in your home folder. It should be in `$HOME/.config/CemuShake.yml`

Valid configs are:
| Key | Value | Description |
| :---: | :---: | :---: |
| port | uint | Network port to use for the server |
| buttons | list | List of actions with its correspending button, see table below to see how to add an entry |

Buttons list elements:
| Key | Value | Description |
| :---: | :---: | :---: |
| id | uint | Controller button to map, see below for possible values |
| accX | float | Accelerometer value in X |
| accY | float | Accelerometer value in Y |
| accZ | float | Accelerometer value in Z |
| pitch | float | Value to apply to pitch over time |
| yaw | float | Value to apply to yaw over time |
| roll | float | Value to apply to roll over time |

Buttons id values:
| Id | Button (Xbox notation) | Button (PS notation) |
| :---: | :---: | :---: |
| 0 | A | X |
| 1 | B | O |
| 2 | X | □ |
| 3 | Y | △ |
| 4 | Back | Share |
| 5 | Guide | PS |
| 6 | Start | Options |
| 7 | LStick | L3 |
| 8 | RStick | R3 |
| 9 | LB | L1 |
| 10 | RB | R1 |
| 11 | Up | Up |
| 13 | Down | Down |
| 12 | Left | Left |
| 14 | Right | Right |
| 15 | Share | Mic (PS5) |
| 16 | Paddle 1 (Elite) | Not applicable |
| 17 | Paddle 2 (Elite) | Not applicable |
| 18 | Paddle 3 (Elite) | Not applicable |
| 19 | Paddle 4 (Elite) | Not applicable |
| 20 | Not applicable | Touchpad |

### Example config file
```
---
port: 26760
buttons:
    - id: 9 # LB
      accX: 0
      accY: 200 # Shake Up
      accZ: 0
      pitch: -20 # Gyro backwards
      yaw: 0
      roll: 0
    - id: 10 # RB
      accX: 0
      accY: 200 # Shake up
      accZ: 0
      pitch: 20 # Gyro forward
      yaw: 0
      roll: 0
```
# Building
You need SDL2 for the controller and yaml-cpp for the config, so install those through your package manager or with vcpkg as described in the dependencies section.

On linux you just need to run `make`  
On Windows for now you need to set up a VS project, import the files and compile.

## Dependencies
For ubuntu:  
`sudo apt-get install libsdl2-dev libyaml-cpp-dev`

For Fedora:  
`sudo dnf install SDL2-devel yaml-cpp-devel`

For Windows (vcpkg):  
`vcpkg install sdl2 yaml-cpp`

# Acknowledgements
[Valeri](https://github.com/v1993) for his documentation [Cemuhook-Protocol](https://github.com/v1993/cemuhook-protocol)  
[kmicki](https://github.com/kmicki) for his [SteamDeckGyroDSU](https://github.com/kmicki/SteamDeckGyroDSU) wich i borrowed a lot of the server code
