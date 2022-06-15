#include "top.h"

int nmixtures = 3;
mydata_weight alphaT = 0.01f;
mydata_variance Tb = 36.0f;
mydata_weight TB = 0.9f;
mydata_variance Tg = 3.0f * 3.0f;
mydata_variance varInit = 15.0f;
mydata_variance varMin = 4.0f;
mydata_variance varMax = 15.0f * 5;
mydata_weight prune = -0.01 * 0.05f;
mydata_weight alpha1 = mydata_weight(1.f) - alphaT;

template<class T>
void swap(T &a, T &b)
{
#pragma HLS inline
    T temp = a;
    a = b;
    b = temp;
}

void mog(hls::stream<RGB_pixel> &data_in, hls::stream<GRAY_pixel> &data_out,
		hls::stream<bgmodel_pixel> &bgmodel_in, hls::stream<bgmodel_pixel> &bgmodel_out){
#pragma HLS INTERFACE axis port=data_in
#pragma HLS INTERFACE axis port=data_out
#pragma HLS INTERFACE axis port=bgmodel_in
#pragma HLS INTERFACE axis port=bgmodel_out
#pragma HLS INTERFACE s_axilite port=return bundle=control
//#pragma HLS data_pack variable=bgmodel_in
//#pragma HLS data_pack variable=bgmodel_out

	for(int row = 0; row < 720; row++){
		for(int col=0;col<1280;col++){
#pragma HLS pipeline II=2
			unsigned char input[3];
			mydata_dData dData[3];
			//float weights[3];
			//float variance[3];
			//float means[9];
//#pragma HLS array_partition variable=means cyclic factor=3
			bool background = false;
			bool fitsPDF = false;
			mydata_weight totalWeight = 0.f;
			int pixel = row * 1280 + col;
			bgmodel_pixel stream_out;
			bgmodel_pixel stream_in = bgmodel_in.read();
			pixel_gmm bgmodel = stream_in.data;
			GRAY_pixel pix_out;
			RGB_pixel pix = data_in.read();
			input[0] = pix.data.range(7,0);
			input[1] = pix.data.range(15,8);
			input[2] = pix.data.range(23,16);

			int nmodes = bgmodel.modelused;
			for (int mode = 0; mode < 3; mode++){
				if (mode < nmodes){
					mydata_weight weight = alpha1 * bgmodel.model[mode].weight + prune;
					int swap_count = 0;

					if (!fitsPDF)
					{
					////check if it belongs to some of the remaining modes
						mydata_variance var = bgmodel.model[mode].variance;
						dData[0] = bgmodel.model[mode].means[0] - input[0];
						dData[1] = bgmodel.model[mode].means[1] - input[1];
						dData[2] = bgmodel.model[mode].means[2] - input[2];
						mydata_dist dist2 = dData[0] * dData[0] + dData[1] * dData[1] + dData[2] * dData[2];

						if (totalWeight < TB && dist2 < Tb * var)
							background = true;
						if (dist2 < Tg * var){
							fitsPDF = true;

							//updata weight
							weight += alphaT;
							mydata_weight k = alphaT / weight;

							//updata mean
							for (int c = 0; c < 3; c++){
								mydata_dData tmp = k * dData[c];
								bgmodel.model[mode].means[c] -= tmp;
							}

							//update variance
							mydata_variance varnew = var + k * (dist2 - var);
							//limit the variance
							varnew = MAX(varnew, varMin);
							varnew = MIN(varnew, varMax);
							bgmodel.model[mode].variance = varnew;

							for (int j = 2; j > 0; j--)
							{
								if (j < mode + 1){
									//check one up
									if (weight < bgmodel.model[j-1].weight)
										break;

									swap_count++;
									//swap one up
									swap(bgmodel.model[j], bgmodel.model[j-1]);
								}
							}
							//belongs to the mode - bFitsPDF becomes 1
						}
					}//!bFitsPDF)

					if (weight < -prune)
					{
						weight = 0.0;
						nmodes--;
					}
					bgmodel.model[mode-swap_count].weight = weight;//update weight by the calculated value
					totalWeight += weight;
				}
			}
			//go through all modes

			// not agree with any modes, set weights to zero (a new mode will be added below).
			mydata_weight invWeight = 0.f;
			//if (abs(totalWeight) > 0.001f) {
			invWeight = mydata_weight(1.f) / totalWeight;
			//}

			for (int mode = 0; mode < 3; mode++)
			{
				if (mode < nmodes)
					bgmodel.model[mode].weight *= invWeight;
			}

			//make new mode if needed and exit
			if (!fitsPDF && alphaT > mydata_weight(0.f))
			{
				// replace the weakest or add a new one
				int mode = nmodes == nmixtures ? nmixtures - 1 : nmodes++;

				if (nmodes == 1)
					bgmodel.model[mode].weight = 1.f;
				else
				{
					bgmodel.model[mode].weight = alphaT;

					// renormalize all other weights
					for (int j = 0; j < 2; j++)
						if (j < nmodes - 1)
							bgmodel.model[j].weight *= alpha1;
				}

				// init
				for (int c = 0; c < 3; c++)
					bgmodel.model[mode].means[c] = input[c];

				bgmodel.model[mode].variance = varInit;

				//sort
				//find the new place for it
				for (int j = 2; j > 0; j--)
				{
					if (j < nmodes){
						// check one up
						if (alphaT < bgmodel.model[j-1].weight)
							break;

						// swap one up
						swap(bgmodel.model[j], bgmodel.model[j-1]);
					}
				}
			}
			bgmodel.modelused = nmodes;

			stream_out.data = bgmodel;
			stream_out.keep = stream_in.keep;
			stream_out.strb = stream_in.strb;
			stream_out.user = stream_in.user;
			stream_out.last = stream_in.last;
			stream_out.id = stream_in.id;
			stream_out.dest = stream_in.dest;
			bgmodel_out.write(stream_out);
			pix_out.data = background ? 0 : 255;
			pix_out.keep = pix.keep;
			pix_out.strb = pix.strb;
			pix_out.user = pix.user;
			pix_out.last = pix.last;
			pix_out.id = pix.id;
			pix_out.dest = pix.dest;
			data_out.write(pix_out);
		}
	}
}
