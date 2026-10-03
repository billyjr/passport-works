[简体中文](README.zh_CN.md)

# Passport Works

A personal collection of offline firmware for the FoloToy AI Passport.
This independent repository contains the applications and necessary board support.
It is maintained independently of FoloToy.

| Work | Features |
| --- | --- |
| [Tuner and metronome](docs/tuner.md) | Guitar, bass and violin tuning; a 40–240 BPM metronome. |
| [Personal card](docs/personal-card.md) | Portrait card, battery indicator, brightness controls and dimming. |

![Tuner visual reference](assets/images/tuner/tuner-ui-concept-v2.png)

This image is a design reference, not a tested-device photograph.

## Build

Requires ESP-IDF **5.5.3**, Python 3 and a host C compiler.
Target: ESP32-C3, 8 MB Flash, no PSRAM. Activate ESP-IDF first.

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware tuner
./tools/validate.sh --firmware personal-card
./tools/validate.sh
```

The complete gate checks both applications. Verified merged images are under
`build/<application>/Passport-Works-full.bin`; matching debug bundles are under
`build/<application>/firmware/<sha256>/`. Generated output is Git-ignored.
A merged image is written at `0x0` and can reset stored settings.
Flashing requires device-owner approval; a build does not prove device behavior.

See [development](docs/development.md), [upstream provenance](docs/upstream.md)
and [asset/privacy policy](assets/README.md).
CI checks both apps without automatically publishing releases or uploading artifacts.

## License

The FoloToy board support and adapted tools retain the [upstream MIT license](LICENSE).
Original application source uses the same license. Personal portrait assets are
excluded from Git and are not covered by permission to redistribute this repository.
