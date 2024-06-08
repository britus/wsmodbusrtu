#/*********************************************************************
# * Copyright EoF Software Labs. All Rights Reserved.
# * Copyright EoF Software Labs Authors.
# * Written by B. Eschrich (bjoern.escrich@gmail.com)
# * SPDX-License-Identifier: GPL v3
# **********************************************************************/
QT += core
QT += gui
QT += widgets
QT += network
QT += concurrent
QT += serialbus
QT += serialport
QT += dbus
QT += xml

###
TEMPLATE = app

###
CONFIG += c++17
CONFIG += sdk_no_version_check
CONFIG += nostrip
CONFIG += debug
#CONFIG += lrelease
CONFIG += embed_translations
CONFIG += create_prl
CONFIG += app_bundle

#INCLUDEPATH += $$PWD/libmodbus/src
#INCLUDEPATH += /usr/include/modbus

#LIBS += -lmodbus

SOURCES += \
	dlgadcindatatype.cpp \
	dlgrelaylinkcontrol.cpp \
	main.cpp \
	mainwindow.cpp \
	mbrtuclient.cpp \
	wsanaloginmbrtu.cpp \
	wsmodbusrtu.cpp \
	wsrelaydiginmbrtu.cpp

HEADERS += \
	dlgadcindatatype.h \
	dlgrelaylinkcontrol.h \
	mainwindow.h \
	mbrtuclient.h \
	wsanaloginmbrtu.h \
	wsmodbusrtu.h \
	wsrelaydiginmbrtu.h

RESOURCES += \
	assets.qrc

FORMS += \
	dlgadcindatatype.ui \
	dlgrelaylinkcontrol.ui \
	mainwindow.ui

TRANSLATIONS += \
	modbus-rs485-rtu-m_en_US.ts

# Default rules for deployment.
target.path = /usr/local/bin
INSTALLS += target

DISTFILES += \
	LICENSE.md \
	README.md
