[English](development.md)

# 开发说明

验证命令第二个参数选择 `tuner`、`personal-card` 或 `all`。
CMake 默认选择 `tuner`。每次固件检查创建新的临时构建目录和 sdkconfig，
使用 `sdkconfig.defaults` 和 `config/<application>.defaults`。
本地 sdkconfig 不会悄悄改变交付固件。

应用代码在 `apps/`，`main/CMakeLists.txt` 每次只选择一个应用。
共享驱动在 `components/bsp/`，不包含上游测试菜单。

增量开发使用独立的构建与配置目录：

```bash
idf.py -B build/tuner-dev -D SDKCONFIG=build/tuner-dev/sdkconfig -D PASSPORT_APP=tuner build
idf.py -B build/card-dev -D SDKCONFIG=build/card-dev/sdkconfig -D PASSPORT_APP=personal-card build
```

交付使用验证入口，检查镜像偏移、分区边界与 MD5、合并字节和 ELF 身份。
默认分区为 `0x9000` 的 NVS、`0xF000` 的 PHY，以及 `0x10000` 起占满剩余
8 MB 的 factory 应用；没有 OTA 分区。

保留已有修改，其他任务访问 LVGL 时加锁，按钮回调不阻塞，音高与节拍逻辑运行主机测试。
不提交秘密、二维码标识和未脱敏日志。保持中英文文档一致，
分别报告 Build、Host tests、Device tests 和 Unverified。刷写须明确授权。

驱动更新时，对照固定的[上游版本](upstream.zh_CN.md)审查差异，
同时更新驱动和校验值，再验证两个应用。没有自动合并上游。
