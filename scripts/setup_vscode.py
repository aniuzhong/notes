#!/usr/bin/env python3
"""
VSCode Configuration Script
跨平台 VSCode 配置自动化脚本
"""

import os
import sys
import json
import subprocess
import platform
from pathlib import Path


class VSCodeConfigurator:
    def __init__(self):
        self.os_type = platform.system()
        self.home_dir = Path.home()
        self.settings_path = self._get_settings_path()
        
    def _get_settings_path(self):
        """获取平台特定的配置文件路径"""
        if self.os_type == 'Windows':
            return self.home_dir / 'AppData' / 'Roaming' / 'Code' / 'User' / 'settings.json'
        elif self.os_type == 'Darwin':  # macOS
            return self.home_dir / 'Library' / 'Application Support' / 'Code' / 'User' / 'settings.json'
        elif self.os_type == 'Linux':
            return self.home_dir / '.config' / 'Code' / 'User' / 'settings.json'
        else:
            raise ValueError(f'Unsupported platform: {self.os_type}')
    
    def setup_macos_continuous_input(self):
        """macOS 特定设置：启用连续输入"""
        if self.os_type == 'Darwin':
            try:
                subprocess.run([
                    'defaults', 'write', 'com.microsoft.VSCode',
                    'ApplePressAndHoldEnabled', '-bool', 'false'
                ], check=True)
                print('✓ macOS continuous input enabled')
                print('  Please restart VSCode for changes to take effect')
            except subprocess.CalledProcessError as e:
                print(f'✗ Failed to configure macOS settings: {e}')
        else:
            print('⊘ Skipping macOS-specific settings (not on macOS)')
    
    def install_extensions(self):
        """安装 VSCode 插件"""
        extensions = [
            'vscodevim.vim',                    # Vim
            'yzhang.markdown-all-in-one',       # Markdown All in One
            'ms-vscode.cpptools',               # C/C++
            'clangd.clangd',                    # clangd
            'ms-vscode.cmake-tools',            # CMake Tools
            'mhutchie.git-graph',               # Git Graph
            'rokoroku.vscode-icon-theme',       # vscode-icon
            'ms-vscode.cpptools-themes',        # C/C++ Themes
            'TalhaBalaj.actual-font-changer',   # Font Changer
            'anthonyattard.zoomer',             # Zoomer
            'IBM.output-colorizer',             # Output Colorizer
        ]
        
        print('Installing VSCode extensions...')
        for ext in extensions:
            try:
                result = subprocess.run(
                    ['code', '--install-extension', ext, '--force'],
                    capture_output=True,
                    text=True
                )
                if result.returncode == 0:
                    print(f'✓ Installed {ext}')
                else:
                    print(f'✗ Failed to install {ext}: {result.stderr.strip()}')
            except FileNotFoundError:
                print('✗ VSCode command line tool not found. Please install VSCode and add it to PATH')
                return
            except Exception as e:
                print(f'✗ Error installing {ext}: {e}')
    
    def get_settings(self):
        """获取完整的 VSCode 配置"""
        settings = {}
        
        basic_settings = {
            "chat.disableAIFeatures": True,
            "editor.cursorBlinking": "smooth",
            "editor.cursorSmoothCaretAnimation": "on",
            "editor.lineNumbers": "on",
            "editor.renderWhitespace": "all",
            "editor.smoothScrolling": True,
            "files.autoSave": "afterDelay",
            "git.autofetch": True,
            "terminal.integrated.smoothScrolling": True,
            "workbench.list.smoothScrolling": True,
            "workbench.editor.wrapTabs": True,
        }
        settings.update(basic_settings)
        
        vim_settings = {
            "vim.autoindent": True,
            "vim.easymotion": True,
            "vim.foldfix": True,
            "vim.hlsearch": True,
            "vim.highlightedyank.enable": True,
            "vim.highlightedyank.duration": 300,
            "vim.leader": "<Space>",
            "vim.smartcase": True,
            "vim.searchHighlightColor": "#5F00AF",
            "vim.surround": True,
            "vim.useCtrlKeys": True,
            "vim.useSystemClipboard": True,
            "vim.normalModeKeyBindingsNonRecursive": [
                {"before": ["H"], "after": ["g", "T"]},
                {"before": ["L"], "after": ["g", "t"]},
                {"before": ["M"], "commands": ["editor.toggleFold"]},
                {"before": ["<C-h>"], "commands": ["workbench.files.action.focusFilesExplorer"]},
                {"before": ["<C-l>"], "commands": ["terminal.focus"]},
                {"before": ["<C-y>"], "commands": ["editor.action.formatDocument"]},
                {"before": ["<C-e>"], "commands": ["workbench.action.showCommands"]},
                {"before": ["<C-f>"], "commands": ["workbench.action.showCommands"]},
                {"before": ["<C-b>"], "commands": ["workbench.action.showCommands"]},
            ],
            "vim.insertModeKeyBindingsNonRecursive": [
                {"before": ["j", "k"], "after": ["<Esc>"]},
            ],
        }
        settings.update(vim_settings)
        
        # C/C++ 插件配置
        cpp_settings = {
            "C_Cpp.intelliSenseEngine": "disabled",
        }
        settings.update(cpp_settings)
        
        # CMake Tools 配置
        # 注意：CMAKE_TOOLCHAIN_FILE 路径需要根据实际环境调整
        cmake_settings = {
            "cmake.pinnedCommands": [],
            "cmake.configureSettings": {
                # "CMAKE_TOOLCHAIN_FILE": "/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake",
            },
            "cmake.debugConfig": {
                "args": []
            }
        }
        settings.update(cmake_settings)
        
        # vscode-icon 配置 (从笔记第133-137行)
        icon_settings = {
            "workbench.iconTheme": "vscode-icons",
        }
        settings.update(icon_settings)
        
        return settings
    
    def write_settings(self, settings=None):
        """写入配置文件"""
        if settings is None:
            settings = self.get_settings()
        
        # 确保目录存在
        self.settings_path.parent.mkdir(parents=True, exist_ok=True)
        
        # 如果文件已存在，合并配置
        if self.settings_path.exists():
            try:
                with open(self.settings_path, 'r', encoding='utf-8') as f:
                    existing_settings = json.load(f)
                # 合并配置，新配置优先
                existing_settings.update(settings)
                settings = existing_settings
                print('✓ Merged with existing settings')
            except (json.JSONDecodeError, IOError) as e:
                print(f'⚠ Could not read existing settings, creating new file: {e}')
        
        # 写入配置
        with open(self.settings_path, 'w', encoding='utf-8') as f:
            json.dump(settings, f, indent=4, ensure_ascii=False)
        
        print(f'✓ Settings written to {self.settings_path}')
    
    def run_full_setup(self):
        """执行完整的配置流程"""
        print(f'VSCode Configuration Script')
        print(f'Platform: {self.os_type}')
        print(f'Home directory: {self.home_dir}')
        print('-' * 50)
        
        # 1. macOS 特定设置
        print('\n1. Platform-specific settings:')
        self.setup_macos_continuous_input()
        
        # 2. 安装插件
        print('\n2. Installing extensions:')
        self.install_extensions()
        
        # 3. 写入配置
        print('\n3. Writing settings:')
        self.write_settings()
        
        print('\n' + '-' * 50)
        print('✓ VSCode configuration completed!')
        print('Please restart VSCode for all changes to take effect.')


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='VSCode Configuration Script')
    parser.add_argument('--extensions-only', '-e', action='store_true', help='Only install extensions')
    parser.add_argument('--settings-only', '-s', action='store_true', help='Only write settings')
    parser.add_argument('--macos-only', '-m', action='store_true', help='Only run macOS-specific settings')
    
    args = parser.parse_args()
    
    configurator = VSCodeConfigurator()
    
    if args.extensions_only:
        configurator.install_extensions()
    elif args.settings_only:
        configurator.write_settings()
    elif args.macos_only:
        configurator.setup_macos_continuous_input()
    else:
        configurator.run_full_setup()


if __name__ == '__main__':
    main()