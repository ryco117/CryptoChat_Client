#-------------------------------------------------
#
# Project created by QtCreator 2015-06-23T13:50:06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CryptoChat_Client
TEMPLATE = app
win32: DEFINES += WINDOWS
CONFIG += RELEASE
CONFIG -= DEBUG

SOURCES += main.cpp\
    mainwindow.cpp \
    crypto/AES.cpp \
    crypto/fortuna.cpp \
    crypto/curve25519-donna.c \
    CloseSocket.cpp \
    donatewindow.cpp \
    Contact.cpp \
    Conversation.cpp \
    client.cpp \
    getpasswordwidget.cpp \
    crypto/base64.cpp \
    getcontactwidget.cpp

HEADERS  += mainwindow.h \
    crypto/AES.h \
    crypto/ecdh.h \
    crypto/fortuna.h \
    donatewindow.h \
    client.h \
    getpasswordwidget.h \
    getcontactwidget.h

FORMS    += mainwindow.ui \
    donatewindow.ui \
    getpasswordwidget.ui \
    getcontactwidget.ui

linux:!android: LIBS += /usr/local/lib/libscrypt.a
linux:!android: INCLUDEPATH += /usr/local/include
linux:!android: DEPENDPATH += /usr/local/include

linux:android: LIBS += /usr/android-toolchain-21/lib/libscrypt.a
linux:android: INCLUDEPATH += /usr/android-toolchain-21/include
linux:android: DEPENDPATH += /usr/android-toolchain-21/include

QMAKE_CFLAGS_RELEASE -= -O2

QMAKE_CXXFLAGS += -static
QMAKE_CXXFLAGS += -Wno-strict-aliasing
QMAKE_CXXFLAGS += -Wno-unused-function
QMAKE_CXXFLAGS += -Wno-unused-result
QMAKE_CXXFLAGS += -Wno-char-subscripts
QMAKE_CXXFLAGS += -Wno-narrowing
QMAKE_CXXFLAGS += -Wno-maybe-uninitialized
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O0
QMAKE_CXXFLAGS += -fpermissive

QMAKE_LFLAGS_RELEASE -= -Wl,-O1
QMAKE_LFLAGS_RELEASE -= -O3

linux:!android: PRE_TARGETDEPS += /usr/local/lib/libscrypt.a
linux:android: PRE_TARGETDEPS += /usr/android-toolchain-21/lib/libscrypt.a

linux:!android: OBJECTS += ../CryptoChat_Client/crypto/AES.o
linux:!android: OTHER_FILES += \
    ../CryptoChat_Client/crypto/AES.o

win32: LIBS += -lWs2_32
win32: LIBS += -lAdvapi32
win32: LIBS += C:\Library\libscrypt.a

win32: INCLUDEPATH += $$PWD/../../../../../../Include
win32: DEPENDPATH += $$PWD/../../../../../../Include
win32: OBJECTS += ../CryptoChat_Client/crypto/AES.o
win32: OTHER_FILES += \
    ../CryptoChat_Client/crypto/AES.o

CONFIG += mobility
MOBILITY = 

RESOURCES += \
    resource.qrc

OTHER_FILES += \
    LICENSE

DISTFILES += \
    Qt Icon.png
