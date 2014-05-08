#include "hpgv_tf.h"
#include "assert.h"

static float stepBase = 100.f;

void hpgv_tf_raf_integrate(float* tf, int tfsize,
		float left_value, float rite_value, float left_depth, float rite_depth,
		float sampling_spacing, hpgv_raf_t* histogram)
{
    int binBeg = CLAMP((int)(left_value * HPGV_RAF_BIN_NUM), 0, HPGV_RAF_BIN_NUM - 1);
    int binEnd = CLAMP((int)(rite_value * HPGV_RAF_BIN_NUM), 0, HPGV_RAF_BIN_NUM - 1);
    int dir = 0;
    if (binBeg != binEnd)
        dir = (binEnd - binBeg) / abs(binEnd - binBeg);
    double valBeg, valEnd;
    double length;
    float intenBeg, intenEnd, intensity;
    float alpha, attenuation;
    // if binBeg == binEnd
    if (binBeg == binEnd)
    {
        valBeg = left_value;
        valEnd = rite_value;
        float intenBeg = tf[CLAMP((int)(tfsize * valBeg), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
        float intenEnd = tf[CLAMP((int)(tfsize * valEnd), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
        float intensity = (intenBeg + intenEnd) * 0.5;
        float alpha = 1.0 - pow(1.0 - intensity, length / stepBase);
        float attenuation = (1.f - histogram->attenuation) * alpha;
        histogram->attenuation += attenuation;
        histogram->raf[binBeg] += attenuation;
        if (left_depth < histogram->depths[binBeg])
        	histogram->depths[binBeg] = left_depth;
    } else
    {
        assert(dir != 0);
        // the beginning bin, which is partially included
        valBeg = left_value;
        if (dir > 0)
            valEnd = (double)(binBeg + 1) / (double)(HPGV_RAF_BIN_NUM);
        else
            valEnd = (double)(binBeg) / (double)(HPGV_RAF_BIN_NUM);
        length = (valEnd - valBeg) / (rite_value - left_value) * sampling_spacing;
        intenBeg = tf[CLAMP((int)(tfsize * valBeg), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
        intenEnd = tf[CLAMP((int)(tfsize * valEnd), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
        intensity = (intenBeg + intenEnd) * 0.5;
        alpha = 1.0 - pow(1.0 - intensity, length / stepBase);
        attenuation = (1.f - histogram->attenuation) * alpha;
        histogram->attenuation += attenuation;
        histogram->raf[binBeg] += attenuation;
        if (left_depth < histogram->depths[binBeg])
        	histogram->depths[binBeg] = left_depth;
        // the bins in between
        int binId;
        for (binId = binBeg + dir; binId != binEnd; binId += dir)
        {
            valBeg = (double)(binBeg) / (double)(HPGV_RAF_BIN_NUM);
            valEnd = (double)(binEnd) / (double)(HPGV_RAF_BIN_NUM);
            length = (valEnd - valBeg) / (rite_value - left_value) * sampling_spacing;
            intenBeg = tf[CLAMP((int)(tfsize * valBeg), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
            intenEnd = tf[CLAMP((int)(tfsize * valEnd), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
            intensity = (intenBeg + intenEnd) * 0.5;
            alpha = 1.0 - pow(1.0 - intensity, length / stepBase);
            attenuation = (1.f - histogram->attenuation) * alpha;
            histogram->attenuation += attenuation;
            histogram->raf[binId] += attenuation;
            float depth = (valBeg - left_value) / (rite_value - left_value) * (rite_depth - left_depth) + left_depth;
	        if (left_depth < histogram->depths[binId])
	        	histogram->depths[binId] = left_depth;
        }
        // the end bin, which is also partially included
        if (dir > 0)
            valBeg = (double)(binEnd) / (double)(HPGV_RAF_BIN_NUM);
        else
            valBeg = (double)(binEnd + 1) / (double)(HPGV_RAF_BIN_NUM);
        length = (valEnd - valBeg) / (rite_value - left_value) * sampling_spacing;
        intenBeg = tf[CLAMP((int)(tfsize * valBeg), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
        intenEnd = tf[CLAMP((int)(tfsize * valEnd), 0, HPGV_RAF_BIN_NUM - 1) * 4 + 3];
        intensity = (intenBeg + intenEnd) * 0.5;
        alpha = 1.0 - pow(1.0 - intensity, length / stepBase);
        attenuation = (1.f - histogram->attenuation) * alpha;
        histogram->attenuation += attenuation;
        histogram->raf[binEnd] += attenuation;
        float depth = (valBeg - left_value) / (rite_value - left_value) * (rite_depth - left_depth) + left_depth;
        if (left_depth < histogram->depths[binEnd])
        	histogram->depths[binEnd] = left_depth;
    }
}

void hpgv_tf_raf_seg_integrate(float* tf, int tfsize,
		float valbeg, float valend,
		float depbeg, float depend,
		int segid, hpgv_raf_seg_t* seg)
{

}