# reading-goal

给自建的 Kavita 加上类似 Apple Books 的「每日阅读目标」和「连续打卡天数」，并支持从网页直接上传书籍。

程序定时从 Kavita 的统计接口读取每天的阅读时长，算出今天读了几分钟、有没有达标、连续了几天，并提供一个适合手机查看的网页。

## 目录结构

```
src/
  config.hpp          从环境变量读取配置
  kavita_client.*     Kavita API：登录、获取书库列表、按年获取每日阅读秒数
  streak.*            纯逻辑：日期加减、连续天数、最长连续、最近 N 天
  upload.*            上传：文件名清理、格式检查、保存到书库目录
  page.hpp            网页前端（内嵌在程序里的 HTML）
  main.cpp            HTTP 服务：/ 页面，/api/status 数据，/api/upload 上传，/healthz 健康检查
tests/                streak 和 upload 逻辑的测试
deploy/               systemd 服务文件和配置示例
```

## 编译与测试

```bash
sudo apt install -y nlohmann-json3-dev
cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

## 运行

```bash
export KAVITA_API_KEY='你的 key'
./build/reading_goal --once   # 在终端打印一次今天的结果
./build/reading_goal          # 启动网页服务 http://127.0.0.1:5100
```

所有配置项见 `deploy/reading-goal.env.example`，其中最常改的是 `GOAL_MINUTES`（每日目标分钟数，默认 10）。

## 部署（WSL + systemd + Tailscale）

```bash
# 1. 配置文件（只有自己可读）
mkdir -p ~/.config
cp deploy/reading-goal.env.example ~/.config/reading-goal.env
chmod 600 ~/.config/reading-goal.env
nano ~/.config/reading-goal.env           # 填入 KAVITA_API_KEY

# 2. 安装 systemd 服务（如果路径或用户名不同，先改 deploy/reading-goal.service）
sudo cp deploy/reading-goal.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now reading-goal
systemctl status reading-goal             # 应显示 active (running)

# 3. 通过 Tailscale 发布到自己的设备（Kavita 已占用 443，这里用 8443）
tailscale serve --bg --https=8443 http://127.0.0.1:5100
tailscale serve status
```

手机打开 `https://<机器名>.<tailnet>.ts.net:8443`，在 Safari 中「添加到主屏幕」即可。

更新代码后：`cmake --build build && sudo systemctl restart reading-goal`

## 网页上传书籍

在 `~/.config/reading-goal.env` 里加上书库路径，重启服务后，网页底部会出现「添加书籍」卡片：

```bash
LIBRARY_DIR=/home/time/books     # Kavita 书库在本机上的路径（Docker 的话填 -v 左边那个）
UPLOAD_LIBRARY_ID=1              # 可选：只扫描这个书库，不写就扫描全部
```

- 每本书会放进自己的文件夹：`LIBRARY_DIR/<书名>/<书名>.epub`
- 支持 epub、pdf、cbz、cbr、cb7、cbt、zip、rar、7z；同名文件不会被覆盖
- 上传成功后程序会调用 Kavita 的 `POST /api/Library/scan` 触发扫描，**需要管理员账号的 API Key**
- 服务以 `User=` 指定的用户运行，这个用户必须对 `LIBRARY_DIR` 有写权限

## 说明

- Kavita 把一次阅读记为一个会话，停止翻页约 10 分钟后才结束并计入统计，所以网页数据会比实际晚 10–15 分钟。
- 统计接口必须带上书库 ID，否则返回全 0；程序会自动通过 `/api/library/libraries` 获取。
- 同时拉取今年和去年的数据，跨年时连续天数不会中断。
