SUMMARY = "MAX30102 Heart Rate Sensor Kernel Module"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit module

SRC_URI = "file://max30102-med.c \
           file://Makefile \
          "

S = "${WORKDIR}"

# Ensure the module is correctly provided as a kernel module package
PKG:${PN} = "kernel-module-max30102-med"

# Custom install function to avoid installing unnecessary kernel build artifacts
do_install() {
    # Create the destination directory in the target rootfs
    install -d ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra
    
    # Install only the compiled kernel module (.ko) file
    # Use ${B} as it is the standard build directory for modules
    install -m 0644 ${B}/max30102-med.ko ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra
}
