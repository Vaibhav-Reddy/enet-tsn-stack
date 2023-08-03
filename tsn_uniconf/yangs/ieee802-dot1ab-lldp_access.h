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
#ifndef IEEE802_DOT1AB_LLDP_ACCESS_H_
#define IEEE802_DOT1AB_LLDP_ACCESS_H_
#include <tsn_uniconf/yangs/yang_db_access.h>

typedef struct
{
    kvs_t vk;
    uint8_t vk_sz; //!< size of kv
} attribute_pair_t;

int ydbi_get_item_abk1vk0(yang_db_item_access_t *ydbia, void **rval, uint8_t k1,  bool status);

int ydbi_get_item_abk2vk0(yang_db_item_access_t *ydbia, void **rval, uint8_t k1, uint8_t k2, bool status);

/**
 * @brief Get val base on n key from LLDP until end
 * Also provided vkn as attribute
 */
int ydbi_get_item_abknvkn(yang_db_item_access_t *ydbia, 
                            void **rval, 
                            uint8_t kn[], 
                            uint8_t kn_sz,
                            attribute_pair_t attrs[],
                            uint8_t kvs_sz,
                            bool status);

/**
 * @brief Set val base on n key from LLDP until end
 * Also provided vkn as attribute
 */
int ydbi_set_item_abknvkn(yang_db_item_access_t *ydbia,  
                            uint8_t kn[], 
                            uint8_t kn_sz,
                            attribute_pair_t attrs[],
                            uint8_t kvs_sz,
                            bool status,
                            void* value,
                            uint32_t vsize,
                            uint8_t notice);

/**
 * @brief Set val base on n key from LLDP until end
 * Also provided vkn as attribute
 */
int ydbi_del_item_abknvkn(yang_db_item_access_t *ydbia,  
                            uint8_t kn[], 
                            uint8_t kn_sz,
                            attribute_pair_t attrs[],
                            uint8_t kvs_sz,
                            bool status,
                            uint8_t notice);

#endif /* IEEE802_DOT1AB_LLDP_ACCESS_H_ */