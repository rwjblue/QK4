#ifndef CWCONTROLLER_H
#define CWCONTROLLER_H

#include <QObject>
#include <atomic>
#include <memory>

class RadioState;
class ConnectionController;
class PaddleKeyer;
class FunctionLink;
class SidetoneGenerator;
class HalikeyDevice;
class KpodPlusDevice;

// =============================================================================
// CwController — CW keying orchestration
// =============================================================================
//
// Purpose
// -------
// Orchestration layer that wires the CW devices together. Owns no devices and no threads;
// HardwareController constructs and owns the HaliKey, KPOD+, PaddleKeyer (keyer thread, with its
// clock, link, sidetone scheduler and timer) and SidetoneGenerator (sidetone thread). CwController
// takes them as injected pointers and wires the CW behaviour.
//
// The HaliKey keyer is PaddleKeyer, a port of K4-Companion's (docs/cw-keyer-port-plan.md). It
// sends timed KZD####/KZU####. Element timing travels in the command, so nothing on the path from
// the keyer thread to the socket can change what goes on the air. The KPOD+ runs its own keyer on
// the device and its KZ strings are forwarded untouched.
//
// Construction order
// ------------------
//     m_hardwareController = new HardwareController(rs, cc, this);
//     m_cwController = new CwController(rs, cc,
//         m_hardwareController->paddleKeyer(), m_hardwareController->keyerLink(),
//         m_hardwareController->sidetoneGenerator(), m_hardwareController->halikeyDevice(),
//         m_hardwareController->kpodPlusDevice(), this);
//
// Wiring
// ------
//   src                                  | dst                         | thread
//   -------------------------------------|-----------------------------|-----------------------------
//   PaddleKeyer → FunctionLink            | TcpClient::sendCAT          | keyer → I/O (sendCAT queues)
//   HalikeyDevice::paddleChanged (V1.4)  | PaddleKeyer::paddleEvent    | runs ON the HaliKey worker
//   HalikeyDevice MIDI sink (MIDI)       | PaddleKeyer::midiMessage    | runs ON RtMidi's thread
//   RadioState keyer speed/KP/pitch/ML/KZL| keyer + sidetone + KPOD+    | main
//   RadioState::modeChanged, radioReady,  | PaddleKeyer::setInhibited   | main
//     disconnect, KPOD+ gate             |                             |
//   RadioSettings halikey/*              | keyer mode, straight key,   | main
//                                        | dit memory, monitor mute    |
//   KpodPlusDevice::keyerDataReceived    | TcpClient::sendCATBytes     | EP02 worker → I/O (queued)
//
// Inhibit
// -------
// The keyer keys nothing unless the radio is ready, the mode is CW or CW-R, and the KPOD+ does not
// own CW. Raising the inhibit releases whatever is keyed (an open straight-key KZD gets its KZU)
// and drops paddle events at the door. It replaces the per-lever gating the iambic keyer needed:
// the keyer reads its paddles from time-stamped history, so a single release is all a gate needs.
//
// Threading invariants
// --------------------
//   1. The V1.4 line handler runs DirectConnection on the HaliKey worker, and the MIDI sink on
//      RtMidi's thread. Both stamp the edge on arrival; a queued hop would be timing error.
//   2. m_cachedIsV14 and m_reversed are stored on the main thread (release) and loaded on those
//      device threads (acquire).
//   3. FunctionLink serializes software commands onto the I/O route. Session generations reject
//      stale starts; a release retains the generation of its matching key-down.
//   4. KPOD+ EP02 keyer data routes EP02 worker → I/O thread via QueuedConnection.
//   5. On a KPOD+ gate rise, store the gate FIRST and then raise the inhibit.
//   6. Destructor runs disconnect(this) first (CONVENTIONS Rule 11).
//   7. The route always closes a forwarded KZD, even after its source is inhibited. A different
//      source cannot start before that release; unmatched cleanup cannot interrupt its keying.
//
// Footswitch PTT: REMOVED, deliberately
// -------------------------------------
//   Neither transport keys PTT from a footswitch. On V1.4 the pedal and the dit lever both drive
//   CTS and are indistinguishable on the wire; the MIDI variant's note 31 is ignored. A footswitch
//   plugged into the K4 itself is unaffected.
//
// Verification (before a release)
// -------------------------------
//   - HaliKey V1.4 and MIDI paddle keying at 20-50 WPM, Iambic A and B; Ultimatic; straight key
//   - KPOD+ keying unchanged (paddle → on-device keyer → EP02 → K4)
//   - Sidetone audible with a constant delay; silent while the KPOD+ owns CW
//   - Hold a paddle in CW and switch to SSB mid-press: keying stops, nothing stuck
//   - Esc mid-burst: keying stops, radio unkeyed
// =============================================================================

namespace VailSidetoneRouting {
class Route;
}

class CwController : public QObject {
    Q_OBJECT

public:
    CwController(RadioState *radioState, ConnectionController *connection, PaddleKeyer *keyer, FunctionLink *link,
                 SidetoneGenerator *sidetone, HalikeyDevice *halikey, KpodPlusDevice *kpodPlus,
                 QObject *parent = nullptr);
    ~CwController() override;

    /// The KPOD+ has taken over CW keying, or given it back. Driven by HardwareController, which
    /// owns the device lifecycle policy (USB-003). While it's up, QK4's keyer keys nothing.
    void setKpodPlusGate(bool active);

    /// The operator set the K4's remote-keying key-down delay (KZL, 0-1000 ms). Sent, then read
    /// back, so RadioState holds what the radio took. The only place QK4 writes KZL.
    void setKeyDownInitialDelay(int ms);

    /// Esc: stop QK4's own keying now. The burst ends at its next element boundary; an open
    /// straight-key KZD gets its KZU. Vail aborts immediately and waits for held inputs to release.
    void stopKeying();

private:
    void wireKeyingRoutes();
    void configureVailKeyer();
    void updateVailKeying();
    void updateVailSidetone(bool forceReset = false);
    void applySpeed();
    void applyMode();
    void applyReversal();
    void updateInhibit();
    void onPaddle(bool dah, bool asserted, double deltaTime);

    RadioState *m_radioState;
    ConnectionController *m_connection;
    PaddleKeyer *m_keyer;          // owned by HardwareController
    FunctionLink *m_link;          // owned by HardwareController
    SidetoneGenerator *m_sidetone; // owned by HardwareController
    HalikeyDevice *m_halikey;      // owned by HardwareController
    KpodPlusDevice *m_kpodPlus;    // owned by HardwareController

    std::shared_ptr<std::atomic<bool>> m_vailSelected = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<std::atomic<bool>> m_softwareKeyingAllowed = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<std::atomic<quint64>> m_softwareKeyingGeneration = std::make_shared<std::atomic<quint64>>(0);
    std::shared_ptr<std::atomic<bool>> m_vailComputerSidetone = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<std::atomic<quint64>> m_vailSidetoneGeneration = std::make_shared<std::atomic<quint64>>(0);
    std::shared_ptr<std::atomic<bool>> m_vailSidetoneGate = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<VailSidetoneRouting::Route> m_vailSidetoneRoute;
    bool m_vailSidetoneAllowed = false;

    // Main thread only.
    bool m_radioReady = false;
    bool m_kpodPlusGate = false;

    // true only for V1.4 serial (deviceType 0). Vail bypasses the software keyer.
    std::atomic<bool> m_cachedIsV14{true};
    // The K4's KP paddle orientation is Reverse. See invariant 2.
    std::atomic<bool> m_reversed{false};
};

#endif // CWCONTROLLER_H
