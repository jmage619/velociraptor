QT += widgets

HEADERS = window.h
SOURCES = main.cpp window.cpp
LIBS += -ljack

binary.path = /usr/local/bin
binary.files = velociraptor

doc.path = /usr/local/share/doc/velociraptor
doc.files = README.md LICENSE

INSTALLS += binary doc
