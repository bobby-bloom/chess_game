# Chess Game

Welcome to my C-based Chess Game! This is a personal project I started to learn more about C and memory management. 
It’s not fully optimized yet, but it’s playable on a single computer using mouse drag & drop.


## Planned Features

- **Game Variant Selection**: Add options to select different chess variants.
- **Time Control**: Introduce timer options.
- **More Variants**:  Implement variants such as Chess960.
- **Online Play**: Connect to [Lichess.org](https://lichess.org) for online matches.
- **Local Engine**: Integrate the Stockfish chess engine for local AI gameplay.


## Project Template

This project was set up using [TheCherno's Project Template](https://github.com/TheCherno/ProjectTemplate). 
It also uses Premake scripts for easy building. Big thanks to TheCherno for providing such a helpful starting point.


# Getting started

This guide provides instructions to build and run the project on Windows.


## Prerequisites

1. **Git**: Download and install Git from [git-scm.com](https://git-scm.com).
2. **Visual Studio**: Install the latest version of Visual Studio with the "Desktop Development with C++" workload.
3. **vcpkg**: Install vcpkg for dependency management.


## Step 1: Clone the Repository

1. Open a terminal or command prompt.
2. Clone the repository using the following command:
   ```bash
   git clone https://github.com/bobby-bloom/chess_game.git
   ```
3. Navigate to the project directory:
   ```bash
   cd chess_game
   ```


## Step 2: Run the Setup Script

The project includes platform-specific setup scripts to generate the required solution files.
This solution can currently only be build on Windows.

1. Locate the script at `Scripts/Setup-Windows.bat`.
2. Run the script:
   ```cmd
   Scripts\Setup-Windows.bat
   ```
3. After running the script, a `.sln` file (Visual Studio solution) should appear in the project root directory.


## Step 3: Activate Manifest Mode
1. Open the generated `.sln` file in Visual Studio.
2. In the **Solution Explorer**, locate the `Client-Windows` project.
3. Right-click on the project and select **Properties**.
4. Navigate to `Configuration Properties > vcpkg`.
5. Set the following options:
   - **Use vcpkg**: `Yes`
   - **Use vcpkg Manifest**: `Yes`
6. Click **Apply** and **OK** to save the settings.


## Step 4: Build and run the Project

1. In Visual Studio, set the build configuration using the dropdown menu in the toolbar.
2. Build the project by selecting **Build > Build Solution**..
3. Run the project:
   - To run with debugging, press `F5`.
   - To run without debugging, press `Ctrl+F5`.

## License

- UNLICENSE for this repository (see UNLICENSE.txt for more details)
- Premake is licensed under BSD 3-Clause (see included LICENSE.txt file for more details)