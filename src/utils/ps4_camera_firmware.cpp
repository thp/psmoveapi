 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2022 Thomas Perl <m@thp.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/

#include <libusb.h>

struct USBVIDPID {
    uint16_t vendor_id;
    uint16_t product_id;

    bool matches(struct libusb_device_descriptor &desc) const
    {
        return desc.idVendor == vendor_id && desc.idProduct == product_id;
    }

};

static const uint16_t USB_VID_SONY = 0x054c;
static const uint16_t USB_VID_OMNIVISION = 0x05a9;
static const uint16_t USB_VID_SCEE = 0x1415;

// The PS4-to-PS5 adapter exposes two hub devices:
//  - PID=0x0d0a (USB 2.0) --> for USB spec backwards compatibility?
//  - PID=0x0d0b (USB 3.0) --> parent of the boot mode OV580

static const uint16_t USB_PID_CFI_ZAA1_USB2 = 0x0d0a;
static const uint16_t USB_PID_CFI_ZAA1_USB3 = 0x0d0b;

static const uint16_t USB_PID_OV580_BOOT_MODE = 0x0580;
static const uint16_t USB_PID_PS4_CAMERA = 0x058a;
static const uint16_t USB_PID_PS5_CAMERA = 0x058c;
static const uint16_t USB_PID_PS3_EYE = 0x2000;

static const USBVIDPID PS4_TO_PS5_ADAPTER_ID = { USB_VID_SONY,       USB_PID_CFI_ZAA1_USB3 };
static const USBVIDPID OV580_BOOT_MODE_ID =    { USB_VID_OMNIVISION, USB_PID_OV580_BOOT_MODE };
static const USBVIDPID PS4_CAMERA_ID =         { USB_VID_OMNIVISION, USB_PID_PS4_CAMERA };
static const USBVIDPID PS5_CAMERA_ID =         { USB_VID_OMNIVISION, USB_PID_PS5_CAMERA };
static const USBVIDPID PS3_CAMERA_ID =         { USB_VID_SCEE,       USB_PID_PS3_EYE };



static void
upload_firmware(libusb_context *context, libusb_device *dev, const std::vector<char> &firmware_bin)
{
    /**
     * Firmware upload logic based on information from PS4EYECam::firmware_upload()
     * https://github.com/bigboss-ps3dev/PS4EYECam/blob/master/driver/src/ps4eye.cpp
     *
     * PlayStation 4 Camera driver
     * Copyright (C) 2013,2014 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
     *
     * Repository https://github.com/bigboss-ps3dev/PS4EYECam
     * some parts are based on PS3EYECamera driver https://github.com/inspirit/PS3EYEDriver
     * some parts were commited to ps4eye https://github.com/ps4eye/ps4eye
     *
     * This program is free software; you can redistribute it and/or modify
     * it under the terms of the GNU General Public Licence as published by
     * the Free Software Foundation; either version 2 of the Licence, or
     * (at your option) any later version.
     *
     * This program is distributed in the hope that it will be useful,
     * but WITHOUT ANY WARRANTY; without even the implied warranty of
     * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
     * GNU General Public Licence for more details.
     *
     * You should have received a copy of the GNU General Public Licence
     * along with this program; if not, write to the Free Software
     * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
     *
     * If you redistribute this file in source form, modified or unmodified, you may:
     * 1) Leave this header intact and distribute it under the same terms,
     *    accompanying it with the GPL20 or later files, or
     * In all cases you must keep the copyright notice intact
     *
     * Binary distributions must follow the binary distribution requirements of license.
     **/

    libusb_device_handle *handle;

    int res;

    PSMOVE_VERIFY((res = libusb_open(dev, &handle)) == 0, "res = %d", res);
    PSMOVE_VERIFY((res = libusb_reset_device(handle)) == 0, "res = %d", res);
    PSMOVE_VERIFY((res = libusb_set_configuration(handle, 1)) == 0, "res = %d", res);
    PSMOVE_VERIFY((res = libusb_claim_interface(handle, 0)) == 0, "res = %d", res);

    static constexpr const size_t CHUNK_SIZE = 512;
    for (size_t read_offset=0; read_offset < firmware_bin.size(); read_offset+=CHUNK_SIZE) {
        std::vector<uint8_t> chunk(firmware_bin.begin() + read_offset,
                                   firmware_bin.begin() + read_offset + std::min(CHUNK_SIZE, firmware_bin.size() - read_offset));

        res = libusb_control_transfer(handle, 0x40, 0x00, read_offset & 0xFFFF, 20 + (read_offset >> 16), chunk.data(), chunk.size(), 1000);
        PSMOVE_VERIFY(size_t(res) == chunk.size(), "res = %d", res);
    }

    std::vector<uint8_t> chunk = { 0x5b };
    res = libusb_control_transfer(handle, 0x40, 0x00, 0x2200, 0x8018, chunk.data(), chunk.size(), 1000);
    PSMOVE_VERIFY(res == LIBUSB_ERROR_NO_DEVICE, "res = %d", res);

    // As the device gets disconnected, we don't do libusb_close() here, and
    // instead rely on the process being closed and all resources freed. This
    // avoids an error, as the "boot" device get disconnected at the end of
    // the firmware upload.

    PSMOVE_INFO("Firmware uploaded");
}

static std::vector<char>
load_firmware(const std::string &filename)
{
    std::vector<char> firmware_bin;

    FILE *fp = fopen(filename.c_str(), "rb");
    PSMOVE_VERIFY(fp != nullptr, "Could not open firmware file '%s'", filename.c_str());
    PSMOVE_VERIFY(fseek(fp, 0, SEEK_END) == 0, "Could not seek in firmware file '%s'", filename.c_str());

    long size = ftell(fp);
    PSMOVE_VERIFY(size > 0, "Invalid file size for '%s'", filename.c_str());
    firmware_bin.resize(size);

    PSMOVE_VERIFY(fseek(fp, 0, SEEK_SET) == 0, "Could not seek in firmware file '%s'", filename.c_str());
    PSMOVE_VERIFY(fread(firmware_bin.data(), firmware_bin.size(), 1, fp) == 1, "Could not read firmware file '%s'", filename.c_str());
    fclose(fp);

    PSMOVE_INFO("Loaded '%s' with %zu bytes", filename.c_str(), firmware_bin.size());

    return firmware_bin;
}

int
ps4_camera_firmware_main(int argc, char *argv[])
{
    std::vector<char> firmware_bin;
    bool camera_is_ps4 = false;
    bool valid_command_line = true;

    while (argc > 1) {
        if (strcmp(argv[1], "--force-ps4") == 0) {
            camera_is_ps4 = true;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            valid_command_line = false;
            break;
        } else if (firmware_bin.empty()) {
            firmware_bin = load_firmware(argv[1]);
        } else {
            PSMOVE_WARNING("Invalid command line parameter: '%s'", argv[1]);
            valid_command_line = false;
            break;
        }

        --argc;
        ++argv;
    }

    if (!valid_command_line) {
        PSMOVE_FATAL(R"(Usage: %s [--force-ps4] [firmware.bin]

Optional parameters:

    --force-ps4 ..... Treat any OV580 as PS4 camera (when using a homemade AUX-to-USB adapter)
    firmware.bin .... Path to a firmware file to upload to the camera
)", argv[0]);
    }

    libusb_context *usb_context;
    libusb_init(&usb_context);

    libusb_device **devs;
    int count = libusb_get_device_list(usb_context, &devs);

    PSMOVE_VERIFY(count >= 0, "Could not get USB device list: %d", count);

    for (int i=0; i<count; ++i) {
        libusb_device *dev = devs[i];

        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(dev, &desc);

        if (PS3_CAMERA_ID.matches(desc)) {
            PSMOVE_INFO("Found PS3 camera at bus %d, device %d",
                    libusb_get_bus_number(dev), libusb_get_device_address(dev));
        } else if (PS4_CAMERA_ID.matches(desc)) {
            PSMOVE_INFO("Found PS4 camera with firmware already loaded at bus %d, device %d",
                    libusb_get_bus_number(dev), libusb_get_device_address(dev));
        } else if (PS5_CAMERA_ID.matches(desc)) {
            PSMOVE_INFO("Found PS5 camera with firmware already loaded at bus %d, device %d",
                    libusb_get_bus_number(dev), libusb_get_device_address(dev));
        } else if (OV580_BOOT_MODE_ID.matches(desc)) {
            const char *model = nullptr;

            libusb_device *parent_dev = libusb_get_parent(dev);
            struct libusb_device_descriptor pdesc;
            libusb_get_device_descriptor(parent_dev, &pdesc);

            if (PS4_TO_PS5_ADAPTER_ID.matches(pdesc)) {
                model = "PS4 camera w/ PS4 Camera Adapter (CFI-ZAA1)";
                camera_is_ps4 = true;
            } else if (camera_is_ps4) {
                model = "PS4 camera using homemade adapter (--force-ps4)";
            } else {
                model = "PS5 camera";
            }

            PSMOVE_INFO("Found OV580 in Boot Mode at %03d:%03d: %s", libusb_get_bus_number(dev),
                    libusb_get_device_address(dev), model);

            if (firmware_bin.empty()) {
                char *path = nullptr;

                if (camera_is_ps4) {
                    // PS4 CUH-ZEY2 ("round" model v2): fe86162309518a0ffe267075a2fcf728c5856b3e
                    path = psmove_util_get_file_path("camera-firmware-ps4.bin");
                } else {
                    // PS5 CFI-ZEY1: 0fa4da31a12b662a9a80abc8b84932770df8f7e1
                    path = psmove_util_get_file_path("camera-firmware-ps5.bin");
                }

                upload_firmware(usb_context, dev, load_firmware(path));
                psmove_free_mem(path);
            } else {
                upload_firmware(usb_context, dev, firmware_bin);
            }
        }
    }

    libusb_free_device_list(devs, 1);

    libusb_exit(usb_context);

    return 0;
}
