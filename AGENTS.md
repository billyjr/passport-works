[简体中文](AGENTS.zh_CN.md)

# Working rules

This is an independent works collection. Start with git status; preserve edits.
Read [development](docs/development.md), the app manual and
[provenance](docs/upstream.md) before changes. Do not import upstream demo UI.

Keep apps in `apps/`, shared drivers in `components/bsp/`, and app selection
in `main/CMakeLists.txt`. Hardware facts follow BSP headers and pinned upstream.
Use ESP-IDF 5.5.3, ESP32-C3, 8 MB Flash, no PSRAM.
Lock LVGL from other tasks, keep button callbacks non-blocking and stop UI users
before deleting objects.

Run `./tools/validate.sh --static` while iterating and `./tools/validate.sh`
before delivery. Report Build, Host tests, Device tests and Unverified separately.
Ask before flashing or publishing. Never force-add ignored private data.
Private portrait artwork and its firmware stay local unless separately authorized.
Keep English/Chinese documentation aligned.
