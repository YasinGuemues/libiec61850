# Minimal, Zephyr-native build file for libiec61850 core (no GOOSE/SV/R-*)
cmake_minimum_required(VERSION 3.20)

### Library target ###
zephyr_library()
#zephyr_library_compile_definitions(LIBIEC_ZEPHYR_DEBUG=1)


### SETTINGS ###
# FIXED STUFF!
set(CONFIG_INCLUDE_GOOSE_SUPPORT OFF)
set(CONFIG_IEC61850_L2_GOOSE OFF)
set(CONFIG_IEC61850_L2_SMV OFF)
set(CONFIG_IEC61850_R_GOOSE OFF)
set(CONFIG_IEC61850_R_SMV OFF)
set(ASN_DISABLE_PRINT_SUPPORT ON)
set(CONFIG_MAXIMUM_TCP_CLIENT_CONNECTIONS 1)
set(CONFIG_ACTIVATE_TCP_KEEPALIVE OFF)
set(CONFIG_MMS_SERVER_CONFIG_SERVICES_AT_RUNTIME OFF)
set(CONFIG_IEC61850_LOG_SERVICE OFF)
set(CONFIG_IEC61850_SAMPLED_VALUES_SUPPORT OFF)
set(CONFIG_IEC61850_FORCE_MEMORY_ALIGNMENT ON)

# MINIMIZING MEMORY FOOTPRINT
set(CONFIG_MMS_SINGLE_THREADED ON)
set(CONFIG_MMS_THREADLESS_STACK ON)
set(CONFIG_MMS_MAX_NUMBER_OF_DATA_SET_MEMBERS 64)
set(CONFIG_REPORTING_DEFAULT_REPORT_BUFFER_SIZE 2048)
set(CONFIG_COTP_MAX_TPDU_SIZE 2048)
set(CONFIG_MMS_MAXIMUM_PDU_SIZE 2048)

# SERVICES
set(CONFIG_IEC61850_REPORT_SERVICE ON)
set(CONFIG_IEC61850_CONTROL_SERVICE ON)
set(CONFIG_IEC61850_SERVICE_TRACKING ON)
set(CONFIG_IEC61850_SETTING_GROUPS ON)

# MMS SERVICES
set(MMS_READ_SERVICE ON)
set(MMS_WRITE_SERVICE ON)
set(MMS_GET_NAME_LIST ON)
set(MMS_JOURNAL_SERVICE ON)
set(MMS_GET_VARIABLE_ACCESS_ATTRIBUTES ON)
set(MMS_DATA_SET_SERVICE ON)
set(MMS_DYNAMIC_DATA_SETS ON)
set(MMS_GET_DATA_SET_ATTRIBUTES ON)
set(MMS_STATUS_SERVICE ON)
set(MMS_IDENTIFY_SERVICE ON)
set(MMS_FILE_SERVICE OFF)
set(MMS_DELETE_FILE_SERVICE OFF)
set(MMS_RENAME_FILE_SERVICE OFF)

# CUSTOM FLAGS
set(CONFIG_SERVER_POLLING_TIMEOUT_MS 1000)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/config/stack_config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config/stack_config.h
)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/config
  ${CMAKE_CURRENT_LIST_DIR}/src/common/inc
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/inc
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/inc_private
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/inc
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/inc_private
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/asn1c
  ${CMAKE_CURRENT_LIST_DIR}/src/logging
  ${CMAKE_CURRENT_LIST_DIR}/hal/inc  
)

### Library headers for application sources to include ###
zephyr_include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/config
  ${CMAKE_CURRENT_LIST_DIR}/src/common/inc
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/inc
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/inc
  ${CMAKE_CURRENT_LIST_DIR}/src/logging
  ${CMAKE_CURRENT_LIST_DIR}/hal/inc
)

### CORE SOURCES ###
file(GLOB LIBIEC_COMMON     ${CMAKE_CURRENT_LIST_DIR}/src/common/*.c)
file(GLOB LIBIEC_LOGGING    ${CMAKE_CURRENT_LIST_DIR}/src/logging/*.c)
file(GLOB LIBIEC_MMS
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/asn1/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_common/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/common/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/client/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/server/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/asn1c/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_cotp/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_acse/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_client/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_server/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_presentation/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_session/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_transport/*.c
)
file(GLOB LIBIEC_IEC61850
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/common/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/client/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/server/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/server/impl/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/server/mms_mapping/*.c
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/server/model/*.c
)

zephyr_library_sources(
  ${LIBIEC_COMMON}
  ${LIBIEC_LOGGING}
  ${LIBIEC_MMS}
  ${LIBIEC_IEC61850}
)


### TIME SHIMS ###
zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/compat/compat_time.c)
zephyr_library_compile_definitions(
  gmtime_r=lb_gmtime_r
  localtime_r=lb_localtime_r
  timegm=lb_timegm
  mktime=lb_timegm
  _POSIX_C_SOURCE=200809L
  _GNU_SOURCE
)

### IGNORE SOME BUILD ERRORS ###
# ASN.1 generated files can trigger array-bounds under -Werror; relax just there
file(GLOB LIBIEC_ASN1C ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/asn1c/*.c)
set_source_files_properties(${LIBIEC_ASN1C}
  PROPERTIES COMPILE_OPTIONS "-Wno-array-bounds;-Wno-error=array-bounds"
)

# Some MMS common sources trigger float-to-double promotion warnings under -Werror.
# Relax them locally to keep the build strict elsewhere.
file(GLOB LIBIEC_MMS_COMMON ${CMAKE_CURRENT_LIST_DIR}/src/mms/iso_mms/common/*.c)
set_source_files_properties(${LIBIEC_MMS_COMMON}
  PROPERTIES COMPILE_OPTIONS "-Wno-double-promotion;-Wno-error=double-promotion"
)

# Silence specific warnings in selective sources under -Werror
set_source_files_properties(
  ${CMAKE_CURRENT_LIST_DIR}/src/iec61850/server/mms_mapping/mms_mapping.c
  PROPERTIES COMPILE_OPTIONS "-Wno-unused-variable;-Wno-error=unused-variable;-Wno-unused-function;-Wno-error=unused-function"
)

### INCLUDE OUR OWN HAL ###
zephyr_library_sources(
  ${CMAKE_CURRENT_LIST_DIR}/hal/zephyr/hal_lib_memory.c
  ${CMAKE_CURRENT_LIST_DIR}/hal/zephyr/hal_time.c
  ${CMAKE_CURRENT_LIST_DIR}/hal/zephyr/hal_thread.c
  ${CMAKE_CURRENT_LIST_DIR}/hal/zephyr/hal_socket_bsd.c
)
