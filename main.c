#include <stdio.h>
#include <stdlib.h>
#include "SGI.h"
#include "PNG.h"

#define kPNG   1
#define kSGI   2


#pragma mark ---- Phân Tích Đuôi Tập Tin
unsigned char phanTichDuoiTapTin( char *tenTapTin ) {
   
   // ---- đến cuối cùnh tên
   while( *tenTapTin != 0x00 ) {
      tenTapTin++;
   }
   
   // ---- trở lại 3 cái
   tenTapTin -= 3;
   
   // ---- xem có đuôi nào
   unsigned char kyTu0 = *tenTapTin;
   tenTapTin++;
   unsigned char kyTu1 = *tenTapTin;
   tenTapTin++;
   unsigned char kyTu2 = *tenTapTin;
   
   unsigned char loaiTapTin = 0;
   
   // ---- PNG
   if( (kyTu0 == 'p') || (kyTu0 == 'P') ) {
      if( (kyTu1 == 'n') || (kyTu1 == 'N') ) {
         if( (kyTu2 == 'g') || (kyTu2 == 'G') ) {
            loaiTapTin = kPNG;
         }
      }
   }
   
   // ---- SGI
   else if( (kyTu0 == 's') || (kyTu0 == 'S') ) {
      if( (kyTu1 == 'g') || (kyTu1 == 'G') ) {
         if( (kyTu2 == 'i') || (kyTu2 == 'I') ) {
            loaiTapTin = kSGI;
         }
      }
   }
 
   else if( (kyTu0 == 'r') || (kyTu0 == 'R') ) {
      if( (kyTu1 == 'g') || (kyTu1 == 'G') ) {
         if( (kyTu2 == 'b') || (kyTu2 == 'B') ) {
            loaiTapTin = kSGI;
         }
      }
   }
   
   return loaiTapTin;
}

#pragma mark ==== Đuôi Tập Tin
void tenAnhPNG( char *tenAnhGoc, char *tenAnhPNG ) {
   
   // ---- chép tên ảnh gốc
   while( *tenAnhGoc != 0x00 ) {
      *tenAnhPNG = *tenAnhGoc;
      tenAnhPNG++;
      tenAnhGoc++;
   }
   
   // ---- trở lại 3 cái
   tenAnhPNG -= 3;
   
   // ---- kèm đuôi PNG
   *tenAnhPNG = 'p';
   tenAnhPNG++;
   *tenAnhPNG = 'n';
   tenAnhPNG++;
   *tenAnhPNG = 'g';
   tenAnhPNG++;
   *tenAnhPNG = 0x0;
}

void tenAnhSGI( char *tenAnhGoc, char *tenAnhSGI ) {
   
   // ---- chép tên ảnh gốc
   while( *tenAnhGoc != 0x00 ) {
      *tenAnhSGI = *tenAnhGoc;
      tenAnhSGI++;
      tenAnhGoc++;
   }
   
   // ---- trở lại 3 cái
   tenAnhSGI -= 3;
   
   // ---- kèm đuôi SGI
   *tenAnhSGI = 's';
   tenAnhSGI++;
   *tenAnhSGI = 'g';
   tenAnhSGI++;
   *tenAnhSGI = 'i';
   tenAnhSGI++;
   *tenAnhSGI = 0x0;
}


#pragma ==== Main
int main( int argc, char **argv ) {
   
   
   if( argc > 1 ) {
      // ---- phân tích đuôi tập tin
      unsigned char loaiTapTin = 0;
      loaiTapTin = phanTichDuoiTapTin( argv[1] );
      
      if( loaiTapTin == kSGI ) {
         printf( " SGI file\n" );
         sgi_image_data anh_SGI = decode_sgi( argv[1] );
         
         // ---- pha trộn các kênh
         unsigned int beDaiDemPhaTron = anh_SGI.width*anh_SGI.height;
         if( anh_SGI.num_channels == 3 )
            beDaiDemPhaTron *= 3;
         else if( anh_SGI.num_channels == 4 )
            beDaiDemPhaTron <<= 2;
         else {
            printf( "No support SGI file have channels: %d\n", anh_SGI.num_channels );
            exit(0);
         }
         
         // ---- tên tập tin
         char tenTep[255];
         tenAnhPNG( argv[1], tenTep );
         printf( " Save: %s\n", tenTep );
         
         // ---- mix channels for PNG file
         unsigned int chiSoCuoi = beDaiDemPhaTron;
         unsigned char *demPhaTron = malloc( beDaiDemPhaTron );
         
         if( demPhaTron ) {
            
            if( anh_SGI.num_channels == 3 ) {
               unsigned int chiSo = 0;
               unsigned int chiSoKenh = 0;
               while( chiSo < chiSoCuoi ) {
                  demPhaTron[chiSo] = anh_SGI.channel_R[chiSoKenh];
                  demPhaTron[chiSo+1] = anh_SGI.channel_G[chiSoKenh];
                  demPhaTron[chiSo+2] = anh_SGI.channel_B[chiSoKenh];
                  chiSo += 3;
                  chiSoKenh++;
               }
               // ---- lưu tập tin PNG
               luuAnhPNG( tenTep, demPhaTron, anh_SGI.width, anh_SGI.height, kPNG_BGR );
            }
            else if( anh_SGI.num_channels == 4 ) {
               unsigned int chiSo = 0;
               unsigned int chiSoKenh = 0;
               while( chiSo < chiSoCuoi ) {
                  demPhaTron[chiSo] = anh_SGI.channel_R[chiSoKenh];
                  demPhaTron[chiSo+1] = anh_SGI.channel_G[chiSoKenh];
                  demPhaTron[chiSo+2] = anh_SGI.channel_B[chiSoKenh];
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  chiSoKenh++;
               }
               
               // ---- lưu tập tin PNG
               luuAnhPNG( tenTep, demPhaTron, anh_SGI.width, anh_SGI.height, kPNG_BGRO );
            }
            
            
         }
      }
      else if( loaiTapTin == kPNG ) {
         printf( " PNG file\n" );
         
         unsigned int beRong = 0;
         unsigned int beCao = 0;
         unsigned char canLatMau = 0;
         unsigned char loaiPNG;
         unsigned char *duLieuAnhPNG = docPNG( argv[1], &beRong, &beCao, &canLatMau, &loaiPNG );
         
         if( duLieuAnhPNG == NULL ) {
            printf( "No support this type PNG file\n" );
            exit(0);
         }
         
         
         sgi_image_data anhPNG;
         anhPNG.width = beRong;
         anhPNG.height = beCao;
         printf( "Size %d x %d (%p)  loai %d\n", beRong, beCao, duLieuAnhPNG, loaiPNG );
         
         // ---- create channel
         anhPNG.num_channels = 3;
         anhPNG.channel_R = malloc( beRong*beCao );
         anhPNG.channel_G = malloc( beRong*beCao );
         anhPNG.channel_B = malloc( beRong*beCao );
         if( (anhPNG.channel_R == NULL ) || (anhPNG.channel_G == NULL)
            || (anhPNG.channel_B == NULL) ) {
            printf( "Problem create RGB channel for PNG image\n" );
            exit(0);
         }
         
         if( loaiPNG == kPNG_BGRO ) {
            anhPNG.num_channels = 4;
            anhPNG.channel_A = malloc( beRong*beCao );
            if( anhPNG.channel_A == NULL ) {
               printf( "Problem create A channel for PNG image\n" );
               exit(0);
            }
         }
         
         unsigned int chiSoDuLieuAnh = 0;
         unsigned int chiSoDuLieuAnhCuoi = beRong*beCao;
         unsigned int chiSoKenh = 0;
         
         if( loaiPNG == kPNG_BGR ) {
            chiSoDuLieuAnhCuoi *= 3;
            while( chiSoDuLieuAnh < chiSoDuLieuAnhCuoi ) {
               anhPNG.channel_R[chiSoKenh] = duLieuAnhPNG[chiSoDuLieuAnh];
               anhPNG.channel_G[chiSoKenh] = duLieuAnhPNG[chiSoDuLieuAnh+1];
               anhPNG.channel_B[chiSoKenh] = duLieuAnhPNG[chiSoDuLieuAnh+2];
               
               chiSoKenh++;
               chiSoDuLieuAnh += 3;
            }
         }
         else if( loaiPNG == kPNG_BGRO ) {
            chiSoDuLieuAnhCuoi <<= 2;
            while( chiSoDuLieuAnh < chiSoDuLieuAnhCuoi ) {
               anhPNG.channel_A[chiSoKenh] = duLieuAnhPNG[chiSoDuLieuAnh+3];
               anhPNG.channel_R[chiSoKenh] = duLieuAnhPNG[chiSoDuLieuAnh];
               anhPNG.channel_G[chiSoKenh] = duLieuAnhPNG[chiSoDuLieuAnh+1];
               anhPNG.channel_B[chiSoKenh] = duLieuAnhPNG[chiSoDuLieuAnh+2];
               
               chiSoKenh++;
               chiSoDuLieuAnh += 4;
            }
         }
         
         // ---- chuẩn bị tên tập tin
         char tenTep[255];
         tenAnhSGI( argv[1], tenTep );
         printf( " %s\n", tenTep );
         
         // ---- encode SGI image
         encode_sgi( tenTep, &anhPNG );
      }
      else {
         printf( "No support this file format/ Only suport .png; .sgi; or .rgb\n" );
      }
      
   }
   else {
      printf( " Please type file name for convert: './sgi_png <file name>'\n" );
   }
   
   return 1;
}
/*

#pragma mark ==== Mẫu Tên Tập Tin
void tenAnhChoCacAnhTuDia( char *tenAnhGoc, char *tenAnhPNG ) {
   
   // ---- chép tên ảnh gốc
   while( *tenAnhGoc != 0x00 ) {
      *tenAnhPNG = *tenAnhGoc;
      tenAnhPNG++;
      tenAnhGoc++;
   }
   
   // ---- trở lại 4 cái
   tenAnhPNG -= 4;
   
   // ---- that thế
   *tenAnhPNG = '_';
}


// For find and read SGI image file from IRIX format CD
int main( int argc, char **argv ) {

   if( argc > 1 ) {

      FILE *dia = fopen( argv[1], "rb" );
      unsigned int soLuongTapTin = 0;

      if( dia ) {
         
         // ---- tạo tên các tập tin
         char tenAnhPNG[255];
         tenAnhChoCacAnhTuDia( argv[1], tenAnhPNG );
         
         unsigned char kyTu = 0;

         // ---- tìm mã 01 da 01 01
         while( !feof( dia ) ) {
            kyTu = fgetc( dia );

            if( kyTu == 0x01 ) {
               kyTu = fgetc( dia );

               if( kyTu == 0xda ) {
                  unsigned char nen = fgetc( dia );
                  
                  if( (nen == 0x00) || (nen == 0x01) ) {
                     unsigned char loaiDuLieu = fgetc( dia );

                     if( (loaiDuLieu == 0x01) || (loaiDuLieu == 0x02) ) {
                        // ---- giữ địa chỉ đầu tập tin
                        unsigned int diaChiTapTin = ftell( dia ) - 4;
                        
                        // ---- chiều
                        fgetc( dia );
                        unsigned char chieu = fgetc( dia );
                        
                        if( (chieu == 3) || (chieu == 1) ) {
                           unsigned short beRong = (fgetc( dia ) << 8) | fgetc( dia );
                           unsigned short beCao = (fgetc( dia ) << 8) | fgetc( dia );
                           unsigned short soLuongKenh = (fgetc( dia ) << 8) | fgetc( dia );
                           
                           if( soLuongKenh < 5 ) {
                              
                              // ---- tên tập tin
                              fseek( dia, 12, SEEK_CUR );
                              char tenTapTin[80];
                              unsigned short diaChi = 0;
                              while( diaChi < 80 ) {
                                 tenTapTin[diaChi] = fgetc( dia );
                                 diaChi++;
                              }
                              
                              // ---- tạo tên cho tập tin lưu lại 
                              char tenTapTinLuuLai[255];
                              sprintf( tenTapTinLuuLai, "%s_%03d.png", tenAnhPNG, soLuongTapTin );
                              
                              // ---- tìm chiều dài tập tin
                              unsigned int chieuDaiTapTin = 512;
                              if( nen ) {
                                 // ---- địa chỉ thần phần cuối
                                 fseek( dia, 408 + (beCao << 2) - 4, SEEK_CUR );
                                 unsigned int diaChiThanPhanCuoi = (fgetc( dia ) << 24) | (fgetc( dia ) << 16) | (fgetc( dia ) << 8) | fgetc( dia );
                                 // ---- bề dài thành phần cuối
                                 fseek( dia, (beCao << 2) - 4, SEEK_CUR );
                                 unsigned int beDaiThanPhanCuoi = (fgetc( dia ) << 24) | (fgetc( dia ) << 16) | (fgetc( dia ) << 8) | fgetc( dia );
                                 
                                 chieuDaiTapTin += diaChiThanPhanCuoi + beDaiThanPhanCuoi;
                              }
                              else {
                                 chieuDaiTapTin += beRong*beCao*loaiDuLieu*soLuongKenh;
                              }
                              
                              if( loaiDuLieu == 1 )
                                 printf( "%3d  %d (0x%08x) compress %d  8 bit  dim %d  %4d x %4d  chan %d  size %8d %s\n", soLuongTapTin, diaChiTapTin, diaChiTapTin, nen, chieu, beRong, beCao, soLuongKenh, chieuDaiTapTin, tenTapTin );
                              else
                                 printf( "%3d  %d (0x%08x) compress %d 16 bit  dim %d  %4d x %4d  chan %d  size %8d %s\n", soLuongTapTin, diaChiTapTin, diaChiTapTin, nen, chieu, beRong, beCao, soLuongKenh, chieuDaiTapTin, tenTapTin );
                              
                              soLuongTapTin++;
                           }
                        }
                     }
                  }
               }
            }
         }
      }
      else {
         printf( " Không được tập tin: %s\n", argv[1] );
      }

   }
   return 0;
}
*/