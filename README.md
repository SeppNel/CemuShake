# CemuShake
Linux and Windows app to simulate a shake motion input with a standard controller using the cemuhook protocol.

Anything that uses the cemuhook (dsu) protocol should be compatible with this, if it lets you map a standard controller alongside this.  

The main purpose for the development for this tool was to do motion exclusive throws in Mario Odyssey with a standard Xbox controller (Ryujinx).

# Usage
Configure your client (Ryujinx, Dolphin, etc...) like any other dsu client, with your ip and port 26760, or set up a custom port in the config file.  
Turn on controller, open CemuShake and open your client. It should work.

By default RB (R1) is a shake with no gyro, to change this see the configuration section below.

## Features
 - AutoShake: Press select + start to active the auto shake. Lasts for about 4 seconds. Useful for mapping motion in yuzu/eden
 - Gyro Compensation: Replays your motion back so that the simulated controller goes back to a consistent resting position after every press.

## Configuration
You can configure the actions and some stuff creating a yaml config file in the same directory as the executable or in your home folder. It should be in `$HOME/.config/CemuShake.yml`

Valid configs are:
| Key | Value | Description |
| :---: | :---: | :---: |
| port | uint | Network port to use for the server |
| gyro_compensation | bool | If feature is enabled |
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

### Example config file (My setup for SMO)
```
---
gyro_compensation: true
port: 26760
buttons:
    - id: 7
      accX: 0
      accY: 200
      accZ: 0
      pitch: 30
      yaw: 0
      roll: 10
    - id: 8
      accX: 0
      accY: 200
      accZ: 0
      pitch: -30
      yaw: 0
      roll: 10
```
# Building
You need SDL2 for the controller and yaml-cpp for the config, so install those through your package manager or with vcpkg as described in the dependencies section.

On linux you just need to run `make`  
On Windows, install [MSYS2](https://www.msys2.org/), open the MINGW64 shell, install the dependencies below and run `make`

## Dependencies
For ubuntu:  
`sudo apt-get install libsdl2-dev libyaml-cpp-dev`

For Fedora:  
`sudo dnf install SDL2-devel yaml-cpp-devel`

For Windows (MSYS2 MINGW64 shell):  
`pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-yaml-cpp zip make`

# Acknowledgements
[Valeri](https://github.com/v1993) for his documentation [Cemuhook-Protocol](https://github.com/v1993/cemuhook-protocol)  
[kmicki](https://github.com/kmicki) for his [SteamDeckGyroDSU](https://github.com/kmicki/SteamDeckGyroDSU) wich i borrowed a lot of the server code
