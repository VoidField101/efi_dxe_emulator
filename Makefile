
CC = gcc
CFLAGS = -I unicorn/include -I linenoise-ng/include -I capstone/include
LDFLAGS = -lstdc++ -lm
LIBS = linenoise-ng/build/liblinenoise.a capstone/libcapstone.a unicorn/libunicorn.a

SRC = efi_dxe_emulator
SRC_INIH = inih

SOURCES := ${SRC}/breakpoints.o
SOURCES += ${SRC}/capstone_utils.o
SOURCES += ${SRC}/cmds.o
SOURCES += ${SRC}/debugger.o
SOURCES += ${SRC}/efi_boot_hooks.o
SOURCES += ${SRC}/efi_runtime_hooks.o
SOURCES += ${SRC}/global_cmds.o
SOURCES += ${SRC}/loader.o
SOURCES += ${SRC}/logging.o
SOURCES += ${SRC}/mem_utils.o
SOURCES += ${SRC}/nvram.o
SOURCES += ${SRC}/protocols.o
SOURCES += ${SRC}/string_ops.o
SOURCES += ${SRC}/unicorn_hooks.o
SOURCES += ${SRC}/unicorn_utils.o

SOURCES += ${SRC_INIH}/ini.o
# SOURCES += ${SRC}/main.o

all: emulator

$(SRC)/%.o: $(SRC)/%.c
	${CC} ${CFLAGS} -o $@ -c $<

$(SRC_INIH)/%.o: $(SRC_INIH)/%.c
	${CC} ${CFLAGS} -o $@ -c $<


%: $(SRC)%.o
	$(CC) $(CFLAGS) -o $* $(LIBS) $<

emulator: ${SOURCES}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(SOURCES) $(SRC)/main.c $(LIBS)
	chmod +x emulator

clean:
	rm -f ${SRC}/*.o
	rm -f ${SRC_INIH}/*.o
	rm -f emulator
