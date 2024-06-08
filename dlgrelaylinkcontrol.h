/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.escrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#pragma once
#include <QDialog>
#include <QPushButton>
#include <wsrelaydiginmbrtu.h>

namespace Ui {
class DlgRelayLinkControl;
}

class DlgRelayLinkControl: public QDialog
{
    Q_OBJECT

public:
    explicit DlgRelayLinkControl(WSRelayDigInMbRtu* rtu, QWidget* parent = nullptr);

    ~DlgRelayLinkControl();

    uint controlMode(quint8 channel);
    void setControlModes(const QMap<quint8, WSRelayDigInMbRtu::TControlMode>& modes);
    const QMap<quint8, WSRelayDigInMbRtu::TControlMode>& controlModes() const;

private slots:
    void on_cbChannel_activated(int index);
    void on_rbMode0_clicked(bool checked);
    void on_rbMode1_clicked(bool checked);
    void on_rbMode2_clicked(bool checked);
    void on_buttonBox_accepted();

private:
    Ui::DlgRelayLinkControl* ui;
    WSRelayDigInMbRtu* m_rtu;
    quint8 m_channel;
    /* holds current relay control mode */
    QMap<quint8, WSRelayDigInMbRtu::TControlMode> m_control;
    QPushButton* m_btnSave;

    inline void setModeOption(quint8 channel, WSRelayDigInMbRtu::TControlMode mode);
};
