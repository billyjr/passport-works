[English](upstream.md)

# 上游来源

来源：[FoloToy/ai-passport](https://github.com/FoloToy/ai-passport)。
基础版本：`0b9e4c81ee4421c0bac39ca3561d65a8285acd4a`。
[LICENSE](../LICENSE) 保留原始 MIT 声明。

板级支持、分区表、依赖锁定文件和驱动测试桩保持原样，
[upstream.json](upstream.json) 记录 SHA-256。
验证、归档工具和相应测试只将固件工程名从 `FoloToy-AI-Passport` 改为 `Passport-Works`。
构建与检查脚本适配本作品集，不包含上游演示、发布工作流、技能和整套文档。

应用来自所有者工作目录，不继承上游 Git 历史。个人素材放在 Git 忽略的 `assets/private/`。
ESP-IDF 根据清单和锁定文件解析依赖，生成被忽略的 `managed_components/`，
各依赖原有许可证仍适用。
