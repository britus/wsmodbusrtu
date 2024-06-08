/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#pragma once
#include <QDialog>
#include <QMap>
#include <QPushButton>
#include <wsanaloginmbrtu.h>

namespace Ui {
class DlgAdcInDataType;
}

class DlgAdcInDataType: public QDialog
{
    Q_OBJECT

public:
    explicit DlgAdcInDataType(WSAnalogInMbRtu* rtu, QWidget* parent = nullptr);

    ~DlgAdcInDataType();

    uint channelType(quint8 channel);

    void setChannelTypes(const QMap<quint8, WSAnalogInMbRtu::TChannelType>& types);
    const QMap<quint8, WSAnalogInMbRtu::TChannelType>& channelTypes() const;

private slots:
    void on_cbChannel_activated(int index);
    void on_rbMode0_clicked(bool checked);
    void on_rbMode1_clicked(bool checked);
    void on_rbMode2_clicked(bool checked);
    void on_rbMode3_clicked(bool checked);
    void on_rbMode4_clicked(bool checked);

    void on_buttonBox_accepted();

private:
    Ui::DlgAdcInDataType* ui;
    WSAnalogInMbRtu* m_rtu;
    QMap<quint8, WSAnalogInMbRtu::TChannelType> m_channelTypes;
    quint8 m_channel;
    QPushButton* m_btnSave;

    inline void setTypeOption(quint8 channel, WSAnalogInMbRtu::TChannelType type);
};
