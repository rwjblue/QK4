#include "cwcontroller.h"

#include <QLoggingCategory>
#include <mutex>

#include "audio/sidetonegenerator.h"
#include "connectioncontroller.h"
#include "hardware/halikeydevice.h"
#include "hardware/keyerruntime.h"
#include "hardware/kpodplusdevice.h"
#include "hardware/paddlekeyer.h"
#include "hardware/vail_sidetone_routing.h"
#include "models/radiostate.h"
#include "network/tcpclient.h"
#include "settings/radiosettings.h"

Q_LOGGING_CATEGORY(cwKeyer, "cw.keyer")

CwController::CwController(RadioState *radioState, ConnectionController *connection, PaddleKeyer *keyer,
                           FunctionLink *link, SidetoneGenerator *sidetone, HalikeyDevice *halikey,
                           KpodPlusDevice *kpodPlus, QObject *parent)
    : QObject(parent), m_radioState(radioState), m_connection(connection), m_keyer(keyer), m_link(link),
      m_sidetone(sidetone), m_halikey(halikey), m_kpodPlus(kpodPlus) {

    // =========================================================================
    // Keyer → K4
    // =========================================================================
    // The I/O route preserves matching releases across source changes; see invariant 7.

    // =========================================================================
    // Configuration from the K4 (the source of truth) and from settings
    // =========================================================================
    auto *settings = RadioSettings::instance();
    m_cachedIsV14.store(settings->halikeyDeviceType() == HalikeyDevice::Serial, std::memory_order_release);
    m_vailSelected->store(settings->halikeyDeviceType() == HalikeyDevice::Vail, std::memory_order_release);
    m_vailComputerSidetone->store(settings->vailComputerSidetone(), std::memory_order_release);
    m_vailSidetoneRoute = std::make_shared<VailSidetoneRouting::Route>();
    wireKeyingRoutes();
    connect(settings, &RadioSettings::vailComputerSidetoneChanged, this, [this](bool computer) {
        m_vailComputerSidetone->store(computer, std::memory_order_release);
        if (m_vailSelected->load(std::memory_order_acquire))
            updateVailSidetone(true);
    });
    m_keyer->setStraightKeying(settings->halikeyStraightKey());
    m_keyer->setDitMemoryDelayPercent(settings->halikeyDitMemoryDelay());
    m_keyer->setMuteRadioMonitor(settings->halikeyMuteRadioMonitor());
    if (m_radioState->monitorLevelCW() >= 0)
        m_keyer->setMonitorLevel(m_radioState->monitorLevelCW());
    if (m_radioState->keyDownInitialDelayMs() >= 0)
        m_keyer->setKeyDownInitialDelayMs(m_radioState->keyDownInitialDelayMs());
    applySpeed();
    applyMode();
    applyReversal();
    if (m_radioState->cwPitch() > 0)
        QMetaObject::invokeMethod(m_sidetone, "setFrequency", Qt::QueuedConnection,
                                  Q_ARG(int, m_radioState->cwPitch()));

    connect(m_radioState, &RadioState::keyerSpeedChanged, this, [this](int wpm) {
        applySpeed();
        configureVailKeyer();
        // K4 is the source of truth — mirror the speed onto the KPOD+ keyer.
        if (m_kpodPlus->isPolling())
            m_kpodPlus->setKeyerSpeed(wpm);
    });

    connect(m_radioState, &RadioState::keyerPaddleChanged, this, [this](QChar iambic, QChar paddle, int /*weight*/) {
        applySpeed(); // the weight
        applyMode();
        applyReversal();
        configureVailKeyer();
        if (m_kpodPlus->isPolling())
            m_kpodPlus->setKeyerParams(iambic == 'B' ? 1 : 0, paddle == 'R');
    });

    connect(m_radioState, &RadioState::cwPitchChanged, this, [this](int pitchHz) {
        configureVailKeyer();
        QMetaObject::invokeMethod(m_sidetone, "setFrequency", Qt::QueuedConnection, Q_ARG(int, pitchHz));
        // K4 is the source of truth — mirror the CW pitch onto the KPOD+ keyer.
        if (m_kpodPlus->isPolling())
            m_kpodPlus->setCwPitch(pitchHz);
    });

    connect(m_radioState, &RadioState::monitorLevelChanged, this, [this](int mode, int level) {
        if (mode == 0)
            m_keyer->setMonitorLevel(level);
    });

    connect(m_radioState, &RadioState::keyDownInitialDelayChanged, this,
            [this](int ms) { m_keyer->setKeyDownInitialDelayMs(ms); });

    connect(settings, &RadioSettings::halikeyDeviceTypeChanged, this, [this](int type) {
        m_cachedIsV14.store(type == HalikeyDevice::Serial, std::memory_order_release);
        m_vailSelected->store(type == HalikeyDevice::Vail, std::memory_order_release);
        m_keyer->releasePaddles();
        updateVailSidetone(true);
        configureVailKeyer();
        updateInhibit();
    });
    connect(settings, &RadioSettings::halikeyKeyerModeChanged, this, [this](int) { applyMode(); });
    m_halikey->setStraightKey(settings->halikeyStraightKey());
    connect(settings, &RadioSettings::halikeyStraightKeyChanged, this, [this](bool straight) {
        m_keyer->setStraightKeying(straight);
        m_halikey->setStraightKey(straight);
        applyReversal();
    });
    connect(settings, &RadioSettings::halikeyDitMemoryDelayChanged, this,
            [this](int percent) { m_keyer->setDitMemoryDelayPercent(percent); });
    connect(settings, &RadioSettings::halikeyMuteRadioMonitorChanged, this,
            [this](bool mute) { m_keyer->setMuteRadioMonitor(mute); });

    // =========================================================================
    // Inhibit: radio ready, in CW, KPOD+ not owning CW
    // =========================================================================
    connect(m_radioState, &RadioState::modeChanged, this, [this](RadioState::Mode) { updateInhibit(); });
    connect(m_connection, &ConnectionController::radioReady, this, [this]() {
        m_radioReady = true;
        configureVailKeyer();
        updateInhibit();
    });
    connect(m_connection, &ConnectionController::connectionStateChanged, this,
            [this](TcpClient::ConnectionState state) {
                if (state != TcpClient::Connected) {
                    m_radioReady = false;
                    updateInhibit();
                }
            });
    updateInhibit();

    // =========================================================================
    // HaliKey → keyer
    // =========================================================================
    // V1.4 serial: one edge per change from the worker's 1 ms poll, on the worker thread
    // (invariant 1). The poller has already applied rest state, straight-key debounce and the
    // per-line delta.
    connect(
        m_halikey, &HalikeyDevice::paddleChanged, this,
        [this](bool dah, bool asserted, double deltaTime) { onPaddle(dah, asserted, deltaTime); },
        Qt::DirectConnection);

    // MIDI: raw messages with RtMidi's delta, on RtMidi's thread (invariant 1). MoMIDI's own time
    // stamps are decoded by the keyer.
    m_halikey->setMidiSink([this](int status, int note, int velocity, double deltaTime) {
        if (!m_cachedIsV14.load(std::memory_order_acquire) && !m_vailSelected->load(std::memory_order_acquire))
            m_keyer->midiMessage(status, note, velocity, deltaTime);
    });

    // A lost device releases the key: a held paddle's release would otherwise never arrive.
    connect(m_halikey, &HalikeyDevice::disconnected, this, [this]() {
        m_keyer->releasePaddles();
        updateVailSidetone(true);
    });

    // =========================================================================
    // KPOD+ EP02 keyer data routing — unchanged
    // =========================================================================
    // The KPOD+ delivers complete KZ/KX strings in 32-byte transfers, zero-padded after the
    // last ';'. By targeting TcpClient as the receiver (lives on the I/O thread) with a
    // queued connection we skip the main thread entirely on this hot path. sendCATBytes
    // trims NUL padding on the I/O thread and hands off to sendCAT().
    connect(m_kpodPlus, &KpodPlusDevice::keyerDataReceived, m_connection->tcpClient(), &TcpClient::sendCATBytes,
            Qt::QueuedConnection);
    configureVailKeyer();
    if (m_vailSelected->load(std::memory_order_acquire))
        updateVailSidetone(true);
}

CwController::~CwController() {
    // CONVENTIONS Rule 11. What actually protects the device-thread handlers is MainWindow::closeEvent
    // calling HardwareController::shutdownDevices() while ConnectionController is still alive; this
    // is the second line of defence.
    disconnect(this);
    // WHY the MIDI sink is NOT cleared here: HardwareController, and with it the HaliKey, is
    // destroyed before this (construction order), so m_halikey dangles by now. Clearing it here
    // crashed QK4 on every quit. Nothing needs it cleared: shutdownDevices() closes the HaliKey,
    // which stops RtMidi's callbacks, before either object goes away, and the sink dies with the
    // device.
}

void CwController::applySpeed() {
    int wpm = m_radioState->keyerSpeed();
    if (wpm <= 0)
        wpm = 20;
    const int weight = m_radioState->keyingWeight();
    m_keyer->setSpeed(wpm, weight > 0 ? weight / 100.0 : 1.0);
}

void CwController::applyMode() {
    switch (RadioSettings::instance()->halikeyKeyerMode()) {
    case 1:
        m_keyer->setMode(PaddleKeyer::Mode::PlainIambic);
        break;
    case 2:
        m_keyer->setMode(PaddleKeyer::Mode::Ultimatic);
        break;
    default:
        m_keyer->setMode(m_radioState->iambicMode() == 'A' ? PaddleKeyer::Mode::IambicA : PaddleKeyer::Mode::IambicB);
        break;
    }
}

void CwController::applyReversal() {
    // A straight key is on the dit line whatever the paddle orientation.
    const bool reversed = m_radioState->paddleOrientation() == 'R' && !RadioSettings::instance()->halikeyStraightKey();
    const bool changed = m_reversed.exchange(reversed, std::memory_order_acq_rel) != reversed;
    m_keyer->setDitNotes({reversed ? 21 : 20});
    m_keyer->setDahNotes({reversed ? 20 : 21});
    // WHY only on a change: KP arrives again whenever the weight or iambic mode moves, which must
    // not cut a burst in progress. A real swap must, or a held lever would change meaning mid-key.
    if (changed)
        m_keyer->releasePaddles();
}

void CwController::updateInhibit() {
    const auto mode = m_radioState->mode();
    const bool inCw = mode == RadioState::CW || mode == RadioState::CW_R;
    const bool inhibited = !m_radioReady || !inCw || m_kpodPlusGate || m_vailSelected->load(std::memory_order_acquire);
    if (m_softwareKeyingAllowed->load(std::memory_order_acquire) != !inhibited) {
        m_softwareKeyingAllowed->store(false, std::memory_order_release);
        m_softwareKeyingGeneration->fetch_add(1, std::memory_order_acq_rel);
        m_softwareKeyingAllowed->store(!inhibited, std::memory_order_release);
    }
    m_keyer->setInhibited(inhibited);
    updateVailKeying();
}

void CwController::onPaddle(bool dah, bool asserted, double deltaTime) {
    if (!m_cachedIsV14.load(std::memory_order_acquire))
        return; // MIDI reaches the keyer through the raw sink
    if (m_reversed.load(std::memory_order_acquire))
        dah = !dah;
    if (dah)
        m_keyer->paddleEvent(asserted ? PaddleKeyer::Event::DahDown : PaddleKeyer::Event::DahUp, deltaTime);
    else
        m_keyer->paddleEvent(asserted ? PaddleKeyer::Event::DitDown : PaddleKeyer::Event::DitUp, deltaTime);
}

void CwController::setKeyDownInitialDelay(int ms) {
    m_connection->sendCAT(QString("KZL%1;").arg(qBound(0, ms, 1000), 4, 10, QChar('0')));
    m_connection->sendCAT(QStringLiteral("KZL;"));
}

void CwController::stopKeying() {
    m_keyer->releasePaddles();
    if (m_vailSelected->load(std::memory_order_acquire)) {
        m_halikey->setVailKeyingEnabled(false);
        updateVailSidetone(true);
        updateVailKeying();
    }
}

void CwController::setKpodPlusGate(bool active) {
    // The single most useful line in a CW bench log: it says who is generating the elements.
    qCInfo(cwKeyer) << "KPOD+ keyer gate"
                    << (active ? "UP - the KPOD+ owns CW; QK4's keyer keys nothing"
                               : "DOWN - QK4's own keyer owns CW again");
    // Invariant 5: the store first, then the inhibit (which releases anything keyed).
    m_connection->setKpodPlusKeyerActive(active);
    m_kpodPlusGate = active;
    updateInhibit();
}

void CwController::wireKeyingRoutes() {
    auto *tc = m_connection->tcpClient();
    auto *cc = m_connection;
    const auto vailSelected = m_vailSelected;
    const auto vailControl = m_halikey->vailRoutingControl();
    const auto vailRoute = std::make_shared<VailCatRouting::Route>();
    const auto softwareAllowed = m_softwareKeyingAllowed;
    const auto softwareGeneration = m_softwareKeyingGeneration;
    const auto softwareMutex = std::make_shared<std::mutex>();
    const auto softwareDownGeneration = std::make_shared<quint64>(0);
    m_link->setSender([tc, softwareAllowed, softwareGeneration, softwareMutex, softwareDownGeneration,
                       vailRoute](const std::string &bytes) {
        std::lock_guard lock(*softwareMutex);
        const auto command = QString::fromLatin1(bytes);
        auto generation = softwareGeneration->load(std::memory_order_acquire);
        if (command.startsWith(QLatin1String("KZD")))
            *softwareDownGeneration = generation;
        else if (command.startsWith(QLatin1String("KZU")))
            generation = *softwareDownGeneration;
        const bool eligibleAtEmission = softwareAllowed->load(std::memory_order_acquire);
        QMetaObject::invokeMethod(
            tc,
            [tc, softwareAllowed, softwareGeneration, vailRoute, command, generation, eligibleAtEmission]() {
                const bool eligible = eligibleAtEmission && softwareAllowed->load(std::memory_order_acquire) &&
                                      generation == softwareGeneration->load(std::memory_order_acquire);
                if (tc->isConnected() && vailRoute->acceptSoftware(command, generation, eligible))
                    tc->sendCAT(command);
            },
            Qt::QueuedConnection);
    });
    const auto computerSidetone = m_vailComputerSidetone;
    const auto sidetoneGeneration = m_vailSidetoneGeneration;
    const auto sidetoneGate = m_vailSidetoneGate;
    const auto sidetoneRoute = m_vailSidetoneRoute;
    auto *sg = m_sidetone;
    connect(
        m_halikey, &HalikeyDevice::routedVailKeyingStateChanged, sg,
        [sg, cc, vailSelected, computerSidetone, vailControl, sidetoneGeneration, sidetoneGate,
         sidetoneRoute](bool down, quint64 epoch, qint64 timestampNs) {
            // Capture the feedback generation on the emitting worker thread, before
            // audio queuing. A rapid source switch cannot revive an old queued DOWN.
            const quint64 generation = sidetoneGeneration->load(std::memory_order_acquire);
            const bool eligibleAtEmission = sidetoneGate->load(std::memory_order_acquire);
            QMetaObject::invokeMethod(
                sg,
                [sg, cc, vailSelected, computerSidetone, vailControl, sidetoneGeneration, sidetoneGate, sidetoneRoute,
                 down, epoch, generation, timestampNs, eligibleAtEmission]() {
                    const bool eligible = eligibleAtEmission && sidetoneGate->load(std::memory_order_acquire) &&
                                          vailSelected->load(std::memory_order_acquire) &&
                                          computerSidetone->load(std::memory_order_acquire) &&
                                          !cc->isKpodPlusKeyerActive();
                    if (sidetoneRoute->accept(down, epoch, generation,
                                              sidetoneGeneration->load(std::memory_order_acquire), *vailControl,
                                              eligible))
                        sg->setKeyDownAt(down, timestampNs);
                },
                Qt::QueuedConnection);
        },
        Qt::DirectConnection);
    connect(
        m_halikey, &HalikeyDevice::routedVailKeyingCommand, tc,
        [tc, cc, vailSelected, vailControl, vailRoute](const QString &command, quint64 epoch) {
            const bool eligible = vailSelected->load(std::memory_order_acquire) && !cc->isKpodPlusKeyerActive();
            if (tc->isConnected() && vailRoute->accept(command, epoch, *vailControl, eligible))
                tc->sendCAT(command);
        },
        Qt::QueuedConnection);
    // Reset on the I/O thread itself, before a blocked GUI can process disconnect.
    // Old UP/RX cleanup must never target a newly authenticated radio connection.
    connect(
        tc, &TcpClient::stateChanged, tc,
        [vailControl, vailRoute, sidetoneGeneration, sidetoneGate, softwareAllowed,
         softwareGeneration](TcpClient::ConnectionState state) {
            if (state != TcpClient::Connected) {
                softwareAllowed->store(false, std::memory_order_release);
                softwareGeneration->fetch_add(1, std::memory_order_acq_rel);
                vailControl->invalidate();
                vailRoute->reset();
                sidetoneGate->store(false, std::memory_order_release);
                sidetoneGeneration->fetch_add(1, std::memory_order_acq_rel);
            }
        },
        Qt::DirectConnection);
    // A queued receiver context keeps teardown safe after the audio object is
    // destroyed. The I/O-side gate above closes before a stalled GUI can react.
    connect(
        tc, &TcpClient::stateChanged, sg,
        [sg, vailSelected, sidetoneGate, sidetoneRoute](TcpClient::ConnectionState state) {
            if (state != TcpClient::Connected && vailSelected->load(std::memory_order_acquire) &&
                !sidetoneGate->load(std::memory_order_acquire)) {
                sidetoneRoute->reset();
                sg->resetKeyedTone();
                sg->setKeyDown(false);
            }
        },
        Qt::QueuedConnection);
    connect(m_halikey, &HalikeyDevice::connected, this, [this]() {
        configureVailKeyer();
        updateVailKeying();
    });
}

void CwController::configureVailKeyer() {
    m_halikey->configureVailKeyer(m_radioState->keyerSpeed() > 0 ? m_radioState->keyerSpeed() : 20,
                                  m_radioState->cwPitch() > 0 ? m_radioState->cwPitch() : 600,
                                  m_radioState->iambicMode() == 'B' ? 1 : 0);
}

void CwController::updateVailKeying() {
    const bool inCw = m_radioState->mode() == RadioState::CW || m_radioState->mode() == RadioState::CW_R;
    // Queue audio reset/priming before a newly enabled worker can emit DOWN.
    updateVailSidetone();
    m_halikey->setVailKeyingEnabled(m_radioReady && inCw && !m_kpodPlusGate);
}

void CwController::updateVailSidetone(bool forceReset) {
    const bool inCw = m_radioState->mode() == RadioState::CW || m_radioState->mode() == RadioState::CW_R;
    const bool allowed = m_vailSelected->load(std::memory_order_acquire) &&
                         m_vailComputerSidetone->load(std::memory_order_acquire) && m_halikey->isConnected() &&
                         m_radioReady && inCw && !m_kpodPlusGate;
    if (!forceReset && allowed == m_vailSidetoneAllowed &&
        allowed == m_vailSidetoneGate->load(std::memory_order_acquire))
        return;
    m_vailSidetoneAllowed = allowed;
    // Close eligibility until the reset is enqueued. An edge racing a source
    // change cannot enter the audio queue ahead of the reset for its generation.
    m_vailSidetoneGate->store(false, std::memory_order_release);
    const quint64 generation = m_vailSidetoneGeneration->fetch_add(1, std::memory_order_acq_rel) + 1;
    QMetaObject::invokeMethod(
        m_sidetone,
        [sg = m_sidetone, route = m_vailSidetoneRoute, generations = m_vailSidetoneGeneration,
         selected = m_vailSelected, generation]() {
            if (!route->resetIfCurrent(generation, generations->load(std::memory_order_acquire)))
                return;
            sg->resetKeyedTone();
            if (selected->load(std::memory_order_acquire))
                sg->setKeyDown(false);
        },
        Qt::QueuedConnection);
    m_vailSidetoneGate->store(allowed, std::memory_order_release);
}
