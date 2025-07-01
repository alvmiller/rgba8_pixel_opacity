#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**************************************************************************************************/

#define RGBA8_R_COMPONENT_MASK      UINT32_C(0xFF000000)
#define RGBA8_R_COMPONENT_SHIFT     UINT32_C(24)
#define RGBA8_G_COMPONENT_MASK      UINT32_C(0x00FF0000)
#define RGBA8_G_COMPONENT_SHIFT     UINT32_C(16)
#define RGBA8_B_COMPONENT_MASK      UINT32_C(0x0000FF00)
#define RGBA8_B_COMPONENT_SHIFT     UINT32_C(8)
#define RGBA8_ALPHA_COMPONENT_MASK  UINT32_C(0x000000FF)
#define RGBA8_ALPHA_COMPONENT_SHIFT UINT32_C(0)

#define COLOR_ADD_NORMALIZATION_FACTOR UINT8_C(2)
#define COLOR_MUL_NORMALIZATION_FACTOR UINT16_C(255)

#define GET_R_FROM_RGBA(_rgba) \
    ((uint8_t) ((((uint32_t) (_rgba)) & RGBA8_R_COMPONENT_MASK) >> RGBA8_R_COMPONENT_SHIFT))
#define GET_G_FROM_RGBA(_rgba) \
    ((uint8_t) ((((uint32_t) (_rgba)) & RGBA8_G_COMPONENT_MASK) >> RGBA8_G_COMPONENT_SHIFT))
#define GET_B_FROM_RGBA(_rgba) \
    ((uint8_t) ((((uint32_t) (_rgba)) & RGBA8_B_COMPONENT_MASK) >> RGBA8_B_COMPONENT_SHIFT))
#define GET_ALPHA_FROM_RGBA(_rgba) \
    ((uint8_t) ((((uint32_t) (_rgba)) & RGBA8_ALPHA_COMPONENT_MASK) >> RGBA8_ALPHA_COMPONENT_SHIFT))

#define GET_RGBA_FROM_COMPONENTS(_r, _g, _b, _a) \
    ((uint32_t) ((((uint32_t) (_r)) << RGBA8_R_COMPONENT_SHIFT) \
               | (((uint32_t) (_g)) << RGBA8_G_COMPONENT_SHIFT) \
               | (((uint32_t) (_b)) << RGBA8_B_COMPONENT_SHIFT) \
               | (((uint32_t) (_a)) << RGBA8_ALPHA_COMPONENT_SHIFT)))

/**************************************************************************************************/

typedef uint32_t rgba8_t;
//__attribute__ ((packed))

/**************************************************************************************************/

static inline void rgba8_opacity_opaque(rgba8_t *dst, const rgba8_t *src);

static inline void rgba8_opacity_source_alpha(rgba8_t *dst, const rgba8_t *src);

static inline void rgba8_opacity_target_alpha(rgba8_t *dst, const rgba8_t *src);

static inline void rgba8_opacity_transparent(rgba8_t *dst, const rgba8_t *src);

static inline void rgba8_opacity_blend_source(rgba8_t *dst, const rgba8_t *src);

static inline void rgba8_opacity_blend_target(rgba8_t *dst, const rgba8_t *src);

static inline void rgba8_opacity_blend_target(rgba8_t *dst, const rgba8_t *src);

static inline void rgba8_opacity_blend(rgba8_t *dst, const rgba8_t *src);

/**************************************************************************************************/

#define COLOR_ADD_NORMALIZATION_FACTOR_SHIFT UINT16_C(1) /* Division by 2 */
#define OPACITY_ADDITION_BLENDING_OPERATION(_x, _y) \
    (uint8_t) (((uint16_t) (_x) + (uint16_t) (_y)) >> COLOR_ADD_NORMALIZATION_FACTOR_SHIFT)
#define OPACITY_MULTIPLICATION_SCALING_OPERATION(_x, _y) \
    (uint8_t) ((((uint16_t) (_x)) * ((uint16_t) (_y))) / COLOR_MUL_NORMALIZATION_FACTOR)

#define RGBA32_R_INDEX UINT32_C(3)
#define RGBA32_G_INDEX UINT32_C(2)
#define RGBA32_B_INDEX UINT32_C(1)
#define RGBA32_A_INDEX UINT32_C(0)

#define OPACITY_ALPHA_BLENDING_OPERATION(_dst, _src) ((uint8_t) (_dst))

#define RGB_MUL_ALPHA_OP(_dst, _src, _alpha) \
    ({ \
        const uint8_t *_src_macro_tmp = (uint8_t *) (_src); \
        uint8_t *_dst_macro_tmp = (uint8_t *) (_dst); \
        const uint8_t _alpha_macro_tmp = ((const uint8_t *) (_alpha))[RGBA32_A_INDEX]; \
        \
        for (uint32_t i = RGBA32_B_INDEX; i <= RGBA32_R_INDEX; ++i) { \
            _dst_macro_tmp[i] = OPACITY_MULTIPLICATION_SCALING_OPERATION(_src_macro_tmp[i], \
                                                                         _alpha_macro_tmp); \
        } \
        \
        (*(uint32_t *) _dst_macro_tmp); \
    })

#define RGBA_MUL_ALPHA_OP(_dst, _src, _alpha) \
    ({ \
        uint8_t *_dst_macro_tmp = (uint8_t *) (_dst); \
        const uint8_t *_alpha_macro_tmp = (uint8_t *) (_alpha); \
        \
        RGB_MUL_ALPHA_OP((_dst), (_src), (_alpha)); \
        \
        _dst_macro_tmp[RGBA32_A_INDEX] = OPACITY_MULTIPLICATION_SCALING_OPERATION( \
            _alpha_macro_tmp[RGBA32_A_INDEX], \
            _dst_macro_tmp[RGBA32_A_INDEX]); \
        \
        (*(uint32_t *) _dst_macro_tmp); \
    })

#define RGBA_ADD_OP(_dst, _src) \
    ({ \
        const uint8_t *_src_macro_tmp = (uint8_t *) (_src); \
        uint8_t *_dst_macro_tmp = (uint8_t *) (_dst); \
        \
        for (uint32_t i = RGBA32_B_INDEX; i <= RGBA32_R_INDEX; ++i) { \
            _dst_macro_tmp[i] = OPACITY_ADDITION_BLENDING_OPERATION(_src_macro_tmp[i], \
                                                                    _dst_macro_tmp[i]); \
        } \
        _dst_macro_tmp[RGBA32_A_INDEX] = OPACITY_ALPHA_BLENDING_OPERATION( \
            _dst_macro_tmp[RGBA32_A_INDEX], \
            _src_macro_tmp[RGBA32_A_INDEX]); \
        \
        (*(uint32_t *) _dst_macro_tmp); \
    })

/**************************************************************************************************/

static inline void rgba8_opacity_opaque(rgba8_t *dst, const rgba8_t *src)
{
    /* 'target.RGBA = source.RGBA' */
    *dst = *src;

    return;
}

static inline void rgba8_opacity_source_alpha(rgba8_t *dst, const rgba8_t *src)
{
    /* 'target.RGB = source.RGB * source.Alpha' */
    RGB_MUL_ALPHA_OP(dst, src, src);

    return;
}

static inline void rgba8_opacity_target_alpha(rgba8_t *dst, const rgba8_t *src)
{
    /* 'target.RGB = source.RGB * target.Alpha' */
    RGB_MUL_ALPHA_OP(dst, src, dst);

    return;
}

static inline void rgba8_opacity_transparent(rgba8_t *dst, const rgba8_t *src)
{
    /* 'target.RGBA = target.RGBA + source.RGBA' */
    RGBA_ADD_OP(dst, src);

    return;
}

static inline void rgba8_opacity_blend_source(rgba8_t *dst, const rgba8_t *src)
{
    /* 'target.RGBA = target.RGBA * source.Alpha + source.RGBA' */

    RGBA_MUL_ALPHA_OP(dst, dst, src);
    RGBA_ADD_OP(dst, src);

    return;
}

static inline void rgba8_opacity_blend_target(rgba8_t *dst, const rgba8_t *src)
{
    /* 'target.RGBA = target.RGBA + source.RGBA * target.Alpha' */
    rgba8_t src_tmp = *src;
    RGBA_MUL_ALPHA_OP(&src_tmp, &src_tmp, dst);
    RGBA_ADD_OP(dst, &src_tmp);

    return;
}

static inline void rgba8_opacity_blend(rgba8_t *dst, const rgba8_t *src)
{
    /* 'target.RGBA = target.RGBA * source.Alpha + source.RGBA * target.Alpha' */
    rgba8_t src_tmp = *src;
    RGBA_MUL_ALPHA_OP(&src_tmp, &src_tmp, dst);
    RGBA_MUL_ALPHA_OP(dst, dst, src);
    RGBA_ADD_OP(dst, &src_tmp);

    return;
}

/**************************************************************************************************/

/**************************************************************************************************/

uint8_t src_arr[4] = {0xaa, 0xbb, 0xcc, 0xdd};
uint8_t dst_arr[4] = {0x11, 0x22, 0x33, 0x44};

int main()
{
    printf("VAR = %x\n", *(uint32_t *)src_arr);
    uint8_t src_arr1[4] = {0x11, 0x22, 0x33, 0x44};
    printf("VAR Stack = %x\n\n\n", *(uint32_t *)src_arr1);

    //const uint32_t var_src = 0xaabbccdd;
    const uint32_t var_src = 0xeeaadd33;
    //const uint32_t var_src = 0x11223344;
    //const uint32_t var_src = 0x88776655;
    //const uint32_t var_dst = 0xff11ee88;
    const uint32_t var_dst = 0x1a117722;
    //const uint32_t var_dst = 0xaabbccdd;
    //const uint32_t var_dst = 0x11223344;
    uint32_t var_updated_new = 0x0;

    var_updated_new = 0x0;
    printf("Opaque:\n");
    printf("var_src = %x\n", var_src);
    printf("var_dst = %x\n", var_dst);
    printf("var_updated_new = %x\n", var_updated_new);
    var_updated_new = var_dst;
    rgba8_opacity_opaque(&var_updated_new, &var_src);
    printf("var_updated_new = %x\n", var_updated_new);

    var_updated_new = 0x0;
    printf("Source Alpha:\n");
    printf("var_src = %x\n", var_src);
    printf("var_dst = %x\n", var_dst);
    printf("var_updated_new = %x\n", var_updated_new);
    var_updated_new = var_dst;
    rgba8_opacity_source_alpha(&var_updated_new, &var_src);
    printf("var_updated_new = %x\n", var_updated_new);

    var_updated_new = 0x0;
    printf("Target Alpha:\n");
    printf("var_src = %x\n", var_src);
    printf("var_dst = %x\n", var_dst);
    printf("var_updated_new = %x\n", var_updated_new);
    var_updated_new = var_dst;
    rgba8_opacity_target_alpha(&var_updated_new, &var_src);
    printf("var_updated_new = %x\n", var_updated_new);

    var_updated_new = 0x0;
    printf("transparent:\n");
    printf("var_src = %x\n", var_src);
    printf("var_dst = %x\n", var_dst);
    printf("var_updated_new = %x\n", var_updated_new);
    var_updated_new = var_dst;
    rgba8_opacity_transparent(&var_updated_new, &var_src);
    printf("var_updated_new = %x\n", var_updated_new);

    var_updated_new = 0x0;
    printf("Blend Source:\n");
    printf("var_src = %x\n", var_src);
    printf("var_dst = %x\n", var_dst);
    printf("var_updated_new = %x\n", var_updated_new);
    var_updated_new = var_dst;
    rgba8_opacity_blend_source(&var_updated_new, &var_src);
    printf("var_updated_new = %x\n", var_updated_new);

    var_updated_new = 0x0;
    printf("Blend Target:\n");
    printf("var_src = %x\n", var_src);
    printf("var_dst = %x\n", var_dst);
    printf("var_updated_new = %x\n", var_updated_new);
    var_updated_new = var_dst;
    rgba8_opacity_blend_target(&var_updated_new, &var_src);
    printf("var_updated_new = %x\n", var_updated_new);

    var_updated_new = 0x0;
    printf("Blend:\n");
    printf("var_src = %x\n", var_src);
    printf("var_dst = %x\n", var_dst);
    printf("var_updated_new = %x\n", var_updated_new);
    var_updated_new = var_dst;
    rgba8_opacity_blend(&var_updated_new, &var_src);
    printf("var_updated_new = %x\n", var_updated_new);

    return 0;
}
