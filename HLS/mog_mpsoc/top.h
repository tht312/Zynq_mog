#ifndef _MOG_H_
#define _MOG_H_
#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>

#ifndef MIN
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

typedef ap_ufixed<10,1,AP_RND,AP_SAT> mydata_weight;
typedef ap_ufixed<16,7,AP_RND,AP_SAT> mydata_variance;
typedef ap_ufixed<16,8,AP_RND,AP_SAT> mydata_mean;
typedef ap_ufixed<32,16,AP_RND,AP_SAT> mydata_dist;
typedef ap_fixed<17,9,AP_RND,AP_SAT> mydata_dData;

struct GMM
{
	mydata_weight weight;
	mydata_variance variance;
	mydata_mean means[3];
};

struct pixel_gmm
{
	GMM model[3];
	ap_uint<16> modelused;
	ap_int<18> bubble;
};

template<int D,int U,int TI,int TD>
struct ap_axim{
	pixel_gmm       data;
    ap_uint<(D+7)/8> keep;
    ap_uint<(D+7)/8> strb;
    ap_uint<U>       user;
    ap_uint<1>       last;
    ap_uint<TI>      id;
    ap_uint<TD>      dest;
};

typedef ap_axiu<32,1,1,1> RGB_pixel;
typedef ap_axiu<8,1,1,1> GRAY_pixel;
typedef ap_axim<256,1,1,1> bgmodel_pixel;

void mog(hls::stream<RGB_pixel> &data_in, hls::stream<GRAY_pixel> &data_out,
		hls::stream<bgmodel_pixel> &bgmodel_in, hls::stream<bgmodel_pixel> &bgmodel_out);


#endif
