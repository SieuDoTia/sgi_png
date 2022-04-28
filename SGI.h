/* IRIS.h */

#pragma once


/* image data */
typedef struct {
   unsigned short width;    /* width      */
   unsigned short height;   /* height     */
   unsigned char *channel_A;        /* channel a  */
   unsigned char *channel_B;        /* channel b  */
   unsigned char *channel_G;        /* channel g  */
   unsigned char *channel_R;        /* channel r  */
   unsigned char num_channels;     /* number channel */
} image_data;


/* đọc tệp IRIS */
image_data decode_iris( const char *sfile );

