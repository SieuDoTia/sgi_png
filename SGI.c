// SGI.c   Silicon Graphics Image

/* HEAD - ĐẦU
   2 bytes  | ushort | MAGIC     | Magic number
   1 byte   | uchar  | COMPRESS  | Compress RLE
   1 byte   | uchar  | BPC       | Number of bytes per pixel channel
   2 bytes  | ushort | DIMENSION | Number of dimensions
   2 bytes  | ushort | XSIZE     | X size in pixels
   2 bytes  | ushort | YSIZE     | Y size in pixels
   2 bytes  | ushort | ZSIZE     | Number of channels
   4 bytes  | int    | PIXMIN    | Minimum pixel value
   4 bytes  | int    | PIXMAX    | Maximum pixel value
   4 bytes  | int    | DUMMY     | Ignore
  80 bytes  | char   | IMAGENAME | Image name
   4 bytes  | uint   | COLORMAP  | Colormap ID
 404 bytes  | uchar  | DUMMY     | Ignored
 
 image data start at byte 512
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SGI.h"
#include "PNG.h"


#define kMAGIC_NUMBER_SGI 0x01da

typedef enum {
   SGI_COMPRESSION_NO,       /* no compression                  */
   SGI_COMPRESSION_RLE,      /* 8-bit run-length-encoded        */
} sgi_compression_enum;

typedef enum {
   SGI_DATA_UCHAR,           /* unsigned char 8 bit   */
   SGI_DATA_USHORT,          /* unsigned short 16 bit */
   SGI_DATA_UNKNOW,          /* unknow type           */
} sgi_dataType_enum;

typedef enum {
   SGI_COLORMAP_NO,          /* no color map  */
   SGI_COLORMAP_DITHER,      /* pack: R 3bit, G bit 3 bit, B = 2 bit */
   SGI_COLORMAP_SCREEN,      /* RGB image have 1 channel use color map */
   SGI_COLORMAP_COLORMAP,    /* this file store color map only */
} sgi_color_map;

typedef enum {
   RED,
   GREEN,
   BLUE,
   OPACITY,
} sgi_channel_enum;  /* index for stadard channels */

typedef struct {
   char fileName[256];         /* file name max 255 char UTF8 */
   char imageName[80];        /* image name max 79 char UTF8  */
   unsigned char compression; /* compression: no or RLE       */
   unsigned char data_type;   /* channel data type uchar, ushort */
   unsigned char dimension;   /* width * height * num_channel */
   unsigned short width;      /* image width                  */
   unsigned short height;     /* image height                 */
   unsigned int pixel_min;    /* pixel value minimum          */
   unsigned int pixel_max;    /* pixel value maximum          */
   unsigned int color_map;    /* color map                    */

   unsigned short num_channels; /* number channels       */
} sgi_attributes;



/* sgi chunk data - only if use RLE */
typedef struct {
   unsigned int num_chunks;            /* number chunks */
   unsigned int *chunk_table;          /* chunk table list */
   unsigned int *chunk_size;         /* chunk size */
   unsigned int *chunk_table_position; /* list for chunk's position in table */
} sgi_chunk_data;



#pragma mark ---- Read Attributes
sgi_attributes readAttributes( FILE *sgi_fp ) {
   
   sgi_attributes attributes;

   // ---- compression
   attributes.compression = fgetc(sgi_fp);

   // ---- byte depth
   unsigned char depth = fgetc(sgi_fp);
   
   if( depth == 1 )
      attributes.data_type = SGI_DATA_UCHAR;
   else if( depth == 2 )
      attributes.data_type = SGI_DATA_USHORT;
   else
      attributes.data_type = SGI_DATA_UNKNOW;
   
   // ---- dimension
   attributes.dimension = fgetc(sgi_fp) << 8 | fgetc(sgi_fp);

   // ---- width
   attributes.width = fgetc(sgi_fp) << 8 | fgetc(sgi_fp);
   
   // ---- height
   attributes.height = fgetc(sgi_fp) << 8 | fgetc(sgi_fp);
   
   // ---- number channels
   attributes.num_channels = fgetc(sgi_fp) << 8 | fgetc(sgi_fp);
   
   // ---- pixel min
   attributes.pixel_min = fgetc(sgi_fp) << 24 | fgetc(sgi_fp) << 16 | fgetc(sgi_fp) << 8 | fgetc(sgi_fp);
   
   // ---- pixel max
   attributes.pixel_max = fgetc(sgi_fp) << 24 | fgetc(sgi_fp) << 16 | fgetc(sgi_fp) << 8 | fgetc(sgi_fp);
   
   // ---- skip 4 byte
   fgetc(sgi_fp);
   fgetc(sgi_fp);
   fgetc(sgi_fp);
   fgetc(sgi_fp);
   
   // ---- image name (always use 80 byte)
   unsigned char index = 0;
   while( index < 80 ) {
      attributes.imageName[index] = fgetc(sgi_fp);
      index++;
   }
   
   // ---- color map type
   attributes.color_map = fgetc(sgi_fp) << 24 | fgetc(sgi_fp) << 16 | fgetc(sgi_fp) << 8 | fgetc(sgi_fp);

   return attributes;
}

#pragma mark ---- Read Chunk Data
sgi_chunk_data read_chunk_data( FILE *sgi_fp, unsigned int table_length ) {

   sgi_chunk_data chunk_data;
   chunk_data.num_chunks = table_length;

   // ---- get memory for chunk table
   chunk_data.chunk_table = malloc( table_length << 2 );  // 4 bytes
   chunk_data.chunk_table_position = malloc( table_length << 2 );  // 4 bytes
   chunk_data.chunk_size = malloc( table_length << 2 );  // 4 bytes
   
   // ----- read address for chunks
   unsigned int chunk_index = 0;
   while( chunk_index < table_length ) {
      chunk_data.chunk_table_position[chunk_index] = ftell( sgi_fp );
      chunk_data.chunk_table[chunk_index] = fgetc(sgi_fp) << 24 | fgetc(sgi_fp) << 16 | fgetc(sgi_fp) << 8 | fgetc(sgi_fp);

      chunk_index++;
   }
   
   // ----- read size for chunks
   chunk_index = 0;
   while( chunk_index < table_length ) {

      chunk_data.chunk_size[chunk_index] = fgetc(sgi_fp) << 24 | fgetc(sgi_fp) << 16 | fgetc(sgi_fp) << 8 | fgetc(sgi_fp);
/*      printf( "%d %d %x - %d  %d\n", chunk_index, chunk_data.chunk_table[chunk_index], chunk_data.chunk_table[chunk_index], chunk_data.chunk_table_position[chunk_index], chunk_data.chunk_size[chunk_index] ); */
      chunk_index++;
   }

   return chunk_data;
}


#pragma mark ----- RLE Compress
void uncompress_rle( unsigned char *compressed_buffer, int compressed_buffer_length, unsigned char *uncompressed_buffer, int uncompressed_buffer_length) {
   
   unsigned char *outStart = compressed_buffer;
   
   // ---- while not finish all in buffer
   while( compressed_buffer_length > 0 ) {   // có một byte cuối = 0x00
      // ---- if byte value
      if( *compressed_buffer > 127 ) {
         // ---- count for not same byte value
         int count = *compressed_buffer & 0x7f;
//         printf( " chép count %d  compressed_buffer_length %d  uncompressed_buffer_length %d\n", count, compressed_buffer_length, uncompressed_buffer_length );
         compressed_buffer++;
         // ---- reduce amount count of bytes remaining for in buffer
         compressed_buffer_length -= count + 1;
         // ---- if count larger than out buffer length still available
         if (0 > (uncompressed_buffer_length -= count)) {
            printf( "uncompress_rle: Error buffer not length not enough\n" );
            return;
         }
         // ---- copy not same byte and move to next byte
         while( count-- > 0 )
            *uncompressed_buffer++ = *(compressed_buffer++);
      }
      else {
         // ---- count number bytes same
         int count = *compressed_buffer++;
//         printf( " count %d  compressed_buffer_length %d  uncompressed_buffer_length %d\n", count, compressed_buffer_length, uncompressed_buffer_length );
         // ---- reduce amount count of remaining bytes for in buffer
         compressed_buffer_length -= 2;
         // ---- if count larger than out buffer length still available
         if (0 > (uncompressed_buffer_length -= count)) {
            printf( "uncompress_rle: Error buffer not length not enough\n" );
            return;
         }
         // ---- copy same byte
         while( count-- > 0 )
            *uncompressed_buffer++ = *compressed_buffer;
         // ---- move to next byte
         compressed_buffer++;
      }
   }

}

// From OpenEXR but change for SGI format
unsigned int compress_rle( unsigned char *uncompressed_buffer, unsigned int uncompressed_buffer_length, unsigned char *compressed_buffer ) {
   const unsigned char *inEnd = uncompressed_buffer + uncompressed_buffer_length;
   const unsigned char *runStart = uncompressed_buffer;
   const unsigned char *runEnd = uncompressed_buffer + 1;
   unsigned char *outWrite = compressed_buffer;
   
   // ---- while not at end of in buffer
   while (runStart < inEnd) {
      
      // ---- count number bytes same value; careful not go beyond end of chunk, chunk longer than 125?
      while (runEnd < inEnd && *runStart == *runEnd && runEnd - runStart - 1 < 125) {
         ++runEnd;
      }
      // ---- if number bytes same value >= MIN_RUN_LENGTH
      if (runEnd - runStart >= 3) {
         //
         // Compressable run
         //
         // ---- number bytes same value - 1
         *outWrite++ = (runEnd - runStart);
         // ---- byte value
         *outWrite++ = *runStart;
         // ---- move to where different value found or MAX_RUN_LENGTH (smallest of these two)
         runStart = runEnd;
      }
      else {
         //
         // Uncompressable run
         //
         // ---- count number of bytes not same; careful end of chunk,
         while (runEnd < inEnd &&
                ((runEnd + 1 >= inEnd || *runEnd != *(runEnd + 1)) || (runEnd + 2 >= inEnd || *(runEnd + 1) != *(runEnd + 2))) &&
                runEnd - runStart < 127) {
            ++runEnd;
         }
         // ---- number bytes not same
         *outWrite++ = (runEnd - runStart) | 0x80;
         // ---- not same bytes
         while (runStart < runEnd) {
            *outWrite++ = *(runStart++);
         }
      }
      // ---- move to next byte
      ++runEnd;
   }
   
   return (unsigned int)(outWrite - compressed_buffer);
}

#pragma mark ---- Copy Data
void copyBufferUchar( unsigned char *destination, unsigned char *ucharSource, unsigned int length ) {

   unsigned int index = 0;
   while( index < length ) {
      *destination = *ucharSource;
      destination++;
      ucharSource++;
      index++;
   }
}

void copyBufferUshort( unsigned short *destination, unsigned short *ushortSource, unsigned int length ) {
   
   unsigned int index = 0;
   while( index < length ) {
      *destination = *ushortSource;
      destination++;
      ushortSource++;
      index++;
   }
}


#pragma mark ---- No Compression
/* compression no */
void read_data_compression_no__scanline( FILE *sgi_fp, sgi_attributes *attributes, sgi_image_data *image_data ) {
  
   unsigned char channel_data_type = attributes->data_type;

   unsigned int lastIndex = attributes->width*attributes->height;

   // ---- begin scan line
   fseek( sgi_fp, 512, SEEK_SET );
   
   // ----
   unsigned int index = 0;
   while( index < lastIndex ) {
      image_data->channel_R[index] = fgetc(sgi_fp);
      index++;
   }
   
   index = 0;
   while( index < lastIndex ) {
      image_data->channel_G[index] = fgetc(sgi_fp);
      index++;
   }
   
   index = 0;
   while( index < lastIndex ) {
      image_data->channel_B[index] = fgetc(sgi_fp);
      index++;
   }
   
   if( image_data->channel_A != NULL ) {
      index = 0;
      while( index < lastIndex ) {
         image_data->channel_A[index] = fgetc(sgi_fp);
         index++;
      }
   }
}

#pragma mark ---- RLE Compression
void read_data_compression_rle__scanline( FILE *sgi_fp, sgi_chunk_data *chunk_data, sgi_attributes *attributes, sgi_image_data *image_data ) {

   unsigned int num_columns = attributes->width;
  
   unsigned short channel_data_type = attributes->data_type;

   unsigned int uncompressed_data_length = num_columns << channel_data_type;

//   printf( "uncompressed_data_length %d  num_columns %d  channel_data_type %d\n", uncompressed_data_length, num_columns, channel_data_type );

   unsigned char *compressed_buffer = malloc( uncompressed_data_length << 1 );
   unsigned char *uncompressed_buffer = malloc( uncompressed_data_length );
   
   unsigned char channel_number = 0;
   unsigned int channelDataIndex = 0;

   unsigned short chunk_number = 0;
   while( chunk_number < chunk_data->num_chunks ) {
      // ---- begin chunk
      fseek( sgi_fp, chunk_data->chunk_table[chunk_number], SEEK_SET );


      // ---- read data length
      unsigned int data_length = chunk_data->chunk_size[chunk_number];
//       printf( " chunk %d  data_length %d  channelDataIndex %d  channel_number %d  %d\n", chunk_number, data_length, channelDataIndex, channel_number,
//              chunk_number % attributes->height );
      // ---- read compressed data
      fread( compressed_buffer, 1, data_length, sgi_fp );

      // ---- uncompress rle data
      uncompress_rle( compressed_buffer, data_length, uncompressed_buffer, uncompressed_data_length );

//      printf( "  uncompressed_data_length %d\n", uncompressed_data_length );

      if( channel_number == RED ) {
         copyBufferUchar( &(image_data->channel_R[channelDataIndex]), uncompressed_buffer, uncompressed_data_length );
      }
      else if( channel_number == GREEN ) {
         copyBufferUchar( &(image_data->channel_G[channelDataIndex]), uncompressed_buffer, uncompressed_data_length );
      }
      else if( channel_number == BLUE ) {
         copyBufferUchar( &(image_data->channel_B[channelDataIndex]), uncompressed_buffer, uncompressed_data_length );
      }
      else if( channel_number == OPACITY ) {
         copyBufferUchar( &(image_data->channel_A[channelDataIndex]), uncompressed_buffer, uncompressed_data_length );
      }
      
      // ---- next chunk
      chunk_number++;
      
      // ---- check if finish data for this channel, switch for next channel
      if( (chunk_number % attributes->height) == 0 ) {
         channelDataIndex = 0;
         channel_number++;
      }
      else
         channelDataIndex += uncompressed_data_length;
   }

   // ---- free memory
   free( compressed_buffer );
   free( uncompressed_buffer );
}

#pragma mark ---- Decode SGI
sgi_image_data decode_sgi( const char *sfile ) {

   FILE *sgi_fp;
   sgi_image_data image_data;
   image_data.width = 0;
   image_data.height = 0;
   
//   printf( "decode_sgi: TenTep %s\n", sfile );
   sgi_fp = fopen( sfile, "rb" );
    
   if (!sgi_fp) {
      printf("%-15.15s: Error open exr file %s\n","read_exr",sfile);
      return image_data;
   }
    
   // ---- check magic number/file signature
   unsigned short magicNumber = 0x0;
   magicNumber = fgetc(sgi_fp) << 8 | fgetc(sgi_fp);
   if( magicNumber != kMAGIC_NUMBER_SGI ) {
      printf( "%-15.15s: failed to read magic number expected 0x%08x read 0x%08x\n","read_exr", kMAGIC_NUMBER_SGI, magicNumber );
      printf( "%s is not a valid EXR file", sfile);
      return image_data;
   }

   // ---- read EXR attritubes - 108 byte
   sgi_attributes attributes = readAttributes( sgi_fp );

   // ---- show data about file
   printf( " compression %d\n", attributes.compression );

   printf( " dimension %d\n", attributes.dimension );
   printf( " dataType %d\n", attributes.data_type );
   printf( " color map %d\n", attributes.color_map );
   printf( " width %d\n", attributes.width );
   printf( " height %d\n", attributes.height );
   printf( " num channels %d\n", attributes.num_channels );
   
   if( attributes.color_map == SGI_COLORMAP_COLORMAP ) {
      printf( "Only color map in SGI file. No image data\n");
      return image_data;
   }


   // ---- check compression
   if( attributes.compression > SGI_COMPRESSION_RLE ) {
      printf( "Only support NO, RLE compression in SGI file\n");
      return image_data;
   }
   
   image_data.width = attributes.width;
   image_data.height = attributes.height;
   image_data.num_channels = attributes.num_channels;
   
   // ---- create buffers for image data
   unsigned int beDaiDem = attributes.width*attributes.height * sizeof( unsigned short );
//   printf( " beDaiDem %d\n", beDaiDem );
   
   image_data.channel_B = malloc( beDaiDem );
   image_data.channel_G = malloc( beDaiDem );
   image_data.channel_R = malloc( beDaiDem );
   
   if( (image_data.channel_R == NULL ) || (image_data.channel_G == NULL) || (image_data.channel_B == NULL) ) {
      printf( "Problem create channel R, G, B for SGI image\n" );
      exit(0);
   }

   if( attributes.num_channels == 4 ) {
      image_data.channel_A = malloc( beDaiDem );
      if( image_data.channel_A == NULL ) {
         printf( "Problem create channel A for SGI image\n" );
         exit(0);
      }
   }
   else
      image_data.channel_A = NULL;
   

   
   // ---- skip 404 byte (image data start at byte 512)
   fseek( sgi_fp, 512, SEEK_SET );

   // ---- read offset table for RLE compression
   if( attributes.compression == SGI_COMPRESSION_RLE ) {
      sgi_chunk_data chunk_data = read_chunk_data( sgi_fp, attributes.height*attributes.num_channels );
      printf( " number chunks: %d\n", chunk_data.num_chunks );

      // ---- read file data
     read_data_compression_rle__scanline( sgi_fp, &chunk_data, &attributes, &image_data );
      
      // ---- free memory
      free( chunk_data.chunk_table );
   }
   else {
   // ---- read file data
      read_data_compression_no__scanline( sgi_fp, &attributes, &image_data );
   }

   fclose( sgi_fp );

   return image_data;
}

// ===================================
void tenAnh_RGBO( char *tenAnhGoc, char *tenAnhPNG ) {
   
   // ---- chép tên ảnh gốc
   while( (*tenAnhGoc != 0x00) && (*tenAnhGoc != '.') ) {
      *tenAnhPNG = *tenAnhGoc;
      tenAnhPNG++;
      tenAnhGoc++;
   }
   
   // ---- kèm đuôi
   *tenAnhPNG = '.';
   tenAnhPNG++;
   *tenAnhPNG = 'p';
   tenAnhPNG++;
   *tenAnhPNG = 'n';
   tenAnhPNG++;
   *tenAnhPNG = 'g';
   tenAnhPNG++;
   *tenAnhPNG = 0x0;
   tenAnhPNG++;
}


#pragma mark ---- Write SGI
void writeAttributes( FILE *sgi_fp, sgi_attributes *attributes );
void write_image_data( FILE *sgi_fp, sgi_attributes *attributes, sgi_image_data *anh );


void encode_sgi( char *tenTep, sgi_image_data *image_data ) {
   
   // ---- attributes for file
   sgi_attributes attributes;

   // ---- copy name (max 79 characters)
   unsigned char index = 0;
   unsigned char charCount = 0;
   while( index < 79 ) {
      char kyTu = tenTep[index];
      if( kyTu ) {
         attributes.imageName[index] = kyTu;
         charCount++;
         index++;
      }
      else
         index = 79;  // kyTu == 0, end of file name
   }
   
   // ---- zero for all last characters
   while( charCount < 80 ) {
      attributes.imageName[charCount] = 0x00;
      charCount++;
   }

   // ---- compression
   attributes.compression = SGI_COMPRESSION_RLE;    // use RLE
   
   // ---- byte per channel
   attributes.data_type = SGI_DATA_UCHAR;  // 1 byte = 8 bit
   
   // ---- dimension
   attributes.dimension = 3;  // 3  for RGB

   // ---- size
   attributes.width = (unsigned short)image_data->width;
   attributes.height = (unsigned short)image_data->height;
   
   // ---- color map
   attributes.color_map = SGI_COLORMAP_NO;   // normal
   
   // ---- minimum pixel value 
   attributes.pixel_min = 0x00;
   
   // ---- maximum pixel value 
   attributes.pixel_max = 0xff;

   // ---- number channel
   attributes.num_channels = image_data->num_channels;
//   printf( "%d %d %d\n", attributes.width, attributes.height, attributes.num_channels );
   
   FILE *tep = fopen( tenTep, "wb" );
   
   if( tep ) {
      // ---- image attribute
      writeAttributes( tep, &attributes );

      // ---- image
      write_image_data( tep, &attributes, image_data );

      // ---- đóng tệp
      fclose( tep );
   }
   else {
      printf( "Problem create SGI file\n" );
      exit(0);
   }
}

#pragma mark ---- Write Attributes
void writeAttributes( FILE *sgi_fp, sgi_attributes *attributes ) {

   // ---- mã số SGI
   fputc( 0x01, sgi_fp );
   fputc( 0xda, sgi_fp );
   
   // ---- compression
   fputc( 0x01, sgi_fp );  // use RLE
   
   // ---- byte per channel
   unsigned char data_type = attributes->data_type;
   if( data_type == SGI_DATA_USHORT )
      fputc( 0x02, sgi_fp );
   else // if( data_type == SGI_DATA_UCHAR )
      fputc( 0x01, sgi_fp );

   
   // ---- dimension
   fputc( 0x00, sgi_fp );
   fputc( attributes->dimension, sgi_fp );
   
   unsigned short width = attributes->width;
   unsigned short height = attributes->height;
   
   // ---- x size
   fputc( (width >> 8), sgi_fp );
   fputc( width & 0xff, sgi_fp );
   
   // ---- y size
   fputc( (height >> 8), sgi_fp );
   fputc( height & 0xff, sgi_fp );
   
   // ---- number channel
   unsigned short num_channel = attributes->num_channels;
   fputc( (num_channel >> 8), sgi_fp );
   fputc( num_channel & 0xff, sgi_fp );
   
   // ---- minimnum pixel value
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );
   unsigned short pixel_min = attributes->pixel_min;
   if( data_type == 1 )         // 8 bit
      fputc( pixel_min, sgi_fp );
   else {    // 16 bit
      fputc( (pixel_min >> 8), sgi_fp );
      fputc( pixel_min & 0xff, sgi_fp );
   }
   
   // ---- maximum pixel value
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );
   unsigned short pixel_max = attributes->pixel_max;
   if( data_type == 1 )         // 8 bit
      fputc( pixel_max, sgi_fp );
   else {    // 16 bit
      fputc( (pixel_max >> 8), sgi_fp );
      fputc( pixel_max & 0xff, sgi_fp );
   }
   
   // ---- dummy
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );

   // ---- image name
   unsigned short index = 0;
   while( index < 80 ) {
      char kyTu = attributes->imageName[index];
         fputc( kyTu, sgi_fp );
      index++;
   }

   // ---- color map ID
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );
   fputc( 0x00, sgi_fp );

   // ---- 404 byte == 0
   index = 0;
   while( index < 404 ) {
      fputc( 0x00, sgi_fp );
      index++;
   }

}

#pragma mark ---- Write Chunk Data
void write_image_data( FILE *sgi_fp, sgi_attributes *attributes, sgi_image_data *image_data ) {

   sgi_chunk_data chunk_data;
   chunk_data.num_chunks = attributes->height*attributes->num_channels;
   chunk_data.chunk_table = malloc( chunk_data.num_chunks * sizeof( unsigned int ) );
   chunk_data.chunk_size = malloc( chunk_data.num_chunks * sizeof( unsigned int ) );
   
   unsigned int chunk_table_start = (unsigned int)ftell( sgi_fp );
   
   // ---- skip space in file for chunk address
   unsigned int index = 0;
   unsigned int chunkTableSize = chunk_data.num_chunks << 2;
   while( index < chunkTableSize ) {
      fputc( 0x00, sgi_fp );
      index++;
   }
   
   // ---- skip space in file for chunk size
   index = 0;
   while( index < chunkTableSize ) {
      fputc( 0x00, sgi_fp );
      index++;
   }
   
   unsigned int uncompressed_buffer_length = attributes->width << attributes->data_type;
   unsigned char *uncompressed_buffer = malloc( uncompressed_buffer_length );
   unsigned char *compressed_buffer = malloc( uncompressed_buffer_length << 1 );
   
   unsigned int chunk_number = 0;
   unsigned char channel_number = RED;
   unsigned int channelDataIndex = 0;
//   printf( "uncompressed_buffer_length %d  numChunk %d\n", uncompressed_buffer_length, chunk_data.num_chunks );

   while( chunk_number < chunk_data.num_chunks ) {
      
      // ---- get chunk data
      if( channel_number == RED ) {
         copyBufferUchar( uncompressed_buffer, &(image_data->channel_R[channelDataIndex]), uncompressed_buffer_length );
      }
      else if( channel_number == GREEN ) {
         copyBufferUchar( uncompressed_buffer, &(image_data->channel_G[channelDataIndex]), uncompressed_buffer_length );
      }
      else if( channel_number == BLUE ) {
         copyBufferUchar( uncompressed_buffer, &(image_data->channel_B[channelDataIndex]), uncompressed_buffer_length );
      }
      else if( channel_number == OPACITY ) {
         copyBufferUchar( uncompressed_buffer, &(image_data->channel_A[channelDataIndex]), uncompressed_buffer_length );
      }
      
      // ---- compress chunk
      unsigned int chunk_size = compress_rle( uncompressed_buffer, uncompressed_buffer_length, compressed_buffer );
//      printf( "chan: %d  chunkNum: %d  size: %d  uncompressed_buffer_length %d\n", channel_number, chunk_number, chunk_size, uncompressed_buffer_length );

      // ---- save chunk size
      chunk_data.chunk_size[chunk_number] = chunk_size + 1;
      
      // ---- save chunk address
      chunk_data.chunk_table[chunk_number] = (unsigned int)ftell( sgi_fp );
      
      // ---- write data
      fwrite( compressed_buffer, 1, chunk_size, sgi_fp );
      fputc( 0x00, sgi_fp );   // zero byte after each chunk
      
      // ---- next chunk
      chunk_number++;
      
      // ---- check if finish data for this channel, switch for next channel
      if( (chunk_number % attributes->height) == 0 ) {
         channelDataIndex = 0;
         channel_number++;
      }
      else
         channelDataIndex += uncompressed_buffer_length;
   }
   
   // ---- write chunk table
   fseek( sgi_fp, chunk_table_start, SEEK_SET );
   chunk_number = 0;
   while( chunk_number < chunk_data.num_chunks ) {
      unsigned int chunk_address = chunk_data.chunk_table[chunk_number];
      fputc( (chunk_address >> 24) & 0xff, sgi_fp );
      fputc( (chunk_address >> 16) & 0xff, sgi_fp );
      fputc( (chunk_address >> 8) & 0xff, sgi_fp );
      fputc( chunk_address & 0xff, sgi_fp );
      chunk_number++;
   }
   
   // ---- write chunk size
   chunk_number = 0;
   while( chunk_number < chunk_data.num_chunks ) {
      unsigned int chunk_size = chunk_data.chunk_size[chunk_number];
      fputc( (chunk_size >> 24) & 0xff, sgi_fp );
      fputc( (chunk_size >> 16) & 0xff, sgi_fp );
      fputc( (chunk_size >> 8) & 0xff, sgi_fp );
      fputc( chunk_size & 0xff, sgi_fp );
      chunk_number++;
   }
   
   // ----
   free( compressed_buffer );
   free( uncompressed_buffer );
   free( chunk_data.chunk_table );
   free( chunk_data.chunk_size );
}

