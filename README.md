# Omni Media Linker

[中文](README_zh.md)

Omni Media Linker is a Windows desktop utility for binding global hotkeys to specific media windows.

Instead of relying on the system media focus, it lets you control the exact target you want:

- `Alt+1` can pause a video player
- `Alt+2` can skip a music player
- Different apps can use different shortcuts without interfering with each other

## What Problem It Solves

Traditional media keys often depend on whichever app currently owns media focus.
That can lead to confusing behavior:

- The wrong player reacts
- Nothing reacts
- Music and video controls conflict with each other

Omni Media Linker avoids that by sending media commands directly to a chosen window.

## Core Features

- Scan active Windows desktop windows
- Create bindings between a target window, a media action, and a global hotkey
- Manage bindings with `enable`, `disable`, `edit`, and `delete`
- Persist bindings to a local `ini` configuration file
- Register global hotkeys and dispatch commands in the background
- Prevent multiple instances from running at the same time

## Supported Media Actions

- Play / Pause
- Next Track
- Previous Track
- Stop
- Volume Up
- Volume Down
- Mute

## Matching Modes

Each binding can choose one of the following matching modes:

1. `Title Exact`
Matches the window by exact title and class.

2. `Title Contains`
Matches when the current window title contains the saved title text.

3. `Process Name`
Matches by executable name, useful for apps with highly dynamic titles such as Spotify.

4. `Process Name + Class`
Matches by both executable name and window class for stricter targeting.

## Project Structure

- `scanner`
Finds active windows and collects metadata such as title, class name, process id, and process name.

- `actions`
Sends media-related Windows messages to the chosen target window.

- `mapper`
Stores the relationship between hotkeys and target bindings.

- `config_loader`
Loads and saves binding data in `ini` format.

- `dispatcher`
Registers global hotkeys and dispatches actions through the Windows message loop.

- `main`
Provides the CLI interface and ties all modules together.

## Typical Flow

1. Start the application
2. Scan visible windows
3. Choose a target window
4. Choose an action
5. Choose a matching mode
6. Record a hotkey
7. Save the binding
8. Enable bindings and start listening

## Platform

- Windows
- C++
- CMake
- Win32 API

## Notes

- This project targets desktop media control scenarios
- Global hotkeys are registered through the Windows API
- The tool is designed to support both stable titles and highly dynamic titles
