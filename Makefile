TARGET1=datarec.exe
TARGET2=datarec_client.exe
DATAREC_SRC=*.c ./log/*.c ./server/*.c ./handler_component/*.c
DATAREC_CLIENT_SRC=./client/client.c
TRD_INC=-Iserver/civetweb/include -Iserver/cjson/include -Ilibdatarec/include -Ilibdatarec/3rd_lib/ntfsprogs/include/ntfs -Ilibdatarec/libjpeg/include -Ilibdatarec/libewf/include
TRD_LIB=-Lserver/civetweb/lib -Lserver/cjson/lib -Llibdatarec/lib -Llibdatarec/3rd_lib/ntfsprogs/lib -Llibdatarec/3rd_lib/libjpeg/lib -Llibdatarec/3rd_lib/libewf/lib -lcivetweb -lcjson -ldatarec -lntfs -ljpeg -lewf -lz
VERBOSE=> /dev/null 2>&1

$(TARGET1):${DATAREC_SRC}
	@echo "Builing Target $(TARGET1)"
	@cp /bin/cyggcc_s-1.dll ./
	@cp /bin/cygwin1.dll ./
	@cp /bin/cygz.dll ./
	@cp ./libdatarec/3rd_lib_src/ntfsprogs/libntfs/.libs/cygntfs-10.dll ./
	@gcc -g -O0 -o $(TARGET1) $^ ${TRD_INC} ${TRD_LIB} -Wl,--stack=52428800,--enable-stdcall-fixup
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
	@rm -rf *.dll
	@echo "Cleaning Done"

all:$(TARGET1) $(TARGET2)
