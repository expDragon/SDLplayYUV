#include <iostream>
#include <cstdio>

extern "C"{
#include "SDL2/SDL.h"
}

//set '1' to choose a type of file to play  
#define LOAD_BGRA    0  
#define LOAD_RGB24   0  
#define LOAD_BGR24   0  
#define LOAD_YUV420P 1  
  
//Bit per Pixel  
#if LOAD_BGRA  
const int bpp=32;  
#elif LOAD_RGB24|LOAD_BGR24  
const int bpp=24;  
#elif LOAD_YUV420P  
const int bpp=12;  
#endif  
  
int screen_w=852,screen_h=480;  
const int pixel_w=426,pixel_h=240;  
  
unsigned char buffer[pixel_w*pixel_h*bpp/8];  
//BPP=32  
unsigned char buffer_convert[pixel_w*pixel_h*4];  

#define SEEK_SET	0
  
//Convert RGB24/BGR24 to RGB32/BGR32  
//And change Endian if needed  

void CONVERT_24to32(unsigned char *image_in,unsigned char *image_out,int w,int h){  
    for(int i =0;i<h;i++)  
        for(int j=0;j<w;j++){  
            //Big Endian or Small Endian?  
            //"ARGB" order:high bit -> low bit.  
            //ARGB Format Big Endian (low address save high MSB, here is A) in memory : A|R|G|B  
            //ARGB Format Little Endian (low address save low MSB, here is B) in memory : B|G|R|A  
            if(SDL_BYTEORDER==SDL_LIL_ENDIAN){  
                //Little Endian (x86): R|G|B --> B|G|R|A  
                image_out[(i*w+j)*4+0]=image_in[(i*w+j)*3+2];  
                image_out[(i*w+j)*4+1]=image_in[(i*w+j)*3+1];  
                image_out[(i*w+j)*4+2]=image_in[(i*w+j)*3];  
                image_out[(i*w+j)*4+3]='0';  
            }else{  
                //Big Endian: R|G|B --> A|R|G|B  
                image_out[(i*w+j)*4]='0';  
                memcpy(image_out+(i*w+j)*4+1,image_in+(i*w+j)*3,3);  
            }  
        }  
}  

  
//Refresh Event  
#define REFRESH_EVENT  (SDL_USEREVENT + 1)  
  
int thread_exit=0;  
  
int refresh_video(void *opaque){  
    while (thread_exit==0) {  
        SDL_Event event;  
        event.type = REFRESH_EVENT;  
        SDL_PushEvent(&event);  
        SDL_Delay(40);  
    }  
    return 0;  
}  

int main(int argc, char **argv) {
    if(SDL_Init(SDL_INIT_VIDEO)) {    
        std::cout<< "Could not initialize SDL - "<< SDL_GetError()<<std::endl;   
        return -1;  
    }   
  
    SDL_Window *screen;   
    //SDL 2.0 Support for multiple windows  
    screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,  
        screen_w, screen_h,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);  
    if(!screen) {    
        std::cout<<"SDL: could not create window - exiting:"<<SDL_GetError();    
        return -1;  
    }  
    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);    
  
    Uint32 pixformat=0;  
#if LOAD_BGRA  
    //Note: ARGB8888 in "Little Endian" system stores as B|G|R|A  
    pixformat= SDL_PIXELFORMAT_ARGB8888;    
#elif LOAD_RGB24  
    pixformat= SDL_PIXELFORMAT_RGB888;    
#elif LOAD_BGR24  
    pixformat= SDL_PIXELFORMAT_BGR888;    
#elif LOAD_YUV420P  
    //IYUV: Y + U + V  (3 planes)  
    //YV12: Y + V + U  (3 planes)  
    pixformat= SDL_PIXELFORMAT_IYUV;    
#endif  
  
    SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer,pixformat, SDL_TEXTUREACCESS_STREAMING,pixel_w,pixel_h);  
  
  
  
    FILE *fp=NULL;  
#if LOAD_BGRA  
    fp=std::fopen("../test_bgra_320x180.rgb","rb+");  
#elif LOAD_RGB24  
    fp=std::fopen("../test_rgb24_320x180.rgb","rb+");  
#elif LOAD_BGR24  
    fp=std::fopen("../test_bgr24_320x180.rgb","rb+");  
#elif LOAD_YUV420P  
    fp=std::fopen("test_yuv420p_426x240.yuv","rb+");  
#endif  
    if(fp==NULL){  
        std::cout<<"cannot open this file"<<std::endl;;  
        return -1;  
    }  
  
    SDL_Rect sdlRect;    
  
    SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video,NULL,NULL);  
    SDL_Event event;  
    while(1){  
        //Wait  
        SDL_WaitEvent(&event);  
        if(event.type==REFRESH_EVENT){  
            if (std::fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp) != pixel_w*pixel_h*bpp/8){  
                // Loop  
                std::fseek(fp, 0, SEEK_SET);  
                std::fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp);  
            }  
  
#if LOAD_BGRA  
            //We don't need to change Endian  
            //Because input BGRA pixel data(B|G|R|A) is same as ARGB8888 in Little Endian (B|G|R|A)  
            SDL_UpdateTexture( sdlTexture, NULL, buffer, pixel_w*4);    
#elif LOAD_RGB24|LOAD_BGR24  
            //change 24bit to 32 bit  
            //and in Windows we need to change Endian  
            CONVERT_24to32(buffer,buffer_convert,pixel_w,pixel_h);  
            SDL_UpdateTexture( sdlTexture, NULL, buffer_convert, pixel_w*4);    
#elif LOAD_YUV420P  
            SDL_UpdateTexture( sdlTexture, NULL, buffer, pixel_w);    
#endif  
            //FIX: If window is resize  
            sdlRect.x = 0;    
            sdlRect.y = 0;    
            sdlRect.w = screen_w;    
            sdlRect.h = screen_h;    
              
            SDL_RenderClear( sdlRenderer );     
            SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);    
            SDL_RenderPresent( sdlRenderer );    
            //Delay 40ms  
            SDL_Delay(40);  
              
        }else if(event.type==SDL_WINDOWEVENT){  
            //If Resize  
            SDL_GetWindowSize(screen,&screen_w,&screen_h);  
        }else if(event.type==SDL_QUIT){  
            break;  
        }  
    }  
  
    return 0;  
}
