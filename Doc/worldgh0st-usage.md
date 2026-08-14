# worldgh0st 使用说明

本文档说明 **worldgh0st** 编译成功后的运行与联调方法。编译与环境配置见 [worldgh0st.md](./worldgh0st.md)。

## 目录与角色

| 组件 | 路径 | 作用 |
|------|------|------|
| **Client（控制端）** | `worldgh0st/Bin/V-Eye.exe` | 监听上线、管理主机、生成安装包 |
| **Server 模板（Debug）** | `worldgh0st/Bin/Dat/Install.exe` | 被控端程序（Debug 编译产物） |
| **Server 模板（Release）** | `worldgh0st/Bin/Dat/Install.dat` | 供 Client「生成」页使用的模板 |
| **Client 配置** | `worldgh0st/Bin/V-Eye.ini` | 与 `V-Eye.exe` 同目录 |
| **被控端配置** | `C:\V-Eye\V-Eye.ini` | 安装后写入，记录控制端 IP |

默认通信：**TCP 端口 2019**（Client 监听，Server 主动连接）。

## 推荐使用顺序

```
1. 准备 Server 模板 (Install.dat 或 Install.exe)
2. 以管理员身份启动 Client (V-Eye.exe) 并开始监听
3. 在 Client 里「生成」安装包，或手动部署 Server
4. 目标机运行安装包 → 回连 Client
5. 在 Client 里对上线主机进行远程管理
```

## 控制端 Client（V-Eye.exe）

### 启动

- **以管理员身份**运行 `worldgh0st/Bin/V-Eye.exe`（工程要求 UAC 提升）。
- 启动后读取 `V-Eye.ini` 中 `[Settings] ListenPort`，**默认为 2019**，并在本机监听该端口。

### 配置文件 V-Eye.ini

与 `V-Eye.exe` 同目录，示例：

```ini
[Settings]
ListenPort=2019

[Setting]
CompanyName=V-Eye Terminal Security
```

| 项 | 含义 |
|----|------|
| `ListenPort` | Client 监听端口，默认 2019 |
| `CompanyName` | 左侧分组树顶层显示名称 |

### 界面结构（Ribbon）

| 页签 | 主要功能 |
|------|----------|
| **Home** | 用户视图、生成安装包、设置 |
| **Tools** | 文件、进程、服务、系统、注册表、键盘记录、屏幕监视等 |
| **Build** | 生成被控安装包 |

左侧为 **分组树**；主区为 **已上线主机列表** 与统计信息。可最小化到 **系统托盘**，双击托盘图标恢复窗口。

### 对上线主机的操作

在主机列表中选中一台后，通过 Ribbon **Tools** 或右键菜单打开：文件管理、屏幕监视、键盘记录、注册表、服务管理、系统信息/进程等。

## 被控端 Server（Install.exe / 生成的安装包）

### 工作方式

- 从 `C:\V-Eye\V-Eye.ini` 读取 `[Login] IPAddr`，主动连接 `IPAddr:2019`。
- 连接失败会随机 sleep 后重试（最长约 120 秒）。
- 同一控制地址仅允许一个实例（互斥）。

### 安装流程（运行安装包时）

安装包文件名格式（由 Client 生成）：

```text
Install_<公网IP>_v1.exe   → 带安装界面
Install_<公网IP>_v2.exe   → 静默安装
```

Server 从 **文件名解析 IP** 并写入配置，然后：

1. （v1）弹出安装对话框
2. 创建目录 `C:\V-Eye\`
3. 写入 `C:\V-Eye\V-Eye.ini`：`[Login] IPAddr=<控制端IP>`
4. 复制自身为 `C:\V-Eye\V-Eye.exe`
5. 写注册表开机自启（`Run` → `LiteX`）
6. 持续回连 Client

### 手动联调（不通过「生成」页）

1. 在被控机创建 `C:\V-Eye\V-Eye.ini`，`IPAddr` 填控制端 IP。
2. 运行 `Bin\Dat\Install.exe`。
3. 确保 Client 已启动且 **2019 端口可达**（防火墙放行）。

## 用 Client 生成安装包（Build 页）

### 前提

- 需有 `Bin\Dat\Install.dat`（Release Server 产物；Debug 可临时将 `Install.exe` 复制为 `Install.dat`）。

### 操作步骤

1. 以管理员身份启动 `V-Eye.exe`
2. 打开 **Build** 页，选择 v1（带界面）或 v2（静默）
3. 点击 **生成**（需外网获取公网 IP）
4. 得到 `Install_<公网IP>_v1.exe` 或 `_v2.exe`

## 网络与防火墙

| 方向 | 说明 |
|------|------|
| Client | 监听 `0.0.0.0:2019` |
| Server | 出站连接 `控制端IP:2019` |

Client 机器需 **入站**放行 2019；跨公网需端口映射或公网 IP。

## 界面主题（XTP）

当前使用 **XTP 15.2.1**，`Client/res/Client.rc2` 嵌入 Office 2007 样式：

```text
#include <Styles/Office2007/Office2007.rc>
```

默认主题为 **Office 2010 Blue**（映射到内嵌 `OFFICE2007BLUE.INI`）。可选将样式 DLL 放到 `Bin\Styles\`；无 DLL 也可运行。

若启动报 `Error Locating Office2007 Resources`，请 **重新编译 Client**。

## 典型联调（局域网）

1. 机器 A 运行 `V-Eye.exe`
2. 机器 B 配置 `C:\V-Eye\V-Eye.ini` 后运行 `Install.exe`
3. 机器 A 列表出现主机后即可操作

## Debug 与 Release

| 配置 | Server 输出 | 用途 |
|------|-------------|------|
| Debug | `Bin/Dat/Install.exe` | 直接测试被控端 |
| Release | `Bin/Dat/Install.dat` | Client 正式「生成」模板 |

## 运行时依赖

| 组件 | 说明 |
|------|------|
| Server Debug | 共享 MFC（`/MDd`），需 VS2013 MFC 运行库 |
| Client | 静态链接 XTP；Office 2007 主题资源编入 exe |
| 两者 | 均需 **管理员权限** |
