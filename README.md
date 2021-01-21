# Computercraft-Websocket-Controller

this is a program that allows you to remotely control computercraft turtles  
this project is not finished and is still being worked on 

&nbsp;

this is made to work with websocket-slave.lua from [computercraft-suite](https://github.com/SpaceCat-Chan/computercraft_suite/blob/master/websocket_slave.lua)

## dependancies:
- boost serialization  
- boost asio  
- SDL2  
- OpenGL  
- glew  
- glm  
- any git submodules in this repository

## compiling:
standard cmake setup.
```
$ mkdir build && cd build
$ cmake ..
$ make
```

the executable will be in build/src  
it does not require installation and can safely be run from that folder

## how to use:
ui should be intuitive enough, drag on empty screen to look around, space to swith to freecam mode, where you can use w and s  
the screen will be black and useless until a turtle connects.

## how to connect turtles:
use [websocket-slave.lua](https://github.com/SpaceCat-Chan/computercraft_suite/blob/master/websocket_slave.lua) and change line 147 to be your computers ip, computercraft might not allow connections to localhost  
this program will host on port 8080, but turtles will attempt to connect using port 80, to change this go to `src/Server.hpp` and change line 43 to the desired port, port 80 requires root on linux

[websocket-slave.lua](https://github.com/SpaceCat-Chan/computercraft_suite/blob/master/websocket_slave.lua) expects 6 arguments: x, y, z, o, dimension name, server name  
x,y, and z are the turtles coordinates, o is the turtles orientation (0 = north, 1 = east, 2 = south, 3 = west)

when sending evals to the turtle, use position.forward (and position.turnLeft/turnRight) and the other position functions for movement instead of the standard turtle functions, this is necessary for the turtle to be able to track it's movement
