/**
 * hpgv_render.c
 *
 * Copyright (c) 2008 Hongfeng Yu
 *
 * Contact:
 * Hongfeng Yu
 * hfstudio@gmail.com
 * 
 * 
 * All rights reserved.  May not be used, modified, or copied 
 * without permission.
 *
 */

#include "hpgv_render.h"
#include <vector>

#define INBOX(point_3d, ll_tick, ur_tick) \
(((point_3d).x3d >= (ll_tick)[0]) && ((point_3d).x3d < (ur_tick[0])) && \
 ((point_3d).y3d >= (ll_tick)[1]) && ((point_3d).y3d < (ur_tick[1])) && \
 ((point_3d).z3d >= (ll_tick)[2]) && ((point_3d).z3d < (ur_tick[2])))

#define TAG_PARTICLE_NUM            0x2143
#define TAG_PARTICLE_DAT            0x2144

int MY_STEP_GHOST_PARTICLE_TIME         = HPGV_TIMING_UNIT_134;
int MY_STEP_VOLUME_RENDER_TIME          = HPGV_TIMING_UNIT_135;
int MY_STEP_PARTICLE_RENDER_TIME        = HPGV_TIMING_UNIT_136;

int MY_STEP_MULTI_COMPOSE_TIME          = HPGV_TIMING_UNIT_150;
int MY_STEP_MULTI_VOLREND_TIME          = HPGV_TIMING_UNIT_151;
int MY_STEP_MULTI_PARREND_TIME          = HPGV_TIMING_UNIT_152;
int MY_STEP_MULTI_GHOST_TIME            = HPGV_TIMING_UNIT_153;

/**
 * rgb_t
 *
 */
typedef struct rgb_t {
    float red, green, blue;
} rgb_t;


/**
 * rgba_t
 *
 */
typedef struct rgba_t {
    float red, green, blue, alpha;
} rgba_t;


/**
 * ray_t
 *
 */
typedef struct ray_t {
    point_3d_t start;
    point_3d_t dir;
} ray_t;


/**
 * pixel_t
 *
 */
typedef struct pixel_t {
    int x, y;
} pixel_t;


/**
 * pixel_ctl_t
 *
 */
typedef struct cast_ctl_t {
    pixel_t     pixel;
    int32_t     firstpos;
    int32_t     lastpos;
    ray_t       ray;
    uint64_t    offset;
} cast_ctl_t;



/**
 * vis_control_t
 *
 */
typedef struct vis_control_t {
    /* MPI */
    int             id, groupsize, root; 
    MPI_Comm        comm;  
    
    /* image type */
    int             type;
    int             format;
    
    /* rendering parameters*/
    int             updateview;
    hpgv::Parameter para;
    float           sampling_spacing;
    int             colormapsize;
    int             colormapformat;
    int             colormaptype;
    
    /* data */
    block_t         *block;
    
    /* internal rendering parameters */
    point_3d_t      eye_obj;
    double          screen_min_z;
    double          screen_max_z;
    float           block_depth;
    cast_ctl_t      *cast_ctl;
    uint64_t        castcount;
    
    
    /* compositing output */ 
    void            *colorimage;
    int             colorimagesize;
    int             colorimagetype;
    int             colorimageformat;
    int             rendercount;
    
    
    /* data rendering output */    
    void            *databuf_collect;
    void            *databuf;
    int             databuf_size;
    int             databuf_type;
    int             databuf_format;
    
    /* rendering choice */
    //int             renderparticle;
    int             rendervolume;
    
} vis_control_t;

static vis_control_t *theVisControl = NULL;


/**
 * vis_pixel_to_ray: 
 * 
 */
void 
vis_pixel_to_ray(vis_control_t *visctl, pixel_t pixel, 
               ray_t *ray, point_3d_t eye_obj)
{
    /* pixel object coordinate*/
    point_3d_t pixel_obj;
        
    /* calcuate the object coordinate*/
    hpgv_gl_unproject(pixel.x,
                      pixel.y,
//                      visctl->screen_max_z,
                      0.0,
                      &(pixel_obj.x3d),
                      &(pixel_obj.y3d),
                      &(pixel_obj.z3d));
    
    /* ray's start */
//    VEC_CPY(eye_obj, ray->start);
    VEC_CPY(pixel_obj, ray->start);
    
    /* ray's direction*/    
    VEC_MINUS(pixel_obj, eye_obj, ray->dir);

    normalize(&(ray->dir));
}


/**
 * ray_intersect_box
 *
 */
int
vis_ray_intersect_box(ray_t *ray, float lend[3], float hend[3],
                      double *tnear, double *tfar)
{
    double ray_d[3] , ray_o[3], temp,t1, t2;
    uint8_t i, isfirst = 1;
    
    ray_d[0] = ray->dir.x3d;
    ray_d[1] = ray->dir.y3d;
    ray_d[2] = ray->dir.z3d;

    ray_o[0] = ray->start.x3d;
    ray_o[1] = ray->start.y3d;
    ray_o[2] = ray->start.z3d;

    for (i=0; i<3; i++) {
        if (ray_d[i]==0) {
            if (ray_o[i] < lend[i] || ray_o[i] > hend[i]) {
                return HPGV_FALSE;
            }
        } else {
            t1 = (lend[i] - ray_o[i]) / ray_d[i];
            t2 = (hend[i] - ray_o[i]) / ray_d[i];
            if (t1 > t2) {
                temp = t2;
                t2 = t1;
                t1 = temp;
            }    
            if (isfirst == 1) {
                *tnear = t1;
                *tfar = t2;
                isfirst = 0;
            } else  {
                if (t1 > *tnear) {
                    *tnear = t1;
                }
    
                if (t2 < *tfar) {
                    *tfar = t2;
                }
            }
            if (*tnear > *tfar) {
                return HPGV_FALSE;
            }
            if (*tfar < 0) {
                return HPGV_FALSE;
            }
        }
    }

    return HPGV_TRUE;
}


/**
 * vis_ray_clip_box
 *
 */
int
vis_ray_clip_box(int id, ray_t *ray, double sampling_spacing, 
                 float ll[3], float ur[3],
                 int32_t *pnfirst, int32_t *pnlast)
{
    double tnear = 0, tfar = 0;
    int32_t nstart, nend, sample_pos;
    point_3d_t real_sample;
    
    HPGV_ASSERT_P(id, pnfirst, "pnfirst is null.", HPGV_ERR_MEM);
    HPGV_ASSERT_P(id, pnlast,  "pnlast is null.",  HPGV_ERR_MEM);
    
    /* A quick check */
    if (vis_ray_intersect_box(ray, ll, ur, &tnear, &tfar) == HPGV_FALSE) {
        return HPGV_FALSE;
    }
    
    nstart = (int32_t)(ceil(tnear / sampling_spacing) + 0.5);
    nstart = std::max(0, nstart);
    nend = (int32_t)(ceil(tfar / sampling_spacing) + 0.5);
    if (nend < nstart)
        return HPGV_FALSE;
   
    /* make sure that nstart is not greater than nend */
    HPGV_ASSERT_P(id, 
                  nstart <= nend, 
                  "ray_clip_box: fatal internal error. Abort!", 
                  HPGV_ERROR);

    *pnfirst = nstart;
    *pnlast = nend;
    return HPGV_TRUE;
    
    HPGV_ASSERT_P(id, 
                  *pnlast - *pnfirst >= 1,
                  "ray_clip_box: fatal internal error. Abort!", 
                  HPGV_ERROR);
    
    /* Must clip */
    return HPGV_TRUE;
}


/**
 * vis_update_eyedepth
 *
 */
void
vis_update_eyedepth(vis_control_t *visctl)
{
    point_3d_t eye_org;
    point_3d_t blk_obj_center;
    
    /* the eye position in the eye coordinate*/
    eye_org.x3d = 0;
    eye_org.y3d = 0;
    eye_org.z3d = 0;
    
    /* transform the eye position to the object space*/
    double view_matrix_inv[16];
    hpgv_gl_get_viewmatrixinv(view_matrix_inv);
    MAT_TIME_VEC(view_matrix_inv, eye_org, visctl->eye_obj);
        
    /* the block depth */
    VEC_SET(blk_obj_center,
            visctl->block->blk_header.blk_obj_center[0],
            visctl->block->blk_header.blk_obj_center[1],
            visctl->block->blk_header.blk_obj_center[2]);
    
    visctl->block_depth = DIST_SQR_VEC(blk_obj_center, visctl->eye_obj);
}




/**
 * vis_update_projarea
 *
 */
void
vis_update_projarea(vis_control_t *visctl, float sampling_spacing) 
{
    int i;
    int32_t x, y;
    int32_t minscrx = 0, minscry = 0, maxscrx = 0, maxscry = 0;
    double minscrz = 0, maxscrz = 0;
    uint64_t totalcasts, index, all_index;    
    point_3d_t vertex[8], screen[8];
    ray_t ray;
    pixel_t pixel;
    int32_t local_firstpos, local_lastpos;

    
    /* allocate an array to keep track of non-background local pixels */
    totalcasts = hpgv_gl_get_framesize();    
    visctl->cast_ctl = (cast_ctl_t *)realloc(visctl->cast_ctl, 
                        sizeof(cast_ctl_t) * totalcasts);
    HPGV_ASSERT_P(visctl->id, visctl->cast_ctl, "Out of memory.", HPGV_ERR_MEM);

    
    /* eight vertexes */
    for (i = 0 ; i < 8 ; i++) {
        if (i % 2 == 0)
            vertex[i].x3d = visctl->block->blk_header.domain_obj_near[0];
        else
            vertex[i].x3d = visctl->block->blk_header.domain_obj_far[0];
        
        if ((i>>1) % 2 == 0)
            vertex[i].y3d = visctl->block->blk_header.domain_obj_near[1];
        else
            vertex[i].y3d = visctl->block->blk_header.domain_obj_far[1];
        
        if ((i>>2) % 2 == 0)
            vertex[i].z3d = visctl->block->blk_header.domain_obj_near[2];
        else
            vertex[i].z3d = visctl->block->blk_header.domain_obj_far[2];
    }    
    
    /* eight projected screen vertexes and bounding box*/
    int framebuf_size_x = hpgv_gl_get_framewidth();
    int framebuf_size_y = hpgv_gl_get_frameheight();
    
    for ( i = 0; i < 8; i++) {        
        hpgv_gl_project(vertex[i].x3d, vertex[i].y3d, vertex[i].z3d,
                        &(screen[i].x3d), &(screen[i].y3d), &(screen[i].z3d));
    
        if (i == 0){
            minscrx = maxscrx = (int32_t)screen[i].x3d;
            minscry = maxscry = (int32_t)screen[i].y3d;
            minscrz = maxscrz = screen[i].z3d;
        }else{
            if (minscrx > (int32_t)screen[i].x3d)
                minscrx = (int32_t)screen[i].x3d;
            if (maxscrx < (int32_t)screen[i].x3d)
                maxscrx = (int32_t)screen[i].x3d;
            if (minscry > (int32_t)screen[i].y3d)
                minscry = (int32_t)screen[i].y3d;
            if (maxscry < (int32_t)screen[i].y3d)
                maxscry = (int32_t)screen[i].y3d;
            if (minscrz > screen[i].z3d)
                minscrz = screen[i].z3d;
            if (maxscrz < screen[i].z3d)
                maxscrz = screen[i].z3d;
        }
    }
    
    if (minscrx < 0 ) {
        minscrx = 0;
    }
    if (minscrx > framebuf_size_x - 1) {
        minscrx = framebuf_size_x - 1;
    }
    if (maxscrx < 0 ) {
        maxscrx = 0;
    }
    if (maxscrx > framebuf_size_x - 1) {
        maxscrx = framebuf_size_x - 1;
    }
        
    if (minscry < 0 ) {
        minscry = 0;
    }
    if (minscry > framebuf_size_y - 1) {
        minscry = framebuf_size_y - 1;
    }
    if (maxscry < 0 ) {
        maxscry = 0;
    }
    if (maxscry > framebuf_size_y - 1) {
        maxscry = framebuf_size_y - 1;
    }
    
    visctl->screen_max_z = maxscrz;
    visctl->screen_min_z = minscrz;

    /* filter out the background pixels */
    index = 0;
    all_index = 0;
    for (y = minscry; y <= maxscry; y++) {
        for (x = minscrx; x <= maxscrx; x++) {
            
            pixel.x = x;
            pixel.y = y;
            vis_pixel_to_ray(visctl, pixel, &ray, visctl->eye_obj);
            
                
            if ( vis_ray_clip_box(visctl->id,
                                    &ray, 
                                    sampling_spacing,
                                    visctl->block->blk_header.blk_obj_raynear,
                                    visctl->block->blk_header.blk_obj_rayfar, 
                                    &local_firstpos,
                                    &local_lastpos) == HPGV_TRUE) 
            {
                /* record the pixel */
                visctl->cast_ctl[index].pixel    = pixel;
                visctl->cast_ctl[index].firstpos = local_firstpos;
                visctl->cast_ctl[index].lastpos  = local_lastpos;
                visctl->cast_ctl[index].ray      = ray;
                visctl->cast_ctl[index].offset   = all_index;

                index++;
                
            }
        }
    }


    /* shrink the array */
    visctl->castcount = index;
    visctl->cast_ctl = (cast_ctl_t *)realloc(visctl->cast_ctl,
                       visctl->castcount * sizeof(cast_ctl_t));
    if (index != 0) {
        HPGV_ASSERT_P(visctl->id, visctl->cast_ctl, "Out of memory.",
                    HPGV_ERR_MEM);
    }
    
}



/**
 * vis_color_comoposite
 * 
 */
void
vis_color_comoposite(rgba_t *partialcolor, rgba_t *compositecolor) 
{
    float a = 1 - compositecolor->alpha;
    compositecolor->red   += a * partialcolor->red;
    compositecolor->green += a * partialcolor->green;
    compositecolor->blue  += a * partialcolor->blue;
    compositecolor->alpha += a * partialcolor->alpha;
}


/**
 * vis_volume_lighting
 *
 */
void
vis_volume_lighting(block_t *block, point_3d_t gradient, const float lightpar[4],
                    point_3d_t *pos, point_3d_t eye, float *amb_diff, 
                    float *specular)
{
    point_3d_t lightdir, raydir, H;
    float ambient, diffuse, dotHV;   
    
    VEC_MINUS(eye, *pos, raydir);
    normalize(&raydir);
    
    //VEC_SET(lightdir, 0, 0, 1);
    //VEC_SET(lightdir, 1, 1, 1);
    VEC_SET(lightdir, raydir.x3d, raydir.y3d, raydir.z3d);
    
    normalize(&lightdir);

    float dot_a = VEC_DOT_VEC(lightdir, gradient);

    gradient.x3d *= -1;
    gradient.y3d *= -1;
    gradient.z3d *= -1;

    float dot_b = VEC_DOT_VEC(lightdir, gradient);

    float dot = (dot_a > dot_b ? dot_a : dot_b);
//    float dot = VEC_DOT_VEC(lightdir, gradient);

    ambient = lightpar[0];
    diffuse = lightpar[1] * dot;
    
    VEC_ADD(lightdir, raydir, H);
    normalize(&H);
   

    dot_a = VEC_DOT_VEC(H, gradient);

    gradient.x3d *= -1;
    gradient.y3d *= -1;
    gradient.z3d *= -1;

    dot_b = VEC_DOT_VEC(H, gradient);

    dotHV = (dot_a > dot_b ? dot_a : dot_b);
//    dotHV = VEC_DOT_VEC(H, gradient);

    *specular = 0.0;
    
    if (dotHV > 0.0) {
        *specular = lightpar[2] * pow(dotHV, lightpar[3]);
        *specular = CLAMP(*specular, 0, 1);
    }

    *amb_diff = ambient + diffuse;

    *amb_diff = CLAMP(*amb_diff, 0, 1);
}


/**
 * vis_render_pos
 *
 */
float
vis_render_pos(vis_control_t    *visctl,
               block_t          *block,
               ray_t            *ray,
               point_3d_t       *pos,
               void             *pixel_data,
               int              num_vol,
               float            sampling_spacing,
               const hpgv::Parameter::Image& image)
               // int              *id_vol,
               // para_tf_t        *tf_vol,
               // para_light_t     *light_vol)
{
    rgba_t partialcolor;

    int format = visctl->format;
        
#ifdef DRAW_PARTITION
    if (visctl->id % 3 == 0) {
        partialcolor.red   = (float)(visctl->id + 1)/visctl->groupsize;
        partialcolor.green = 0;
        partialcolor.blue  = 0;
        partialcolor.alpha = 1;
    }
    
    if (visctl->id % 3 == 1) {
        partialcolor.red   = 0;
        partialcolor.green = (float)(visctl->id + 1)/visctl->groupsize;
        partialcolor.blue  = 0;
        partialcolor.alpha = 1;
    }

    if (visctl->id % 3 == 2) {
        partialcolor.red   = 0;
        partialcolor.green = 0;
        partialcolor.blue  = (float)(visctl->id + 1)/visctl->groupsize;
        partialcolor.alpha = 1;
    }
    
    vis_color_comoposite(&partialcolor, color);
    
#else
    float sample, v;
    int id;
    int vol;
    float stepBase = 1.f;
    
    for (vol = 0; vol < num_vol; vol++) {

        if (block_get_value(block, image.volumes[vol].id, pos->x3d, pos->y3d, pos->z3d,
                            &v) == HPGV_FALSE)
        {
            continue;
        }

        sample = v;

        v = (visctl->colormapsize - 1) * v;
        v = CLAMP(v, 0, visctl->colormapsize - 1);
        
        id = (int)v;
        id *= 4;

        if (format == HPGV_RGBA) { 
            partialcolor.red    = image.tf[id+0];
            partialcolor.green  = image.tf[id+1];
            partialcolor.blue   = image.tf[id+2];
            partialcolor.alpha  = image.tf[id+3];

            if (image.volumes[vol].light.enable && partialcolor.alpha > 0.01){
                
                float amb_diff, specular;
                point_3d_t gradient;
                
                if (block_get_gradient(block, image.volumes[vol].id, 
                                    pos->x3d, pos->y3d, pos->z3d, 
                                    &gradient) == HPGV_FALSE)
                {
                    amb_diff = image.volumes[vol].light.parameter[0];
                    specular = 0;
                } else {
                    vis_volume_lighting(block,
                                        gradient,
                                        &image.volumes[vol].light.parameter[0],
                                        pos,
                                        visctl->eye_obj,
                                        &amb_diff,
                                        &specular);
                }

                partialcolor.red   = partialcolor.red   * amb_diff +
                                    specular * partialcolor.alpha;
                partialcolor.green = partialcolor.green * amb_diff +
                                    specular * partialcolor.alpha;
                partialcolor.blue  = partialcolor.blue  * amb_diff +
                                    specular * partialcolor.alpha;
            }
            
            // adjust to step size
            partialcolor.alpha = 1.0 - pow(1.0 - partialcolor.alpha, sampling_spacing / stepBase);
            partialcolor.red *= partialcolor.alpha;
            partialcolor.green *= partialcolor.alpha;
            partialcolor.blue *= partialcolor.alpha;

            vis_color_comoposite(&partialcolor, (rgba_t *)pixel_data);
        } else if (format == HPGV_RAF) {
            hpgv_raf_t * histogram = (hpgv_raf_t *)(pixel_data);
            
            int binsize = HPGV_RAF_BIN_NUM;
            int bin = (int)(binsize * sample);
            if (bin > binsize) {
                bin = binsize;
            }

            float intensity = image.tf[id+3];
            // adjust step size
            float alpha = 1.0 - pow(1.0 - intensity, sampling_spacing / stepBase);
            float attenuation 
                = (1.0 - histogram->raf[HPGV_RAF_ALPHA_BIN_NUM - 1]) * alpha;
            // histogram->raf[HPGV_RAF_ALPHA_BIN_NUM - 1] += alpha;
            histogram->raf[HPGV_RAF_ALPHA_BIN_NUM - 1] += attenuation;

            // basic way
            //======================================
            // histogram->raf[bin] += attenuation;
            // better method
            //======================================            
            float dx = sample * binsize - bin;
            float factor;

            if (dx < 0.5f) {
                factor = pow((0.5f - dx), 3.0f);
                histogram->raf[bin] += (1.0f - factor) * attenuation;
                 
                if (bin > 0) {
                    histogram->raf[bin - 1] += factor * attenuation;
                }
            } else {
                factor = pow((dx - 0.5f), 3.0f);
                histogram->raf[bin] += (1.0f - factor) * attenuation;
                 
                if (bin < binsize - 1) {
                    histogram->raf[bin + 1] += factor * attenuation;
                }
            }

        } else {
            HPGV_ABORT("Unsupported format", HPGV_ERROR);
        }
    }
    return sample;
#endif
}



/**
 * vis_ray_render_positions
 *
 */
void 
vis_ray_render_positions(vis_control_t  *visctl,
                         block_t        *block,
                         ray_t          *ray,
                         int32_t        firstpos,
                         int32_t        lastpos, 
                         void           *pixel_data, 
                         int            frag_x,
                         int            frag_y,
                         int            num_vol,
                         float          sampling_spacing,
                         const hpgv::Parameter::Image& image)
                         // int            *id_vol,
                         // para_tf_t      *tf_vol,
                         // para_light_t   *light_vol)
{
    int32_t pos;
    point_3d_t real_sample;
    double prev_value = -1.0;
    double prev_depth = 0.0;
    
    int format = visctl->format;
    
    for (pos = firstpos; pos < lastpos; pos++) {
    
        real_sample.x3d
            = ray->start.x3d + ray->dir.x3d * sampling_spacing * pos;

        real_sample.y3d
            = ray->start.y3d + ray->dir.y3d * sampling_spacing * pos;

        real_sample.z3d
            = ray->start.z3d + ray->dir.z3d * sampling_spacing * pos;
 
        float curr_value = vis_render_pos(visctl,
                                     block,
                                     ray,
                                     &real_sample,
                                     pixel_data,
                                     num_vol,
                                     sampling_spacing,
                                     image);

        // depth -- isosurface
        if (format == HPGV_RAF) {
            hpgv_raf_t *raf = (hpgv_raf_t*)(pixel_data);
            point_3d_t screen;
            hpgv_gl_project(real_sample.x3d, real_sample.y3d, real_sample.z3d,
                            &screen.x3d, &screen.y3d, &screen.z3d);
            double curr_depth = screen.z3d;
            if (prev_value < 0.0) {
                prev_value = curr_value;
                prev_depth = curr_depth;
            } else {
                double left_value, rite_value, left_depth, rite_depth;
                if (prev_value <= curr_value) {
                    left_value = prev_value;
                    rite_value = curr_value;
                    left_depth = prev_depth;
                    rite_depth = curr_depth;
                } else {
                    left_value = curr_value;
                    rite_value = prev_value;
                    left_depth = curr_depth;
                    rite_depth = prev_depth;
                }
                for (int binId = 0; binId < HPGV_RAF_BIN_NUM; ++binId)
                {
                    double binAvg = (double(binId) + 0.5) / double(HPGV_RAF_BIN_NUM);
                    if (left_value <= binAvg && binAvg < rite_value) {
                        float depth = (binAvg - left_value) / (rite_value - left_value) * (rite_depth - left_depth) + left_depth;
                        if (depth < raf->depths[binId]) {
                            raf->depths[binId] = depth;
                        }
                    }
                }
                prev_value = curr_value;
                prev_depth = curr_depth;
            }
        }

        // early ray termination
        if (format == HPGV_RGBA) {
            rgba_t * color = (rgba_t *)pixel_data;
            if (color->alpha > 0.999) {
                color->alpha = 1;
                break;
            }
        } else if (format == HPGV_RAF) {
            hpgv_raf_t *raf = (hpgv_raf_t *)(pixel_data);            
            if (raf->raf[HPGV_RAF_ALPHA_BIN_NUM - 1] > 0.999) {
                raf->raf[HPGV_RAF_ALPHA_BIN_NUM - 1] = 1;
                break;
            }
        } else {
            HPGV_ABORT("Unsupported format", HPGV_ERROR);
        }
    }

   // if (frag_x == 300 && frag_y == 300)
   // {
   //     hpgv_raf_t *raf = (hpgv_raf_t*)(pixel_data);
   //     std::cout << "values: ";
   //     for (int i = 0; i < HPGV_RAF_ALPHA_BIN_NUM; ++i)
   //         std::cout << raf->raf[i] << ",";
   //     std::cout << std::endl;
   //     std::cout << "depths: ";
   //     for (int i = 0; i < HPGV_RAF_ALPHA_BIN_NUM; ++i)
   //         std::cout << raf->depths[i] << ",";
   //     std::cout << std::endl;
   // }
    
    return;
}


/**
 * hpgv_vis_clear_color
 *
 */
void
hpgv_vis_clear_color(vis_control_t *visctl)
{
    hpgv_gl_clear_color();
}


/**
 * vis_render_volume
 *
 */
void
vis_render_volume(vis_control_t *visctl, const hpgv::Parameter::Image& image)
    // int num_vol, float sampling_spacing,
    // int *id_vol, para_tf_t *tf_vol, para_light_t *light_vol)
{
    HPGV_TIMING_BARRIER(visctl->comm);
    HPGV_TIMING_BEGIN(MY_STEP_VOLUME_RENDER_TIME);
    HPGV_TIMING_COUNT(MY_STEP_VOLUME_RENDER_TIME);

    int num_vol = image.volumes.size();
    float sampling_spacing = visctl->sampling_spacing;
    
    uint64_t index;
    ray_t *ray;
    int32_t firstpos, lastpos;
    pixel_t *pixel;    
    rgba_t color;
    
    hpgv_raf_t  raf;
    
    void *pixel_data = NULL;

    /* clear the color buffer */
    hpgv_vis_clear_color(visctl);
    hpgv_gl_clear_databuf();
    
    for (index = 0; index < visctl->castcount; index++) {

        if (visctl->format == HPGV_RAF)
        {
//            memset(&raf, 0, sizeof(hpgv_raf_t));
            for (int i = 0; i < HPGV_RAF_ALPHA_BIN_NUM; ++i)
            {
                raf.raf[i] = 0.f;
                raf.depths[i] = 1.f;
            }
            pixel_data = (void *)(&raf);
        } else
        {
            color.red = color.green = color.blue = color.alpha = 0.0;
            pixel_data = &color;
        }
        
        ray = &(visctl->cast_ctl[index].ray);
        firstpos = visctl->cast_ctl[index].firstpos;
        lastpos = visctl->cast_ctl[index].lastpos;
        
        /* ray casting for normal compositing*/
        pixel = &(visctl->cast_ctl[index].pixel);

        vis_ray_render_positions(visctl,
                                 visctl->block,
                                 ray,
                                 firstpos,
                                 lastpos,
                                 pixel_data,
                                 pixel->x,
                                 pixel->y,
                                 num_vol,
                                 sampling_spacing,
                                 image);
        
        if (visctl->format == HPGV_RGBA) {
            hpgv_gl_fragcolor(pixel->x, pixel->y, 
                              color.red, color.green, color.blue, color.alpha);
        } else if (visctl->format == HPGV_RAF) {
            hpgv_gl_fragdata(pixel->x, pixel->y, visctl->format, &raf);
        } else {
            HPGV_ABORT("Unsupported format", HPGV_ERROR);
        }
    }

    HPGV_TIMING_END(MY_STEP_VOLUME_RENDER_TIME);
}


/**
 * hpgv_vis_viewmatrix
 *
 */
void
hpgv_vis_viewmatrix(const double m[16])
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return;
    }
        
    theVisControl->updateview |= hpgv_gl_viewmatrix(m);
  
}
        
/**
 * hpgv_vis_projmatrix
 *
 */
void
hpgv_vis_projmatrix(const double m[16])
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return;
    }
        
    theVisControl->updateview |= hpgv_gl_projmatrix(m);
    
}

        
/**
 * hpgv_vis_viewport
 *
 */
void
hpgv_vis_viewport(const int viewport[4])
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return;
    }
    
    int temp[4] = { viewport[0], viewport[1], viewport[2], viewport[3] };
    theVisControl->updateview |= hpgv_gl_viewport(temp);
}


/**
 * hpgv_vis_framesize
 *
 */
void
hpgv_vis_framesize(int width, int height, int type, int format, int framenum)
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return;          
    }

    int updateview = HPGV_FALSE;
        
    updateview = hpgv_gl_framesize(width, height);
    
    int framesize = hpgv_gl_get_framesize();
    
    int bytenum = framesize * hpgv_typesize(type) * hpgv_formatsize(format);

    int realnum = 1;
    if (framenum > 1) {
        realnum = framenum;
    }
    
    
    if (format == HPGV_RGBA) {
        theVisControl->colorimagetype = type;
        theVisControl->colorimageformat = format;
        theVisControl->colorimagesize = framesize;
    } else if (format == HPGV_RAF) {
        
        theVisControl->databuf_type = type;
        theVisControl->databuf_format = format;
        theVisControl->databuf_size = framesize;
        
        theVisControl->databuf
            = (void *)realloc(theVisControl->databuf, 
                              bytenum * realnum);

        // for (int i = 0; i < framesize; ++i)
        // {
        //     int formatsize = hpgv_formatsize(format);
        //     for (int j = 0; j < formatsize; ++j)
        //     {
        //         reinterpret_cast<float*>(theVisControl->databuf)[formatsize * i + j] = 1.f;
        //     }
        // }
        
        HPGV_ASSERT_P(theVisControl->id, theVisControl->databuf,
                      "Out of memory.", HPGV_ERR_MEM);
        
        hpgv_gl_bind_databuf(theVisControl->databuf);
        hpgv_gl_dbtype(type);
        hpgv_gl_dbformat(format);
        
    } else {
        HPGV_ABORT("Unsupported format", HPGV_ERROR);
    }        
    
    if (theVisControl->id == theVisControl->root) {
        
        int bytenum = framesize * hpgv_typesize(type) * hpgv_formatsize(format);
        
        int realnum = 1;
        if (framenum > 1) {
            realnum = framenum;
        }
        
        if (format == HPGV_RGBA) {
            theVisControl->colorimage
                = (void *)realloc(theVisControl->colorimage, bytenum * realnum);
            
            HPGV_ASSERT_P(theVisControl->id, theVisControl->colorimage,
                          "Out of memory.", HPGV_ERR_MEM);
            
        } else if (format == HPGV_RAF) {
            theVisControl->databuf_collect
            = (void *)realloc(theVisControl->databuf_collect, 
                              bytenum * realnum);
            
            HPGV_ASSERT_P(theVisControl->id, theVisControl->databuf_collect,
                          "Out of memory.", HPGV_ERR_MEM);
        } else {
            HPGV_ABORT("Unsupported format", HPGV_ERROR);
        }
    }
    
    theVisControl->updateview |= updateview;
    
}

/**
 * hpgv_vis_tf_para
 *
 */
void
hpgv_vis_tf_para(int colormapsize, int format, int type)
{    
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return;
    }
    
    if (format != HPGV_RGBA || type != HPGV_FLOAT) {
        fprintf(stderr,
                "HPGV currently only supports the colormap with RGBA and float format.");
        return;
    }
    
    theVisControl->colormapformat = format;
    theVisControl->colormaptype = type;
    theVisControl->colormapsize = colormapsize;
}


/**
 * hpgv_vis_para
 *
 */
void
hpgv_vis_para(const hpgv::Parameter& para)
{
    theVisControl->format = para.getFormat();
    int framenum = para.getImages().size();
    hpgv_vis_tf_para(para.getColormap().size,
            para.getColormap().format,
            para.getColormap().type);
    hpgv_vis_framesize(para.getView().width,
            para.getView().height,
            para.getType(),
            para.getFormat(),
            framenum);
    hpgv_vis_viewmatrix(&para.getView().modelview[0]);
    hpgv_vis_projmatrix(&para.getView().projection[0]);
    hpgv_vis_viewport(&para.getView().viewport[0]);
    // We assume that all the volume rendering use the same sampling spacing.
    // This will be fixed later
    int updateview = HPGV_FALSE;
    if (framenum > 0) {
        if (theVisControl->sampling_spacing != para.getImages()[0].sampleSpacing)
        {
            theVisControl->sampling_spacing = para.getImages()[0].sampleSpacing;
            updateview = HPGV_TRUE;
        }
    }
    theVisControl->format = para.getFormat();
    theVisControl->updateview |= updateview;
    theVisControl->para = para;
}


/**
 * hpgv_vis_get_imageptr
 *
 */
const void *
hpgv_vis_get_imageptr()
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return NULL;
    }

    return theVisControl->colorimage;
}


/**
 * hpgv_vis_get_databufptr
 *
 */
const void *
hpgv_vis_get_databufptr()
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return NULL;
    }
    
    return theVisControl->databuf_collect;
}

/**
 * hpgv_vis_get_imagetype
 *
 */
int
hpgv_vis_get_imagetype()
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return HPGV_ERROR;
    }

    return theVisControl->colorimagetype;
}


/**
 * hpgv_vis_get_imageformat
 *
 */
int
hpgv_vis_get_imageformat()
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return HPGV_ERROR;
    }

    return theVisControl->colorimageformat;
}



/**
 * hpgv_vis_set_rendervolume
 *
 */
int
hpgv_vis_set_rendervolume(int b)
{
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return HPGV_ERROR;
    }

    theVisControl->rendervolume = b;
    return HPGV_TRUE;
}



/**
 * hpgv_vis_valid
 *
 */
int
hpgv_vis_valid()
{
    if (theVisControl) {
        return HPGV_TRUE;
    }
    
    return HPGV_FALSE;
}

/**
 * hpgv_vis_composite_init
 *
 */
void
hpgv_vis_composite_init(MPI_Comm comm)
{
    hpgv_composite_init(comm);
}



/**
 * hpgv_vis_render_one_composite
 *
 */
void
hpgv_vis_render_one_composite(block_t *block, int root, MPI_Comm comm)
{
    int img = 0;
    
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return;
    }

    if (theVisControl->para.getImages().size() == 0) {
        /* there is no image to render */
        if (block->mpiid == root) {
            fprintf(stderr, "There is no image to render.\n");
        }
        return;
    }
        
    if (theVisControl->block != block) {
        theVisControl->block = block;
        theVisControl->updateview |= HPGV_TRUE;
    }
    
    if (!(theVisControl->block)) {
        fprintf(stderr, "Skip the empty block.\n");
        return;
    }

    if (theVisControl->comm != comm) {
        MPI_Comm_rank(comm, &(theVisControl->id));
        MPI_Comm_size(comm, &(theVisControl->groupsize));
        theVisControl->comm = comm;
    }
    
    if (theVisControl->updateview) {
        vis_update_eyedepth(theVisControl);
    }    

    /* we assume that all volume rendering use the same sampling spacing, which
       will be fixed later */

    /* update the projection area */
    if (theVisControl->updateview) {
        vis_update_projarea(theVisControl, theVisControl->sampling_spacing);
        theVisControl->updateview = HPGV_FALSE;
    }

    /* prepare the buffer which holds all images */
    int i;
    int framebuf_size_x = hpgv_gl_get_framewidth();
    int framebuf_size_y = hpgv_gl_get_frameheight();
    int framebuf_size = hpgv_gl_get_framesize();
    
    int cbformat = hpgv_gl_get_cbformat();
    
    int destype = theVisControl->colorimagetype;
    int desformat = theVisControl->colorimageformat;
    
    HPGV_ASSERT_P(theVisControl->id,
                  desformat == HPGV_RGBA && cbformat == HPGV_RGBA,
                  "Unsupported pixel format.", HPGV_ERROR);
    
    int formatsize = hpgv_formatsize(desformat);
    
    void *colorbuf = NULL;
    colorbuf = (void *) calloc (framebuf_size,
                                hpgv_typesize(destype) *
                                hpgv_formatsize(desformat) *
                                theVisControl->para.getImages().size());
    HPGV_ASSERT_P(theVisControl->id, colorbuf, "Out of memory.",
                  HPGV_ERR_MEM);
    
    uint8_t  *colorbuf_uint8 = (uint8_t *)colorbuf;
    uint16_t *colorbuf_uint16 = (uint16_t *)colorbuf;
    float    *colorbuf_float = (float *)colorbuf;
    long     colorbuf_offset = framebuf_size * formatsize;

    HPGV_TIMING_COUNT(MY_STEP_MULTI_COMPOSE_TIME);
    HPGV_TIMING_COUNT(MY_STEP_MULTI_VOLREND_TIME);
    HPGV_TIMING_COUNT(MY_STEP_MULTI_PARREND_TIME);
    HPGV_TIMING_COUNT(MY_STEP_MULTI_GHOST_TIME);
    
    for (img = 0; img < theVisControl->para.getImages().size(); img++) {
        
        /* processing each image */
        const hpgv::Parameter::Image& image = theVisControl->para.getImages()[img];
        // para_image_t  *para_image
            // = &(theVisControl->para.getImages()[img]);

        if (image.volumes.size() > 0) {
            hpgv_vis_set_rendervolume(HPGV_TRUE);
        } else {
            hpgv_vis_set_rendervolume(HPGV_FALSE);
        }

        HPGV_TIMING_BARRIER(theVisControl->comm);
        HPGV_TIMING_BEGIN(MY_STEP_MULTI_VOLREND_TIME);
        
        std::vector<int> volIds(image.volumes.size());
        for (unsigned int i = 0; i < image.volumes.size(); ++i)
            volIds[i] = image.volumes[i].id;
        /* volume render */
        if (image.volumes.size() > 0) {
            vis_render_volume(theVisControl, image);
        }

        HPGV_TIMING_END(MY_STEP_MULTI_VOLREND_TIME);
        
        float *cbptr = hpgv_gl_get_cbptr();
        
        switch (destype) {
        case HPGV_UNSIGNED_BYTE :
            for (int i = 0; i < framebuf_size * formatsize; i++) {
                if (cbptr[i] >= 1.0f) {
                    colorbuf_uint8[i] = 0xFF;
                } else if (cbptr[i] < 0.0f) {
                    colorbuf_uint8[i] = 0x00;
                } else {
                    colorbuf_uint8[i] = (uint8_t)(cbptr[i] * 0xFF);
                }
            }
            colorbuf_uint8 += colorbuf_offset;
            break;
        case HPGV_UNSIGNED_SHORT:
            for (int i = 0; i < framebuf_size * formatsize; i++) {
                colorbuf_uint16[i] = (uint16_t)(cbptr[i] * 0xFFFF);
            }
            colorbuf_uint16 += colorbuf_offset;
            break;
        case HPGV_FLOAT:
            for (int i = 0; i < framebuf_size * formatsize; i++) {
                colorbuf_float[i] = cbptr[i];
            }
            colorbuf_float += colorbuf_offset;
            break;
        default:
            HPGV_ABORT_P(theVisControl->id, "Unsupported pixel format.",
                         HPGV_ERROR);
        }
    }

    HPGV_TIMING_BARRIER(theVisControl->comm);
    HPGV_TIMING_BEGIN(MY_STEP_MULTI_COMPOSE_TIME);
    
    hpgv_composite(framebuf_size_x * framebuf_size_y,
            theVisControl->para.getImages().size(),
            theVisControl->colorimageformat,
            theVisControl->colorimagetype,
            colorbuf,
            theVisControl->colorimage,
            theVisControl->block_depth,
            theVisControl->root,
            theVisControl->comm,
            HPGV_TTSWAP);

    HPGV_TIMING_END(MY_STEP_MULTI_COMPOSE_TIME);
    
    free(colorbuf);

    theVisControl->updateview = HPGV_FALSE;
    theVisControl->rendercount++;
}



/**
 * hpgv_vis_render_multi_composite
 *
 */
void
hpgv_vis_render_multi_composite(block_t *block, int root, MPI_Comm comm)
{
    int img = 0;
    
    if (!theVisControl) {
        fprintf(stderr, "HPGV has not been initialized.\n");
        return;
    }
    
    if (theVisControl->para.getImages().size() == 0) {
        /* there is no image to render */
        if (block->mpiid == root) {
            fprintf(stderr, "There is no image to render.\n");
        }
        return;
    }
    
    if (theVisControl->block != block) {
        theVisControl->block = block;
        theVisControl->updateview |= HPGV_TRUE;
    }
    
    if (!(theVisControl->block)) {
        fprintf(stderr, "Skip the empty block.\n");
        return;
    }
    
    if (theVisControl->comm != comm) {
        MPI_Comm_rank(comm, &(theVisControl->id));
        MPI_Comm_size(comm, &(theVisControl->groupsize));
        theVisControl->comm = comm;
    }
    
    if (theVisControl->updateview) {
        vis_update_eyedepth(theVisControl);
    }
    
    /* we assume that all volume rendering use the same sampling spacing, which
     *       will be fixed later */
    
    /* update the projection area */
    if (theVisControl->updateview) {
        vis_update_projarea(theVisControl, theVisControl->sampling_spacing);
        theVisControl->updateview = HPGV_FALSE;
    }
    
    HPGV_TIMING_COUNT(MY_STEP_MULTI_COMPOSE_TIME);
    HPGV_TIMING_COUNT(MY_STEP_MULTI_VOLREND_TIME);
    HPGV_TIMING_COUNT(MY_STEP_MULTI_PARREND_TIME);
    HPGV_TIMING_COUNT(MY_STEP_MULTI_GHOST_TIME);
    
    for (img = 0; img < theVisControl->para.getImages().size(); img++) {
        
        /* processing each image */
        const hpgv::Parameter::Image& image = theVisControl->para.getImages()[img];
        // para_image_t  *para_image
        // = &(theVisControl->para_input->para_image[img]);
        
        if (image.volumes.size() > 0) {
            hpgv_vis_set_rendervolume(HPGV_TRUE);
        } else {
            hpgv_vis_set_rendervolume(HPGV_FALSE);
        }
        
        HPGV_TIMING_BARRIER(theVisControl->comm);
        HPGV_TIMING_BEGIN(MY_STEP_MULTI_VOLREND_TIME);
        
        /* volume render */
        if (image.volumes.size() > 0) {
            vis_render_volume(theVisControl, image);
        }
        
        HPGV_TIMING_END(MY_STEP_MULTI_VOLREND_TIME);
        
#ifdef HPGV_DEBUG_LOCAL_IMG
        uint64_t index, offset;
        pixel_t *pixel;

        char filename[MAXLINE];
        snprintf(filename, MAXLINE, "./image_local_t%04d_p%04d_g%04d.ppm",
                 theVisControl->rendercount, theVisControl->id, img);

        hpgv_vis_saveppm(hpgv_gl_get_framewidth(),
                         hpgv_gl_get_frameheight(),
                         hpgv_gl_get_cbformat(),
                         hpgv_gl_get_cbtype(),
                         hpgv_gl_get_cbptr(),
                         filename);
#endif

        int format = theVisControl->format;

        int i;
        int framebuf_size_x = hpgv_gl_get_framewidth();
        int framebuf_size_y = hpgv_gl_get_frameheight();


        if (format == HPGV_RGBA) {
            int framebuf_size = hpgv_gl_get_framesize();
            
            float *cbptr = hpgv_gl_get_cbptr();
            int cbtype = hpgv_gl_get_cbtype();
            int cbformat = hpgv_gl_get_cbformat();
            
            int destype = theVisControl->colorimagetype;
            int desformat = theVisControl->colorimageformat;
            
            HPGV_ASSERT_P(theVisControl->id,
                          desformat == HPGV_RGBA && cbformat == HPGV_RGBA,
                          "Unsupported pixel format.", HPGV_ERROR);
            
            int formatsize = hpgv_formatsize(desformat);
            
            void *colorbuf = NULL;
            uint8_t  *colorbuf_uint8 = NULL;
            uint16_t *colorbuf_uint16 = NULL;
            
            if (desformat == cbformat && destype == cbtype) {
                // same format and type
                colorbuf = cbptr;
            } else {
                // different format and type
                colorbuf = (void *) calloc (framebuf_size,
                                            hpgv_typesize(destype) *
                                            hpgv_formatsize(desformat));
                
                HPGV_ASSERT_P(theVisControl->id, colorbuf, "Out of memory.",
                              HPGV_ERR_MEM);
                
                switch (destype) {
                    case HPGV_UNSIGNED_BYTE :
                        colorbuf_uint8 = (uint8_t *) colorbuf;
                        for (i = 0; i < framebuf_size * formatsize; i++) {
                            if (cbptr[i] >= 1.0f) {
                                colorbuf_uint8[i] = 0xFF;
                            } else if (cbptr[i] < 0.0f) {
                                colorbuf_uint8[i] = 0x00;
                            } else {
                                colorbuf_uint8[i] = (uint8_t)(cbptr[i] * 0xFF);
                            }
                        }
                        break;
                    case HPGV_UNSIGNED_SHORT:
                        colorbuf_uint16 = (uint16_t *) colorbuf;
                        for (i = 0; i < framebuf_size * formatsize; i++) {
                            colorbuf_uint16[i] = (uint16_t)(cbptr[i] * 0xFFFF);
                        }
                        break;
                    default:
                        HPGV_ABORT_P(theVisControl->id, "Unsupported pixel format.",
                                     HPGV_ERROR);
                }
            }
            
            long offset = img *
            framebuf_size *
            hpgv_typesize(destype) *
            hpgv_formatsize(desformat);
            
            char *desimage = ((char *)theVisControl->colorimage) + offset;
            
            HPGV_TIMING_BARRIER(theVisControl->comm);
            HPGV_TIMING_BEGIN(MY_STEP_MULTI_COMPOSE_TIME);
            
            hpgv_composite(framebuf_size_x,
                           framebuf_size_y,
                           theVisControl->colorimageformat,
                           theVisControl->colorimagetype,
                           colorbuf,
                           desimage,
                           theVisControl->block_depth,
                           theVisControl->root,
                           theVisControl->comm,
                           HPGV_TTSWAP);
            
            HPGV_TIMING_END(MY_STEP_MULTI_COMPOSE_TIME);
            
            if (!(desformat == cbformat && destype == cbtype)) {
                free(colorbuf);
            }

        } else if (format == HPGV_RAF) {
            int framebuf_size = theVisControl->databuf_size;

            long offset = img *
                    framebuf_size *
                    hpgv_typesize(theVisControl->databuf_type) *
                    hpgv_formatsize(theVisControl->databuf_format);

            char *desimage = ((char *)theVisControl->databuf) + offset;

            HPGV_TIMING_BARRIER(theVisControl->comm);
            HPGV_TIMING_BEGIN(MY_STEP_MULTI_COMPOSE_TIME);


            /* === split RAF into 4 segments and composite them individually */
            /* ============================================================= */
            static float *raf_seg = NULL;
            static float *raf_collect = NULL;
            
            if (raf_seg == NULL) {
                raf_seg = (float *)realloc(raf_seg, 
                        framebuf_size_x * framebuf_size_y *
                        hpgv_typesize(HPGV_FLOAT) *
                        hpgv_formatsize(HPGV_RAF_SEG));
            }

            if (theVisControl->id == theVisControl->root) {
                if (raf_collect == NULL) {
                    raf_collect = (float *)realloc(raf_collect, 
                            framebuf_size_x * framebuf_size_y *
                            hpgv_typesize(HPGV_FLOAT) *
                            hpgv_formatsize(HPGV_RAF_SEG));
                }
            }

            float *tempimage = (float *)(desimage);
            int totalpixel = framebuf_size_x * framebuf_size_y;

            int s, m, n;            
            int total_seg 
            = (HPGV_RAF_ALPHA_BIN_NUM - 1) / (HPGV_RAF_SEG_NUM - 1);

            /* 
             *            HPGV_ASSERT_P(theVisControl->id, total_seg == 4, 
             *                          "the total_seg should be 2.", HPGV_ERR_MEM);
             */
            
            for (s = 0; s < total_seg; s++) {
                for (m = 0; m < totalpixel; m++) {


// std::cout << "raf: [";
// for (int i = 0; i < HPGV_RAF_BIN_NUM; ++i)
//     std::cout << tempimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2 + HPGV_RAF_ALPHA_BIN_NUM * 1 + i] << ", ";
// std::cout << "]" << std::endl;


                    for (n = 0; n <  HPGV_RAF_SEG_NUM - 1; n++) {
                        // raf
                        raf_seg[m * HPGV_RAF_SEG_NUM * 2 + HPGV_RAF_SEG_NUM * 0 + n]
                        = tempimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2 + HPGV_RAF_ALPHA_BIN_NUM * 0
                        + s * (HPGV_RAF_SEG_NUM - 1) + n];
                        // depth
                        raf_seg[m * HPGV_RAF_SEG_NUM * 2 + HPGV_RAF_SEG_NUM * 1 + n]
                        = 1.0 - tempimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2 + HPGV_RAF_ALPHA_BIN_NUM * 1
                        + s * (HPGV_RAF_SEG_NUM - 1) + n];
                        // raf_seg[m * HPGV_RAF_SEG_NUM * 2 + HPGV_RAF_SEG_NUM * 1 + n]
                        // = 1.0 - double(n + 1) / double(HPGV_RAF_SEG_NUM);
                    }
                    // raf
                    raf_seg[m * HPGV_RAF_SEG_NUM * 2 + HPGV_RAF_SEG_NUM * 0 + HPGV_RAF_SEG_NUM - 1]
                    = tempimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2 + HPGV_RAF_ALPHA_BIN_NUM * 0 + HPGV_RAF_ALPHA_BIN_NUM - 1];
                    // depth
                    raf_seg[m * HPGV_RAF_SEG_NUM * 2 + HPGV_RAF_SEG_NUM * 1 + HPGV_RAF_SEG_NUM - 1]
                    = 1.0 - tempimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2 + HPGV_RAF_ALPHA_BIN_NUM * 1 + HPGV_RAF_ALPHA_BIN_NUM - 1];
                }

                hpgv_composite(framebuf_size_x,
                               framebuf_size_y,
                               HPGV_RAF_SEG,
                               HPGV_FLOAT,
                               raf_seg,
                               raf_collect,                           
                               theVisControl->block_depth,
                               theVisControl->root,
                               theVisControl->comm,
                               HPGV_TTSWAP);
                
                if (theVisControl->id == theVisControl->root) {        
                    float *collectimage 
                    = (float *)theVisControl->databuf_collect;    
                    
                    for (m = 0; m < totalpixel; m++) {
                        for (n = 0; n < HPGV_RAF_SEG_NUM - 1; n++) {
                            // raf
                            collectimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2
                            + HPGV_RAF_ALPHA_BIN_NUM * 0
                            + s * (HPGV_RAF_SEG_NUM - 1) + n]
                            = raf_collect[m * HPGV_RAF_SEG_NUM * 2
                            + HPGV_RAF_SEG_NUM * 0 + n];
                            // depth
                            collectimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2
                            + HPGV_RAF_ALPHA_BIN_NUM * 1
                            + s * (HPGV_RAF_SEG_NUM - 1) + n]
                            = 1.0 - (raf_collect[m * HPGV_RAF_SEG_NUM * 2
                            + HPGV_RAF_SEG_NUM * 1 + n]);
                        }
                        // raf
                        collectimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2
                        + HPGV_RAF_ALPHA_BIN_NUM * 0
                        + HPGV_RAF_ALPHA_BIN_NUM - 1] 
                        = raf_collect[m * HPGV_RAF_SEG_NUM * 2
                        + HPGV_RAF_SEG_NUM * 0
                        + HPGV_RAF_SEG_NUM - 1];
                        // depth
                        collectimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2
                        + HPGV_RAF_ALPHA_BIN_NUM * 1
                        + HPGV_RAF_ALPHA_BIN_NUM - 1]
                        = 1.0 - (raf_collect[m * HPGV_RAF_SEG_NUM * 2
                        + HPGV_RAF_SEG_NUM * 1
                        + HPGV_RAF_SEG_NUM - 1]);

//                        if (m == 192900)
//                        {
//                            std::cout << "composite: ";
//                            hpgv_raf_t *raf = (hpgv_raf_t*)(&collectimage[m * HPGV_RAF_ALPHA_BIN_NUM * 2]);
//                            std::cout << "values: ";
//                            for (int i = 0; i < HPGV_RAF_ALPHA_BIN_NUM; ++i)
//                                std::cout << raf->raf[i] << ",";
//                            std::cout << std::endl;
//                            std::cout << "depths: ";
//                            for (int i = 0; i < HPGV_RAF_ALPHA_BIN_NUM; ++i)
//                                std::cout << raf->depths[i] << ",";
//                            std::cout << std::endl;
//                        }
                    }
                }
            }
            
            /* ============================================================= */
            
            HPGV_TIMING_END(MY_STEP_MULTI_COMPOSE_TIME);
        } else {
            HPGV_ABORT("Unsupported format", HPGV_ERROR);
        }
        
        theVisControl->updateview = HPGV_FALSE;
        theVisControl->rendercount++;
    }
}
        

/**
 * hpgv_vis_render
 *
 */
void
hpgv_vis_render(block_t *block, int root, MPI_Comm comm, int opt)
{
    if (opt == 0) {
        hpgv_vis_render_multi_composite(block, root, comm);
    } else {
        hpgv_vis_render_one_composite(block, root, comm);        
    }
}

/**
 * hpgv_vis_init
 *
 */
void
hpgv_vis_init(MPI_Comm comm, int root)
{
    int id, groupsize;
    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &groupsize);
    
    /* init vis control struct */
    theVisControl = (vis_control_t *)calloc(1, sizeof(vis_control_t));
    HPGV_ASSERT_P(id, theVisControl, "Out of memory.", HPGV_ERR_MEM);
    
    theVisControl->id = id;
    theVisControl->groupsize = groupsize;
    theVisControl->root = root;
    theVisControl->comm = comm;

    /* init hpgv gl moduel */
    if (!hpgv_gl_valid()) {
        hpgv_gl_init();
    }
    
    /* init hpgv compositing module */    
    if (!hpgv_composite_valid()) {
        hpgv_vis_composite_init(theVisControl->comm);
    }

    if (!HPGV_TIMING_VALID()) {
        HPGV_TIMING_INIT(root, comm);
    }
    
    static int init_timing = HPGV_FALSE;
    
    if (init_timing == HPGV_FALSE) {
        init_timing = HPGV_TRUE;
        
        HPGV_TIMING_NAME(MY_STEP_GHOST_PARTICLE_TIME,       "T_ghost");
        HPGV_TIMING_NAME(MY_STEP_VOLUME_RENDER_TIME,        "T_volrend");
        HPGV_TIMING_NAME(MY_STEP_PARTICLE_RENDER_TIME,      "T_parrend");

        HPGV_TIMING_NAME(MY_STEP_MULTI_COMPOSE_TIME,        "T_mcomp");
        HPGV_TIMING_NAME(MY_STEP_MULTI_VOLREND_TIME,        "T_mvolrend");
        HPGV_TIMING_NAME(MY_STEP_MULTI_PARREND_TIME,        "T_mparrend");
        HPGV_TIMING_NAME(MY_STEP_MULTI_GHOST_TIME,          "T_mghost");
    }
} 

/**
 * hpgv_vis_finalize
 *
 */
void 
hpgv_vis_finalize()
{
    
    if (!theVisControl) {
        fprintf(stderr, "Can not find visualization module.\n");
        return;
    }
    
    if (theVisControl->cast_ctl) {
        free(theVisControl->cast_ctl);
    }

    if (theVisControl->colorimage) {
        free(theVisControl->colorimage);
    }
       
    hpgv_composite_finalize(theVisControl->comm);
    
    hpgv_gl_finalize();
    
    free(theVisControl);

    /*
    fprintf(stderr, "Memory cleared on PE %d.\n", theVisControl->id);
    */
}

