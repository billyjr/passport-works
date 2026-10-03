[简体中文](development.zh_CN.md)

# Development

Select `tuner`, `personal-card`, or `all` as the validation command's second argument.
CMake defaults to `tuner`. Each firmware gate creates a fresh temporary build and
sdkconfig using `sdkconfig.defaults` plus `config/<application>.defaults`.
A local sdkconfig cannot silently change the delivered firmware.

Application code lives in `apps/`; `main/CMakeLists.txt` selects exactly one app.
Shared drivers live in `components/bsp/`. No upstream test menu is included.

For incremental development, keep separate build/config directories:

```bash
idf.py -B build/tuner-dev -D SDKCONFIG=build/tuner-dev/sdkconfig -D PASSPORT_APP=tuner build
idf.py -B build/card-dev -D SDKCONFIG=build/card-dev/sdkconfig -D PASSPORT_APP=personal-card build
```

Use the gate for deliverables. It checks image offsets, partition bounds and MD5,
merged bytes, and matching ELF identity. The default partitions are NVS at
`0x9000`, PHY at `0xF000`, and a factory app at `0x10000` spanning the remaining
8 MB; there are no OTA slots.

Preserve edits, lock LVGL access from other tasks, keep button callbacks
non-blocking, and host-test isolated pitch/timing logic.
Never commit secrets, QR identifiers or unsanitized logs.
Keep English/Chinese documents aligned. Report Build, Host tests, Device tests
and Unverified separately. Device flashing requires explicit authorization.

Driver updates are deliberate: review differences against the pinned
[upstream](upstream.md), update driver hashes with the files, then validate both apps.
No automatic upstream merging is configured.
