# QK4 Vail fork

This is Robert's unofficial QK4 fork for using Vail adapters as local CW keyers.
The adapter generates the elements; QK4 forwards the timed keying to the K4.
Sidetone comes from the Vail buzzer by default, with computer audio available as
an option. QK4 does not run a second software keyer for this device. HaliKey
remains available as a separate device type.

The first release is based on upstream `v0.8.0-beta.1` (`cc3fb6d1`) and is a
**prerelease pending physical Vail + K4 testing**. Automated tests and virtual
MIDI tests cannot verify RF timing, the adapter's buzzer, or every firmware
variant. Use the acceptance check below before relying on it on air.

## Downloads and installation

Download the Vail prerelease from
[rwjblue/QK4 releases](https://github.com/rwjblue/QK4/releases). Releases include
SHA-256 checksums and these two assets:

| Asset | Supported system |
| --- | --- |
| `QK4-<version>-macos.dmg` | Apple Silicon, macOS 15 or newer |
| `QK4-<version>-linux-x86_64.flatpak` | x86_64 Linux with Flatpak and KDE runtime 6.10 |

Intel Macs, Linux ARM, and Windows are outside this fork's initial release
matrix. Upstream still provides its own broader platform builds.

The macOS application is ad-hoc signed, without Developer ID signing or Apple
notarization. Copy `QK4.app` from the DMG to Applications. If macOS blocks first
launch, attempt to open it, then use System Settings -> Privacy & Security ->
Open Anyway. See [Apple's first-launch instructions](https://support.apple.com/en-us/102445).
No Apple signing credentials are required to build this fork.

On Linux, install the downloaded bundle as your normal user:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.kde.Platform//6.10
flatpak install --user ./QK4-<version>-linux-x86_64.flatpak
flatpak run io.github.mikeg_dal.QK4
```

The fork currently keeps upstream's application IDs and settings. The Flatpak
replaces an installed upstream QK4 with the same ID. On macOS, keep a copy of the
original app if you want to switch back; both apps use the same QK4 settings.
Close either version before launching the other.

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
delayed tone from the radio, set the K4's remote TX monitor/sidetone level to zero
using its MON controls or TX Monitor Level in the Remote menu.

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
  keyer. Computer audio has MIDI and audio-device latency, and selecting it does
  not mute the adapter's buzzer.

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
- Change WPM, sidetone pitch, and iambic A/B in QK4/K4. Check the adapter follows.
- Select computer audio after manually muting the disconnected adapter. Verify
  the computer volume changes only the computer sound. Restore the Vail buzzer
  source and USB power-cycle the adapter; verify computer sidetone stops and
  the adapter's sound returns.
- Release a squeeze and verify both paddles return to idle. Check dit/dah
  orientation physically.
- Disconnect USB while keying, disconnect the radio, change device type, and
  exit QK4 while keying. Verify that the radio returns to receive each time.
- Hold the key longer than 2.4 seconds. Verify forced key-up and receive, no
  automatic retrigger while held, and normal sending after release.
- Check that reconnecting, switching back to HaliKey, and using ordinary CW
  macros still work. Repeat on Linux before calling that platform verified.

Keep the release marked as needing hardware testing until these checks have
passed on the real adapter and radio. Record failures with exact versions and
reproduction steps.

## Branches, versions, and releases

- `upstream` points to `mikeg-dal/QK4`; `origin` points to `rwjblue/QK4`.
- `rwjblue/vail` is the maintained fork branch and should be the fork's default
  branch. Keep the upstream branches available for comparison.
- Keep Vail runtime/tests and fork packaging/docs in focused commits so the
  patch stack is small and easy to reapply.
- Fork tags use `fork-v<upstream-version>.<fork-revision>`. The first is
  `fork-v0.8.0-beta.1.1`; its binary version is `0.8.0-beta.1.1-vail`.
  Increment the final revision for a new fork release against the same base.
- Do not move published tags. The `fork-` prefix starts with `f`, avoiding
  upstream's `v*` release workflow, which requires the maintainer's signing
  secrets and gh-pages setup. A `vail-` prefix would still match `v*`; do not use
  it for fork release tags.

`fork-ci.yml` runs on pushes and pull requests to `rwjblue/vail`, and on demand.
It runs the upstream-pinned formatting check and the full unit-test suite on
Linux, both normally and under ASAN/UBSAN. The macOS builder runs the six Vail
suites, including live CoreMIDI integration, the options-page regression test,
live sidetone rendering (`VailLiveSidetoneTests`), and sidetone source routing
(`VailSidetoneRoutingTests`), then verifies the packaged app. The existing Flatpak
verification checks the installed files and version stamp. Artifacts last 14
days.
The virtual duplex MIDI fixture uses CoreMIDI and is skipped on Linux; the
protocol, routing, and page tests still run there. A real Linux adapter test
remains part of hardware acceptance.

The macOS test selection is intentional: the first hosted Mac run passed the
Vail suites available at that time but failed upstream's software IambicKeyer
timing tests when short waits stretched under runner scheduling. The complete
suite, including IambicKeyer, remains required in both Linux test jobs. Vail uses
the adapter's hardware keyer rather than that software keyer.

The macOS builder is called with `sign: false`. The Flatpak builder rewrites the
manifest's source repository and SHA to the checked-out fork commit, so it cannot
silently build upstream instead of the patch.

`fork-release.yml` runs the same complete check/build path. A manual dispatch is
a rehearsal: it creates downloadable Actions artifacts and checksums but
publishes no release. A `fork-v*` tag publishes a GitHub prerelease and permanent
release assets after all checks pass. It does not update gh-pages. No secrets
beyond GitHub's per-run token are needed.

GitHub requires a dispatchable workflow to exist on the repository's default
branch. This fork uses `rwjblue/vail` as its default. During initial setup, GitHub
did not register the new workflows on the branch's first push; setting the fork
default to `rwjblue/vail` registered them, after which dispatching Fork CI started
the first check/build run. For a fresh fork, enable Actions, set the fork branch
as default, and dispatch Fork CI before tagging a release. Later branch pushes
start CI normally. See
[GitHub workflow events](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#workflow_dispatch).

After committing the tested changes, push with the normal signed `jj` workflow:

```sh
jj bookmark set rwjblue/vail -r <tested-change>
jj git push --remote origin --bookmark rwjblue/vail
gh run list --repo rwjblue/QK4 --workflow fork-ci.yml --branch rwjblue/vail
gh run watch --repo rwjblue/QK4 <run-id> --exit-status

# Optional release-path rehearsal, after rwjblue/vail becomes the default branch.
gh workflow run fork-release.yml --repo rwjblue/QK4 --ref rwjblue/vail

# Only tag the same signed commit whose checks passed.
jj tag set fork-v0.8.0-beta.1.1 -r rwjblue/vail
jj git push --remote origin --tag fork-v0.8.0-beta.1.1
gh run list --repo rwjblue/QK4 --workflow fork-release.yml
gh run watch --repo rwjblue/QK4 <release-run-id> --exit-status
gh release view fork-v0.8.0-beta.1.1 --repo rwjblue/QK4
```

## Updating from upstream

Review new upstream release tags periodically, or when a needed fix ships.
There is no scheduled auto-rebase. Start with a released tag rather than a
moving development branch, inspect the upstream changelog, and check for native
Vail support before carrying the patch forward.

For an untagged mutable patch stack, `jj rebase -b rwjblue/vail --onto <new-upstream-tag>`
is sufficient after creating a backup bookmark. A published stack is protected
by release tags: duplicate it onto the new base to preserve existing releases
and avoid moving tags while rewriting commits:

```sh
jj git fetch --remote upstream
jj bookmark create archive/vail-before-update -r rwjblue/vail

# Substitute the currently documented base tag and the chosen new release tag.
jj duplicate 'v0.8.0-beta.1..rwjblue/vail' --onto <new-upstream-tag>
# Inspect the new changes and resolve conflicts; copy the new tip ID from output.
jj new <new-fork-tip>
```

Review the delta against the new base, especially MIDI lifecycle handling,
CW ownership, timed keying protocol, and device settings. Repeat the local
format/build/tests, CI packaging, and hardware acceptance checks. Update this
document's base version and first-release wording as appropriate. After review,
move the fork branch to the new tip with
`jj bookmark set rwjblue/vail -r <new-fork-tip> --allow-backwards`, push with `jj`,
wait for CI, and publish a fresh `fork-v<new-upstream-version>.1` tag. Keep the
old release assets and tags available for rollback.

Homebrew dependencies can change independently of QK4. When a previously green
macOS package stops building or launching, inspect upstream builder changes and
the selected Qt version before changing runtime code. Intel support would also
need portable Homebrew discovery in CMake and a separate Intel build; simply
renaming the ARM64 DMG would not produce an Intel application.
