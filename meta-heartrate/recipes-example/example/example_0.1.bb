SUMMARY = "Advanced Heart Rate Monitor Application"
DESCRIPTION = "Python application to process MAX30102 sensor data via custom kernel driver."
SECTION = "examples"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# 1. Source the Python script from your 'files' directory
SRC_URI = "file://heartrate_monitor.py"

# 2. Set the source directory to the workspace
S = "${WORKDIR}"

# 3. Add Runtime Dependencies (CRITICAL for NumPy and Python)
RDEPENDS:${PN} += " \
    python3-core \
    python3-io \
    python3-os \
    python3-math \
    python3-numpy \
"

# 4. Installation Logic: Replace the 'display_banner' with 'do_install'
do_install() {
    # Create /usr/bin in the target image
    install -d ${D}${bindir}
    
    # Install the script and rename it for easy CLI access
    # Permissions 0755 make it executable
    install -m 0755 ${S}/heartrate_monitor.py ${D}${bindir}/heartrate_monitor
}

# 5. Ensure the file is included in the package
FILES:${PN} += "${bindir}/heartrate_monitor"