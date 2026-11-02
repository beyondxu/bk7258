#pragma once

#define LV_COLOR_DEPTH      16                 //lvgl的颜色深度，需要让lvgl输出rgb565填16，输出agb888填32
#define LV_COLOR_16_SWAP    0                  //一般不修改，spi或qspi屏，需调整为1


typedef unsigned char  		  uint8_t;          /* unsigned  8 bit quantity        */
typedef unsigned char  		  uint8;          /* unsigned  8 bit quantity        */
typedef signed   char  		  int8_t;           /* signed    8 bit quantity        */
typedef unsigned short 		  uint16_t;         /* unsigned 16 bit quantity        */
typedef signed   short 		  int16_t;          /* signed   16 bit quantity        */
typedef unsigned int   		  uint32_t;         /* unsigned 32 bit quantity        */
typedef signed   int   		  int32_t;          /* signed   32 bit quantity        */
typedef unsigned long long    uint64_t;         /* unsigned 32 bit quantity        */
typedef signed   long long    int64_t;          /* signed   32 bit quantity        */

typedef struct {

    uint32_t cf : 5;          /*Color format: See `lv_img_color_format_t`*/
    uint32_t always_zero : 3; /*It the upper bits of the first byte. Always zero to look like a
                                 non-printable character*/

    uint32_t reserved : 2; /*Reserved to be used later*/

    uint32_t w : 11; /*Width of the image map*/
    uint32_t h : 11; /*Height of the image map*/
} lv_img_header_t;

enum {
    LV_IMG_CF_UNKNOWN = 0,

    LV_IMG_CF_RAW,              /**< Contains the file as it is. Needs custom decoder function*/
    LV_IMG_CF_RAW_ALPHA,        /**< Contains the file as it is. The image has alpha. Needs custom decoder
                                   function*/
    LV_IMG_CF_RAW_CHROMA_KEYED, /**< Contains the file as it is. The image is chroma keyed. Needs
                                   custom decoder function*/

    LV_IMG_CF_TRUE_COLOR,              /**< Color format and depth should match with LV_COLOR settings*/
    LV_IMG_CF_TRUE_COLOR_ALPHA,        /**< Same as `LV_IMG_CF_TRUE_COLOR` but every pixel has an alpha byte*/
    LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED, /**< Same as `LV_IMG_CF_TRUE_COLOR` but LV_COLOR_TRANSP pixels
                                          will be transparent*/

    LV_IMG_CF_INDEXED_1BIT, /**< Can have 2 different colors in a palette (can't be chroma keyed)*/
    LV_IMG_CF_INDEXED_2BIT, /**< Can have 4 different colors in a palette (can't be chroma keyed)*/
    LV_IMG_CF_INDEXED_4BIT, /**< Can have 16 different colors in a palette (can't be chroma keyed)*/
    LV_IMG_CF_INDEXED_8BIT, /**< Can have 256 different colors in a palette (can't be chroma keyed)*/

    LV_IMG_CF_ALPHA_1BIT, /**< Can have one color and it can be drawn or not*/
    LV_IMG_CF_ALPHA_2BIT, /**< Can have one color but 4 different alpha value*/
    LV_IMG_CF_ALPHA_4BIT, /**< Can have one color but 16 different alpha value*/
    LV_IMG_CF_ALPHA_8BIT, /**< Can have one color but 256 different alpha value*/

    LV_IMG_CF_RGB888,
    LV_IMG_CF_RGBA8888,
    LV_IMG_CF_RGBX8888,
    LV_IMG_CF_RGB565,
    LV_IMG_CF_RGBA5658,
    LV_IMG_CF_RGB565A8,

    LV_IMG_CF_RESERVED_15,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_16,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_17,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_18,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_19,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_20,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_21,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_22,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_23,              /**< Reserved for further use.*/

    LV_IMG_CF_USER_ENCODED_0,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_1,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_2,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_3,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_4,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_5,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_6,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_7,          /**< User holder encoding format.*/
};

#if LV_COLOR_DEPTH == 1
#define LV_COLOR_SIZE 8
#elif LV_COLOR_DEPTH == 8
#define LV_COLOR_SIZE 8
#elif LV_COLOR_DEPTH == 16
#define LV_COLOR_SIZE 16
#elif LV_COLOR_DEPTH == 32
#define LV_COLOR_SIZE 32
#else
#error "Invalid LV_COLOR_DEPTH in lv_conf.h! Set it to 1, 8, 16 or 32!"
#endif

#if LV_COLOR_DEPTH == 1 || LV_COLOR_DEPTH == 8
#define LV_IMG_PX_SIZE_ALPHA_BYTE 2
#elif LV_COLOR_DEPTH == 16
#define LV_IMG_PX_SIZE_ALPHA_BYTE 3
#elif LV_COLOR_DEPTH == 32
#define LV_IMG_PX_SIZE_ALPHA_BYTE 4
#endif

/** Image header it is compatible with
 * the result from image converter utility*/
typedef struct {
    lv_img_header_t header; /**< A header describing the basics of the image*/
    uint32_t data_size;     /**< Size of the image in bytes*/
    const uint8_t * data;   /**< Pointer to the data of the image*/
} lv_img_dsc_t;
