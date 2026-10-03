[English](AGENTS.md)

# 工作规则

这是独立作品集。先检查 Git 状态并保留已有修改。
修改前阅读[开发说明](docs/development.zh_CN.md)、应用手册和[来源记录](docs/upstream.zh_CN.md)。
不导入上游演示界面。

应用在 `apps/`，共享驱动在 `components/bsp/`，应用选择在 `main/CMakeLists.txt`。
硬件事实依据 BSP 头文件和固定上游版本。使用 ESP-IDF 5.5.3、ESP32-C3、8 MB Flash，无 PSRAM。
其他任务访问 LVGL 加锁，按钮回调不阻塞，删除对象前停止界面使用者。

迭代运行 `./tools/validate.sh --static`，交付前运行 `./tools/validate.sh`。
分别报告 Build、Host tests、Device tests 和 Unverified。
刷写或发布前取得授权。不得强制提交私人数据。
私人头像及其固件保持本地，除非单独授权。中英文文档同步。
