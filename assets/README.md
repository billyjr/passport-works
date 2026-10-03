[简体中文](README.zh_CN.md)

# Assets and privacy

`images/tuner/` holds generated UI references, not measured hardware screenshots.
`images/personal-card/` holds a code-generated neutral maker template and its
LVGL RGB565 C resource. Text is baked into the artwork.

`private/billy/` is local-only and Git-ignored. It retains the original avatar,
card source, converted card and generation prompt.
Redistribution permission for these identifiable personal assets has not been
established; they must not enter commits or public firmware artifacts.
Local personal-card builds use them when present.

Keep portraits, credentials, QR secrets, EXIF location and raw logs out of Git.
The template contains no personal portrait or contact details.
Application design references and the neutral template may be redistributed
with the MIT-licensed source; this excludes private portraits and third-party assets.
