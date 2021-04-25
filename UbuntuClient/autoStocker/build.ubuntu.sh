#!/bin/bash

GetArch()
{
    echo $(uname -m)
}

BASE_PATH=${PWD}
MACHIN_TYPE=$(GetArch)

IOTBUILD_MODE=release
IOT_PATH=${BASE_PATH}/../iotivity
IOT_OUTPUT=${IOT_PATH}/out/linux/${MACHIN_TYPE}/${IOTBUILD_MODE}

ST_PATH=${BASE_PATH}
STIOT_PATH=${ST_PATH}/inc/iotivity
STIOT_LIB=${ST_PATH}/lib/iotivity

echo "IOT_OUTPUT=${IOT_OUTPUT}"

#############################################
# copy iotivity header files to App folder  #
#############################################
mkdir -p ${STIOT_PATH}/c_common
mkdir -p ${STIOT_PATH}/logger/include
mkdir -p ${STIOT_PATH}/stack/include
mkdir -p ${STIOT_PATH}/security/include
mkdir -p ${STIOT_PATH}/extlibs/cjson
mkdir -p ${STIOT_PATH}/extlibs/boost/boost
mkdir -p ${STIOT_PATH}/oc_logger/include
mkdir -p ${STIOT_PATH}/connectivity/lib/libcoap-4.1.1/include

cp -Rdp ${IOT_PATH}/resource/c_common/*  ${STIOT_PATH}/c_common/
cp -Rdp ${IOT_PATH}/resource/csdk/logger/include/*  ${STIOT_PATH}/logger/include/
cp -Rdp ${IOT_PATH}/resource/csdk/stack/include/*  ${STIOT_PATH}/stack/include/
cp -Rdp ${IOT_PATH}/resource/csdk/security/include/*  ${STIOT_PATH}/security/include/
cp -Rdp ${IOT_PATH}/extlibs/cjson/*         ${STIOT_PATH}/extlibs/cjson/
#cp -Rdp ${IOT_PATH}/extlibs/boost/boost/*   ${STIOT_PATH}/extlibs/boost/boost/
cp -Rdp ${IOT_PATH}/resource/oc_logger/include/*  ${STIOT_PATH}/oc_logger/include/
cp -Rdp ${IOT_PATH}/resource/csdk/connectivity/lib/libcoap-4.1.1/include/*  ${STIOT_PATH}/connectivity/lib/libcoap-4.1.1/include/


mkdir -p ${STIOT_LIB}

cp -Rdp ${IOT_OUTPUT}/*.a                 ${STIOT_LIB}/
cp -Rdp ${IOT_OUTPUT}/*.so                ${STIOT_LIB}/
#cp -Rdp ${IOT_OUTPUT}/liboctbstack.a                 ${STIOT_LIB}/
#cp -Rdp ${IOT_OUTPUT}/libocsrm.a                     ${STIOT_LIB}/
#cp -Rdp ${IOT_OUTPUT}/libconnectivity_abstraction.a  ${STIOT_LIB}/
#cp -Rdp ${IOT_OUTPUT}/libcoap.a                      ${STIOT_LIB}/
#cp -Rdp ${IOT_OUTPUT}/libtinydtls.a                  ${STIOT_LIB}/


if eval scons VERBOSE=True RELEASE=0; then
    echo "Success Build.(${MACHIN_TYPE})"
else
    echo "Failed Build. (${MACHIN_TYPE})"
    exit 1
fi

exit 0 
