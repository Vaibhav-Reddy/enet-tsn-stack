/*
 * Copyright (c) 2023 Texas Instruments Incorporated
 * Copyright (c) 2023 Excelfore Corporation (https://excelfore.com)
 *
 * All rights reserved not granted herein.
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free,
 * non-exclusive license under copyrights and patents it now or hereafter
 * owns or controls to make, have made, use, import, offer to sell and sell ("Utilize")
 * this software subject to the terms herein. With respect to the foregoing patent
 * license, such license is granted solely to the extent that any such patent is necessary
 * to Utilize the software alone. The patent license shall not apply to any combinations which
 * include this software, other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license (including the
 * above copyright notice and the disclaimer and (if applicable) source code license limitations below)
 * in the documentation and/or other materials provided with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided that the following
 * conditions are met:
 *
 * * No reverse engineering, decompilation, or disassembly of this software is permitted with respect to any
 * software provided in binary form.
 * * any redistribution and use are licensed by TI for use only with TI Devices.
 * * Nothing shall obligate TI to provide you with source code for the software licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the source code are permitted
 * provided that the following conditions are met:
 *
 * * any redistribution and use of the source code, including any resulting derivative works, are licensed by
 * TI for use only with TI Devices.
 * * any redistribution and use of any object code compiled from the source code and any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be used to endorse or
 * promote products derived from this software without specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI"S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI"S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/
/* Automatically generated file.  Don't edit this file.*/
#include <inttypes.h>
#include <stdio.h>
#include <inttypes.h>
#include <tsn_unibase/unibase.h>
#include "../yang_modules_capability.h"

const char *yang_modules_capability_list[]={
    "urn:ietf:params:netconf:base:1.0",
    "urn:ietf:params:netconf:base:1.1",
    "urn:ietf:params:netconf:capability:writable-running:1.0",
    "urn:ietf:params:xml:ns:netconf:base:1.0?module=ietf-netconf&amp;revision=2011-06-01&amp;features=writable-running",
    "urn:ietf:params:xml:ns:yang:ietf-interfaces?module=ietf-interfaces&amp;revision=2018-02-20&amp;features=arbitrary-names,pre-provisioning,if-mib",
    "urn:ieee:std:802.1Q:yang:ieee802-dot1q-bridge?module=ieee802-dot1q-bridge&amp;revision=2022-10-29&amp;features=ingress-filtering,extended-filtering-services,port-and-protocol-based-vlan,flow-filtering,simple-bridge-port,flexible-bridge-port",
    "urn:ieee:std:802.1AB:yang:ieee802-dot1ab-lldp?module=ieee802-dot1ab-lldp&amp;revision=2022-03-15&amp;features=",
    "urn:ieee:std:1588:yang:ieee1588-ptp?module=ieee1588-ptp&amp;revision=2023-05-26&amp;features=external-port-config,performance-monitoring",
    "urn:ieee:std:802.1Q:yang:ieee802-dot1q-tsn-config-uni?module=ieee802-dot1q-tsn-config-uni&amp;revision=2023-05-16&amp;features=",
    "urn:ietf:params:xml:ns:yang:ietf-yang-library?module=ietf-yang-library&amp;revision=2019-01-04&amp;features=",
    "urn:ietf:params:xml:ns:yang:ietf-netconf-server?module=ietf-netconf-server&amp;revision=2015-10-09&amp;features=tls-listen",
    "urn:ietf:params:xml:ns:yang:ietf-keychain?module=ietf-keychain&amp;revision=2015-10-09&amp;features=",
    "urn:ieee:vendor:802.1Q:yang:excelfore-tsn-remote?module=excelfore-tsn-remote&amp;revision=2023-04-18&amp;features=",
    "urn:ieee:std:802.1AS:yang:ieee802-dot1as-ptp?module=ieee802-dot1as-ptp&amp;revision=2023-04-04&amp;features=",
    "urn:ieee:std:802.1Q:yang:ieee802-dot1q-sched-bridge?module=ieee802-dot1q-sched-bridge&amp;revision=2023-03-08&amp;features=",
    "urn:ieee:std:802.1Q:yang:ieee802-dot1q-preemption-bridge?module=ieee802-dot1q-preemption-bridge&amp;revision=2023-03-08&amp;features=frame-preemption",
    "urn:ieee:vendor:802.1AS:yang:xl4interfaces?module=xl4-ietf-interfaces&amp;revision=2023-03-25&amp;features=",
    "urn:ieee:vendor:802.1AS:yang:xl4gptp?module=xl4-ieee1588-ptp&amp;revision=2022-11-25&amp;features=",
    "urn:ieee:vendor:802.1Q:yang:xl4-ieee802-dot1q-bridge?module=xl4-ieee802-dot1q-bridge&amp;features=",
    "urn:ieee:vendor:802.1Q:yang:xl4-ieee802-dot1q-tsn-config-uni?module=xl4-ieee802-dot1q-tsn-config-uni&amp;features=",
    "urn:xl4:params:xml:ns:yang:xl4-netconf-server?module=xl4-netconf-server&amp;revision=2023-06-12&amp;features=",
    "urn:xl4:params:xml:ns:yang:xl4-keychain?module=xl4-keychain&amp;revision=2023-06-12&amp;features=",
    NULL
};

const char **yang_db_get_capabilities(void)
{
    return yang_modules_capability_list;
}

