QT       += core gui
QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += multimedia
CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    lobby.cpp \
    login.cpp \
    main.cpp \
    manager.cpp \
    pet.cpp \
    spiritdialog.cpp \
    spiritinfodialog.cpp \
    widget.cpp

HEADERS += \
    define_settings.h \
    lobby.h \
    login.h \
    manager.h \
    pet.h \
    spiritdialog.h \
    spiritinfodialog.h \
    widget.h

FORMS += \
    lobby.ui \
    login.ui \
    spiritdialog.ui \
    spiritinfodialog.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    background.qrc \
    spirit.qrc

CONFIG += moc
