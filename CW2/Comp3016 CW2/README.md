# Dungeon Escape – COMP3016 CW2 Coursework


## Build Instructions
- Open the solution file: Comp3016 CW2.sln
- Set configuration to x64 / Debug
- Build and Run

## Controls
- A / D : Change lane
- W     : Jump
- S     : Crouch
- ESC   : Quit

## GitHub Link
https://github.com/Dixonj23/COMP3016/tree/main/CW2/Comp3016%20CW2

## Dependencies used
The project was developed in unmanaged C++ using OpenGL4 and the following approved libraries:
- GLFW: For window creation, input handling and context management
- GLAD: For the loading of OpenGL functions
- GLM: For all mathematics including vectors, matricies and transformations
- ASSIMP: For importing 3D models and meshes
- IrrKlang: For audio playback of backgound music, ambient sounds and other sound effects

As stated by the assessment brief requirements, no external engines were used in development.

## Gameplay
Dungeon escape is a first-person endless runner-style prototype. The player automatically moves forward through a 
dungeon consisting of three lanes surrounded by walls and a procedural environment. The player can switch lanes,
jump, and crouch to avoid dynamically spawning obstacles.

The game continues indefinitely until the player collides with an obstacle incorrectly (e.g. failing to jump over
low obstacles, avoid tall obstackes or crouch under overhead obstacles). On collision, the game briefly pauses, a 
death sound plays, the screen fades to black, and the game world resets.

As the player survives longer, the game becomes progressively more difficult through increased movement speed,
higher obstacle density, and a change in background music to signal difficulty escalation. 


## Use of AI
Generative AI was used in accordance with the acceptable level stated in the assessment brief, Partnered Work.
As such, ChatGPT was used during development as a code assistant by finding the route cause of errors and 
providing guidance with previously unused libraries. 


## Evaluation
This project was completely new to me both in terms of it being my first time using OpenGL but also the first
project i've undergone with almost zero set requirements. I believe my final submission far exceeds the initial
goal of simple "creating something that does something in OpenGL" by forming a cohesive (and hopefully fun) game
experience.

If i were to do the project again, i would spend more time on planning the additional libraries added as i spent
a fair chunk of time refactoring code to allow for lighting and audio systems, as well as not getting approval for
a UI/Text library making me unable to complete my stretch goal of displaying the players final score on screen.

Overall, this project has greatly increased my knowledge of both modern game architecture and real-time 
software developement.