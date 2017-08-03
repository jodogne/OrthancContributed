# MongoDB plugins for Orthanc DICOM Server

See: https://github.com/Doc-Cirrus/orthanc-mongodb

## OVERVIEW
The repository contains two plugins to store the data in MongoDB database.

## Installation

This chapter describes the process of installation with not too much details and not necessarily contain all how-to to resolve possible problems that might appear.

### Prerequisites
- Install boost (compile or with package manager)
- Install jsoncpp
- Install/build mongoc library http://mongoc.org/libmongoc/current/installing.html
- Install/build mongo-cxx lib https://mongodb.github.io/mongo-cxx-driver/mongocxx-v3/installation/
- Download Ortanc sources

### CentOS 6 Build Instructions

## General Packages
```bash
yum -y install centos-release-scl centos-release-scl-rh
yum -y install gcc gcc-c++ devtoolset-4 cmake boost-devel libuuid-devel openssl-devel cyrus-sasl-devel
```

## Prerequisite: Orthanc 1.2.0
Its not nessesary to build Orthanc, if you have orthanc-devel packages installed. At the moment there exists no package for Orthanc 1.2.0 in any offical repository for CentOS 6.
```bash
yum -y install python curl-devel dcmtk-devel gtest-devel jsoncpp-devel libjpeg-devel libpng-devel sqlite-devel lua-devel >= 5.1.0 mongoose-devel openssl-devel pugixml-devel
curl -L --output Orthanc-1.2.0.tar.gz http://www.orthanc-server.com/downloads/get.php?path=/orthanc/Orthanc-1.2.0.tar.gz
tar -xzf Orthanc-1.2.0.tar.gz
mkdir Orthanc-1.2.0/build
cd Orthanc-1.2.0/build
cmake .. \
	-DCMAKE_BUILD_TYPE=Release \
	-DDCMTK_LIBRARIES=CharLS \
	-DSTANDALONE_BUILD:BOOL=ON \
	-DSTATIC_BUILD:BOOL=ON \
	-DALLOW_DOWNLOADS:BOOL=ON \
	-DUSE_SYSTEM_SQLITE:BOOL=OFF \
	-DUSE_SYSTEM_BOOST:BOOL=OFF \
	-DUSE_SYSTEM_CURL:BOOL=OFF \
	-DSYSTEM_MONGOOSE_USE_CALLBACKS=OFF \
	-DUNIT_TESTS_WITH_HTTP_CONNEXIONS=OFF
make
sudo make install
```

## Prerequisite: Mongo C Driver
```bash
curl -L --output mongo-c-driver-1.6.0.tar.gz https://github.com/mongodb/mongo-c-driver/releases/download/%{mongocversion}/mongo-c-driver-1.6.0.tar.gz
tar -xzf mongo-c-driver-1.6.0.tar.gz
cd mongo-c-driver-1.6.0
./configure --enable-static --with-pic --disable-automatic-init-and-cleanup
make
sudo make install
```

## Prerequisite: cmake 3.2+
```bash
curl -L --output cmake-3.7.2.tar.gz https://cmake.org/files/v3.7/cmake-3.7.2.tar.gz
tar -xzf cmake-3.7.2.tar.gz
cd cmake-3.7.2
scl enable devtoolset-4 "./configure"
scl enable devtoolset-4 "make"
scl enable devtoolset-4 "sudo make install"
```

## Prerequisite: boost 1.60+
```bash
curl -L --output boost_1_63_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.63.0/boost_1_63_0.tar.gz/download
tar -xzf boost_1_63_0.tar.gz
cd boost_1_63_0
./bootstrap.sh
# following operations may exit 1 when some of the targets fail
./b2 cflags=-fPIC -j 2 || :
sudo ./b2 install || :
```

## Prerequisite: MongoDB C++ Driver
```bash
curl -L --output r3.1.1.tar.gz https://github.com/mongodb/mongo-cxx-driver/archive/r3.1.1.tar.gz
tar -xzf r3.1.1.tar.gz
mkdir -p mongo-cxx-driver-r3.1.1/build
cd mongo-cxx-driver-r3.1.1/build
scl enable devtoolset-4 "export CXXFLAGS=-fPIC && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DLIBBSON_DIR=/usr/local -DLIBMONGOC_DIR=/usr/local .."
# for any reason it requires write permissions to /usr/local/include/bsoncxx/v_noabi/bsoncxx/third_party/mnmlstc/share/cmake/core
# use sudo for make too in this case
scl enable devtoolset-4 "sudo make"
scl enable devtoolset-4 "sudo make install"
```

## Prerequisite: JsonCpp
```bash
curl -L --output jsoncpp-1.8.0.tar.gz https://github.com/open-source-parsers/jsoncpp/archive/1.8.0.tar.gz
tar -xzf jsoncpp-1.8.0.tar.gz
mkdir -p jsoncpp-1.8.0/build
cd jsoncpp-1.8.0/build
scl enable devtoolset-4 "export CXXFLAGS=-fPIC && cmake -DCMAKE_BUILD_TYPE=Release .."
scl enable devtoolset-4 "make"
scl enable devtoolset-4 "sudo make install"
```

## Build of this orthanc-mongodb plugin itself
```bash
mkdir -p orthanc-mongodb/build
cd orthanc-mongodb/build
scl enable devtoolset-4 "export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig && cmake -DCMAKE_BUILD_TYPE=Release -DORTHANC_ROOT=/usr/include .."
scl enable devtoolset-4 "make"
```

* ```ORTHANC_ROOT``` - the Orthanc server sources root to include the ```orthanc/OrthancCPlugin.h```
* ```BUILD_TESTS``` - option to build tests, default off
* ```BUILD_WITH_GCOV``` - option to include coverage default off

### Plugin configuration setup

Add plugins in the Ortahc json configuration file:

```json
    ...
  "Plugins" : [
    "libOrthancMongoDBIndex.dylib",
    "libOrthancMongoDBStorage.dylib"
  ],
  // MongoDB plugin confihuration section:
  "MongoDB" : {
    "EnableIndex" : true, // false to use default SQLite 
    "EnableStorage" : true, // false to use default SQLite 
    "ConnectionUri" : "mongodb://localhost:27017/orthanc_db",
    "ChunkSize" : 261120
  },
  ...
```

Also it's possible to configure the plugin with separate config options:

```json
    "Plugins" : [
    "libOrthancMongoDBIndex.dylib",
    "libOrthancMongoDBStorage.dylib"
    ],
    "MongoDB" : {
        "host" : "customhost",
        "port" : 27001,
        "user" : "user",
        "database" : "database",
        "password" : "password",
        "authenticationDatabase" : "admin",
        "ChunkSize" : 261120
    }
```

**NOTE: Setting up the ConnectionUri overrides the host, port, database params. So if the ConnectionUri is set, the other parameters except the ChunkSize will be ignored.**

Testing is described with more details in [here](doc/testing.md)

## Known Issues:

1. ConnectionUri must contain the database name due to the [bug in the mongocxx driver CXX-1187](https://jira.mongodb.org/browse/CXX-1187)
2. The Orthanc server should be compiled in Release or RelWithDebInfo mode to turn off assertions. There are asserts like that 
   `./OrthancServer/ServerIndex.cpp:      assert(index_.currentStorageSize_ == index_.db_.GetTotalCompressedSize());``` it involves full collection scan 
   to aggregate the total AttachedFiles significantly slows the performance down. 



