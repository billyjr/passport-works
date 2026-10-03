[简体中文](personal-card.zh_CN.md)

# Personal card

The card opens directly with battery status. Up/Down adjust brightness, OK
blanks the screen, and any button restores a dimmed or blanked screen.
The backlight dims after 60 seconds without input.

![Public card template](../assets/images/personal-card/card_template.png)

A clean clone uses the 240 x 320 RGB565 maker template.
When `assets/private/billy/billy_card.c` exists locally, it uses the original
portrait instead. Both export the LVGL descriptor `billy_card`.
The private directory must never be force-added. Local portrait firmware contains
the portrait pixels too; publishing it needs separate approval.

Build with `./tools/validate.sh --firmware personal-card` and check the artwork
selection message. Actual rendering, battery, buttons and dimming need device testing.
