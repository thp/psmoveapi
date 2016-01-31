/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2014 Alexander Nitsch <nitsch@ht.tu-berlin.de>
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

#ifndef PSMOVE_WINSUPPORT_H
#define PSMOVE_WINSUPPORT_H

#include <windows.h>
#include <bluetoothapis.h>


/**
 * \brief Interactively establish a Bluetooth connection between Move and a host radio
 *
 * \param move_addr The Move's Bluetooth device address in the format \c "aa:bb:cc:dd:ee:ff"
 * \param radio_addr The host radio's Bluetooth device address
 * \param hRadio A valid handle to the host Bluetooth radio
 *
 * \return Zero on success, or a non-zero value upon error
 **/
int
windows_register_psmove(const char *move_addr, const BLUETOOTH_ADDRESS *radio_addr, const HANDLE hRadio);

/**
 * \brief Get a handle to the first local Bluetooth radio found
 *
 * \param hRadio A pointer to the Bluetooth radio. The caller is responsible
 *               for closing this handle via \c CloseHandle() when it is no
 *               longer needed.
 *
 * \return Zero on success, or a non-zero value upon error
 **/
int
windows_get_first_bluetooth_radio(HANDLE *hRadio);

#endif

