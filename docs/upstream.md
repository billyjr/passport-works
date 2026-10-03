[简体中文](upstream.zh_CN.md)

# Upstream provenance

Source: [FoloToy/ai-passport](https://github.com/FoloToy/ai-passport).
Baseline: `0b9e4c81ee4421c0bac39ca3561d65a8285acd4a`.
The original MIT notice is retained in [LICENSE](../LICENSE).

Board support, partition table, dependency lock and driver stubs are unchanged;
[upstream.json](upstream.json) records SHA-256 hashes.
Verification/archive tools and their tests only rename the firmware project
from `FoloToy-AI-Passport` to `Passport-Works`.
Build/check scripts are adapted to this collection.
Upstream demos, release workflows, skills and the documentation tree are excluded.

Applications come from the owner's working tree, without upstream Git history.
Private artwork stays in Git-ignored `assets/private/`.
ESP-IDF resolves dependencies using the manifest and lock, generating ignored
`managed_components/`; original dependency licenses still apply.
