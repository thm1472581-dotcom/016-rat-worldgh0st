# worldgh0st 使用文档

## 概述

worldgh0st 是 MFC 远控实现，Visual Studio 2013 原始工程，默认工具集 **v120（VS2013）**，字符集 Unicode。包含 Client（控制端）+ Server（被控端）双端完整源码。

- 目录：`worldgh0st/`
- 主工程：`Client.sln`
- 推荐构建：Visual Studio 2013（v120）或 VS2022（需改工具集为 v143），Release | Win32

- **运行与联调**：见 [worldgh0st-usage.md](./worldgh0st-usage.md)

## 工程结构

| 项目 | 说明 | 输出 |
|------|------|------|
| **Server** | 被控端，插件内嵌 | Release → `Bin/Dat/Install.dat`；Debug → `Bin/Dat/Install.exe` |
| **Client** | 控制端 MFC SDI | `Bin/V-Eye.exe` |

Server 内置模块：FileManager、ScreenManager、KeyboardManager、RegManager、ServiceManager、SystemManager、ScreenSpy、UrlManager


## VS2013 编译 Server（重要）

你的环境是 **VS2013 + v120**。VS2013 默认 **不含 MBCS 版 MFC 静态库**，因此 MultiByte 工程会报：

```
error MSB8031: Building an MFC project for a non-Unicode character set is deprecated
```

### 推荐方案：安装 MBCS MFC 附加库（无需改源码）

1. 下载并安装官方组件（64MB）：
   - 页面：https://www.microsoft.com/en-us/download/details.aspx?id=40770
   - 文件名：`vc_mbcsmfc.exe`
2. 安装完成后 **重启 VS2013**
3. 确认 `Server.vcxproj` 字符集为 **MultiByte**（当前已设置）
4. 菜单：**生成 → 清理解决方案**，再 **全部重新生成 Server（Debug | Win32）**

安装此库后，之前 Unicode 迁移引入的 `_T()` / `_tcscpy` 等改动在 MultiByte 下同样兼容（会映射回 ANSI API）。
### 当前工程已验证的 Server 配置（VS2013 Debug 已通过）

| 项 | 值 |
|---|---|
| 平台工具集 | v120 |
| 字符集 | MultiByte |
| MFC 使用 | **在共享 DLL 中使用 MFC**（`/MDd`，非静态 MFC） |
| 运行时库 | Debug: `/MDd`；Release: `/MD` |
| zlib | 源码编入工程（`Public/zlib/*.c` + 1.2.8 内部头文件），勿链接旧 `zlib.lib`（其为 `/MD` 预编译，与静态 CRT 冲突） |

**常见链接错误 LNK2005**（`nafxcwd.lib` 与 `LIBCMTD.lib` 重复 `operator new`）：静态 MFC + `/MTd` 在 VS2013 下会触发。已改为共享 MFC + `/MDd` 解决。

**Debug 成功输出**：`worldgh0st/Bin/Dat/Install.exe`（约 540KB）。

**Release 输出**：`worldgh0st/Bin/Dat/Install.dat`（供 Client 生成安装包）。

### 备选方案：Unicode 字符集

若不安装 MBCS 库，需将字符集改为 Unicode，并修改大量 `char*` 与 `TCHAR` 混用代码（FileManager、SystemManager、InstallService 等），工作量较大。

## VS2022 编译步骤

### 1. 安装组件

在 Visual Studio Installer 中勾选：

- **使用 C++ 的桌面开发**
- **适用于最新 v143 生成工具的 C++ MFC**（x86 和 x64）
- **Windows 10/11 SDK**
- （可选）MSVC v120-v142 旧版工具集

### 2. 编译 Server（被控端）

1. 用 VS2022 打开 `worldgh0st/Client.sln`
2. 右键 **Server** → 设为启动项目（可选）
3. 选择 **Release | Win32**
4. 生成 Server

成功后得到 `worldgh0st/Bin/Dat/Install.dat`，供 Client 的 BuildDlg 生成安装包时使用。

### 3. 编译 Client（控制端）

Client 依赖商业库 **Codejock Xtreme ToolkitPro (XTP)**，需单独安装。

1. 安装 XTP 18.x（或兼容版本），记下安装目录，例如：
   `D:\Libs\Xtreme ToolkitPro v18.5.0`
2. 设置用户环境变量（系统属性 → 环境变量）：
   - 变量名：`XTPROOT`
   - 变量值：XTP 根目录（含 `Source` 和 `Lib` 子目录）
3. **重启 VS2022** 使环境变量生效
4. 确认 `$(XTPROOT)\Lib\vc143` 存在；若只有 `vc100` 等旧目录，需用 XTP 自带工具为 v143 编译库，或将 `Client.vcxproj` 中 `LibraryPath` 改为对应目录
5. 选择 **Release | Win32**，生成 Client

输出：`worldgh0st/Bin/V-Eye.exe`

## 已修复的编译问题（VS2022 升级）

| 问题 | 修复 |
|------|------|
| MSB8031：v120 + MultiByte MFC 已弃用 | 工具集改为 **v143**，字符集改为 **Unicode** |
| 缺少 `zlib.lib` | 将 zlib 1.2.8 源码放入 `Public/zlib/` 并编入 Server 工程 |
| Include 路径中文逗号 | `..\Public，Plugins` → `..\Public;Plugins` |
| 绝对路径图片 | 创建 `Server/res/home.bmp`、`Server/res/Server.ico` 占位资源 |
| Client XTP 硬编码 `G:\` 路径 | 改为环境变量 **`$(XTPROOT)`** |

## BuildDlg 生成逻辑

- 从 `Bin/Dat/Install.dat` 模板复制并修改 IP/端口，生成被控安装文件
- 支持带界面/静默两种模式
- 源码：`Client/BuildDlg.cpp`
- **必须先编译 Server Release**，否则缺少 `Install.dat` 模板

## 清理

运行 `worldgh0st/clean.bat` 清除中间文件。

## 常见问题

### MSB8020：找不到 v143 工具集

说明当前是用 **VS2013** 打开工程，但工程设置了 v143。工程已改回 **v120**；若你安装了 VS2022，可在项目属性 → 常规 → 平台工具集 中改为 **v143**。

### Server 仍报 MSB8031

确认 `Server.vcxproj` 中 `PlatformToolset` 为 `v143`，`CharacterSet` 为 `Unicode`，不要用 v120 工具集。

### 运行 V-Eye 报 Chart Appearance 断言 (XTPChartAppearance.cpp)

InfoDlg 使用 XTP Chart，`CXTPChartAppearance` 构造时需要内嵌 `CHART_APPEARANCE_NATURE` / `CHART_PALETTE_NATURE` HTML 资源。若 `Client.rc2` 定义了 `_XTP_NO_BINARY_RESOURCES`，须在同一文件末尾追加 Chart 的 HTML 资源行（见 `Client/res/XTPChartResources.rc`）。

### 运行 V-Eye 报 Office2007 Resources 错误

XTP 15.2 下 Client 通过 Client/res/Client.rc2 嵌入 Styles/Office2007/Office2007.rc。主题 INI 须使用 OFFICE2007BLUE.INI 等内嵌名称（见 Client/Include/XTPCompat15.h）。VS2013 资源编译若报 LANG_NEUTRAL 相关 RC2135，在 Client.rc2 中定义 _XTP_NO_BINARY_RESOURCES 后再 #include <XTToolkitPro.rc>。

### 找不到 XTToolkitPro.h

未安装 XTP 或未设置 `XTPROOT` 环境变量。Client 大量使用 `CXTPFrameWnd`、`CXTPListCtrl` 等控件，无法在不改源码的情况下去掉该依赖。

### 链接 XTP 库失败

检查 `$(XTPROOT)\Lib\vc143` 是否存在对应 `.lib` 文件；旧版 XTP 可能只有 vc100/v120 库，需重新编译 XTP 或调整 LibraryPath。

### Install.dat 不存在

先以 **Release | Win32** 编译 Server；Debug 配置输出为 `Install.exe` 而非 `.dat`。

## 完整度说明

在本合集 8 款工具中，worldgh0st **源码完整度最高**（约 85%）：Client + Server 双端齐全，主要缺原始预编译资源和 XTP 商业库。

