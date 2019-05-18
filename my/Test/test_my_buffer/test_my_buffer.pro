TEMPLATE = app
CONFIG += console c++98
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++98

SOURCES += \
    ../../Tests/my_buffer_test/main.cpp

HEADERS += \
    ../../include/my_cpp_versions.hpp \
    ../../include/my_debug.h \
    ../../include/my_membuf.h

DISTFILES += \
    ../../include/my_buffer.h.old
