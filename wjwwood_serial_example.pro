TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

win32: QMAKE_CFLAGS_RELEASE -= /MD
win32: QMAKE_CXXFLAGS_RELEASE -= /MD



win32: QMAKE_CFLAGS_RELEASE += /MT

TARGET = myserial

win32: LIBS += -LD:\QtProjects\serial\visual_studio\x64\Release
win32: LIBS += serial.lib
win32: LIBS += KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB COMDLG32.LIB ADVAPI32.LIB SHELL32.LIB
win32: INCLUDEPATH += D:\QtProjects\serial\include D:\QtProjects\serial\include\serial\impl
