SUMMARY = "C library for processing UTF-8 Unicode data"
HOMEPAGE = "https://github.com/JuliaStrings/utf8proc"
DESCRIPTION = "utf8proc is a small, clean C library that provides Unicode normalization, case-folding, and other operations for data in the UTF-8 encoding."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=96d5a3ba306e0f24fb289427af484408"

SRC_URI = "git://github.com/JuliaStrings/utf8proc.git;protocol=https;branch=master"
SRCREV = "e5e799221b45bbb90f5fdc5c69b6b8dfbf017e78"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

EXTRA_OECMAKE = "\
    -DBUILD_SHARED_LIBS=ON \
    -DUTF8PROC_ENABLE_TESTING=OFF \
    -DUTF8PROC_INSTALL=ON \
"

BBCLASSEXTEND = "native nativesdk"
