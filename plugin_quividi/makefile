SHELL = /bin/sh
CC=g++

CFLAGS       = -std=c++11 -O0 -g3 -Wall -c -fmessage-length=0 -fPIC
CFFLAGS       = -std=c++11 -g -c -Wall -Wextra
LDFLAGS      = -shared

DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG

TARGET	= plugin_quividi.so
TARGET_X64	= plugin_quividi_x64.so
TARGET_QUIVIDI	= quividi

#all: 
#	rm -f build/*.so

x86: $(TARGET)

$(TARGET): 
	$(CC) $(CFLAGS) -m32 -o objects_x86/CInfoManager.o CInfoManager.cpp
	$(CC) $(CFLAGS) -m32 -o objects_x86/plugin_quividi.o plugin_quividi.cpp
	$(CC) $(LDFLAGS) -m32 -o build/$(TARGET) objects_x86/*.o
	@rm -f objects_x86/*.o
	@echo 

x64: $(TARGET_X64)
$(TARGET_X64): 	
	$(CC) $(CFLAGS) -m64 -o objects_x64/CInfoManager_x64.o CInfoManager.cpp
	$(CC) $(CFLAGS) -m64 -o objects_x64/plugin_quividi_x64.o plugin_quividi.cpp
	$(CC) $(LDFLAGS) -m64 -o build/$(TARGET_X64) objects_x64/*.o
	@rm -f objects_x64/*.o
	@echo 

application: $(TARGET_QUIVIDI)
$(TARGET_QUIVIDI):
	$(CC) $(CFFLAGS) -m64 -o objects_quividi_x64/quividi.o quividi.cpp
	$(CC) -m64 -pthread -o build/$(TARGET_QUIVIDI) objects_quividi_x64/quividi.o -ldl
	@rm -f objects_quividi_x64/*.o
	@echo 

install:
	cp build/$(TARGET_X64) /lib64/plugin_quividi_x64.so
	cp build/$(TARGET) /lib/plugin_quividi.so

copy:
	cp build/$(TARGET_X64) /home/viking/dev/sber_dvfu/Assets/Plugins/Quividi/x64/plugin_quividi.so
	cp build/$(TARGET) /home/viking/dev/sber_dvfu/Assets/Plugins/Quividi/x86/plugin_quividi.so
	@echo 	

