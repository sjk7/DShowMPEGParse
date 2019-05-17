TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++1z

SOURCES += \
    ../../Tests/my_buffer_test/main.cpp

HEADERS += \
    ../../include/my_buffer.h \
    ../../include/my_cpp_versions.hpp \
    ../../include/my_debug.h
