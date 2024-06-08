#pragma once
#include <QMap>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QWidget>
#include <mbrtuclient.h>
#include <wsmodbusrtu.h>

/**
 * @brief The relay / digital input driver class for Waveshare Modbus RTU (D)
 */
class WSRelayDigInMbRtu: public WSModbusRtu
{
    Q_OBJECT

public:
    enum TRelayFunction {
        UpdateRelay = RtuCustomStart + 0x0101,
        ReadRelayStatus = RtuCustomStart + 0x0102,
        ReadDigitalInput = RtuCustomStart + 0x0103,
        WriteRelayStatus = RtuCustomStart + 0x0104,
        WriteRelayMask = RtuCustomStart + 0x0105,
        ReadControlMode = RtuCustomStart + 0x0106,
        WriteControlMode = RtuCustomStart + 0x0107,
        WriteControlModes = RtuCustomStart + 0x0108,
        SetFlashOnInterval = RtuCustomStart + 0x0109,
        SetFlashOffInterval = RtuCustomStart + 0x0110,
    };
    Q_ENUM(TRelayFunction)

    enum TControlMode {
        NormalMode,
        LinkageMode,
        ToggleMode,
    };
    Q_ENUM(TControlMode)

    explicit WSRelayDigInMbRtu(MBRtuClient* modbus, QObject* parent = nullptr);

    ~WSRelayDigInMbRtu();

    const char* id() const override;
    quint8 maxInputs() const override;
    quint8 maxOutputs() const override;
    QWidget* settingsWidget(QWidget* parent) override;

    void setRelayStatus(const quint8 relay, const bool state);
    void setAllRelays(const quint8 mask);
    void setControlModes(const QMap<quint8, TControlMode>& modes, bool updateDevice = false);
    void setControlMode(quint8 channel, const TControlMode mode, bool updateDevice = false);

    bool relayStatus(const quint8 relay) const;
    TControlMode controlMode(const quint8 relay) const;

    bool digitalInput(const quint8 channel) const;

signals:
    void relayChanged(quint8 relay, bool state);
    void inputChanged(quint8 channel, bool state);
    void modeChanged(quint8 channel, WSRelayDigInMbRtu::TControlMode mode);

protected:
    void startStatusWorker() override;
    void doFunction(uint function) override;
    void doModbusOpened() override;
    bool doMduCoils(const QModbusDataUnit& unit) override;
    bool doMduDiscreteInputs(const QModbusDataUnit& unit) override;
    bool doMduInputRegisters(const QModbusDataUnit& unit) override;
    bool doMduHoldingRegisters(const QModbusDataUnit& unit) override;

private:
    /* holds current relay control mode */
    QMap<quint8, TControlMode> m_control;
    /* holds current relay state */
    QMap<quint8, bool> m_relays;
    /* holds current digital input state */
    QMap<quint8, bool> m_dinputs;

private:
    inline void readRelayStatus();
    inline void readInputStatus();
    inline void readControlModes();
};

Q_DECLARE_METATYPE(WSRelayDigInMbRtu::TRelayFunction)
Q_DECLARE_METATYPE(WSRelayDigInMbRtu::TControlMode);
