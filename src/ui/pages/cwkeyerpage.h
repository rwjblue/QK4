#ifndef CWKEYERPAGE_H
#define CWKEYERPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

class HalikeyDevice;
class RadioState;
class CwController;

/**
 * @brief OptionsDialog "CW Keyer" tab. Chooses the HaliKey device type (V1.4 / MIDI / etc.) and
 *        serial port, tests connection, and tunes the sidetone volume. Forwards configuration to
 *        HalikeyDevice via HardwareController.
 */
class CwKeyerPage : public QWidget {
    Q_OBJECT

public:
    CwKeyerPage(HalikeyDevice *halikeyDevice, RadioState *radioState, CwController *cwController,
                QWidget *parent = nullptr);

    void refresh();

private slots:
    void onCwKeyerConnectClicked();
    void onCwKeyerRefreshClicked();
    void updateCwKeyerStatus();

private:
    void populateCwKeyerPorts();
    void updateCwKeyerDescription();
    void showKeyDownDelay(int ms);
    void sendKeyDownDelay();
    void updateSidetoneControls();

    HalikeyDevice *m_halikeyDevice;
    QComboBox *m_cwKeyerDeviceTypeCombo = nullptr;
    QLabel *m_cwKeyerDescLabel = nullptr;
    QComboBox *m_cwKeyerPortCombo = nullptr;
    QPushButton *m_cwKeyerRefreshBtn = nullptr;
    QPushButton *m_cwKeyerConnectBtn = nullptr;
    QLabel *m_cwKeyerStatusLabel = nullptr;
    QSlider *m_sidetoneVolumeSlider = nullptr;
    QLabel *m_sidetoneVolumeValueLabel = nullptr;
    QWidget *m_halikeyKeyerSettings = nullptr;
    RadioState *m_radioState;
    CwController *m_cwController;
    QSlider *m_keyDownDelaySlider = nullptr;
    QLabel *m_keyDownDelayValueLabel = nullptr;
    QWidget *m_sidetoneSourceRow = nullptr;
    QComboBox *m_sidetoneSourceCombo = nullptr;
    QWidget *m_sidetoneVolumeRow = nullptr;
    QLabel *m_sidetoneHelpLabel = nullptr;
};

#endif // CWKEYERPAGE_H
