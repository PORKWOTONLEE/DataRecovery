#!/bin/bash

ROOT_DIR=`pwd`
LIB_DIR="${ROOT_DIR}/lib"
NTFSPROGS_DIR="${ROOT_DIR}/ntfsprogs"
TESTDISK_DIR="${ROOT_DIR}/testdisk"

TESTDISK_CONFIGURE="./configure --with-ntfs-includes=../ntfsprogs/include --with-ntfs-lib=../ntfsprogs/libntfs/.libs --enable-missing-uuid-ok --without-ncurses --disable-qt"

# 文件存在判断
function FileExist() 
{
    if [ -f "$1" ]
    then
        echo -e ":Success"
    else
        echo -e ":Fail"
        echo -e "-Please Build By Yourself,According To Error Info"
        exit
    fi

    return 0
}

# 文件不存在判断
function FileNotExist() 
{
    if [ ! -f "$1" ]
    then
        echo -e ":Success"
    else
        echo -e ":Fail"
        echo -e "-Please Build By Yourself,According To Error Info"
        exit
    fi

    return 0
}

# 主功能函数
echo -e "【libdatarec Building Script v1.4】"

echo -e "\n【Prepare Task】"
echo -e "-Ignore libdatarec.a Change For Git\n-【Y/N Default:Y】\c"
read IsIgnoreLibtestdisk_a
if [ ${IsIgnoreLibtestdisk_a} == "N" ] > /dev/null 2>&1
then
	echo -e "-Don't Ignore libdatarec.a For Git"
	git update-index --no-assume-unchanged ${LIB_DIR}/libdatarec.a > /dev/null 2>&1
else
	echo -e "-Ignore libdatarec.a For Git"
	git update-index --assume-unchanged ${LIB_DIR}/libdatarec.a > /dev/null 2>&1
fi

# 构建ntfsprogs库
echo -e "\n【Build libntfsprogs】"
cd ${NTFSPROGS_DIR}
if [ -f "${NTFSPROGS_DIR}/libntfs/.libs/libntfs.a" ]
then
    echo -e "-libntfsprogs Exsist\n-Wanna Rebuild？\n-【Y/N Default:N】\c"
    read IsLibNtfsReCompile
    if [ ${IsLibNtfsReCompile} == "Y" ] > /dev/null 2>&1
    then
        # 环境清理
        echo -e "-Cleaning Temporal Files\c"
        make clean > /dev/null 2>&1
        make distclean > /dev/null 2>&1
        FileNotExist ${NTFSPROGS_DIR}/Makefile
        # configure生成 & 环境配置
        echo -e "-Generating configure & Makefile\c"
        ./autogen.sh > /dev/null 2>&1
        FileExist ${NTFSPROGS_DIR}/Makefile
        # 程序构建
        echo -e "-Generating libntfsprogs\c"
        make > /dev/null 2>&1
        FileExist ${NTFSPROGS_DIR}/libntfs/.libs/libntfs.a
    else
        echo -e "-Do Nothing"
    fi
else
    # configure生成 & 环境配置
    echo -e "-Generating configure & Makefile\c"
    ./autogen.sh > /dev/null 2>&1
    FileExist ${NTFSPROGS_DIR}/Makefile
    # 程序构建
    echo -e "-Building libntfsprogs\c"
    make > /dev/null 2>&1
    FileExist ${NTFSPROGS_DIR}/libntfs/.libs/libntfs.a
fi

# 构建libdatarec库
cd ${TESTDISK_DIR}
echo -e "\n【Build libdatarec】"
if [ -f "${TESTDISK_DIR}/configure" ]
then
    if [ -f "${TESTDISK_DIR}/Makefile" ]
    then
        echo -e "-libdatarec Exsist\n-Wanna Rebuild？\n-【Y/N Default:N】\c"
        read IsLibTestdiskReCompile
        if [ ${IsLibTestdiskReCompile} == "Y" ] > /dev/null 2>&1
        then
            # 环境清理
            if [ -f "${LIB_DIR}/libdatarec.a" ]
            then
                rm -rf ${LIB_DIR}/libdatarec.a
            fi
            echo -e "-Cleaning Temporal Files\c"
            make clean > /dev/null 2>&1
            make distclean > /dev/null 2>&1
            FileNotExist ${TESTDISK_DIR}/Makefile
            # Makefile生成
            echo -e "-Generating Makefile\c"
            ${TESTDISK_CONFIGURE} > /dev/null 2>&1
            FileExist ${TESTDISK_DIR}/Makefile
            # 库构建
            echo -e "-Building libdatarec\c"
            make > /dev/null 2>&1
            make static > /dev/null 2>&1
            FileExist ${TESTDISK_DIR}/src/libdatarec.a
        else
            make > /dev/null 2>&1
            make static > /dev/null 2>&1
            echo -e "-Do Nothing"
        fi
    else
        # Makefile生成
        echo -e "-Generating Makefile\c"
        ${TESTDISK_CONFIGURE} > /dev/null 2>&1
        FileExist ${TESTDISK_DIR}/Makefile
        # 库构建
        echo -e "-Building libdatarec\c"
        make > /dev/null 2>&1
        make static > /dev/null 2>&1
        FileExist ${TESTDISK_DIR}/src/libdatarec.a
    fi
else
    # 重新生成configure文件
    echo -e "-Generating configure\c"
    autoreconf --verbose --install --force > /dev/null 2>&1
    FileExist ${TESTDISK_DIR}/configure
    # 重新生成config.h文件
    echo -e "-Generating config.h\c"
    autoheader > /dev/null 2>&1
    FileExist ${TESTDISK_DIR}/config.h.in
    # 重新生成Makefile.in文件
    echo -e "-Generating Makefile.in\c"
    automake --add-missing > /dev/null 2>&1
    FileExist ${TESTDISK_DIR}/src/Makefile.in
    # Makefile生成
    echo -e "-Generating Makefile\c"
    ${TESTDISK_CONFIGURE} > /dev/null 2>&1
    FileExist ${TESTDISK_DIR}/Makefile
    # 库构建
    echo -e "-Building libdatarec\c"
    make > /dev/null 2>&1
    make static > /dev/null 2>&1
    FileExist ${TESTDISK_DIR}/src/libdatarec.a
fi

# 移动libdatarec库文件
cd ${ROOT_DIR}
if [ ! -d "${LIB_DIR}" ]
then
    mkdir ${LIB_DIR}
fi

echo -e ""
if [ -f "${TESTDISK_DIR}/src/libdatarec.a" ]
then
    mv ${TESTDISK_DIR}/src/libdatarec.a ${LIB_DIR}/libdatarec.a
    echo -e "-libdatarec Building Complete\c"
else
    echo -e "-Do Nothing\c"
fi
