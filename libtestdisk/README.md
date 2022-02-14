# libtestdisk库构建文档

## 主要目的

将testdisk源码构建成静态库，以便在我们自己的程序中调用testdisk功能函数



## 实现思路

修改testdisk源码原先的Makefile.am文件，使得Makefile生成的目标不再是可执行文件，而是库函数



## 具体步骤

### 1. 环境选择

使用windows下cygwin的32位环境进行开发

下载地址：https://www.cygwin.com/setup-x86.exe

安装完毕后，cygwin terminal默认启动路径是C:/cygwin/home/你的用户名

【注意】自行在cygwin中安装gcc，git，vim，autoconf，automake等常用软件



### 2. 下载所需源码

* 创建工程目录

  打开cygwin terminal
  
```shell
mkdir Project
```

* 获取源码主要有：

  * testdisk源码：通过github：

  ```shell
  git clone git@github.com:cgsecurity/testdisk.git
  ```

  * ntfsprogs源码：通过网站:

  ```http
  https://sourceforge.net/projects/linux-ntfs/files/NTFS%20Tools%20and%20Library/2.0.0/
  ```

  【注意】下载完成后需要自行解压，改名后，放置到Project文件夹下


* 创建lib文件夹

```shell
mkdir lib
```

用于存放libtestdisk.a文件

* 此时的文件夹结构如下：

​       Project
​           ├── lib
​           ├── ntfsprogs
​           └── testdisk
​    

### 3. 构建ntfsprogs库

* 进入ntfsprogs文件夹

```shell
cd ntfsprogs
```

* 修改autogen.sh和getgccver文件的权限，防止后面编译出错
```shell
chmod 777 autogen.sh
chmod 777 getgccver
```

* 生成configure文件

```shell
./autogen.sh
```

【注意】执行前，确保cygwin中有libtool和libgcrypt库，否则会报错

* 将/usr/include/w32api/minwindef.h文件中的BOOL类型注释掉，防止构建时遇到数据BOOL类型报错

```shell
vim /usr/include/w32api/minwindef.h +131 # +131：我的BOOL类型定义在131行，需要根据实际添加此参数
```

【注意】构建完成后，记得取消此注释

* 构建库

```shell
make
```

构建完毕后，得到libntfs.a库文件

库文件路径：Project\ntfsprogs\libntfs\.libs\libntfs.a
头文件路径：Project\ntfsprogs\include



### 4. 构建libtestdisk库

* 进入testdisk文件夹

```shell
cd testdisk
```

* 修改configure.ac文件

在以下内容之后

```shell
\# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
```

添加

```shell
AM_PROG_AR     
AC_PROG_RANLIB
```

用于构建库和更新库文件的符号表

* 修改src/Makefile.am文件

注释以下内容，防止其他无用文件生成

```shell
bin_PROGRAMS          = testdisk photorec fidentify $(QPHOTOREC)
EXTRA_PROGRAMS        = photorecf fuzzerfidentify
```

并在这两行后面添加以下内容，以构建库

```shell
noinst_LIBRARIES        = testdisk.a
```

* 修改src/testdisk.c文件中的main函数

将main函数返回值由

```c
int main( int argc, char **argv )
```

改为

```c
static int main( int argc, char **argv )
```

防止库文件中含有有main函数接口，导致重定义

* 构建testdisk.a

重新生成configure文件

```shell
autoreconf --verbose --install --force
```

重新生成config.h文件

```shell
autoheader
```

重新生成Makefile.in文件

```shell
automake --add-missing
```

生成Makefile文件

```shell
./configure --with-ntfs-includes=../ntfsprogs/include --with-ntfs-lib=../ntfsprogs/libntfs/.libs --enable-missing-uuid-ok --without-ncurses --disable-qt
```

构建库

```shell
make
```



### 5.移动库文件到lib文件夹

* 移动库文件

```shell
mv ./testdisk/src/testdisk.a ./lib/libtestdisk.a
```



## 编译脚本

以上重点步骤，可以通过libtestdisk文件夹中的make.sh脚本自动生成

