#include "bmp.h"
#include "ff.h"


/****************************************************************************
* Function Name  : BMP_ReadHeader
* Description    : ����ȡ�������麯��ת��λBPM�ļ���Ϣ�ṹ�����͡��������ڴ�
*                * ��������Ĵ洢��ʽ��ṹ�岻ͬ������Ҫת��������SD��ȡ����
*                * �ļ���Ϣ��С��ģʽ����λ�ǵ��ֽڣ���λ�Ǹ��ֽڣ������ǳ���
*                * �������෴���Խ�����ת��������
* Input          : header��Ҫת��������
*                * bmp��ת���ɵĽṹ��
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
* Description    : ��ʾBMP��ʽ��ͼƬ
* Input          : dir��Ҫ��ʾ��ͼƬ·��������
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
		
		/* ��Ҫ��ȡ���ļ� */
		res = f_open(&fsrc, (const TCHAR*)dir, FA_READ);

		if(res == FR_OK)   //�򿪳ɹ�
	    {
			/* ��ȡBMP�ļ����ļ���Ϣ */
	        res = f_read(&fsrc, buffer, sizeof(BMP_HeaderTypeDef), &br);

			/* ��������������ݷ��뵽�ṹ�����У�������� */
			BMP_ReadHeader(buffer, &bmpHeader);

			a = bmpHeader.fileHeader.bfOffBits;    //ȥ���ļ���Ϣ�ſ�ʼ����������

			res=f_lseek(&fsrc, a);

			res = f_read(&fsrc, buf, len, &br);
	    }
        f_close(&fsrc);  //�����Ǵ򿪣������½��ļ���һ���ǵùر�
        return bmpHeader;
}



