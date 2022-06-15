/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "xil_printf.h"
#include "ff.h"
#include "xsdps.h"
#include "bmp.h"
#include "xmog.h"
#include "sleep.h"
#include "xtime_l.h"
#include "xaxidma.h"

#define INPUT_ADDR		0x08000000
#define OUTPUT_ADDR		0x0A000000
#define WEIGHT_ADDR     0x10000000
#define VIANCE_ADDR     0x15000000
#define MEANS_ADDR      0x20000000
#define MUSED_ADDR      0x30000000

#define BUF_SIZE 1280*720*3
u8 RD_Buf[BUF_SIZE];
u8 WR_Buf[BUF_SIZE];
static XMog mog;
static XMog mog1;

char *SD_Path = "0:/";
static FATFS SD_Dev;
int SD_init()
{
	FRESULT result;
	//-----------------------mount dev-----------------------------------------------
	result = f_mount(&SD_Dev,SD_Path, 0);
	if (result != 0) {
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}

void DDR_init()
{
	int i;
	float *p;
	//unsigned char* pp;
	p = (float *)WEIGHT_ADDR;
	for(i=0;i<1280*720*4;i++){
		*(p++) = 0;
	}
	p = (float *)VIANCE_ADDR;
	for(i=0;i<1280*720*4;i++){
		*(p++) = 0;
	}
	/**
	p = (float *)MEANS_ADDR;
	for(i=0;i<1280*720*5*3;i++){
		*(p++) = 0;
	}
	pp = (unsigned char *)MUSED_ADDR;
	for(i=0;i<1280*720;i++){
		*(pp++) = 0;
	}
	**/
}

//void Xil_DCacheFlush(void);

int main()
{
	FIL fsrc;
	FRESULT res;
	BMP_HeaderTypeDef bmp_header;
	UINT write_byte_nums;
	XTime tEnd,tCur;
	u32 tUsed;
	DDR_init();
	Xil_DCacheFlush();
	Xil_DCacheDisable();
	SD_init();
	bmp_header = BMP_Picture((u8 *)"input_1.bmp" , RD_Buf ,BUF_SIZE);
	unsigned char* p;
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}

	int status = XMog_Initialize(&mog, XPAR_MOG_0_DEVICE_ID);
	if(0 != status)
	{
		xil_printf("XHls_Mog_Initialize failed \n");
	}
	XMog_DisableAutoRestart(&mog);
	XMog_InterruptGlobalDisable(&mog);

	status = XMog_Initialize(&mog1, XPAR_MOG_1_DEVICE_ID);
	if(0 != status)
	{
		xil_printf("XHls_Mog_Initialize failed \n");
	}
	XMog_DisableAutoRestart(&mog1);
	XMog_InterruptGlobalDisable(&mog1);

    XAxiDma AxiDma;
    XAxiDma_Config *AxiDma_config;
    AxiDma_config = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
    if (!AxiDma_config) {
        printf("ERROR: Lookup of DMA configuration failed.\n\r");
        return XST_FAILURE;
    }
	status = XAxiDma_CfgInitialize(&AxiDma, AxiDma_config);
	if (status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", status);
		return XST_FAILURE;
	}
	if(XAxiDma_HasSg(&AxiDma)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DMA_TO_DEVICE);

    XAxiDma AxiDma1;
    XAxiDma_Config *AxiDma_config1;
    AxiDma_config1 = XAxiDma_LookupConfig(XPAR_AXIDMA_1_DEVICE_ID);
    if (!AxiDma_config1) {
        printf("ERROR: Lookup of DMA configuration failed.\n\r");
        return XST_FAILURE;
    }
	status = XAxiDma_CfgInitialize(&AxiDma1, AxiDma_config1);
	if (status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", status);
		return XST_FAILURE;
	}
	if(XAxiDma_HasSg(&AxiDma1)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}
	XAxiDma_IntrDisable(&AxiDma1, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&AxiDma1, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DMA_TO_DEVICE);

    XAxiDma AxiDma2;
    XAxiDma_Config *AxiDma_config2;
    AxiDma_config2 = XAxiDma_LookupConfig(XPAR_AXIDMA_2_DEVICE_ID);
    if (!AxiDma_config2) {
        printf("ERROR: Lookup of DMA configuration failed.\n\r");
        return XST_FAILURE;
    }
	status = XAxiDma_CfgInitialize(&AxiDma2, AxiDma_config2);
	if (status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", status);
		return XST_FAILURE;
	}
	if(XAxiDma_HasSg(&AxiDma2)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}
	XAxiDma_IntrDisable(&AxiDma2, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&AxiDma2, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DMA_TO_DEVICE);

    XAxiDma AxiDma3;
    XAxiDma_Config *AxiDma_config3;
    AxiDma_config3 = XAxiDma_LookupConfig(XPAR_AXIDMA_3_DEVICE_ID);
    if (!AxiDma_config3) {
        printf("ERROR: Lookup of DMA configuration failed.\n\r");
        return XST_FAILURE;
    }
	status = XAxiDma_CfgInitialize(&AxiDma3, AxiDma_config3);
	if (status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", status);
		return XST_FAILURE;
	}
	if(XAxiDma_HasSg(&AxiDma3)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}
	XAxiDma_IntrDisable(&AxiDma3, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&AxiDma3, XAXIDMA_IRQ_ALL_MASK,
						XAXIDMA_DMA_TO_DEVICE);

	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);

	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR + 1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR + 1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	//XTime_GetTime(&tEnd);
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	u8 *addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}

	res = f_open(&fsrc,(const TCHAR*)"output_1.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_2.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR + 1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR + 1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
	res = f_open(&fsrc,(const TCHAR*)"output_2.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_3.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR + 1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR + 1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
	res = f_open(&fsrc,(const TCHAR*)"output_3.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_4.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR + 1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR + 1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
	res = f_open(&fsrc,(const TCHAR*)"output_4.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_5.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
	res = f_open(&fsrc,(const TCHAR*)"output_5.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_6.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	XTime_GetTime(&tCur);
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
	res = f_open(&fsrc,(const TCHAR*)"output_6.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_7.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
	res = f_open(&fsrc,(const TCHAR*)"output_7.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_8.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
	res = f_open(&fsrc,(const TCHAR*)"output_8.bmp",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
	if(res != FR_OK){
		return res;
	}
	res = f_sync(&fsrc);
	if(res != FR_OK){
		return res;
	}
	f_close(&fsrc);


    bmp_header = BMP_Picture((u8 *)"input_9.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_9.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_10.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_A.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_11.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_B.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_12.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_C.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_13.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_D.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_14.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_E.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_15.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_F.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_16.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_G.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_17.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_H.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);

    bmp_header = BMP_Picture((u8 *)"input_18.bmp" , RD_Buf ,BUF_SIZE);
	p = (unsigned char *)INPUT_ADDR;
	for(int i=0; i<1280*720; i++){
		*(p++) = RD_Buf[3*i];
		*(p++) = RD_Buf[3*i+1];
		*(p++) = RD_Buf[3*i+2];
		*(p++) = 0;
	}
	//XMog_Set_bgmodel_ddr_model(&mog,(u32)WEIGHT_ADDR);
	XMog_Start(&mog);
	XMog_Start(&mog1);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)OUTPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)OUTPUT_ADDR+1280*360, 1280*360*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
		if (status != XST_SUCCESS){
			return XST_FAILURE;
	}
	XTime_GetTime(&tCur);
	status = XAxiDma_SimpleTransfer(&AxiDma, (u32)INPUT_ADDR, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma1, (u32)WEIGHT_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma2, (u32)INPUT_ADDR+1280*360*4, 1280*360*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	status = XAxiDma_SimpleTransfer(&AxiDma3, (u32)VIANCE_ADDR, 1280*360*8*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}
	while ((XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) ||
			(XAxiDma_Busy(&AxiDma2,XAXIDMA_DEVICE_TO_DMA)) || (XAxiDma_Busy(&AxiDma2,XAXIDMA_DMA_TO_DEVICE)) ){
		usleep(1);
	}
	XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	xil_printf("time elapsed one is %d us\r\n",tUsed);

	addr = (u8 *)OUTPUT_ADDR;
	for(int i=0;i<1280*720;i++){
		WR_Buf[3*i] = *addr;
		WR_Buf[3*i+1] = *addr;
		WR_Buf[3*i+2] = *(addr++);
	}
    res = f_open(&fsrc,(const TCHAR*)"output_I.bmp",FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,&bmp_header,sizeof(BMP_HeaderTypeDef),&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_write(&fsrc,WR_Buf,BUF_SIZE,&write_byte_nums);
    if(res != FR_OK){
    	return res;
    }
    res = f_sync(&fsrc);
    if(res != FR_OK){
    	return res;
    }
    f_close(&fsrc);


    return 0;
}
