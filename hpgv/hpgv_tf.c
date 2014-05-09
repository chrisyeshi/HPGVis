#include "hpgv_tf.h"
#include "assert.h"

//
//
// Local Declarations
//
//

#define nPerColor 4

typedef struct tf_color_t {
    float r, g, b, a;
} tf_color_t;

//
//
// Local Variables
//
//

static float stepBase = 10.f;

//
//
// Local Functions
//
//

tf_color_t tf_raf_bin_integrate(float* tf, int tfsize,
        float valbeg, float valend, float length)
{
    // error checking
    float small = valbeg < valend ? valbeg : valend;
    int binId = small * HPGV_RAF_BIN_NUM;
    float binRange[2] = { (float)binId / HPGV_RAF_BIN_NUM, (float)(binId + 1) / HPGV_RAF_BIN_NUM };
    assert(valbeg >= binRange[0] && valbeg <= binRange[1]);
    assert(valend >= binRange[0] && valend <= binRange[1]);
    // here we go
    tf_color_t colorbeg = hpgv_tf_sample(tf, tfsize, valbeg);
    tf_color_t colorend = hpgv_tf_sample(tf, tfsize, valend);
    tf_color_t color;
    color.r = (colorbeg.r + colorend.r) * 0.5f;
    color.g = (colorbeg.g + colorend.g) * 0.5f;
    color.b = (colorbeg.b + colorend.b) * 0.5f;
    color.a = (colorbeg.a + colorend.a) * 0.5f;
    // adjust to step size
    color.a = 1.0 - pow(1.0 - color.a, length / stepBase);
    color.r *= color.a;
    color.g *= color.a;
    color.b *= color.a;
    return color;
}

//
//
// Public Functions
//
//

tf_color_t hpgv_tf_sample(float* tf, int tfsize, float val)
{
    assert(val >= 0.f && val <= 1.f);
    // sample position in tf space
    float x = val * (float)tfsize - 0.5f;
    // clamp to edge
    if (x < 0.f) {
        tf_color_t color;
        memcpy(&color, &tf[nPerColor * 0], sizeof(tf_color_t));
        return color;
    }
    if (x >= (float)(tfsize - 1)) {
        tf_color_t color;
        memcpy(&color, &tf[nPerColor * (tfsize - 1)], sizeof(tf_color_t));
        return color;
    }
    // linear interpolate
    int head = (int)(x + 0);
    int tail = (int)(x + 1);
    float ratio = x - (float)head;
    assert(tail <= tfsize - 1);
    tf_color_t color_head, color_tail;
    memcpy(&color_head, &tf[nPerColor * head], sizeof(tf_color_t));
    memcpy(&color_tail, &tf[nPerColor * tail], sizeof(tf_color_t));
    tf_color_t color;
    color.r = color_head.r * (1.f - ratio) + color_tail.r * ratio;
    color.g = color_head.g * (1.f - ratio) + color_tail.g * ratio;
    color.b = color_head.b * (1.f - ratio) + color_tail.b * ratio;
    color.a = color_head.a * (1.f - ratio) + color_tail.a * ratio;
    return color;
}

void hpgv_tf_raf_integrate(float* tf, int tfsize,
		float left_value, float rite_value, float left_depth, float rite_depth,
		float sampling_spacing, hpgv_raf_t* histogram)
{
    int binBeg = CLAMP((int)(left_value * HPGV_RAF_BIN_NUM), 0, HPGV_RAF_BIN_NUM - 1);
    int binEnd = CLAMP((int)(rite_value * HPGV_RAF_BIN_NUM), 0, HPGV_RAF_BIN_NUM - 1);
    int dir = 0;
    if (binBeg != binEnd)
        dir = (binEnd - binBeg) / abs(binEnd - binBeg);
    // if binBeg == binEnd
    if (binBeg == binEnd)
    {
        float valBeg = left_value;
        float valEnd = rite_value;
        tf_color_t color = tf_raf_bin_integrate(tf, tfsize, valBeg, valEnd, sampling_spacing);
        float attenuation = (1.f - histogram->attenuation) * color.a;
        histogram->attenuation += attenuation;
        histogram->raf[binBeg] += attenuation;
        if (left_depth < histogram->depths[binBeg])
            histogram->depths[binBeg] = left_depth;
    } else
    {
        assert(dir != 0);
        // the beginning bin, which is partially included
        {
            float valBeg = left_value;
            float valEnd;
            if (dir > 0)
                valEnd = (float)(binBeg + 1) / (float)(HPGV_RAF_BIN_NUM);
            else
                valEnd = (float)(binBeg) / (float)(HPGV_RAF_BIN_NUM);
            float length = (valEnd - valBeg) / (rite_value - left_value) * sampling_spacing;
            tf_color_t color  = tf_raf_bin_integrate(tf, tfsize, valBeg, valEnd, length);
            float attenuation = (1.f - histogram->attenuation) * color.a;
            histogram->attenuation += attenuation;
            histogram->raf[binBeg] += attenuation;
            if (left_depth < histogram->depths[binBeg])
                histogram->depths[binBeg] = left_depth;
        }
        // the bins in between
        int binId;
        for (binId = binBeg + dir; binId != binEnd; binId += dir)
        {
            float valBeg, valEnd;
            if (dir > 0) {
                valBeg = (float)(binId + 0) / (float)(HPGV_RAF_BIN_NUM);
                valEnd = (float)(binId + 1) / (float)(HPGV_RAF_BIN_NUM);
            } else {
                valBeg = (float)(binId + 1) / (float)(HPGV_RAF_BIN_NUM);
                valEnd = (float)(binId + 0) / (float)(HPGV_RAF_BIN_NUM);
            }
            float length = (valEnd - valBeg) / (rite_value - left_value) * sampling_spacing;
            tf_color_t color = tf_raf_bin_integrate(tf, tfsize, valBeg, valEnd, length);
            float attenuation = (1.f - histogram->attenuation) * color.a;
            histogram->attenuation += attenuation;
            histogram->raf[binId] += attenuation;
            float depth = (valBeg - left_value) / (rite_value - left_value) * (rite_depth - left_depth) + left_depth;
            if (depth < histogram->depths[binId])
                histogram->depths[binId] = depth;
        }
        // the end bin, which is also partially included
        {
            float valBeg;
            if (dir > 0)
                valBeg = (float)(binEnd + 0) / (float)(HPGV_RAF_BIN_NUM);
            else
                valBeg = (float)(binEnd + 1) / (float)(HPGV_RAF_BIN_NUM);
            float valEnd = rite_value;
            float length = (valEnd - valBeg) / (rite_value - left_value) * sampling_spacing;
            tf_color_t color  = tf_raf_bin_integrate(tf, tfsize, valBeg, valEnd, length);
            float attenuation = (1.f - histogram->attenuation) * color.a;
            histogram->attenuation += attenuation;
            histogram->raf[binEnd] += attenuation;
            float depth = (valBeg - left_value) / (rite_value - left_value) * (rite_depth - left_depth) + left_depth;
            if (depth < histogram->depths[binEnd])
                histogram->depths[binEnd] = depth;
        }
    }
}

void hpgv_tf_raf_seg_integrate(float* tf, int tfsize,
        float left_value, float rite_value, float left_depth, float rite_depth,
        float sampling_spacing, hpgv_raf_seg_t* seg, int segid)
{
    hpgv_raf_t histogram;
    hpgv_raf_reset(&histogram);
    histogram.attenuation = seg->attenuation;
    hpgv_tf_raf_integrate(tf, tfsize,
            left_value, rite_value, 1.f - left_depth, 1.f - rite_depth,
            sampling_spacing, &histogram);
    // composite
    seg->attenuation = histogram.attenuation;
    int segBin;
    for (segBin = 0; segBin < HPGV_RAF_SEG_NUM; ++segBin)
    {
        int rafBin = segBin + segid * HPGV_RAF_SEG_NUM;
        seg->raf[segBin] += histogram.raf[rafBin];
        if (histogram.depths[rafBin] < 1.f - seg->depths[segBin])
            seg->depths[segBin] = 1.f - histogram.depths[rafBin];
    }
}
