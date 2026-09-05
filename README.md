# Wine 游戏库

轻量 Qt 6 Wine 游戏启动器，直接使用系统 Wine，不依赖 Proton、UMU、
Bottles 或 Lutris。

- 从 `~/galgame` 添加 Windows 游戏
- 每个游戏使用独立 `WINEPREFIX`
- 默认使用 bubblewrap 隔离其他 home 文件
- 自动提取 EXE 图标并配置常见中日文字体
- 提供 Wine 设置、Winetricks、Prefix 和轮转日志入口

依赖：Qt 6 Widgets、Wine、Winetricks、bubblewrap、icoutils、CMake 和 Ninja。

## 构建与安装

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix "$HOME/.local"
```

从应用菜单启动“Wine 游戏库”，或运行 `~/.local/bin/system-wine-launcher`。

## 卸载

```sh
rm "$HOME/.local/bin/system-wine-launcher"
rm "$HOME/.local/share/applications/system-wine-launcher.desktop"
rm "$HOME/.local/share/icons/hicolor/"{64x64,128x128,256x256,512x512}"/apps/wine-game-library.png"
```

游戏、Prefix 和日志不会随程序卸载自动删除。
