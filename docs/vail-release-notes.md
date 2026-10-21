# QK4 0.8.1-beta.1.2-vail (hardware testing)

This unofficial fork adds [Vail adapter](https://vailadapter.com/) support to QK4. Vail is an [open-source hardware and firmware project](https://github.com/Vail-CW/vail-adapter); its onboard keyer and buzzer provide immediate local sidetone while QK4 forwards the timed elements to the K4.

This release refreshes the fork onto upstream development at [2ad70d6](https://github.com/mikeg-dal/QK4/commit/2ad70d69338ed44358567d71cdd82fd5de290f49), incorporating 35 upstream commits since fork 0.8.1-beta.1.1. See the [upstream changes](https://github.com/mikeg-dal/QK4/compare/5191c8b1932148511381bd8574e7365e279463e7...2ad70d69338ed44358567d71cdd82fd5de290f49).

Changes since fork 0.8.1-beta.1.1:

- STO and RCL open a frequency-memory list in QK4 with recall, storage, search, and channel naming. MEM and GEN band pages recall stored channels. These controls use the K4's newer memory commands.
- Click MAIN or SUB beside a receiver-volume slider to mute that receiver while keeping its saved level. Receiver mute operates independently of Vail's buzzer, computer sidetone, and the K4 CW monitor-mute control.
- The graphic equalizer adopts the K4's layout. SSNR noise-floor RESET is available with R40 firmware. Band changes refresh TX power, including transverter power units, and clearing antenna names restores the radio's defaults.
- Waterfall improvements cover smooth scrolling, retained history during resizing, scaled displays, and stronger narrow-signal rendering. Mini-pan calibration now accounts for its own bin width. SPEED uses levels 1-10; older SPEED preferences reset to Fill (level 3).
- Scheduled ThreadSanitizer checks suppress a documented Qt TLS lock-order warning, report useful stacks, and run twice weekly to retain their Qt build cache.
- The CW Keyer screen and both Vail sidetone paths carry forward. Regression coverage now checks K4 monitor-mute ordering and restoration on key-up and graceful disconnect with both buzzer and computer-audio settings.

In **CW Keyer**, **Vail buzzer** remains the default sidetone source. **Computer audio** enables QK4's sidetone volume control, with MIDI and audio-device latency. Selecting computer audio does not mute the adapter: disconnect Vail in QK4 and use its manual buzzer-mute gesture before reconnecting. No adapter firmware update is required.

**Mute the K4's CW monitor while keying** is available for both Vail and HaliKey. Enable it if the radio's delayed sidetone returns through receive audio. It leaves the local buzzer and computer volume independent, then restores the radio's observed monitor level after buffered sending finishes. An abrupt network or power loss can prevent restoration; check the radio's monitor level after reconnecting if it remains silent.

**Remote Keying → Key-Down Delay** keeps the K4's saved delay; it does not delay Vail's buzzer. **Escape** aborts sending; release the paddles before resuming. The [setup guide](https://github.com/rwjblue/QK4/blob/fork-v0.8.1-beta.1.2/docs/vail-fork.md) covers these controls and the manual buzzer-mute procedure.

Downloads:

- **macOS:** Apple Silicon, macOS 15 or newer. Install **QK4 Vail.app** at `/Applications/QK4 Vail.app` alongside the official `/Applications/QK4.app`. Its bundle identifier remains `com.rwjblue.qk4.vail`. The fork is ad-hoc signed without Apple notarization. Both apps share QK4 settings; close one before opening the other.
- **Windows:** Windows 11 x64 portable ZIP. Extract the complete `QK4-windows` folder and run `QK4.exe`. Dependencies and the Microsoft runtime are included. The app is unsigned.
- **Linux:** x86_64 Flatpak using the KDE 6.10 runtime. Its application ID remains shared with upstream QK4.
- **SHA256SUMS.txt** covers all three packages.

Publication is gated on the full Linux test suite normally and under ASAN/UBSAN, RealtimeSanitizer, focused native CW/Vail tests, and package verification on all three platforms. Earlier builds were physically tested with a Vail adapter and K4 through 28 WPM, including key-up/disconnect and both buzzer and computer sidetone paths. This release has automated validation only; fresh on-radio regression checks and physical Vail operation on Windows remain unverified. Previous release tags and assets remain fixed.
