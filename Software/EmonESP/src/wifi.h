/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _EMONESP_WIFI_H
#define _EMONESP_WIFI_H

#include <Arduino.h>

// Wifi mode
// 0 - STA (Client)
// 1 - AP with STA retry
// 2 - AP only
// 3 - AP + STA

#define WIFI_MODE_CLIENT        0
#define WIFI_MODE_AP_STA_RETRY  1
#define WIFI_MODE_AP_ONLY       2
#define WIFI_MODE_AP_AND_STA    3


// The current WiFi mode
extern int wifi_mode;

// Last discovered WiFi access points
extern String st;
extern String rssi;

// Network state
extern String ipaddress;

// mDNS hostname
extern const char *esp_hostname;

extern void wifi_setup();
extern void wifi_loop();
extern void wifi_restart();
extern void wifi_scan();
extern void wifi_disconnect();

#endif // _EMONESP_WIFI_H
