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
/*
 * lldenet.c
*/
#include <enet_appmemutils.h>
#include <enet_apputils.h>
#include <enet_mcm.h>
#include "combase_private.h"

#ifndef ENET_MEM_NUM_RX_PKTS
#define ENET_MEM_NUM_RX_PKTS 16
#endif //ENET_MEM_NUM_RX_PKTS

#ifndef ENET_MEM_NUM_TX_PKTS
#define ENET_MEM_NUM_TX_PKTS 16
#endif //ENET_MEM_NUM_TX_PKTS

#ifndef ENET_MEM_LARGE_POOL_PKT_SIZE
#define ENET_MEM_LARGE_POOL_PKT_SIZE ENET_UTILS_ALIGN(1536U, ENET_UTILS_CACHELINE_SIZE)
#endif //ENET_MEM_LARGE_POOL_PKT_SIZE

typedef struct {
	EnetDma_PktQ rxFreeQ;
	EnetDma_PktQ rxReadyQ;
	EnetDma_PktQ txFreePktInfoQ;
	EnetDma_RxChHandle hRxCh;
	EnetDma_TxChHandle hTxCh;
	uint32_t rxFlowStartIdx;
	uint32_t rxFlowIdx;
	uint32_t txChNum; /* returned by dma channel alloc API */
	uint32_t rxChNum; /* returned by dma channel alloc API */
	int32_t txChId; /* user assign */
	int32_t rxChId; /* user assign */
	void (*txNotifyCb)(void *cbArg);
	void (*rxNotifyCb)(void *cbArg);
	void *txCbArg;
	void *rxCbArg;
	uint32_t nTxPkts;
	uint32_t nRxPkts;
	uint32_t pktSize;
	LLDEnet_t *hLLDEnet;
} LLDEnetDma_t;

struct LLDEnet {
	Enet_Handle hEnet;
	uint32_t coreId;
	uint32_t coreKey;
	LLDEnetDma_t dma;
	Enet_Type enetType;
	uint32_t instId;
};

static void DmaBufQInit(EnetDma_PktQ *q, void *user, int nPkts, uint32_t pktSize)
{
	int i;
	EnetDma_Pkt *pktInfo;

	EnetQueue_initQ(q);
	uint32_t scatterSegments[] = { pktSize };

	for (i = 0; i < nPkts; i++) {
		pktInfo = EnetMem_allocEthPkt(user, ENETDMA_CACHELINE_ALIGNMENT,
							ENET_ARRAYSIZE(scatterSegments),
							scatterSegments);
		EnetAppUtils_assert(pktInfo != NULL);
		ENET_UTILS_SET_PKT_APP_STATE(&pktInfo->pktState, ENET_PKTSTATE_APP_WITH_FREEQ);

		EnetQueue_enq(q, &pktInfo->node);
	}
}

static void DmaTxQInit(LLDEnetDma_t *hLLDma)
{
	DmaBufQInit(&hLLDma->txFreePktInfoQ, NULL, hLLDma->nTxPkts, hLLDma->pktSize);
}

static void DmaRxQInit(LLDEnetDma_t *hLLDma)
{
	EnetDma_PktQ rxReadyQ;
	int32_t status;

	EnetQueue_initQ(&rxReadyQ);
	EnetQueue_initQ(&hLLDma->rxReadyQ);

	DmaBufQInit(&hLLDma->rxFreeQ, NULL, hLLDma->nRxPkts, hLLDma->pktSize);

	/* Retrieve any CPSW packets which are ready */
	status = EnetDma_retrieveRxPktQ(hLLDma->hRxCh, &rxReadyQ);
	EnetAppUtils_assert(status == ENET_SOK);
	/* There should not be any packet with DMA during init */
	EnetAppUtils_assert(EnetQueue_getQCount(&rxReadyQ) == 0U);

	EnetAppUtils_validatePacketState(&hLLDma->rxFreeQ,
			ENET_PKTSTATE_APP_WITH_FREEQ, ENET_PKTSTATE_APP_WITH_DRIVER);

	EnetDma_submitRxPktQ(hLLDma->hRxCh, &hLLDma->rxFreeQ);

	/* Assert here as during init no. of DMA descriptors should be equal to
	 * no. of free Ethernet buffers available with app */
	EnetAppUtils_assert(0U == EnetQueue_getQCount(&hLLDma->rxFreeQ));
}

static void LLDEnetTxNotifyCb(void *cbArg);
static void LLDEnetRxNotifyCb(void *cbArg);

static int32_t DmaOpen(LLDEnetDma_t *hLLDma)
{
	int32_t status = ENET_SOK;
	LLDEnet_t *hLLDEnet = hLLDma->hLLDEnet;
	EnetApp_GetDmaHandleInArgs txInArgs;
	EnetApp_GetDmaHandleInArgs rxInArgs;
	EnetApp_GetTxDmaHandleOutArgs txChInfo;
	EnetApp_GetRxDmaHandleOutArgs rxChInfo;

	if ((hLLDma->txChId < 0) || (hLLDma->rxChId < 0)) {
		UB_LOG(UBL_ERROR,"Invalid dmaTxChId or dmaRxChId: %d, %d\n",
			   hLLDma->txChId, hLLDma->rxChId);
		return ENET_EINVALIDPARAMS;
	}

	/* Open the TX channel */
	txInArgs.cbArg = hLLDma;
	txInArgs.notifyCb = LLDEnetTxNotifyCb;
	EnetApp_getTxDmaHandle(hLLDma->txChId, &txInArgs, &txChInfo);

	hLLDma->txChNum = txChInfo.txChNum;
	hLLDma->hTxCh = txChInfo.hTxCh;
	EnetAppUtils_assert(hLLDma->hTxCh != NULL);

	DmaTxQInit(&hLLDEnet->dma);

	status = EnetDma_enableTxEvent(hLLDma->hTxCh);
	EnetAppUtils_assert(status == ENET_SOK);

	/* Open the RX channel */
	rxInArgs.notifyCb = LLDEnetRxNotifyCb;
	rxInArgs.cbArg = hLLDma;
	EnetApp_getRxDmaHandle(hLLDma->rxChId, &rxInArgs, &rxChInfo);

	hLLDma->rxFlowStartIdx = rxChInfo.rxFlowStartIdx;
	hLLDma->rxFlowIdx = rxChInfo.rxFlowIdx;
	hLLDma->hRxCh = rxChInfo.hRxCh;
	EnetAppUtils_assert(hLLDma->hRxCh != NULL);

	/* Submit all ready RX buffers to DMA.*/
	DmaRxQInit(&hLLDEnet->dma);

	return status;
}

static uint32_t LLDEnetRetrieveFreeTxPkts(EnetDma_TxChHandle hTxCh,
					EnetDma_PktQ *txPktInfoQ)
{
	EnetDma_PktQ txFreeQ;
	EnetDma_Pkt *pktInfo;
	uint32_t txFreeQCnt = 0U;
	int32_t status;

	EnetQueue_initQ(&txFreeQ);

	/* Retrieve any packets that may be free now */
	status = EnetDma_retrieveTxPktQ(hTxCh, &txFreeQ);
	if (status == ENET_SOK) {
		txFreeQCnt = EnetQueue_getQCount(&txFreeQ);

		pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&txFreeQ);
		while (NULL != pktInfo) {
			EnetDma_checkPktState(&pktInfo->pktState, ENET_PKTSTATE_MODULE_APP,
				ENET_PKTSTATE_APP_WITH_DRIVER, ENET_PKTSTATE_APP_WITH_FREEQ);

			EnetQueue_enq(txPktInfoQ, &pktInfo->node);
			pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&txFreeQ);
		}
	}

	return txFreeQCnt;
}

static void DmaClose(LLDEnetDma_t *hLLDma)
{
	EnetDma_PktQ fqPktInfoQ;
	EnetDma_PktQ cqPktInfoQ;
	LLDEnet_t *hLLDEnet = hLLDma->hLLDEnet;

	/* Close RX Flow */
	if (hLLDma->hRxCh != NULL) {
		EnetQueue_initQ(&fqPktInfoQ);
		EnetQueue_initQ(&cqPktInfoQ);

		EnetApp_closeRxDma(hLLDma->rxChId, hLLDEnet->hEnet, hLLDEnet->coreKey,
						   hLLDEnet->coreId, &fqPktInfoQ, &cqPktInfoQ);
		EnetAppUtils_freePktInfoQ(&fqPktInfoQ);
		EnetAppUtils_freePktInfoQ(&cqPktInfoQ);
		EnetAppUtils_freePktInfoQ(&hLLDma->rxFreeQ);
	}

	/* Close TX channel */
	if (hLLDma->hTxCh != NULL) {
		EnetQueue_initQ(&fqPktInfoQ);
		EnetQueue_initQ(&cqPktInfoQ);

		/* Retrieve any pending TX packets from driver */
		LLDEnetRetrieveFreeTxPkts(hLLDma->hTxCh, &hLLDma->txFreePktInfoQ);

		EnetApp_closeTxDma(hLLDma->txChId, hLLDEnet->hEnet, hLLDEnet->coreKey,
					   hLLDEnet->coreId, &fqPktInfoQ, &cqPktInfoQ);

		EnetAppUtils_freePktInfoQ(&fqPktInfoQ);
		EnetAppUtils_freePktInfoQ(&cqPktInfoQ);

		EnetAppUtils_freePktInfoQ(&hLLDma->txFreePktInfoQ);
	}
}

static int FilterVlanDestMac(LLDEnet_t *hLLDEnet, uint8_t *dstMacAddr, uint32_t vlanId)
{
	Enet_IoctlPrms prms;
	CpswAle_SetPolicerEntryOutArgs setPolicerEntryOutArgs;
	CpswAle_SetPolicerEntryInArgs setPolicerEntryInArgs;
	LLDEnetDma_t *hLLDma = &hLLDEnet->dma;
	int32_t status;

	memset(&setPolicerEntryInArgs, 0, sizeof (setPolicerEntryInArgs));
	setPolicerEntryInArgs.policerMatch.policerMatchEnMask =
		CPSW_ALE_POLICER_MATCH_MACDST;
	setPolicerEntryInArgs.policerMatch.dstMacAddrInfo.portNum = 0U;
	setPolicerEntryInArgs.policerMatch.dstMacAddrInfo.addr.vlanId = vlanId;
	memcpy(&setPolicerEntryInArgs.policerMatch.dstMacAddrInfo.addr.addr[0U],
		   &dstMacAddr[0U], ENET_MAC_ADDR_LEN);

	setPolicerEntryInArgs.threadIdEn = true;
	setPolicerEntryInArgs.threadId = hLLDma->rxFlowIdx;

	ENET_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerEntryInArgs, &setPolicerEntryOutArgs);
	ENET_IOCTL(hLLDEnet->hEnet, hLLDEnet->coreId,
			CPSW_ALE_IOCTL_SET_POLICER, &prms, status);
	if (status != ENET_SOK) {
		UB_LOG(UBL_ERROR,"Enet_ioctl failed %d\n", status);
		return LLDENET_E_IOCTL;
	}
	return LLDENET_E_OK;
}

static bool IsMacAddrSet(uint8_t *mac)
{
	return ((mac[0]|mac[1]|mac[2]|mac[3]|mac[4]|mac[5]) != 0);
}

static uint32_t LLDEnetReceiveRxReadyPkts(LLDEnetDma_t *hLLDma)
{
	EnetDma_PktQ rxReadyQ;
	EnetDma_Pkt *pktInfo;
	int32_t status;
	uint32_t rxReadyCnt = 0U;

	EnetQueue_initQ(&rxReadyQ);

	/* Retrieve any CPSW packets which are ready */
	status = EnetDma_retrieveRxPktQ(hLLDma->hRxCh, &rxReadyQ);
	rxReadyCnt = EnetQueue_getQCount(&rxReadyQ);
	if (status == ENET_SOK) {
		/* Queue the received packet to rxReadyQ and pass new ones from rxFreeQ */
		pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&rxReadyQ);
		while (pktInfo != NULL) {
			EnetDma_checkPktState(&pktInfo->pktState, ENET_PKTSTATE_MODULE_APP,
					ENET_PKTSTATE_APP_WITH_DRIVER, ENET_PKTSTATE_APP_WITH_READYQ);

			EnetQueue_enq(&hLLDma->rxReadyQ, &pktInfo->node);
			pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&rxReadyQ);
		}
	}

	return rxReadyCnt;
}

static void LLDEnetRxNotifyCb(void *cbArg)
{
	uint32_t rxReadyCnt = 0U;
	LLDEnetDma_t *hLLDma = cbArg;

	if (hLLDma == NULL) {
		return;
	}

	rxReadyCnt = LLDEnetReceiveRxReadyPkts(hLLDma);
	EnetAppUtils_assert(rxReadyCnt != 0U);
	if (hLLDma->rxNotifyCb == NULL) {
		return;
	}
	hLLDma->rxNotifyCb(hLLDma->rxCbArg);
}

static uint32_t LLDEnetRetrieveTxDonePkts(LLDEnetDma_t *hLLDma)
{
	EnetDma_PktQ txFreeQ;
	EnetDma_Pkt *pktInfo;
	int32_t status;
	uint32_t txFreeQCnt = 0U;

	EnetQueue_initQ(&txFreeQ);

	/* Retrieve any CPSW packets that may be free now */
	status = EnetDma_retrieveTxPktQ(hLLDma->hTxCh, &txFreeQ);
	if (status == ENET_SOK) {
		txFreeQCnt = EnetQueue_getQCount(&txFreeQ);

		pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&txFreeQ);
		while (NULL != pktInfo) {
			EnetDma_checkPktState(&pktInfo->pktState, ENET_PKTSTATE_MODULE_APP,
					ENET_PKTSTATE_APP_WITH_DRIVER, ENET_PKTSTATE_APP_WITH_FREEQ);

			EnetQueue_enq(&hLLDma->txFreePktInfoQ, &pktInfo->node);
			pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&txFreeQ);
		}
	}

	return txFreeQCnt;
}

static void LLDEnetTxNotifyCb(void *cbArg)
{
	uint32_t pktCount;
	LLDEnetDma_t *hLLDma = cbArg;

	if (hLLDma == NULL) {
		return;
	}

	pktCount = LLDEnetRetrieveTxDonePkts(hLLDma);
	EnetAppUtils_assert(pktCount != 0U);

	if (hLLDma->txNotifyCb == NULL) {
		return;
	}
	hLLDma->txNotifyCb(hLLDma->txCbArg);
}

LLDEnet_t *LLDEnetOpen(LLDEnetCfg_t *cfg)
{
	EnetApp_HandleInfo handleInfo;
	EnetPer_AttachCoreOutArgs attachInfo;
	LLDEnet_t *hLLDEnet;
	int res;

	if (cfg == NULL) {
		UB_LOG(UBL_ERROR,"%s:invalid param\n", __func__);
		return NULL;
	}

	hLLDEnet = malloc(sizeof(LLDEnet_t));
	EnetAppUtils_assert(hLLDEnet != NULL);
	memset(hLLDEnet, 0, sizeof(LLDEnet_t));

	hLLDEnet->coreId = EnetSoc_getCoreId();
	EnetApp_acquireHandleInfo(cfg->enetType, cfg->instId, &handleInfo);
	EnetApp_coreAttach(cfg->enetType, cfg->instId,
					   hLLDEnet->coreId, &attachInfo);
	EnetAppUtils_assert(handleInfo.hEnet != NULL);

	hLLDEnet->hEnet = handleInfo.hEnet;
	hLLDEnet->coreKey = attachInfo.coreKey;
	hLLDEnet->enetType = cfg->enetType;
	hLLDEnet->instId = cfg->instId;

	hLLDEnet->dma.hLLDEnet = hLLDEnet;
	hLLDEnet->dma.nRxPkts = cfg->nRxPkts;
	if (hLLDEnet->dma.nRxPkts == 0) {
		hLLDEnet->dma.nRxPkts = ENET_MEM_NUM_RX_PKTS;
	}
	hLLDEnet->dma.nTxPkts = cfg->nTxPkts;
	if (hLLDEnet->dma.nTxPkts == 0) {
		hLLDEnet->dma.nTxPkts = ENET_MEM_NUM_TX_PKTS;
	}
	hLLDEnet->dma.pktSize = cfg->pktSize;
	if (hLLDEnet->dma.pktSize == 0) {
		hLLDEnet->dma.pktSize = ENET_MEM_LARGE_POOL_PKT_SIZE;
	}
	hLLDEnet->dma.txNotifyCb = cfg->txNotifyCb;
	hLLDEnet->dma.rxNotifyCb = cfg->rxNotifyCb;
	hLLDEnet->dma.txCbArg = cfg->txCbArg;
	hLLDEnet->dma.rxCbArg = cfg->rxCbArg;
	hLLDEnet->dma.txChId = cfg->dmaTxChId;
	hLLDEnet->dma.rxChId = cfg->dmaRxChId;

	DmaOpen(&hLLDEnet->dma);
	if (IsMacAddrSet(cfg->dstMacAddr)) {
		res = FilterVlanDestMac(hLLDEnet, cfg->dstMacAddr, cfg->vlanId);
		EnetAppUtils_assert(res == 0);
	}

	return hLLDEnet;
}

void LLDEnetClose(LLDEnet_t *hLLDEnet)
{
	if (hLLDEnet == NULL) {
		return;
	}
	DmaClose(&hLLDEnet->dma);
	EnetApp_coreDetach(hLLDEnet->enetType, hLLDEnet->instId,
					   hLLDEnet->coreId, hLLDEnet->coreKey);
	//FIXME: anything else need to be released?
	free(hLLDEnet);
}

int LLDEnetFilter(LLDEnet_t *hLLDEnet, uint8_t *destMacAddr, uint32_t vlanId)
{
	if ((hLLDEnet == NULL) || (destMacAddr == NULL)) {
		return LLDENET_E_PARAM;
	}
	return FilterVlanDestMac(hLLDEnet, destMacAddr, vlanId);
}

int LLDEnetSendMulti(LLDEnet_t *hLLDEnet, LLDEnetFrame_t *frames, uint32_t nFrames)
{
	EnetDma_PktQ txSubmitQ;
	EnetDma_Pkt *pktInfo;
	uint8_t *txFrame;
	LLDEnetDma_t *hLLDma;
	int32_t status;
	int i = 0;

	if ((hLLDEnet == NULL) || (frames == NULL) || (nFrames == 0)) {
		return LLDENET_E_PARAM;
	}
	hLLDma = &hLLDEnet->dma;
	if (EnetQueue_getQCount(&hLLDma->txFreePktInfoQ) < nFrames) {
		return LLDENET_E_NOBUF;
	}

	EnetQueue_initQ(&txSubmitQ);

	for (i = 0; i < nFrames; i++) {
		pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&hLLDma->txFreePktInfoQ);
		EnetAppUtils_assert(pktInfo != NULL);

		EnetAppUtils_assert(pktInfo->sgList.list[0].segmentAllocLen >= frames[i].size);
		txFrame = (uint8_t *)pktInfo->sgList.list[0].bufPtr;
		memcpy(txFrame, frames[i].buf, frames[i].size);
		pktInfo->sgList.list[0].segmentFilledLen = frames[i].size;
		pktInfo->appPriv = (void *)hLLDEnet;
		pktInfo->txPortNum = (Enet_MacPort)ENET_MACPORT_NORM(frames[i].port);
		if (frames[i].tc < 0) {
			pktInfo->txPktTc = ENET_TRAFFIC_CLASS_INV;
		} else {
			pktInfo->txPktTc = frames[i].tc;
		}

		EnetDma_checkPktState(&pktInfo->pktState, ENET_PKTSTATE_MODULE_APP,
				ENET_PKTSTATE_APP_WITH_FREEQ, ENET_PKTSTATE_APP_WITH_DRIVER);

		EnetQueue_enq(&txSubmitQ, &pktInfo->node);
	}

	status = EnetDma_submitTxPktQ(hLLDma->hTxCh, &txSubmitQ);
	if (status != ENET_SOK) {
		return LLDENET_E_DMA;
	}

	return LLDENET_E_OK;
}

int LLDEnetSend(LLDEnet_t *hLLDEnet, LLDEnetFrame_t *frame)
{
	return LLDEnetSendMulti(hLLDEnet, frame, 1);
}

int LLDEnetRecv(LLDEnet_t *hLLDEnet, LLDEnetFrame_t *frame)
{
	EnetDma_Pkt *pktInfo;
	EthFrame *rxFrame;
	uint32_t rxReadyCnt;
	int res = LLDENET_E_OK;
	LLDEnetDma_t *hLLDma;

	if ((hLLDEnet == NULL) || (frame == NULL) ||
		(frame->buf == NULL) || (frame->size == 0)) {
		return LLDENET_E_PARAM;
	}
	hLLDma = &hLLDEnet->dma;

	rxReadyCnt = EnetQueue_getQCount(&hLLDma->rxReadyQ);
	if (rxReadyCnt == 0) {
		return LLDENET_E_NOAVAIL;
	}
	/* Consume the received packets and release them */
	pktInfo = (EnetDma_Pkt *)EnetQueue_deq(&hLLDma->rxReadyQ);
	EnetDma_checkPktState(&pktInfo->pktState, ENET_PKTSTATE_MODULE_APP,
				ENET_PKTSTATE_APP_WITH_READYQ, ENET_PKTSTATE_APP_WITH_FREEQ);

	EnetAppUtils_assert(pktInfo->sgList.numScatterSegments == 1);

	rxFrame = (EthFrame *)pktInfo->sgList.list[0].bufPtr;
	if (frame->size >= pktInfo->sgList.list[0].segmentFilledLen) {
		frame->size = pktInfo->sgList.list[0].segmentFilledLen;
		memcpy(frame->buf, rxFrame, frame->size);
		frame->port = (uint8_t)pktInfo->rxPortNum;
	} else {
		res = LLDENET_E_BUFSIZE;
	}

	/* Release the received packet */
	EnetQueue_enq(&hLLDma->rxFreeQ, &pktInfo->node);

	EnetAppUtils_validatePacketState(&hLLDma->rxFreeQ, ENET_PKTSTATE_APP_WITH_FREEQ,
				ENET_PKTSTATE_APP_WITH_DRIVER);

	EnetDma_submitRxPktQ(hLLDma->hRxCh, &hLLDma->rxFreeQ);

	return res;
}

void LLDEnetCfgInit(LLDEnetCfg_t *cfg)
{
	if (cfg == NULL) {
		return;
	}
	memset(cfg, 0, sizeof(LLDEnetCfg_t));
	cfg->dmaTxChId = -1;
	cfg->dmaRxChId = -1;
}

bool LLDEnetIsPortUp(LLDEnet_t *hLLDEnet, uint8_t portNum)
{
	if (hLLDEnet == NULL) {
		return false;
	}
	return EnetAppUtils_isPortLinkUp(hLLDEnet->hEnet, hLLDEnet->coreId,
				(Enet_MacPort)(ENET_MACPORT_NORM(portNum)));
}

int LLDEnetGetLinkInfo(LLDEnet_t *hLLDEnet, uint8_t portNum,
					   uint32_t *speed, uint32_t *duplex)
{
	EnetPhy_GenericInArgs phyInArgs;
	EnetMacPort_LinkCfg phyOutArgs;
	Enet_IoctlPrms prms;
	int32_t status;

	if ((hLLDEnet == NULL) || (speed == NULL) || (duplex == NULL)) {
		return LLDENET_E_PARAM;
	}

	/* Get port link state (speed & duplexity) from PHY */
	phyInArgs.macPort = (Enet_MacPort)(ENET_MACPORT_NORM(portNum));
	ENET_IOCTL_SET_INOUT_ARGS(&prms, &phyInArgs, &phyOutArgs);
	ENET_IOCTL(hLLDEnet->hEnet, hLLDEnet->coreId,
			   ENET_PHY_IOCTL_GET_LINK_MODE, &prms, status);
	if (status != ENET_SOK) {
		UB_LOG(UBL_ERROR,"Failed to get link info: %d\n", status);
		return LLDENET_E_IOCTL;
	}

	if (phyOutArgs.speed == ENET_SPEED_10MBIT) {
		*speed = 10;
	} else if (phyOutArgs.speed == ENET_SPEED_100MBIT) {
		*speed = 100;
	} else if (phyOutArgs.speed == ENET_SPEED_1GBIT) {
		*speed = 1000;
	} else {
		*speed = 0;
	}

	if (phyOutArgs.duplexity == ENET_DUPLEX_FULL) {
		*duplex = 1;
	} else {
		*duplex = 0;
	}

	return LLDENET_E_OK;
}

int LLDEnetAllocMac(LLDEnet_t *hLLDEnet, uint8_t *srcMacAddr)
{
	int32_t status;

	if ((hLLDEnet == NULL) || (srcMacAddr == NULL)) {
		return LLDENET_E_PARAM;
	}
	status = EnetAppUtils_allocMac(hLLDEnet->hEnet, hLLDEnet->coreKey,
								   hLLDEnet->coreId, srcMacAddr);
	if (status != ENET_SOK) {
		UB_LOG(UBL_ERROR,"Failed to alloc Mac: %d\n", status);
		return LLDENET_E_IOCTL;
	}
	return LLDENET_E_OK;
}

void LLDEnetFreeMac(LLDEnet_t *hLLDEnet, uint8_t *srcMacAddr)
{
	if ((hLLDEnet == NULL) || (srcMacAddr == NULL)) {
		return;
	}
	EnetAppUtils_freeMac(hLLDEnet->hEnet, hLLDEnet->coreKey,
						 hLLDEnet->coreId, srcMacAddr);
}

int LLDEnetSetTxNotifyCb(LLDEnet_t *hLLDEnet, void (*txNotifyCb)(void *arg), void *arg)
{
	if (hLLDEnet == NULL) {
		return LLDENET_E_PARAM;
	}
	hLLDEnet->dma.txNotifyCb = txNotifyCb;
	hLLDEnet->dma.txCbArg = arg;
	return LLDENET_E_OK;
}

int LLDEnetSetRxNotifyCb(LLDEnet_t *hLLDEnet, void (*rxNotifyCb)(void *arg), void *arg)
{
	if (hLLDEnet == NULL) {
		return LLDENET_E_PARAM;
	}
	hLLDEnet->dma.rxNotifyCb = rxNotifyCb;
	hLLDEnet->dma.rxCbArg = arg;
	return LLDENET_E_OK;
}
