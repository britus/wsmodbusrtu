/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#pragma once
#include <QMainWindow>
#include <QSettings>
#include <mbrtuclient.h>
#include <wsanaloginmbrtu.h>
#include <wsrelaydiginmbrtu.h>

namespace Ui {
class MainWindow;
}

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    enum TRelayFunction {
        Unspecified,
        UpdateRelay,
        ReadVersion,
        ReadDeviceAddr,
        WriteDeviceAddr,
        WriteUartParams,
        SetFlashOnInterval,
        SetFlashOffInterval,
        ReadControlMode,
        WriteControlMode,
        ReadRelayStatus,
        WriteRelayStatus,
        ReadDigitalInput,
    };
    Q_ENUM(TRelayFunction)

    explicit MainWindow(QWidget* parent = nullptr);

    ~MainWindow();

private slots:
    void onAppQuit();
    /* -- */
    void onRelayDriverOpend(quint8 address);
    void onRelayDriverClosed(quint8 address);
    void onRelayFunctionDone(quint8 address, uint function);
    void onRelayChanged(quint8 relay, bool state);
    void onRelayModeChanged(quint8 relay, WSRelayDigInMbRtu::TControlMode mode);
    void onDigInChanged(quint8 channel, bool state);
    /* -- */
    void onAdcDriverOpend(quint8 address);
    void onAdcDriverClosed(quint8 address);
    void onAdcFunctionDone(quint8 address, uint function);
    void onAInTypeChanged(quint8 channel, WSAnalogInMbRtu::TChannelType type);
    void onAInValueChanged(quint8 channel, float value);

private slots:
    void on_edDevAddr_valueChanged(int arg1);
    void on_pbR1_clicked();
    void on_pbR2_clicked();
    void on_pbR3_clicked();
    void on_pbR4_clicked();
    void on_pbR5_clicked();
    void on_pbR6_clicked();
    void on_pbR7_clicked();
    void on_pbR8_clicked();
    void on_pbRAll_clicked();
    void on_pbSetDevAddr_clicked();
    void on_pbSetBaudRate_clicked();
    void on_pbToggleRelays_clicked();
    void on_pbSetLinkControl_clicked();
    void on_pbSetChannelType_clicked();
    void on_pbEnableDevice_clicked();
    void on_cbDeviceList_activated(int index);
    void on_pbOpenPort_clicked();
    void on_pbClosePort_clicked();

private:
    typedef struct {
        MBRtuClient::TConfig mbconf;
        quint8 rlyAddr;
        quint8 adcAddr;
        quint8 selDev;
    } TConfig;

    Ui::MainWindow* ui;
    QSettings m_settings;
    TConfig m_config;
    MBRtuClient m_modbus;
    WSRelayDigInMbRtu* m_rly;
    WSAnalogInMbRtu* m_adc;
    QObject* m_chg;
    quint16 m_devAddress;

    inline void setRelay(quint8 relay);
    inline void loadConfig();
    inline void saveConfig();
};
