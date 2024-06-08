/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <mainwindow.h>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString& locale : uiLanguages) {
        const QString baseName = "modbus-rs485-rtu-m_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    return a.exec();
}
