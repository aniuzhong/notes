# VSCode Configuration

## Contents
- [VSCode Configuration](#vscode-configuration)
  - [Contents](#contents)
  - [Enable Continuous Input on macOS for Visual Studio Code](#enable-continuous-input-on-macos-for-visual-studio-code)
  - [User Settings File Locations in Visual Studio Code](#user-settings-file-locations-in-visual-studio-code)
  - [Basic Settings](#basic-settings)
  - [Extentions](#extentions)
    - [Vim](#vim)
    - [Markdown All in One](#markdown-all-in-one)
    - [C/C++](#cc)
    - [clangd](#clangd)
    - [CMake Tools](#cmake-tools)
    - [Git Graph](#git-graph)
    - [vscode-icon](#vscode-icon)
    - [C/C++ Themes](#cc-themes)
    - [Font Changer](#font-changer)
    - [Zoomer](#zoomer)
    - [Output Colorizer](#output-colorizer)
  - [launch.json](#launchjson)

## Enable Continuous Input on macOS for Visual Studio Code

By default, macOS has a "Press and Hold" feature for keys, which enables accent selection instead of continuous input. This can be problematic for users who rely on repeating key presses. To disable the "Press and Hold" behavior for VS Code, open a terminal and run the following command:

``` sh
defaults write com.microsoft.VSCode ApplePressAndHoldEnabled -bool false
```

After executing the command, restart Visual Studio Code for the changes to take effect.

## User Settings File Locations in Visual Studio Code

- **Windows**

    `%USERPROFILE%\AppData\Roaming\Code\User\settings.json`

- **Linux**

    `~/.config/Code/User/settings.json`
    `~/.vscode-server/data/Machine/settings.json`

- **macOS**

    `~/Library/Application Support/Code/User/settings.json`

## Basic Settings

``` json
{
    "editor.cursorBlinking": "smooth",
    "editor.cursorSmoothCaretAnimation": "on",
    "editor.lineNumbers": "interval",
    "editor.renderWhitespace": "all",
    "editor.smoothScrolling": true,
    "files.autoSave": "afterDelay",
    "git.autofetch": true,
    "terminal.integrated.smoothScrolling": true,
    "workbench.list.smoothScrolling": true,
    "workbench.editor.wrapTabs": true,
}
```

## Extentions

### Vim

``` json
{
    "vim.autoindent": true,
    "vim.easymotion": true,
    "vim.foldfix": true,
    "vim.hlsearch": true,
    "vim.highlightedyank.enable": true,
    "vim.highlightedyank.duration": 300,
    "vim.leader": "<Space>",
    "vim.smartcase": true,
    "vim.searchHighlightColor": "#5F00AF",
    "vim.surround": true,
    "vim.useCtrlKeys": true,
    "vim.useSystemClipboard": true,
    "vim.normalModeKeyBindingsNonRecursive":[
        {"before":["H"],"after":["g","T"]},
        {"before":["L"],"after":["g","t"]},
        {"before":["M"],"commands":["editor.toggleFold"]},
        {"before":["<C-h>"],"commands":["workbench.files.action.focusFilesExplorer"]},
        {"before":["<C-l>"],"commands":["terminal.focus"]},
        {"before":["<C-y>"],"commands":["editor.action.formatDocument"]},
        {"before":["<C-e>"],"commands":["workbench.action.showCommands"]},
        {"before":["<C-f>"],"commands":["workbench.action.showCommands"]},
        {"before":["<C-b>"],"commands":["workbench.action.showCommands"]},
    ],
    "vim.insertModeKeyBindingsNonRecursive":[
        {"before":["j","k"],"after":["<Esc>"]},
    ],
}
```

### Markdown All in One

### C/C++

``` json
{
    "C_Cpp.intelliSenseEngine": "disabled",
}
```

### clangd

### CMake Tools

``` json
{
    "cmake.pinnedCommands": [
    ],

    "cmake.configureSettings": {
        "CMAKE_TOOLCHAIN_FILE": "/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake",
    },
    "cmake.debugConfig": {
        "args": []
    }
}
```

### Git Graph

### vscode-icon

``` json
{
    "workbench.iconTheme": "vscode-icons",
}
```

### C/C++ Themes

### Font Changer

### Zoomer

### Output Colorizer

## launch.json

``` json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "a.out",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/a.out",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build/bin",
            "environment": [
                {
                    "name": "LD_LIBRARY_PATH",
                    "value": "${workspaceFolder}/lib"
                }
            ],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        },
    ]
}
```

