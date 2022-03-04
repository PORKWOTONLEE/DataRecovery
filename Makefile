TARGET1=datarec.exe
TARGET2=datarec_client.exe
DATAREC_SRC=*.c ./log/*.c ./server/*.c ./handler_component/*.c
DATAREC_CLIENT_SRC=./client/client.c
TRD_INC=-Iserver/civetweb/include -Iserver/cjson/include -Ilibtestdisk/include -Ilibtestdisk/ntfsprogs/include/ntfs
TRD_LIB=-Lserver/civetweb/lib -Lserver/cjson/lib -Llibtestdisk/lib -L libtestdisk/ntfsprogs/libntfs/.libs -lcivetweb -lcjson -ltestdisk -lntfs
VERBOSE=> /dev/null 2>&1

$(TARGET1):${DATAREC_SRC}
	@echo "Builing Target $(TARGET1)"
	@cp /bin/cyggcc_s-1.dll ./
	@cp /bin/cygwin1.dll ./
	@cp ./libtestdisk/ntfsprogs/libntfs/.libs/cygntfs-10.dll ./
	@gcc -o $(TARGET1) $^ ${TRD_INC} ${TRD_LIB} -Wl,--enable-stdcall-fixup
	@echo "Builing Target Success"

$(TARGET2):${DATAREC_CLIENT_SRC}
	@echo "Builing Target $(TARGET2)"
	@gcc -o $(TARGET2) $^ ${TRD_INC} ${TRD_LIB}
	@echo "Builing Target Success"

.PHONY:verbose clean all 

verbose:
	@VERBOSE=
	@make all

clean:
	@echo "Cleaning"
	@rm -rf *.exe
	@rm -rf datarec.log
	@rm -rf cyggcc_s-1.dll
	@rm -rf cygwin1.dll
	@rm -rf cygntfs-10.dll
	@echo "Cleaning Done"

all:$(TARGET1) $(TARGET2)

