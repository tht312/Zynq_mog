#include "bmp.h"
#include "ff.h"


/****************************************************************************
* Function Name  : BMP_ReadHeader
* Description    : 将读取到的数组函数转换位BPM文件信息结构体类型。由于在内存
*                * 上面数组的存储方式与结构体不同，所以要转换，而且SD读取到的
*                * 文件信息是小端模式。高位是低字节，低位是高字节，跟我们常用
*                * 的正好相反所以将数据转换过来。
* Input          : header：要转换的数组
*                * bmp：转换成的结构体
* Output         : None
* Return         : None
****************************************************************************/

void BMP_ReadHeader(uint8_t *header, BMP_HeaderTypeDef *bmp)
{

	bmp->fileHeader.bfType = (*(header + 1) << 8) | (*(header));
	header += 2;

	bmp->fileHeader.bfSize = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                         ((*(header + 1)) << 8) | (*header);
	header += 8;

	bmp->fileHeader.bfOffBits = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                            ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.bitSize = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                          ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biWidth = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                          ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biHeight = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                           ((*(header + 1)) << 8) | (*header);
	header += 6;

	bmp->infoHeader.biBitCount = ((*(header + 1)) << 8) | (*header);

	header += 2;

	bmp->infoHeader.biCompression = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                                ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biSizeImage = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                              ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biXPelsPerMeter = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                                  ((*(header + 1)) << 8) | (*header);
	header += 4;

	bmp->infoHeader.biYPelsPerMeter = ((*(header + 3)) << 24) | ((*(header + 2)) << 16) |
		                                  ((*(header + 1)) << 8) | (*header);
}


/****************************************************************************
* Function Name  : BMP_Picture
* Description    : 显示BMP格式的图片
* Input          : dir：要显示的图片路径和名字
* Output         : None
* Return         : None
****************************************************************************/


BMP_HeaderTypeDef BMP_Picture(uint8_t *dir , uint8_t  * buf ,uint32_t len)
{
		FRESULT res;
		FIL fsrc;
		UINT  br;
		UINT  a;

		uint8_t buffer[1024];

		BMP_HeaderTypeDef bmpHeader;
		
		/* 打开要读取的文件 */
		res = f_open(&fsrc, (const TCHAR*)dir, FA_READ);

		if(res == FR_OK)   //打开成功
	    {
			/* 读取BMP文件的文件信息 */
	        res = f_read(&fsrc, buffer, sizeof(BMP_HeaderTypeDef), &br);

			/* 将数组里面的数据放入到结构数组中，并排序好 */
			BMP_ReadHeader(buffer, &bmpHeader);

			a = bmpHeader.fileHeader.bfOffBits;    //去掉文件信息才开始是像素数据

			res=f_lseek(&fsrc, a);

			res = f_read(&fsrc, buf, len, &br);
	    }
        f_close(&fsrc);  //不论是打开，还是新建文件，一定记得关闭
        return bmpHeader;
}



