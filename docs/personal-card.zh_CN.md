[English](personal-card.md)

# 个人名片

启动直接显示名片及电量。Up/Down 调亮度，OK 熄屏，
任意按钮恢复调暗或熄灭的屏幕。60 秒无操作后自动调暗。

![公开名片模板](../assets/images/personal-card/card_template.png)

干净下载的仓库使用 240 x 320 RGB565 创客模板。
本地有 `assets/private/billy/billy_card.c` 时使用原始头像。
两者都导出 LVGL 描述符 `billy_card`。
不得强制提交私人目录。头像固件也包含头像像素，发布须另行授权。

使用 `./tools/validate.sh --firmware personal-card` 构建并确认素材选择提示。
真实显示、电量、按钮和调暗需要设备测试。
