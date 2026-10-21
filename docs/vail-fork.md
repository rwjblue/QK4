# QK4 Vail fork

This is Robert's unofficial QK4 fork for using Vail adapters as local CW keyers.
The adapter generates the elements; QK4 forwards the timed keying to the K4.
Sidetone comes from the Vail buzzer by default, with computer audio available as
an option. QK4 does not run a second software keyer for this device. HaliKey
remains available as a separate device type.

Published releases `fork-v0.8.0-beta.1.1` and `fork-v0.8.0-beta.1.2` are based on
upstream `v0.8.0-beta.1` (`cc3fb6d1`). They remain frozen at their original commits.
The Windows release, `fork-v0.8.0-beta.1.3`, uses upstream `development`
(`1d7de138`) with the Vail patch and fork release tooling. The refresh release,
`fork-v0.8.0-beta.1.4`, keeps that same baseline: upstream `development` was
still at `1d7de138` when checked on October 26, 2026, so no runtime rebase was
needed. The .1.7 update moves to upstream `development` at `72025dfe` on
October 28, 2026, adapting Vail to its new keyer/controller and sidetone output.
The maintained branches use a two-layer stack on that baseline; earlier release
tags and assets remain fixed. The unreleased .1.5 and .1.6 attempts exposed
wall-clock assumptions in upstream threaded-keyer tests on hosted macOS. The
patch allows an in-flight element's matching key-up in the inhibit test while
continuing to reject new key-downs. The .1.7 packaging checks retain deterministic
keyer coverage on macOS and Windows and full threaded coverage on Linux; runtime
keyer behavior is unchanged by these test and CI corrections. The .1.8 update
moves to upstream `development` at `78f0aac9`, adds shared K4 CW monitor muting
for Vail and HaliKey, and packages the Mac fork as a distinct `QK4 Vail.app`.
The upstream refresh brings lock-free keying egress, real-time scheduling,
asynchronous MIDI-port scans, and the pinned Qt and TLS bundling checks.

Releases are **prereleases for testing**. Earlier builds were tested with a
physical Vail and K4 through 28 WPM, including key-up/disconnect and both buzzer
and computer sidetone. The .1.7 integration passed the local build and all 58
CTest suites. The .1.8 monitor-mute change and upstream integration need fresh
on-radio regression checks. Windows packages have
automated build, test, and launch checks; a physical adapter on Windows remains
untested.

## Downloads and installation

Download the Vail prerelease from
[rwjblue/QK4 releases](https://github.com/rwjblue/QK4/releases). Releases include
SHA-256 checksums and these assets:

| Asset | Supported system |
| --- | --- |
| `QK4-<version>-macos.dmg` | Apple Silicon, macOS 15 or newer |
| `QK4-<version>-windows-x86_64.zip` | Windows 11, x64 |
| `QK4-<version>-linux-x86_64.flatpak` | x86_64 Linux with Flatpak and KDE runtime 6.10 |

Intel Macs, Windows ARM64, and Linux ARM are outside this fork's release
matrix. Upstream still provides its own broader platform builds.

From .1.8, the macOS DMG contains `QK4 Vail.app`, with the bundle identifier
`com.rwjblue.qk4.vail`. Copy it to `/Applications/QK4 Vail.app`; keep the official
application at `/Applications/QK4.app`. The fork's application name and identity
are separate so installing it does not replace the official app.

The fork is ad-hoc signed, without Developer ID signing or Apple notarization.
If macOS blocks first launch, attempt to open it, then use System Settings ->
Privacy & Security -> Open Anyway. See
[Apple's first-launch instructions](https://support.apple.com/en-us/102445).
No Apple signing credentials are required to build this fork.

On Windows, download the ZIP, choose **Extract All**, and run `QK4.exe` from
the extracted folder. Keep the bundled DLLs and plugin folders beside the
executable; running it from inside the ZIP will not work. No installer,
administrator access, or separate Visual C++ runtime installation is required.
The application is unsigned, so Windows may show a publisher or SmartScreen
prompt when first launching it.

On Linux, install the downloaded bundle as your normal user:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.kde.Platform//6.10
flatpak install --user ./QK4-<version>-linux-x86_64.flatpak
flatpak run io.github.mikeg_dal.QK4
```

The Mac application identity is separate, but both versions still use the
explicit shared QK4 settings store. Windows also shares QK4 settings. The
Flatpak keeps upstream's application ID and replaces an installed upstream
QK4 with that ID. Close either version before launching the other.

## Vail setup

1. Put the Vail adapter in computer/MIDI mode, not radio mode, and enable its
   local buzzer. Connect the paddles and USB cable.
2. Open QK4's CW Keyer options. Choose **Vail Adapter (local keyer)** as Device
   Type, select its MIDI port, and click Connect.
3. Use CW mode on the K4. QK4 sends the K4's WPM, pitch, and iambic A/B settings
   to the adapter. Listen to the Vail buzzer for immediate sidetone.
4. If dit and dah are swapped, reverse the paddle wiring or orientation
   physically. The K4's reverse-paddles option does not reverse this Vail input;
   QK4 does not send the newer firmware's paddle-swap command.

Some adapters advertise the microcontroller board name rather than "Vail", such
as Seeed XIAO M0, QT Py M0, TRRS Trinkey, or Arduino Micro. The port picker lists
all MIDI inputs so you can select that port explicitly. If you hear a second
delayed tone from the radio, enable **Mute the K4's CW monitor while keying** in
CW Keyer options. This leaves the Vail buzzer and computer volume independent;
see [K4 CW monitor](#k4-cw-monitor) below.

Windows MIDI input and output names can end in different port numbers. QK4
matches their shared device name and requires a unique input/output pair. If
several connected adapters have the same name, disconnect the extras before
connecting in QK4; matching them by their unrelated port numbers could select
the wrong adapter.

The implementation follows the Vail firmware's MIDI interface reviewed at
[`0e134d0`](https://github.com/Vail-CW/vail-adapter/tree/0e134d0d29987952ec70eec8349abf0da45f813d),
including timed key events on notes 0, 1, and 2, and
[Elecraft's K4 programmer's reference, rev. D12](https://ftp.elecraft.com/K4/Manuals%20Downloads/K4ProgrammersReferencerev.D12.html).
No adapter firmware changes are required. QK4 selects MIDI mode on every
connection. Speed, pitch, and keyer mode changes are saved by the adapter's
firmware; QK4 avoids repeating unchanged settings during a session and waits
until all key inputs release before changing them. Pitch follows the nearest
MIDI note.

### Sidetone source and volume

Choose the sidetone source in **CW Keyer** options:

- **Vail buzzer** uses the adapter's local sound, which starts without a computer
  audio round trip. Computer sidetone is off, and the computer volume control is
  hidden. QK4 cannot read or change the adapter's buzzer mute state or volume.
- **Computer audio** generates sidetone from the adapter's already-timed key
  events and enables QK4's computer volume control. The adapter still owns the
  keyer. From .1.7, this uses upstream's paced push output and device-format
  conversion, with a separate sidetone sink. Computer audio has MIDI and
  audio-device latency, and selecting it does not mute the adapter's buzzer.

To use only computer audio, mute the adapter manually before reconnecting its
keying path:

1. Click **Disconnect** for the Vail device in QK4, leaving its USB power on.
2. Hold the paddle that currently sends DIT for more than five seconds. Wait
   for the confirmation tone, then release it. This disables both the buzzer and
   the adapter's headphone output.
3. Choose computer audio and reconnect the Vail device in QK4.

To restore the Vail sound, disconnect it in QK4, unplug and replug its USB cable,
then select the Vail buzzer source and reconnect. Changing MIDI mode, keyer mode,
speed, or pitch does not restore a muted buzzer. A USB power cycle does restore
it, so a computer-audio preference saved in QK4 cannot guarantee the adapter
will stay silent after reconnecting USB. The
[Vail manual](https://vailadapter.com/manual) documents the five-second gesture
and power-cycle recovery. It also explains that adapters with a headphone jack
mute their speaker when headphones are inserted; headphone volume needs an
external inline volume control.

Current stock firmware has no MIDI buzzer-enable, mute, or volume command while
retaining the local keyer. CC2 selects a musical note; note zero is 8 Hz, not a
mute setting. MIDI Note Off stops a host-requested tone, rather than disabling
the local keyer's sidetone. Radio mode suppresses USB keying too, so it is not a
sidetone switch for QK4. See the
[v5.1.0 MIDI handler](https://github.com/Vail-CW/vail-adapter/blob/v5.1.0/adapter.cpp#L848)
and [tone table](https://github.com/Vail-CW/vail-adapter/blob/v5.1.0/equal_temperament.cpp).

### K4 CW monitor

Enable **Mute the K4's CW monitor while keying** in **CW Keyer** if the radio's
monitor comes back late through receive audio. The same saved checkbox applies
to Vail and HaliKey and defaults to off. It mutes the K4's reported CW monitor
level during accepted keying, then restores that level after the queued radio
keying and a short tail margin finish. Restoration accounts for Key-Down Delay,
so the monitor stays muted while the K4 finishes replaying buffered elements.

This controls the radio monitor, independently of **Vail buzzer** or **Computer
audio** and its volume. Selecting computer audio still requires the manual Vail
buzzer-mute procedure above if you want only one local sound source.

Turning the checkbox off restores the observed level. On an intentional radio
disconnect, QK4 sends a restore command before closing the connection. An abrupt
network or power loss can prevent that command from reaching the K4; check the
radio's CW monitor level after reconnecting if it remains silent. You can adjust
it with the K4's MON controls or TX Monitor Level in the Remote menu.

### Remote keying delay and operator stop

In **CW Keyer -> Remote Keying**, **Key-Down Delay** shows the K4's saved remote
keying delay (`KZL`). It applies to HaliKey, Vail, and KPOD+ and becomes editable
after the radio reports its value. Choose 0-1000 ms in 10 ms steps: raise it if
CW breaks up over a slow or busy link; lower it for a quicker start on a fast
LAN. The radio's default is 80 ms. QK4 keeps your radio setting when Vail
connects. This controls when the radio starts sending; it does not delay the
adapter's local buzzer.

Press **Escape** to abort sending. Release the paddles before starting again.

### Firmware compatibility

The existing local-keyer path uses the MIDI commands shared by older firmware
and [Vail v5.1.0](https://github.com/Vail-CW/vail-adapter/releases/tag/v5.1.0),
released October 21, 2026. That release adds CC3 paddle swap and fixes a crash
when selecting passthrough with `C0 00`. Older firmware reviewed for this fork
had no paddle-swap command and could crash on that passthrough command.

QK4 keeps using onboard iambic A/B modes (`C0 07` / `C0 08`) on both generations.
It does not automatically enable passthrough or send CC3. On v5.1.0, passthrough
reports raw paddle holds and omits their local sidetone; using it would transfer
keyer timing to the host, rather than mute the existing local-keyer path. A
dedicated straight-key input still has sidetone in passthrough. The
[v5.1.0 MIDI specification](https://github.com/Vail-CW/vail-adapter/blob/v5.1.0/docs/MIDI_INTEGRATION_SPEC.md)
describes these distinctions. CC3 also changes which physical paddle is treated
as DIT for the five-second mute gesture. Physical reversal remains the setup
method supported across firmware versions by this fork.

The first release exposes paddle keying. It does not add a Vail straight-key
mode to the UI. A continuous key-down is forcibly released after 2.4 seconds and
the radio is returned to receive. Release the paddles before sending again.
Holding an iambic paddle produces repeated elements with key-up gaps, so it does
not trigger this safeguard. Disconnect QK4 before using the five-second mute
gesture. A continuous carrier requires a different radio control.

### Linux MIDI access

Vail uses the ALSA sequencer MIDI port on Linux. Check that the host sees the
adapter with `aconnect -l` (provided by `alsa-utils`) and that `/dev/snd/seq`
exists. If the sequencer device is missing, check the distro's `snd_seq` module
configuration. If access is denied, fix the host's sound-device session ACLs or
group permissions, then reconnect or log in again as required by the distro.

The Flatpak already requests `--socket=pulseaudio` and `--device=all`.
Flatpak documents that the PulseAudio permission includes MIDI and ALSA devices
under `/dev/snd`; see
[sandbox permissions](https://docs.flatpak.org/en/latest/sandbox-permissions.html).
Inspect the sandbox when the host sees the device but QK4 does not:

```sh
flatpak info --show-permissions io.github.mikeg_dal.QK4
flatpak run --command=sh io.github.mikeg_dal.QK4 -c 'ls -l /dev/snd/seq'
```

The KPOD udev rule in the main README is for Elecraft USB hardware. It does not
grant Vail MIDI permissions. Running QK4 as root is not a MIDI setup step.

## Hardware acceptance

Test into a dummy load at low power. Record adapter model/firmware, OS, QK4
version, and K4 firmware in the release's test notes or a fork issue.

- Confirm local sidetone works even before connecting QK4 to the radio.
- Connect QK4 and verify short dits, dahs, and a mixed phrase at several speeds.
  Check that key-up occurs on every element and that the transmitted pattern
  follows the local sound without a duplicate delayed sidetone in the headphones.
- Enable **Mute the K4's CW monitor while keying** with a known nonzero radio
  monitor level. Verify the delayed radio tone is suppressed for Vail and
  HaliKey, including a Vail/HaliKey handover during the restore tail. Check the
  original level returns after sending, after disabling the checkbox, and on
  an intentional radio disconnect. Repeat with a longer Key-Down Delay. If the
  network is lost abruptly, reconnect and check the K4's monitor level manually.
- Change WPM, sidetone pitch, and iambic A/B in QK4/K4. Check the adapter follows.
- Change Key-Down Delay, reconnect Vail, and verify that the radio's value is
  retained.
- Select computer audio after manually muting the disconnected adapter. Verify
  the computer volume changes only the computer sound. Restore the Vail buzzer
  source and USB power-cycle the adapter; verify computer sidetone stops and
  the adapter's sound returns.
- Release a squeeze and verify both paddles return to idle. Check dit/dah
  orientation physically.
- Disconnect USB while keying, disconnect the radio, change device type, and
  exit QK4 while keying. Verify that the radio returns to receive each time.
- Press Escape while sending. Verify radio key-up, computer-tone shutdown, no
  held-input retrigger, and normal sending after releasing the paddles.
- Hold the key longer than 2.4 seconds. Verify forced key-up and receive, no
  automatic retrigger while held, and normal sending after release.
- Check reconnecting, switching back to HaliKey, KPOD+ takeover, CW/CW-R mode
  changes, and ordinary CW macros against the new controller and sidetone output.
  Repeat on Linux before calling that platform verified.

These checks help validate another adapter or operating system; they are not
all completed on every platform. Record failures with exact versions and
reproduction steps.

## Branches, versions, and releases

- `upstream` points to `mikeg-dal/QK4`; `origin` points to `rwjblue/QK4`.
- `rwjblue/vail-upstream` contains the Vail runtime, tests, CMake changes, and
  `docs/k4-protocol-quirks.md` changes on `development@upstream`. This is the
  source branch for an upstream PR targeting `development`. A separate CI repair
  caps the upstream RealtimeSanitizer build at four compiler jobs; a bare
  `--parallel` started an unbounded Make build and both hosted runners shut down.
- `rwjblue/vail` is its child and the fork's default branch. Its change contains
  fork CI, packaging, the README introduction, and this guide.
- Keep fixes in the layer that owns them. The lower layer includes both Vail
  buzzer and optional computer sidetone support; the upper change owns fork
  distribution and maintenance.
- Fork tags use `fork-v<upstream-version>.<fork-revision>`. The first is
  `fork-v0.8.0-beta.1.1`; its binary version is `0.8.0-beta.1.1-vail`.
  `fork-v0.8.0-beta.1.2` adds optional computer sidetone on the same release base.
  `fork-v0.8.0-beta.1.3` adds Windows packages on the development baseline noted
  above. `fork-v0.8.0-beta.1.4` refreshes the packages without runtime changes on
  that same baseline. `fork-v0.8.0-beta.1.7` integrates upstream `72025dfe` and
  its new CW keyer and paced sidetone output. `fork-v0.8.0-beta.1.8` moves to
  upstream `78f0aac9`, adds shared K4 CW monitor muting, and gives the Mac app
  its separate QK4 Vail identity. Use a fresh fork revision for each release and record its
  upstream baseline in this guide and the release notes.
- Do not move published tags. The `fork-` prefix starts with `f`, avoiding
  upstream's `v*` release workflow, which requires the maintainer's signing
  secrets and gh-pages setup. A `vail-` prefix would still match `v*`; do not use
  it for fork release tags.

```text
development@upstream
  -> rwjblue/vail-upstream  (runtime, tests, protocol notes)
    -> rwjblue/vail         (fork CI, packaging, README, guide; default)
```

`fork-ci.yml` runs on pushes and pull requests to `rwjblue/vail`, and on demand.
It runs the upstream-pinned formatting check and the full unit-test suite on
Linux, both normally and under ASAN/UBSAN. macOS and Windows run seventeen
focused suites: six Vail suites, two CW monitor suites, two virtual-clock
PaddleKeyer suites, SidetoneScheduler, SerialLinePoller, MIDI port matching,
KzEgress, RtScheduling, ThreadShutdown, and HaliKeyPortScan.
Windows keeps compiler results in the GitHub Actions cache and saves vcpkg
packages immediately after successful configuration, so a later test failure
does not discard the dependency build. The CW monitor integration suite writes
a text log which the Windows job always prints, including when CTest captures
no console output.

The Vail page suite also links the real controller to check settings and
device-switch behavior; CW monitor routing is exercised over a local TCP
connection. The full Linux suite contains 80 CTest suites at the .1.8 baseline,
including PaddleKeyerThreadedTests.
That real-clock suite is excluded only from macOS/Windows packaging gates: a
loaded hosted Mac failed its fixed-duration throughput assumption even while
the deterministic keyer and all Vail suites passed. Its threaded coverage stays
required in both full Linux jobs. A third Linux gate carries upstream's
RealtimeSanitizer check with Clang 22 and `QK4_TESTS_ONLY`: it runs the 77
Qt-base suites and three sanitizer self-tests. The three device/UI integration
suites remain covered by the full normal and ASAN/UBSAN jobs.

The macOS builder verifies the mounted DMG's app name and bundle identity,
strict code signature, dependencies, and executable version. Upstream's bundle
verification also requires the TLS self-check to load OpenSSL from inside the
app. Windows uses MSVC and vcpkg,
bundles Qt, OpenSSL, and the Visual C++ runtime, and launches the packaged app
with `--version`. Its portable ZIP is included in the release checksums. Flatpak
verification checks the installed files and version stamp. Artifacts last 14
days. The virtual duplex MIDI fixture uses CoreMIDI and is skipped on Linux and
Windows; protocol, routing, and page tests still run there. Real adapter testing
remains part of platform acceptance.

The macOS builder is called with `sign: false` and `fork-branding: true`. Fork
branding renames the bundle and updates its name, display name, and identifier
before deployment and signing. Other callers retain the official QK4 identity
because this input defaults to false. Windows builds do not require
code-signing credentials. The Flatpak builder rewrites the
manifest's source repository and SHA to the checked-out fork commit, so it cannot
silently build upstream instead of the patch.

`fork-release.yml` runs the same complete check/build path. A manual dispatch is
a rehearsal: it creates downloadable Actions artifacts and checksums but
publishes no release. A `fork-v*` tag publishes a GitHub prerelease and permanent
release assets after all checks pass. It does not update gh-pages. No secrets
beyond GitHub's per-run token are needed.

GitHub requires a dispatchable workflow to exist on the repository's default
branch. This fork uses `rwjblue/vail` as its default. For a fresh fork, enable
Actions, set that branch as default, and dispatch Fork CI before tagging a
release. Pushes to the upper branch start CI for the complete stack. See
[GitHub workflow events](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#workflow_dispatch).

After describing and validating both layers, push both bookmarks with the
normal signed `jj` workflow:

```sh
jj git push --remote origin --bookmark rwjblue/vail-upstream --bookmark rwjblue/vail
gh run list --repo rwjblue/QK4 --workflow fork-ci.yml --branch rwjblue/vail
gh run watch --repo rwjblue/QK4 <run-id> --exit-status

# Optional release-path rehearsal.
gh workflow run fork-release.yml --repo rwjblue/QK4 --ref rwjblue/vail
```

Publish only when a new release is intended. Choose a fresh tag and tag the same
signed upper-layer commit whose checks passed:

```sh
jj tag set <new-fork-tag> -r rwjblue/vail
jj git push --remote origin --tag <new-fork-tag>
gh run list --repo rwjblue/QK4 --workflow fork-release.yml
gh run watch --repo rwjblue/QK4 <release-run-id> --exit-status
gh release view <new-fork-tag> --repo rwjblue/QK4
```

## Updating from upstream

The mutable stack tracks upstream `development`, currently `78f0aac9`. Review
its changes before rebasing, especially MIDI lifecycle handling, CW ownership,
timed keying, sidetone output, and device settings.

Published release tags protect their original history. This upstream refresh began by
duplicating the tagged .1.4 layers into mutable changes. If the current stack
has since been tagged, duplicate its complete range before rewriting and move
the same two bookmarks to the copied lower-layer tip and fork change:

```sh
jj git fetch --remote upstream
jj duplicate 'development@upstream..rwjblue/vail' --onto development@upstream
jj bookmark set rwjblue/vail-upstream -r <copied-lower-layer-tip> --allow-backwards
jj bookmark set rwjblue/vail -r <copied-fork-change> --allow-backwards
```

Keep every published tag and asset fixed; no additional named bookmarks are
needed. For an already mutable stack, rebase from the first upstream-bound
change, including any separate CI repair below the lower bookmark. Its fork
layer moves with it:

```sh
jj git fetch --remote upstream
jj rebase -s 'roots(development@upstream..rwjblue/vail-upstream)' -d development@upstream
jj diff --from development@upstream --to rwjblue/vail-upstream
jj diff --from rwjblue/vail-upstream --to rwjblue/vail
```

The first diff should contain runtime, tests, CMake, protocol notes, and the
small upstream CI repair if it has not landed separately. The
second should contain only the fork's distribution and maintenance files.
Resolve conflicts and run the local format/build/tests, fork CI packaging, and
hardware acceptance checks appropriate to the change before pushing both signed
bookmarks. There is no scheduled auto-rebase.

For a feature fix, inspect the lower range and insert a temporary change after
the change that owns the runtime, then squash into it after validation. This
keeps the CI repair separate, and all descendants follow automatically:

```sh
jj log -r 'development@upstream..rwjblue/vail-upstream'
jj new --insert-after <owning-feature-change>
# Edit runtime, tests, CMake, or protocol notes; validate the fix.
jj squash
```

For fork CI, packaging, README, or guide changes, work on a temporary child of
the upper layer and squash there:

```sh
jj new rwjblue/vail
# Edit the fork files and validate them.
jj squash
```

Once upstream has landed all the required feature behavior, retain the fork
distribution layer directly on the updated upstream branch:

```sh
jj git fetch --remote upstream
# Confirm upstream includes the feature and its regression coverage first.
jj rebase -r rwjblue/vail -d development@upstream
jj bookmark set rwjblue/vail-upstream -r development@upstream --allow-backwards
```

Validate and push both bookmarks as above. If upstream lands only part of the
feature, keep the remaining runtime delta in `rwjblue/vail-upstream` until it is
also covered upstream. Existing released builds remain available for rollback.

Every native builder and Linux test job reads the exact Qt version from
`.github/qt-version` (currently 6.10.3). Keep `scripts/check-qt-version.sh` green
when updating packaging. The Mac workflow uses official Qt and upstream's
`macos-bundle.sh` and `macos-verify-bundle.sh`; Homebrew supplies only the non-Qt
libraries. When a package stops building or launching, inspect these scripts,
the Qt pin, and Homebrew dependencies before changing runtime code. Intel support
would also need portable Homebrew discovery in CMake and a separate Intel build; simply
renaming the ARM64 DMG would not produce an Intel application.
