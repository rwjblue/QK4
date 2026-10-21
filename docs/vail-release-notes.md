# QK4 0.8.1-beta.1.1-vail (hardware testing)

This unofficial fork adds [Vail adapter](https://vailadapter.com/) support to QK4. Vail is an [open-source hardware and firmware project](https://github.com/Vail-CW/vail-adapter); its onboard keyer and buzzer provide immediate local sidetone while QK4 forwards the timed elements to the K4.

This release refreshes the fork onto upstream development at [5191c8b](https://github.com/mikeg-dal/QK4/commit/5191c8b1932148511381bd8574e7365e279463e7), following upstream's 0.8.1 beta version. It includes the previously unreleased October 31 and November 2 integrations. See the [upstream changes since the last published fork release](https://github.com/mikeg-dal/QK4/compare/78f0aac97319528d920a642b70d84b7ce5cd916f...5191c8b1932148511381bd8574e7365e279463e7).

Changes since fork 0.8.0-beta.1.8:

- Connection teardown, reconnect state, and shutdown follow upstream's ordered lifecycle. The Vail integration preserves radio-monitor restoration on an intentional disconnect and retries restoration if its timer fires before the queued keying has finished.
- The CW Keyer page keeps background MIDI-port rescans, updated connection-status styling, and macOS MIDI hotplug detection. Sidetone rendering incorporates upstream's allocation-free audio path.
- Upstream fixes improve spectrum and waterfall calibration, averaging, scaling, mini-pan alignment, and GPU resource handling. Other changes cover first reports after connecting, popup behavior, message-bank display, menu entry, and transverter settings.
- The side-panel version appears on one readable line, and MSG: I/II follows the radio's actual message bank. Buffer byte counts and waterfall offsets use the appropriate size type.
- Packaging keeps the separate Mac application identity, native MIDI lifecycle coverage, and dependency-cache fixes while incorporating upstream's current build changes.

In **CW Keyer**, **Vail buzzer** remains the default sidetone source. **Computer audio** enables QK4's sidetone volume control, with MIDI and audio-device latency. Selecting computer audio does not mute the adapter: disconnect Vail in QK4 and use its manual buzzer-mute gesture before reconnecting. No adapter firmware update is required.

**Mute the K4's CW monitor while keying** is available for both Vail and HaliKey. Enable it if the radio's delayed sidetone returns through receive audio. It leaves the local buzzer and computer volume independent, then restores the radio's observed monitor level after buffered sending finishes. An abrupt network or power loss can prevent restoration; check the radio's monitor level after reconnecting if it remains silent.

**Remote Keying → Key-Down Delay** keeps the K4's saved delay; it does not delay Vail's buzzer. **Escape** aborts sending; release the paddles before resuming. The [setup guide](https://github.com/rwjblue/QK4/blob/fork-v0.8.1-beta.1.1/docs/vail-fork.md) covers these controls and the manual buzzer-mute procedure.

Downloads:

- **macOS:** Apple Silicon, macOS 15 or newer. Install **QK4 Vail.app** at `/Applications/QK4 Vail.app` alongside the official `/Applications/QK4.app`. Its bundle identifier remains `com.rwjblue.qk4.vail`. The fork is ad-hoc signed without Apple notarization. Both apps share QK4 settings; close one before opening the other.
- **Windows:** Windows 11 x64 portable ZIP. Extract the complete `QK4-windows` folder and run `QK4.exe`. Dependencies and the Microsoft runtime are included. The app is unsigned.
- **Linux:** x86_64 Flatpak using the KDE 6.10 runtime. Its application ID remains shared with upstream QK4.
- **SHA256SUMS.txt** covers all three packages.

Publication is gated on the full Linux test suite normally and under ASAN/UBSAN, RealtimeSanitizer, focused native CW/Vail tests, and package verification on all three platforms. Earlier builds were physically tested with a Vail adapter and K4 through 28 WPM, including key-up/disconnect and both buzzer and computer sidetone paths. This release has automated validation only; fresh on-radio regression checks and physical Vail operation on Windows remain unverified. Previous release tags and assets remain fixed.
