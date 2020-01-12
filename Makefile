

REPO := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))


NO_GIT := $(shell test -e $(REPO)/.git/ || echo 1)
ifeq ($(NO_GIT),1)
all: release
else
GIT_BRANCH ?= $(shell test -e $(REPO)/.git/ && git rev-parse --abbrev-ref HEAD)
ifeq ($(GIT_BRANCH),master)
all: release
else
all: test
endif
endif

BUILD_ROOT = $(REPO)/build

DESTDIR ?=

CC ?= cc
CXX ?= c++

BUILD_TYPE ?= Debug

PYTHON ?= python
PYTHON3 ?= python3

FORMAT ?= clang-format-8

SETCAP ?= which setcap && setcap cap_net_admin,cap_net_bind_service=+eip

SHADOW_ROOT ?= $(HOME)/.shadow
SHADOW_BIN=$(SHADOW_ROOT)/bin/shadow
SHADOW_CONFIG=$(REPO)/shadow.config.xml
SHADOW_PLUGIN=$(BUILD_ROOT)/libshadow-plugin-lokinet-shared.so
SHADOW_LOG=$(REPO)/shadow.log.txt

SHADOW_SRC ?= $(HOME)/local/shadow
SHADOW_PARSE ?= $(PYTHON) $(SHADOW_SRC)/src/tools/parse-shadow.py - -m 0 --packet-data
SHADOW_PLOT ?= $(PYTHON) $(SHADOW_SRC)/src/tools/plot-shadow.py -d $(REPO) LokiNET -c $(SHADOW_CONFIG) -r 10000 -e '.*'
SHADOW_OPTS ?=

LIBUV_VERSION ?= v1.30.1
LIBUV_PREFIX = $(BUILD_ROOT)/libuv

LIBCURL_PREFIX = $(BUILD_ROOT)/curl
LIBCURL_VERSION = 7.67.0
LIBCURL_URL = https://github.com/curl/curl/releases/download/curl-7_67_0/curl-7.67.0.tar.xz
LIBCURL_SHA256 = f5d2e7320379338c3952dcc7566a140abb49edb575f9f99272455785c40e536c

TESTNET_ROOT=/tmp/lokinet_testnet_tmp
TESTNET_CONF=$(TESTNET_ROOT)/supervisor.conf
TESTNET_LOG=$(TESTNET_ROOT)/testnet.log

TESTNET_EXE=$(REPO)/lokinet-testnet
TESTNET_CLIENTS ?= 50
TESTNET_SERVERS ?= 50
TESTNET_DEBUG ?= 0
TESTNET_IFNAME ?= lo
TESTNET_BASEPORT ?= 1900
TESTNET_IP ?= 127.0.0.1
TESTNET_NETID ?= loopback

ANDROID_NDK ?= $(HOME)/Android/Ndk
ANDROID_SDK ?= $(HOME)/Android/Sdk
ANDROID_ABI ?= armeabi-v7a
ANDROID_API_LEVEL ?= 24

ANDROID_DIR=$(REPO)/android
JNI_DIR=$(ANDROID_DIR)/jni
ANDROID_MK=$(JNI_DIR)/Android.mk
ANDROID_PROPS=$(ANDROID_DIR)/gradle.properties
ANDROID_LOCAL_PROPS=$(ANDROID_DIR)/local.properties

GRADLE ?= gradle
JAVA_HOME ?= /usr/lib/jvm/default-java

TOOLCHAIN ?=

# native avx2 code
AVX2 ?= OFF
# statically link everything
STATIC_LINK ?= OFF
# statically link dependencies
STATIC ?= OFF
# enable network namespace isolation
NETNS ?= OFF
# enable shell hooks callbacks
SHELL_HOOKS ?= OFF
# cross compile?
CROSS ?= OFF
# build liblokinet-shared.so
SHARED_LIB ?= OFF
# enable generating coverage
COVERAGE ?= OFF
# allow downloading libsodium if >= 1.0.17 not installed
DOWNLOAD_SODIUM ?= OFF

COVERAGE_OUTDIR ?= "$(TMPDIR)/lokinet-coverage"

# tracy profiler
TRACY_ROOT ?=
# enable sanitizer
XSAN ?= False

# cmake generator type
CMAKE_GEN ?= Unix Makefiles


ifdef NINJA
	MAKE = $(NINJA)
	CMAKE_GEN = Ninja
endif



SCAN_BUILD ?= scan-build

UNAME = $(shell which uname)

COMMON_CMAKE_OPTIONS = -DSTATIC_LINK_RUNTIME=$(STATIC_LINK) -DUSE_NETNS=$(NETNS) -DUSE_AVX2=$(AVX2) -DWITH_SHARED=$(SHARED_LIB) -DDOWNLOAD_SODIUM=$(DOWNLOAD_SODIUM) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ifeq ($(shell $(UNAME)),SunOS)
CONFIG_CMD = $(shell gecho -n "cd '$(BUILD_ROOT)' && " ; gecho -n "cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=$(CROSS) -DUSE_SHELLHOOKS=$(SHELL_HOOKS) $(COMMON_CMAKE_OPTIONS) '$(REPO)'")
CONFIG_CMD_WINDOWS = $(shell gecho -n "cd '$(BUILD_ROOT)' && " ; gecho -n "cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=ON -DUSE_SHELLHOOKS=$(SHELL_HOOKS) $(COMMON_CMAKE_OPTIONS) '$(REPO)'")

ANALYZE_CONFIG_CMD = $(shell gecho -n "cd '$(BUILD_ROOT)' && " ; gecho -n "$(SCAN_BUILD) cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=$(CROSS) $(COMMON_CMAKE_OPTIONS) '$(REPO)'")

COVERAGE_CONFIG_CMD = $(shell gecho -n "cd '$(BUILD_ROOT)' && " ; gecho -n "cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=$(CROSS) -DWITH_COVERAGE=yes $(COMMON_CMAKE_OPTIONS) '$(REPO)'")
else
CONFIG_CMD = $(shell /bin/echo -n "cd '$(BUILD_ROOT)' && " ; /bin/echo -n "cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=$(CROSS) -DUSE_SHELLHOOKS=$(SHELL_HOOKS) -DTRACY_ROOT=$(TRACY_ROOT) $(COMMON_CMAKE_OPTIONS) '$(REPO)'")
CONFIG_CMD_WINDOWS = $(shell /bin/echo -n "cd '$(BUILD_ROOT)' && " ; /bin/echo -n "cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=ON -DUSE_SHELLHOOKS=$(SHELL_HOOKS) $(COMMON_CMAKE_OPTIONS) '$(REPO)'")

ANALYZE_CONFIG_CMD = $(shell /bin/echo -n "cd '$(BUILD_ROOT)' && " ; /bin/echo -n "$(SCAN_BUILD) cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=$(CROSS) -DXSAN=$(XSAN) $(COMMON_CMAKE_OPTIONS) '$(REPO)'")

COVERAGE_CONFIG_CMD = $(shell /bin/echo -n "cd '$(BUILD_ROOT)' && " ; /bin/echo -n "cmake -G'$(CMAKE_GEN)' -DCMAKE_CROSSCOMPILING=$(CROSS) -DWITH_COVERAGE=yes -DXSAN=$(XSAN) $(COMMON_CMAKE_OPTIONS) '$(REPO)'")
endif

TARGETS = $(REPO)/lokinet
SIGS = $(TARGETS:=.sig)
EXE = $(BUILD_ROOT)/daemon/lokinet
TEST_EXE = $(BUILD_ROOT)/test/testAll
ABYSS_EXE = $(BUILD_ROOT)/abyss-main

LINT_FILES = $(wildcard llarp/*.cpp)

LINT_CHECK = $(LINT_FILES:.cpp=.cpp-check)

clean: android-clean
	rm -f $(TARGETS)
	rm -rf $(BUILD_ROOT)
	rm -f $(SHADOW_PLUGIN) $(SHADOW_CONFIG)
	rm -f *.sig
	rm -f *.a *.so

android-clean:
	rm -rf $(ANDROID_DIR)/.externalNativeBuild

debug-configure:
	mkdir -p '$(BUILD_ROOT)'
	(test x$(TOOLCHAIN) = x && $(CONFIG_CMD) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_C_FLAGS='$(CFLAGS)' -DCMAKE_CXX_FLAGS='$(CXXFLAGS)') || (test x$(TOOLCHAIN) != x && $(CONFIG_CMD) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_C_FLAGS='$(CFLAGS)' -DCMAKE_CXX_FLAGS='$(CXXFLAGS)' -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) )


release-configure: clean
	mkdir -p '$(BUILD_ROOT)'
	$(CONFIG_CMD) -DCMAKE_BUILD_TYPE=Release -DRELEASE_MOTTO="$(shell cat motto.txt)" -DCMAKE_C_FLAGS='$(CFLAGS)' -DCMAKE_CXX_FLAGS='$(CXXFLAGS)'

debug: debug-configure
	$(MAKE) -C $(BUILD_ROOT)
	cp $(EXE) $(REPO)/lokinet

release-compile: release-configure
	$(MAKE) -C $(BUILD_ROOT)
	strip $(EXE)

$(TARGETS): release-compile

release: $(TARGETS)

shadow-configure: clean
	mkdir -p $(BUILD_ROOT)
	$(CONFIG_CMD) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DSHADOW=ON

shadow-build: shadow-configure
	$(MAKE) -C $(BUILD_ROOT)

shadow-run: shadow-build
	$(PYTHON3) $(REPO)/contrib/shadow/genconf.py $(SHADOW_CONFIG)
	cp $(SHADOW_PLUGIN) $(REPO)/libshadow-plugin-lokinet.so
	$(SHADOW_BIN) $(SHADOW_OPTS) $(SHADOW_CONFIG) | $(SHADOW_PARSE)

shadow-plot: shadow-run
	$(SHADOW_PLOT)

shadow: shadow-plot

testnet-clean: clean
	rm -rf $(TESTNET_ROOT)

testnet-configure: testnet-clean
	mkdir -p $(BUILD_ROOT)
	$(CONFIG_CMD) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DTESTNET=1

testnet-build: testnet-configure
	$(MAKE) -C $(BUILD_ROOT)

testnet:
	cp $(EXE) $(TESTNET_EXE)
	mkdir -p $(TESTNET_ROOT)
	$(PYTHON3) $(REPO)/contrib/testnet/genconf.py --bin=$(TESTNET_EXE) --svc=$(TESTNET_SERVERS) --clients=$(TESTNET_CLIENTS) --dir=$(TESTNET_ROOT) --out $(TESTNET_CONF) --ifname=$(TESTNET_IFNAME) --baseport=$(TESTNET_BASEPORT) --ip=$(TESTNET_IP) --netid=$(TESTNET_NETID)
	LLARP_DEBUG=$(TESTNET_DEBUG) supervisord -n -d $(TESTNET_ROOT) -l $(TESTNET_LOG) -c $(TESTNET_CONF)

$(TEST_EXE): debug

test: $(TEST_EXE)
	test x$(CROSS) = xOFF && $(TEST_EXE) || test x$(CROSS) = xON

static-configure: $(LIBUV_PREFIX) $(LIBCURL_PREFIX)
	(test x$(TOOLCHAIN) = x && $(CONFIG_CMD) -DCMAKE_BUILD_TYPE=Release -DSTATIC_LINK=ON -DRELEASE_MOTTO="$(shell cat motto.txt)" -DCMAKE_C_FLAGS='$(CFLAGS)' -DCMAKE_CXX_FLAGS='$(CXXFLAGS)' -DLIBUV_ROOT='$(LIBUV_PREFIX)' -DLIBCURL_ROOT='$(LIBCURL_PREFIX)' ) || (test x$(TOOLCHAIN) != x && $(CONFIG_CMD) -DCMAKE_BUILD_TYPE=Release -DSTATIC_LINK=ON -DRELEASE_MOTTO="$(shell cat motto.txt)" -DCMAKE_C_FLAGS='$(CFLAGS)' -DCMAKE_CXX_FLAGS='$(CXXFLAGS)' -DLIBUV_ROOT='$(LIBUV_PREFIX)' -DLIBCURL_ROOT='$(LIBCURL_PREFIX)' -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) -DNATIVE_BUILD=OFF )

static: static-configure
	$(MAKE) -C '$(BUILD_ROOT)'
	cp $(EXE) $(REPO)/lokinet-static

$(LIBCURL_PREFIX):
	mkdir -p '$(BUILD_ROOT)'
	wget '$(LIBCURL_URL)' -O '$(BUILD_ROOT)/curl.tar.xz'
	bash -c 'sha256sum -c <<<"$(LIBCURL_SHA256) $(BUILD_ROOT)/curl.tar.xz"'
	tar -xJf '$(BUILD_ROOT)/curl.tar.xz' -C '$(BUILD_ROOT)'
	mv '$(BUILD_ROOT)/curl-$(LIBCURL_VERSION)' '$(LIBCURL_PREFIX)'

$(LIBUV_PREFIX):
	mkdir -p $(BUILD_ROOT)
	git clone -b "$(LIBUV_VERSION)" https://github.com/libuv/libuv "$(LIBUV_PREFIX)"

ios:
	cmake -S ui-ios -B build -G Xcode -DCMAKE_TOOLCHAIN_FILE=$(shell pwd)/ui-ios/ios-toolchain.cmake -DCMAKE_SYSTEM_NAME=iOS "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=12.2 -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO -DCMAKE_IOS_INSTALL_COMBINED=YES
	cmake --build build

android-gradle-prepare: $(LIBUV_PREFIX)
	rm -f $(ANDROID_PROPS)
	rm -f $(ANDROID_LOCAL_PROPS)
	echo "#auto generated don't modify kthnx" >> $(ANDROID_PROPS)
	echo "libuvsrc=$(LIBUV_PREFIX)" >> $(ANDROID_PROPS)
	echo "lokinetCMake=$(REPO)/CMakeLists.txt" >> $(ANDROID_PROPS)
	echo "org.gradle.parallel=true" >> $(ANDROID_PROPS)
	echo "org.gradle.jvmargs=-Xmx1536M" >> $(ANDROID_PROPS)
	echo "#auto generated don't modify kthnx" >> $(ANDROID_LOCAL_PROPS)
	echo "sdk.dir=$(ANDROID_SDK)" >> $(ANDROID_LOCAL_PROPS)
	echo "ndk.dir=$(ANDROID_NDK)" >> $(ANDROID_LOCAL_PROPS)

android-gradle: android-gradle-prepare
	cd $(ANDROID_DIR) && JAVA_HOME=$(JAVA_HOME) $(GRADLE) clean assemble

android: android-gradle
	cp -f $(ANDROID_DIR)/build/outputs/apk/*.apk $(REPO)

windows-debug-configure: $(LIBUV_PREFIX)
	mkdir -p '$(BUILD_ROOT)'
	$(CONFIG_CMD_WINDOWS) -DCMAKE_TOOLCHAIN_FILE='$(REPO)/contrib/cross/mingw.cmake'  -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_ASM_FLAGS='$(ASFLAGS)' -DCMAKE_C_FLAGS='$(CFLAGS)' -DCMAKE_CXX_FLAGS='$(CXXFLAGS)' -DLIBUV_ROOT=$(LIBUV_PREFIX)

windows-debug: windows-debug-configure
	$(MAKE) -C '$(BUILD_ROOT)'
	cp '$(BUILD_ROOT)/daemon/lokinet.exe' '$(REPO)/lokinet.exe'

windows-release-configure: $(LIBUV_PREFIX)
	mkdir -p '$(BUILD_ROOT)'
	$(CONFIG_CMD_WINDOWS) -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE='$(REPO)/contrib/cross/mingw.cmake'  -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_ASM_FLAGS='$(ASFLAGS)' -DCMAKE_C_FLAGS='$(CFLAGS)' -DCMAKE_CXX_FLAGS='$(CXXFLAGS)' -DLIBUV_ROOT=$(LIBUV_PREFIX)

windows-release: windows-release-configure
	$(MAKE) -C '$(BUILD_ROOT)'
	cp '$(BUILD_ROOT)/daemon/lokinet.exe' '$(REPO)/lokinet.exe'

windows: windows-debug

abyss: debug
	$(ABYSS_EXE)

format:
	$(FORMAT) -i $$(find jni daemon llarp include libabyss | grep -E '\.[h,c](pp)?$$')

format-verify: format
	(type $(FORMAT))
	$(FORMAT) --version
	git diff --quiet || (echo 'Please run make format!!' && git --no-pager diff ; exit 1)

analyze-config: clean
	mkdir -p '$(BUILD_ROOT)'
	$(ANALYZE_CONFIG_CMD)

analyze: analyze-config
	$(SCAN_BUILD) $(MAKE) -C $(BUILD_ROOT)

coverage-config: clean
	mkdir -p '$(BUILD_ROOT)'
	$(COVERAGE_CONFIG_CMD)

coverage: coverage-config
	$(MAKE) -C $(BUILD_ROOT)
	test x$(CROSS) = xOFF && $(TEST_EXE) || true # continue even if tests fail
	mkdir -p "$(COVERAGE_OUTDIR)"
ifeq ($(CLANG),OFF)
	gcovr -r . --branches --html --html-details -o "$(COVERAGE_OUTDIR)/lokinet.html"
else
	llvm-profdata merge default.profraw -output $(BUILD_ROOT)/profdata
	llvm-cov show -format=html -output-dir="$(COVERAGE_OUTDIR)" -instr-profile "$(BUILD_ROOT)/profdata" "$(BUILD_ROOT)/testAll" $(shell find ./llarp -type f)
endif

lint: $(LINT_CHECK)

%.cpp-check: %.cpp
	clang-tidy $^ -- -I$(REPO)/include -I$(REPO)/crypto/include -I$(REPO)/llarp -I$(REPO)/vendor/cppbackport-master/lib

docker-kubernetes:
	docker build -f docker/loki-svc-kubernetes.Dockerfile .

install-pylokinet:
	cd $(REPO)/contrib/py/pylokinet && $(PYTHON3) setup.py install

kubernetes-install: install install-pylokinet

docker-debian:
	docker build -f docker/debian.Dockerfile .

docker-fedora:
	docker build -f docker/fedora.Dockerfile .

debian-configure:
	mkdir -p '$(BUILD_ROOT)'
	$(CONFIG_CMD) -DDEBIAN=ON -DRELEASE_MOTTO="$(shell cat $(REPO)/motto.txt)" -DCMAKE_BUILD_TYPE=Release

debian: debian-configure
	$(MAKE) -C '$(BUILD_ROOT)'
	cp $(EXE) lokinet

debian-test:
	test x$(CROSS) = xOFF && $(TEST_EXE) || test x$(CROSS) = xON

install:
	DESTDIR=$(DESTDIR) $(MAKE) -C '$(BUILD_ROOT)' install

.PHONY: debian-install
