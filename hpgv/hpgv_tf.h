#ifndef HPGV_TF_H
#define HPGV_TF_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include "hpgv_gl.h"
#include "hpgv_utilmath.h"
#include "hpgv_util.h"

void hpgv_tf_raf_integrate(float* tf, int tfsize,
		float left_value, float rite_value, float left_depth, float rite_depth,
		float sampling_spacing, hpgv_raf_t* histogram);

void hpgv_tf_raf_seg_integrate(float* tf, int tfsize,
		float valbeg, float valend,
		float depbeg, float depend,
		int segid, hpgv_raf_seg_t* seg);

#ifdef __cplusplus
}
#endif

#endif
