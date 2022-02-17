TARGET=datarec.exe
DATAREC_SRC=*.c ./log/*.c ./server/*.c ./handler_component/*.c
TRD_INC=-Iserver/civetweb/include -Iserver/cjson/include -I./libtestdisk/include
TRD_LIB=-Lserver/civetweb/lib -Lserver/cjson/lib -L./libtestdisk/lib -lcivetweb -lcjson -ltestdisk
VERBOSE=> /dev/null 2>&1

$(TARGET):${DATAREC_SRC}
	@echo "Builing Target $(TARGET)"
	@cp /bin/cyggcc_s-1.dll ./
	@cp /bin/cygwin1.dll ./
	@gcc -o $(TARGET) $^ ${TRD_INC} ${TRD_LIB} ${VERBOSE}
	@echo "Builing Target Success"

.PHONY:verbose clean

verbose:
	@echo "Builing Target $(TARGET)"
	@cp /bin/cyggcc_s-1.dll ./
	@cp /bin/cygwin1.dll ./
	@gcc -o $(TARGET) ${DATAREC_SRC} ${TRD_INC} ${TRD_LIB}
	@echo "Builing Target Success"

clean:
	@echo "Cleaning"
	@rm -rf datarec.exe
	@rm -rf datarec.log
	@rm -rf cyggcc_s-1.dll
	@rm -rf cygwin1.dll
	@echo "Cleaning Done"


