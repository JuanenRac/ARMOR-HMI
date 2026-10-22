# Building the firmware on GitHub Actions (manual)

The workflow `.github/workflows/firmware-build.yml` compiles the universal image of one node repository in the official ESP-IDF container (`espressif/idf:v5.5.5` by default, the
same container `tools/build_node.sh` uses) and uploads it as an artifact. It is for people who have no ESP-IDF toolchain at hand; the usual way to build stays `tools/build_node.sh`.

**It consumes GitHub Actions minutes.** A build takes many minutes of a hosted runner, so the workflow never starts on its own (no push, no pull request): only someone with write access
starts it, from the Actions tab (*Firmware build (manual)* > *Run workflow*) or with `gh workflow run firmware-build.yml -f project=ARMOR-HMI`.

## Inputs

| Input | Meaning |
|---|---|
| `project` | the node repository to build: ARMOR-HMI, ARMOR-ALARM, ARMOR-ELECTRICAL, ARMOR-RADAR or ARMOR-SOLAR |
| `board` | the board (for example `lcd7box`, `s3-wifi`, `s3-eth`); empty means the default board of that project |
| `ref` | branch, tag or commit of that repository (default `main`) |
| `idf_image` | the ESP-IDF container, `espressif/idf:vX.Y[.Z]` (default `espressif/idf:v5.5.5`) |

## What it does and checks

1. Checks out the chosen repository and refuses to go on if a `secrets/generic.conf` is in it.
2. Runs `tools/build_node.sh generic [board]`: the **public** image, built without any fleet secret.
3. Checks that the image carries the version of `armor.project.json` (the version has to be raised before building), and that the build configuration has no fleet secret.
4. Uploads `dist/generic-<board>.bin` and its SHA-256 as an artifact kept for 14 days, and writes the size and the hash in the run summary.

It uses no repository secret and **does not sign** anything: the signature of an image that is published stays a local step. It does not publish a release.
