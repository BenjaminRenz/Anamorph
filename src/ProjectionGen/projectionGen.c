#include <float.h>
#include <math.h>
#include <complex.h>
#define M_PI 3.14159265358979323846
#include <stdint.h>
#include "debug/debug.h"
#include "gui/guiUserDefined.h"
#include "ProjectionGen/projectionGen.h"    //For enum for projection_modes
#include <stdlib.h> //for exit();
//Settings
#define ENDIANESS 0 //0=littleEND,1=bigEND
#define enable_colormap 1

//Internal Conversions
#define mm_per_px_256 (widthMM/(screenresx*256.0f))
#define px_256_per_mm ((screenresx*256.0f)/widthMM)
#define mm_per_px (widthMM/(float)screenresx)
#define px_per_mm (screenresx/(float)widthMM)

//Proportioons of cone in mm
#define r_cone  57.0f
#define h_cone  80.0f
#define h_cone_obs   800.0f

//Proportioons of cone in mm
#define r_cyl 37.5f
#define h_cyl 150.0f
#define h_cyl_obs 325.0f
#define r_cyl_obs (250.0f+r_cyl)

//Grid Settings in mm
#define grid_linedist 15.0f
//in mm from left bottom corner for small display map grid
#define scale_cone 1
#define scale_cyl 1

//Position of the minigrid's center in mm
#define lb_conerep_x 10.0f
#define lb_conerep_y 10.0f

//#define shift_cyl (scale_cyl*cos_atan*(h_cyl_obs*(1-r_cyl_obs/(r_cyl_obs-r_cyl))))
#define lb_cylrep_x 10.0f
#define lb_cylrep_y 10.0f
//#define center_cylrep_y (scale_cyl*cos_atan*h_cyl/2+10.0f-shift_cyl)



char sign_int(int x){
    return (x<0)?(-1):(1);
}

int max_int(int a, int b){
    return (a<b)?(b):(a);
}

int min_int(int a, int b){
    return (a<b)?(a):(b);
}

int max_float(float a, float b){
    return (a<b)?(b):(a);
}

int min_float(float a, float b){
    return (a<b)?(a):(b);
}

int sign_float(float x){
    return (x<0)?(-1):(1);
}

uint8_t mapPoints(int32_t* mapped_xpos, int32_t* mapped_ypos, int32_t touch_xpos, int32_t touch_ypos,uint32_t screenresx, uint32_t screenresy,uint32_t widthMM, uint32_t heightMM);
float cone_project_s2o(float rad_screen);
uint8_t grid_value(int32_t x_256,int32_t y_256,uint32_t gradient_256, uint32_t linewidth_px_256, uint32_t grid_center_offset_px_256, uint32_t grid_distance_px_256);
float cone_project_o2s(float rad_obs);  //Projection from the observers view of the reflection onto the screen
int cylinder_project_s2o(float* x_screenp,float* y_screenp);
int cylinder_project_o2s(float* x_screenp,float* y_screenp);
void cylinder_map_inter_to_plane(float intersect_vec[3],float* x_screenp, float* y_screenp);
void quadSampleTexture(uint8_t* TextureOutp,uint8_t* PictureInp,uint32_t TextureSidelength,float relpos_x,float relpos_y,float max_x,float min_x, float max_y, float min_y);


void smooth_hsv2rgb(uint8_t* r,uint8_t* g, uint8_t* b,float h_in_rad,float sat,float val){
    //float cos_res=(val-val*sat/2)-(val*sat/2)*cosf(3*h_in_rad);
    //todo handle cases where the angle is outside of (0-2*M_PI) range
    if(h_in_rad<M_PI/2.0f){
        *r=(uint8_t)255*val;
        *g=255*(val-val*sat);
        *b=255*(val-val*sat);
    }else if(h_in_rad<M_PI){
        *r=255*(val-val*sat);
        *g=(uint8_t)255*val;
        *b=255*(val-val*sat);
    }else if(h_in_rad<3*M_PI/2.0f){
        *r=255*(val-val*sat);
        *g=255*(val-val*sat);
        *b=(uint8_t)255*val;
    }else{
        *r=(uint8_t)255*val;
        *g=255*(val-val*sat);
        *b=(uint8_t)255*val;
    }
    /*if(h_in_rad<M_PI/3.0f){
        *r=255*val;
        *g=255*cos_res;
        *b=255*(val-val*sat);
    }else if(h_in_rad<M_PI*2/3.0f){
        *r=255*cos_res;
        *g=255*val;
        *b=255*(val-val*sat);
    }else if(h_in_rad<M_PI){
        *r=255*(val-val*sat);
        *g=255*val;
        *b=255*cos_res;
    }else if(h_in_rad<M_PI*4/3.0f){
        *r=255*(val-val*sat);
        *g=255*cos_res;
        *b=255*val;
    }else if(h_in_rad<M_PI*5/3.0f){
        *r=255*cos_res;
        *g=255*(val-val*sat);
        *b=255*val;
    }else{
        *r=255*val;
        *g=255*(val-val*sat);
        *b=255*cos_res;
    }*/
    return;
}

uint8_t mapPoints(int32_t* mapped_xpos, int32_t* mapped_ypos, int32_t touch_xpos, int32_t touch_ypos,uint32_t screenresx, uint32_t screenresy,uint32_t widthMM, uint32_t heightMM){
    if(GlobalAppState.switch_cone_cyl==switch_cone_cyl_is_cone){
        dprintf(DBGT_INFO,"%d,%d",touch_xpos,touch_ypos);
        static float outerCone_rad=UINT32_MAX;
        if(outerCone_rad==UINT32_MAX){
            outerCone_rad=h_cone*px_per_mm*tanf(2*atanf(r_cone/h_cone));
        }
        int32_t mini_proj_size_center_x=(int32_t)(floor(lb_conerep_x*px_per_mm)+scale_cone*r_cone*px_per_mm);
        int32_t mini_proj_size_center_y=(int32_t)(floor(lb_conerep_y*px_per_mm)+scale_cone*r_cone*px_per_mm);
        uint32_t large_pix_for_rad=(uint32_t)floor(r_cone*scale_cone*px_per_mm);
        int32_t mini_proj_pix_x=(int32_t)touch_xpos-mini_proj_size_center_x;
        int32_t mini_proj_pix_y=(int32_t)touch_ypos-mini_proj_size_center_y;
        float mini_proj_radq=mini_proj_pix_x*mini_proj_pix_x+mini_proj_pix_y*mini_proj_pix_y;
        int32_t large_proj_pix_x=(int32_t)touch_xpos-screenresx/2;
        int32_t large_proj_pix_y=(int32_t)touch_ypos-screenresy/2;
        float large_proj_radq=(float)large_proj_pix_x*large_proj_pix_x+large_proj_pix_y*large_proj_pix_y;
        dprintf(DBGT_INFO,"%f,%f",sqrtf(mini_proj_radq),sqrtf(large_proj_radq));
        if((uint32_t)mini_proj_radq<large_pix_for_rad*large_pix_for_rad&&mini_proj_radq>px_per_mm){
            //Pixel is in mini cone representation
            float mulfac=cone_project_o2s(mm_per_px*sqrtf(mini_proj_radq)/scale_cone);
            dprintf(DBGT_INFO,"mini,%f",mulfac);
            *mapped_xpos=(int32_t)(mini_proj_pix_x/scale_cone*mulfac+screenresx/2);
            *mapped_ypos=(int32_t)(mini_proj_pix_y/scale_cone*mulfac+screenresy/2);
            return 1;   //we generated a mapped point
        }else if(r_cone*px_per_mm*r_cone*px_per_mm<large_proj_radq && large_proj_radq<outerCone_rad*outerCone_rad){
            //Pixel is in the large map representation
            float mulfac=cone_project_s2o(sqrtf(large_proj_radq)*mm_per_px);
            dprintf(DBGT_INFO,"larg,%f",mulfac);
            *mapped_xpos=(int32_t)(large_proj_pix_x*scale_cone*mulfac+mini_proj_size_center_x);
            *mapped_ypos=(int32_t)(large_proj_pix_y*scale_cone*mulfac+mini_proj_size_center_y);
            return 1;   //we generated a mapped point
        }
        return 0;   //Do not draw a mapping point
    }else if(GlobalAppState.switch_cone_cyl==switch_cone_cyl_is_cyl){
        int32_t mini_proj_size_lb_x=(uint32_t)floor(lb_cylrep_x*px_per_mm);
        int32_t mini_proj_size_lb_y=(uint32_t)floor(lb_cylrep_y*px_per_mm);

        int32_t large_proj_pix_x=(int32_t)touch_xpos-screenresx/2;
        int32_t large_proj_pix_y=(int32_t)touch_ypos-screenresy/2;


        float intersect_tan_vec[3]={
            r_cyl*sqrtf(1-r_cyl*r_cyl/(r_cyl_obs*r_cyl_obs)),
            r_cyl*r_cyl/r_cyl_obs,
            h_cyl
        };
        float lowest_screen_vec2[3]={
            0.0,r_cyl,0.0
        };
        float half_x_width;
        float height_y_posit;
        float height_y_negat;
        cylinder_map_inter_to_plane(intersect_tan_vec,&half_x_width,&height_y_posit);
        cylinder_map_inter_to_plane(lowest_screen_vec2,0, &height_y_negat);
        int32_t mini_proj_pix_x=(int32_t)floor(touch_xpos-px_per_mm*(half_x_width*scale_cyl+lb_cylrep_x));        //TODO dont 500
        int32_t mini_proj_pix_y=(int32_t)floor(touch_ypos-px_per_mm*(lb_cylrep_y-height_y_negat*scale_cyl));

        float tempx=mm_per_px/scale_cyl*mini_proj_pix_x;
        float tempy=mm_per_px/scale_cyl*mini_proj_pix_y;
        dprintf(DBGT_INFO,"DBDB %f,%f",tempx,tempy);
        if(cylinder_project_o2s(&tempx,&tempy)){
            *mapped_xpos=(int32_t)floor(screenresx/2+px_per_mm*tempx);
            *mapped_ypos=(int32_t)floor(screenresy/2+px_per_mm*tempy);
            dprintf(DBGT_INFO,"COULDMAP O2S to %d,%d",*mapped_xpos,*mapped_ypos);
            return 1;
        }

        float tempx_mm=mm_per_px*large_proj_pix_x;
        //int sign_x=sign_float(tempx_mm);
        float tempy_mm=mm_per_px*large_proj_pix_y;
        if(cylinder_project_s2o(&tempx_mm,&tempy_mm)){        //check if function is able to map from large so small grid
            float tempx_px=scale_cyl*px_per_mm*tempx_mm;
            float tempy_px=scale_cyl*px_per_mm*tempy_mm;
            *mapped_xpos=(int32_t)floor(tempx_px+px_per_mm*(half_x_width*scale_cyl+lb_cylrep_x));    //TODO scale_cyl
            *mapped_ypos=(int32_t)floor(tempy_px+px_per_mm*(lb_cylrep_y-height_y_negat*scale_cyl)); //TODO subtract offset from center of minigrid
            return 1;
        }
        return 0;   //Do not map point
    }
    dprintf(DBGT_ERROR,"Neither cyl nor cone mapping active");
    return 0;
}


void mixPixel(uint8_t* TextureDatap,uint32_t OffsetInTex,uint32_t rgba){
    uint8_t r=(rgba&(0xff<<(8*3)))>>(8*3);
    uint8_t g=(rgba&(0xff<<(8*2)))>>(8*2);
    uint8_t b=(rgba&(0xff<<(8*1)))>>(8*1);
    uint8_t a=(rgba&(0xff<<(8*0)))>>(8*0);
    if(ENDIANESS){
        uint32_t old_r=TextureDatap[OffsetInTex+0];
        uint32_t old_g=TextureDatap[OffsetInTex+1];
        uint32_t old_b=TextureDatap[OffsetInTex+2];
        uint32_t old_a=TextureDatap[OffsetInTex+3];
        TextureDatap[OffsetInTex+3]=(old_a+a>255)?(255):((uint8_t)(old_a+a));
        if(old_a+a!=0){ //If source and target 100% transparent do not change color
            TextureDatap[OffsetInTex+0]=(old_a*old_r+a*r)/(old_a+a);
            TextureDatap[OffsetInTex+1]=(old_a*old_g+a*g)/(old_a+a);
            TextureDatap[OffsetInTex+2]=(old_a*old_b+a*b)/(old_a+a);
        }


    }else{
        uint32_t old_r=TextureDatap[OffsetInTex+3];
        uint32_t old_g=TextureDatap[OffsetInTex+2];
        uint32_t old_b=TextureDatap[OffsetInTex+1];
        uint32_t old_a=TextureDatap[OffsetInTex+0];
        TextureDatap[OffsetInTex+0]=(old_a+a>255)?(255):((uint8_t)(old_a+a));
        if(old_a+a!=0){ //If source and target 100% transparent do not change color
            TextureDatap[OffsetInTex+3]=(old_a*old_r+a*r)/(old_a+a);
            TextureDatap[OffsetInTex+2]=(old_a*old_g+a*g)/(old_a+a);
            TextureDatap[OffsetInTex+1]=(old_a*old_b+a*b)/(old_a+a);
        }
    }
}

void connect_pixels_solid(uint8_t* TextureDatap,uint32_t TextureSidelength,uint32_t rgba, uint32_t p1x, uint32_t p1y, uint32_t p2x, uint32_t p2y){
    uint8_t r=(rgba&(0xff<<(8*3)))>>(8*3);
    uint8_t g=(rgba&(0xff<<(8*2)))>>(8*2);
    uint8_t b=(rgba&(0xff<<(8*1)))>>(8*1);

    uint8_t swapxy=0;
    int32_t deltay=(int32_t)p2y-(int32_t)p1y;
    int32_t deltax=(int32_t)p2x-(int32_t)p1x;
    if(abs(deltay)>abs(deltax)){
        swapxy=1;
        int32_t temp=deltax;
        deltax=deltay;
        deltay=temp;
    }
    //deltay/deltax
    for(int32_t rel_x=0;abs(rel_x)<abs(deltax);rel_x+=deltax/abs(deltax)){
        int32_t rel_x_numerator=rel_x*deltay;
        int32_t rel_y_lowerpix=rel_x_numerator/deltax;
        uint32_t OffsetInTex1;
        uint32_t OffsetInTex2;
        if(swapxy){
            OffsetInTex1=4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x));
            if((deltax*deltay>=0)==((deltax<0)&&(deltay==0))){
                OffsetInTex2=4*(p1x+sign_int(deltay)+rel_y_lowerpix+TextureSidelength*(p1y+rel_x));
            }else{
                OffsetInTex2=4*(p1x-sign_int(deltay)+rel_y_lowerpix+TextureSidelength*(p1y+rel_x));
            }
        }else{          //TODO sign function instead of abs(x)/x
            OffsetInTex1=4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix));
            if((deltax*deltay>=0)!=((deltax<0)&&(deltay==0))){
                OffsetInTex2=4*(p1x+rel_x+TextureSidelength*(p1y+sign_int(deltay)+rel_y_lowerpix));
            }else{
                OffsetInTex2=4*(p1x+rel_x+TextureSidelength*(p1y-sign_int(deltay)+rel_y_lowerpix));
            }
        }
        if(ENDIANESS){
            TextureDatap[OffsetInTex1+0]=r;
            TextureDatap[OffsetInTex1+1]=g;
            TextureDatap[OffsetInTex1+2]=b;
            TextureDatap[OffsetInTex1+3]=255;
            TextureDatap[OffsetInTex2+0]=r;
            TextureDatap[OffsetInTex2+1]=g;
            TextureDatap[OffsetInTex2+2]=b;
            TextureDatap[OffsetInTex2+3]=255;
        }else{
            TextureDatap[OffsetInTex1+3]=r;
            TextureDatap[OffsetInTex1+2]=g;
            TextureDatap[OffsetInTex1+1]=b;
            TextureDatap[OffsetInTex1+0]=255;
            TextureDatap[OffsetInTex2+3]=r;
            TextureDatap[OffsetInTex2+2]=g;
            TextureDatap[OffsetInTex2+1]=b;
            TextureDatap[OffsetInTex2+0]=255;
        }

    }
}

void connect_pixels_aliased_oriented(uint8_t* TextureDatap,uint32_t TextureSidelength,uint32_t rgba, uint32_t p1x, uint32_t p1y, uint32_t p2x, uint32_t p2y){
    uint8_t r=(rgba&(0xff<<(8*3)))>>(8*3);
    uint8_t g=(rgba&(0xff<<(8*2)))>>(8*2);
    uint8_t b=(rgba&(0xff<<(8*1)))>>(8*1);
    uint8_t a=(rgba&(0xff<<(8*0)))>>(8*0);

    //TODO handle same point for p1 p2
    uint8_t swapxy=0;
    int32_t deltay=(int32_t)p2y-(int32_t)p1y;
    int32_t deltax=(int32_t)p2x-(int32_t)p1x;
    if(abs(deltay)>abs(deltax)){
        swapxy=1;
        int32_t temp=deltax;
        deltax=deltay;
        deltay=temp;
    }
    //deltay/deltax
    for(int32_t rel_x=0;abs(rel_x)<abs(deltax);rel_x+=sign_int(deltax)){
        int32_t rel_x_numerator=rel_x*deltay;
        int32_t rel_y_lowerpix=rel_x_numerator/deltax;
        int64_t rel_y_256_mod=(((int64_t)abs(rel_x_numerator)*256)/abs(deltax))%256;
        if(swapxy){
            if(ENDIANESS){
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+0]=r;
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+1]=g;
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+2]=b;
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+3]=255;
            }else{
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+0]=255;
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+1]=b;
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+2]=g;
                TextureDatap[4*(p1x+rel_y_lowerpix+TextureSidelength*(p1y+rel_x))+3]=r;
            }
            if((deltax*deltay>=0)==((deltax<0)&&(deltay==0))){
                mixPixel(TextureDatap,4*(p1x+sign_int(deltay)+rel_y_lowerpix+TextureSidelength*(p1y+rel_x)),(rgba&0xffffff00)|rel_y_256_mod);
            }else{
                mixPixel(TextureDatap,4*(p1x-sign_int(deltay)+rel_y_lowerpix+TextureSidelength*(p1y+rel_x)),(rgba&0xffffff00)|(255-rel_y_256_mod));
            }
            //mixPixel(TextureDatap,4*(p1x+sign_int(deltay)+rel_y_lowerpix+TextureSidelength*(p1y+rel_x)),r,g,b,rel_y_256_mod);
        }else{
            if(ENDIANESS){
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+0]=r;
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+1]=g;
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+2]=b;
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+3]=255;
            }else{
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+0]=255;
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+1]=b;
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+2]=g;
                TextureDatap[4*(p1x+rel_x+TextureSidelength*(p1y+rel_y_lowerpix))+3]=r;
            }
            if((deltax*deltay>=0)!=((deltax<0)&&(deltay==0))){
                mixPixel(TextureDatap,4*(p1x+rel_x+TextureSidelength*(p1y+sign_int(deltay)+rel_y_lowerpix)),(rgba&0xffffff00)|rel_y_256_mod);
            }else{
                mixPixel(TextureDatap,4*(p1x+rel_x+TextureSidelength*(p1y-sign_int(deltay)+rel_y_lowerpix)),(rgba&0xffffff00)|(255-rel_y_256_mod));    //pfusch 255-
            }

        }

    }

}

float angle_map_zero_to_twopie(float angle){
    int numpy=(int)floor(angle/(2*M_PI));
    return numpy*2*M_PI+angle;
}

void connect_points(uint8_t* TextureDatap,uint32_t TextureSidelength,uint32_t rgba,int32_t p1x,int32_t p1y,int32_t r1,int32_t p2x,int32_t p2y,int32_t r2){
    int32_t deltax=p2x-p1x;
    int32_t deltay=p2y-p1y;

    if((!deltax)&&(!deltay)){
        return;
    }

    if((p1x-r1)<1||(p1y+r1)>(TextureSidelength-1)||(p2x-r2)<1||(p2y+r2)>(TextureSidelength-1)){
        return;
    }

    int32_t deltaLen=sqrt(deltax*deltax+deltay*deltay);
    float phi=atan2f(deltay,deltax);    //angle between vector of midpoints and horizontal
    float alpha=asinf((r2-r1)/(float)deltaLen);
    float angle_top_res=angle_map_zero_to_twopie(alpha+phi);
    float angle_bottom_res=angle_map_zero_to_twopie(phi-alpha);
    int32_t rel_top_1_x=(int32_t)sign_float(-sinf(angle_top_res))*floor(abs(r1*sinf(angle_top_res)));
    int32_t rel_top_1_y=(int32_t)sign_float(cosf(angle_top_res))*floor(abs(r1*cosf(angle_top_res)));
    int32_t rel_top_2_x=(int32_t)sign_float(-sinf(angle_top_res))*floor(abs(r2*sinf(angle_top_res)));
    int32_t rel_top_2_y=(int32_t)sign_float(cosf(angle_top_res))*floor(abs(r2*cosf(angle_top_res)));
    int32_t rel_bottom_1_x=(int32_t)sign_float(sinf(angle_bottom_res))*floor(abs(r1*sinf(angle_bottom_res)));
    int32_t rel_bottom_1_y=(int32_t)sign_float(-cosf(angle_bottom_res))*floor(abs(r1*cosf(angle_bottom_res)));
    int32_t rel_bottom_2_x=(int32_t)sign_float(sinf(angle_bottom_res))*floor(abs(r2*sinf(angle_bottom_res)));
    int32_t rel_bottom_2_y=(int32_t)sign_float(-cosf(angle_bottom_res))*floor(abs(r2*cosf(angle_bottom_res)));


    //draw rest of lines
    uint8_t swap_1_xy=0;
    uint8_t swap_2_xy=0;
    int32_t delta1x;
    int32_t delta1y;
    int32_t delta2x;
    int32_t delta2y;
    if(abs(rel_top_1_y)>abs(rel_top_1_x)){
        swap_1_xy=1;
        delta1x=rel_top_1_y;
        delta1y=rel_top_1_x;
    }else{
        delta1x=rel_top_1_x;
        delta1y=rel_top_1_y;
    }
    if(abs(rel_top_2_y)>abs(rel_top_2_x)){
        swap_2_xy=1;
        delta2x=rel_top_2_y;
        delta2y=rel_top_2_x;
    }else{
        delta2x=rel_top_2_x;
        delta2y=rel_top_2_y;
    }



    int32_t i=0;    //Iterate over pixels of line of point1
    int32_t j=0;    //Iterate over pixels of line of point2
    uint8_t max_rad1_reached_flag=0;
    uint8_t max_rad2_reached_flag=0;
    while((!max_rad1_reached_flag)||(!max_rad2_reached_flag)){
        if(i==abs(rel_top_1_x*(1-swap_1_xy)+swap_1_xy*rel_top_1_y)-1){
             max_rad1_reached_flag=1;
        }
        if(j==abs(rel_top_2_x*(1-swap_2_xy)+swap_2_xy*rel_top_2_y)-1){
            max_rad2_reached_flag=1;
        }
        int32_t rel1_x=sign_int(delta1x)*i;
        int32_t rel1_y=(delta1y*sign_int(delta1x)*i)/delta1x;
        int32_t rel2_x=sign_int(delta2x)*j;
        int32_t rel2_y=(delta2y*sign_int(delta2x)*j)/delta2x;
        if(swap_1_xy){
            int32_t tmp=rel1_x;
            rel1_x=rel1_y;
            rel1_y=tmp;
        }
        if(swap_2_xy){
            int32_t tmp=rel2_x;
            rel2_x=rel2_y;
            rel2_y=tmp;
        }
        if(max_rad1_reached_flag||max_rad2_reached_flag){
            connect_pixels_solid(TextureDatap,TextureSidelength,rgba,p1x+rel1_x,p1y+rel1_y,p2x+rel2_x,p2y+rel2_y);
        }else{
            connect_pixels_solid(TextureDatap,TextureSidelength,rgba,p1x+rel1_x,p1y+rel1_y,p2x+rel2_x,p2y+rel2_y);
        }

        if(!max_rad1_reached_flag){
            i++;
        }
        if(!max_rad2_reached_flag){
            j++;
        }
    }
    //Draw top aliased line
    //something is wrong here
    //connect_pixels_aliased_oriented(TextureDatap,TextureSidelength,r,g,b,p1x+rel_top_1_x-sign_int(rel_top_1_x),p1y+rel_top_1_y-sign_int(rel_top_1_y),p2x+rel_top_2_x-sign_int(rel_top_2_x),p2y+rel_top_2_y-sign_int(rel_top_2_y));
    connect_pixels_aliased_oriented(TextureDatap,TextureSidelength,rgba,p1x+rel_top_1_x,p1y+rel_top_1_y,p2x+rel_top_2_x,p2y+rel_top_2_y);
    /*if((delta1x*delta1y<=0)!=swap_1_xy){
        connect_pixels_aliased_oriented(TextureDatap,TextureSidelength,r,g,b,p1x+rel_top_1_x,p1y+rel_top_1_y,p2x+rel_top_2_x,p2y+rel_top_2_y);
    }else{
        connect_pixels_aliased_oriented(TextureDatap,TextureSidelength,r,g,b,p2x+rel_top_2_x,p2y+rel_top_2_y,p1x+rel_top_1_x,p1y+rel_top_1_y);
    }*/


    i=0;    //Don't draw center line in this run
    j=0;
    max_rad1_reached_flag=0;
    max_rad2_reached_flag=0;
    if(abs(rel_bottom_1_y)>abs(rel_bottom_1_x)){
        swap_1_xy=1;
        delta1x=rel_bottom_1_y;
        delta1y=rel_bottom_1_x;
    }else{
        delta1x=rel_bottom_1_x;
        delta1y=rel_bottom_1_y;
    }
    if(abs(rel_bottom_2_y)>abs(rel_bottom_2_x)){
        swap_2_xy=1;
        delta2x=rel_bottom_2_y;
        delta2y=rel_bottom_2_x;
    }else{
        delta2x=rel_bottom_2_x;
        delta2y=rel_bottom_2_y;
    }
    while((!max_rad1_reached_flag)||(!max_rad2_reached_flag)){
        if(i==abs(rel_bottom_1_x*(1-swap_1_xy)+swap_1_xy*rel_bottom_1_y)-1){
             max_rad1_reached_flag=1;
        }
        if(j==abs(rel_bottom_2_x*(1-swap_2_xy)+swap_2_xy*rel_bottom_2_y)-1){
            max_rad2_reached_flag=1;
        }
        int32_t rel1_x=sign_int(delta1x)*i;
        int32_t rel1_y=(delta1y*sign_int(delta1x)*i)/delta1x;
        int32_t rel2_x=sign_int(delta2x)*j;
        int32_t rel2_y=(delta2y*sign_int(delta2x)*j)/delta2x;
        if(swap_1_xy){
            int32_t tmp=rel1_x;
            rel1_x=rel1_y;
            rel1_y=tmp;
        }
        if(swap_2_xy){
            int32_t tmp=rel2_x;
            rel2_x=rel2_y;
            rel2_y=tmp;
        }
        if(max_rad1_reached_flag||max_rad2_reached_flag){
            connect_pixels_solid(TextureDatap,TextureSidelength,rgba,p2x+rel2_x,p2y+rel2_y,p1x+rel1_x,p1y+rel1_y);
        }else{
            connect_pixels_solid(TextureDatap,TextureSidelength,rgba,p2x+rel2_x,p2y+rel2_y,p1x+rel1_x,p1y+rel1_y);
        }
        if(!max_rad1_reached_flag){
            i++;
        }
        if(!max_rad2_reached_flag){
            j++;
        }
    }


    connect_pixels_aliased_oriented(TextureDatap,TextureSidelength,rgba,p2x+rel_bottom_2_x,p2y+rel_bottom_2_y,p1x+rel_bottom_1_x,p1y+rel_bottom_1_y);
/*
    if((delta1x*delta1y>=0)==swap_1_xy){
        connect_pixels_aliased_oriented(TextureDatap,TextureSidelength,255,255,0,p2x+rel_bottom_2_x,p2y+rel_bottom_2_y,p1x+rel_bottom_1_x,p1y+rel_bottom_1_y);
    }else{
        connect_pixels_aliased_oriented(TextureDatap,TextureSidelength,0,255,0,p1x+rel_bottom_1_x,p1y+rel_bottom_1_y,p2x+rel_bottom_2_x,p2y+rel_bottom_2_y);
    }
*/

}





uint8_t grid_value(int32_t x_256,int32_t y_256,uint32_t gradient_256, uint32_t linewidth_px_256, uint32_t grid_center_offset_px_256, uint32_t grid_distance_px_256){
    uint8_t white=0;
    uint32_t tempval_512=(uint32_t)abs(2*(int32_t)((abs(x_256)-(int32_t)grid_center_offset_px_256)%grid_distance_px_256)-(int32_t)grid_distance_px_256); //512 because grid_distance might be odd, so no loss in precision
    if(tempval_512<grid_distance_px_256-(linewidth_px_256+2*gradient_256)){
        white=255;
    }else if(tempval_512>grid_distance_px_256-linewidth_px_256){
        return 0;
    }else{
        //dprintf(DBGT_INFO,"%d,%d",grid_distance_px_256,tempval_512);
        white=255-(uint8_t)(((tempval_512-(grid_distance_px_256-(linewidth_px_256+2*gradient_256)))*255)/(2*gradient_256));
    }

    tempval_512=(uint32_t)abs(2*(int32_t)((abs(y_256)-(int32_t)grid_center_offset_px_256)%grid_distance_px_256)-(int32_t)grid_distance_px_256); //512 because grid_distance might be odd, so no loss in precision
    if(tempval_512<grid_distance_px_256-(linewidth_px_256+2*gradient_256)){
        //already darker (do nothing), or white -> do nothing
    }else if(tempval_512>grid_distance_px_256-linewidth_px_256){
        return 0;
    }else{
        uint8_t white_tmp=255-(uint8_t)(((tempval_512-(grid_distance_px_256-(linewidth_px_256+2*gradient_256)))*255)/(2*gradient_256));
        if(white<white_tmp){
            return white;
        }else{
            return white_tmp;
        }
    }
    return white;
}

void projection_gen_largeConeGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM){
    if(mode==projection_gen_mode_grid_pic){
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"In Pic mode, but no valid picture pointer");
            exit(1);
        }
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"Picturesidelength 0");
            exit(1);
        }
    }

    for(uint32_t y=TextureSidelength-screenresy;y<TextureSidelength;y++){
        for(uint32_t x=0;x<screenresx;x++){
            //Transform to mm coordinateSystem
            float yproj_mm=((y-(TextureSidelength-screenresy))-(float)screenresy/2)*mm_per_px;
            float xproj_mm=(x-(float)screenresx/2)*mm_per_px;
            float mulfac=cone_project_s2o(sqrt(xproj_mm*xproj_mm+yproj_mm*yproj_mm));
            if(mulfac<0.0f||r_cone*r_cone>xproj_mm*xproj_mm+yproj_mm*yproj_mm){
                for(uint32_t rgba_index=0;rgba_index<4;rgba_index++){
                    TextureData[(x+y*TextureSidelength)*4+rgba_index]=0xff;
                }
                continue;
            }
            switch(mode){
                case projection_gen_mode_grid_white:{
                    uint8_t white=grid_value((int32_t)(mulfac*xproj_mm*px_256_per_mm),(int32_t)(mulfac*yproj_mm*px_256_per_mm),256*1,256*2,0,(uint32_t)(px_256_per_mm*grid_linedist));
                    float fwhite=1-(1-white/255.0f)/4.0f;
                    if(ENDIANESS){
                        TextureData[(x+y*TextureSidelength)*4+0]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength)*4+1]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength)*4+2]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength)*4+3]=0xff;    //No Trensparency
                    }else{
                        TextureData[(x+y*TextureSidelength)*4+0]=0xff;    //No Trensparency
                        TextureData[(x+y*TextureSidelength)*4+1]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength)*4+2]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength)*4+3]=(uint8_t)(fwhite*255);
                    }
                }break;
                case projection_gen_mode_grid_color:{
                    uint8_t white=grid_value((int32_t)(mulfac*xproj_mm*px_256_per_mm),(int32_t)(mulfac*yproj_mm*px_256_per_mm),256*1,256*2,0,(uint32_t)(px_256_per_mm*grid_linedist));
                    float fwhite=1-(1-white/255.0f)/4.0f;
                    uint8_t r=0;
                    uint8_t g=0;
                    uint8_t b=0;
                    smooth_hsv2rgb(&r,&b,&g,atan2f(yproj_mm,xproj_mm)+M_PI,0.5f*mulfac*sqrt(xproj_mm*xproj_mm+yproj_mm*yproj_mm)/r_cone,fwhite);
                    if(ENDIANESS){
                        TextureData[(x+y*TextureSidelength)*4+0]=r;
                        TextureData[(x+y*TextureSidelength)*4+1]=g;
                        TextureData[(x+y*TextureSidelength)*4+2]=b;
                        TextureData[(x+y*TextureSidelength)*4+3]=0xff;    //No Trensparency
                    }else{
                        TextureData[(x+y*TextureSidelength)*4+0]=0xff;    //No Trensparency
                        TextureData[(x+y*TextureSidelength)*4+1]=b;
                        TextureData[(x+y*TextureSidelength)*4+2]=g;
                        TextureData[(x+y*TextureSidelength)*4+3]=r;
                    }
                }break;
                case projection_gen_mode_grid_pic:
                    quadSampleTexture(&(TextureData[4*(x+y*TextureSidelength)]),optionalPicDataP,optionalPicSidelength,mulfac*xproj_mm,mulfac*yproj_mm,r_cone,-r_cone,r_cone,-r_cone);
                break;
            }
        }
    }
}

void projection_gen_largeCylGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM){
    if(mode==projection_gen_mode_grid_pic){
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"In Pic mode, but no valid picture pointer");
            exit(1);
        }
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"Picturesidelength 0");
            exit(1);
        }
    }
    #define line_tickness 1     //inner black border
    float intersect_tan_vec[3]={
        r_cyl*sqrtf(1-r_cyl*r_cyl/(r_cyl_obs*r_cyl_obs)),
        r_cyl*r_cyl/r_cyl_obs,
        h_cyl
    };
    float lowest_screen_vec2[3]={
        0.0,r_cyl,0.0
    };
    float half_x_width;
    float height_y_posit;
    float height_y_negat;
    cylinder_map_inter_to_plane(intersect_tan_vec,&half_x_width,&height_y_posit);
    cylinder_map_inter_to_plane(lowest_screen_vec2,0, &height_y_negat);


    for(uint32_t y=TextureSidelength-screenresy;y<TextureSidelength;y++){
        for(uint32_t x=0;x<screenresx;x++){
            //Transform to mm coordinateSystem
            float xproj_mm=(x-(float)screenresx/2)*mm_per_px;
            float yproj_mm=((y-(TextureSidelength-screenresy))-(float)screenresy/2)*mm_per_px;

            int returntest=cylinder_project_s2o(&xproj_mm,&yproj_mm);
            if(returntest==0){
                if(ENDIANESS){
                    TextureData[(x+y*TextureSidelength)*4+0]=0xff;
                    TextureData[(x+y*TextureSidelength)*4+1]=0xff;
                    TextureData[(x+y*TextureSidelength)*4+2]=0xff;
                    TextureData[(x+y*TextureSidelength)*4+3]=0xff;    //No Trensparency
                }else{
                    TextureData[(x+y*TextureSidelength)*4+0]=0xff;    //No Trensparency
                    TextureData[(x+y*TextureSidelength)*4+1]=0xff;
                    TextureData[(x+y*TextureSidelength)*4+2]=0xff;
                    TextureData[(x+y*TextureSidelength)*4+3]=0xff;
                }
                continue;   //Do not paint over grid
            }else if(returntest==2){
                TextureData[(x+y*TextureSidelength)*4+0]=0xff;    //No Trensparency
                TextureData[(x+y*TextureSidelength)*4+1]=0xff;
                TextureData[(x+y*TextureSidelength)*4+2]=0x00;
                TextureData[(x+y*TextureSidelength)*4+3]=0xff;
                continue;
            }

            switch(mode){
            case projection_gen_mode_grid_white:{
                uint8_t white=grid_value((int32_t)(xproj_mm*px_256_per_mm),(int32_t)(yproj_mm*px_256_per_mm),256*1,256*2,0,(uint32_t)(px_256_per_mm*grid_linedist));
                float fwhite=1-(1-white/255.0f)/4.0f;
                if(ENDIANESS){
                    TextureData[(x+y*TextureSidelength)*4+0]=(uint8_t)(fwhite*255);
                    TextureData[(x+y*TextureSidelength)*4+1]=(uint8_t)(fwhite*255);
                    TextureData[(x+y*TextureSidelength)*4+2]=(uint8_t)(fwhite*255);
                    TextureData[(x+y*TextureSidelength)*4+3]=0xff;    //No Trensparency
                }else{
                    TextureData[(x+y*TextureSidelength)*4+0]=0xff;    //No Trensparency
                    TextureData[(x+y*TextureSidelength)*4+1]=(uint8_t)(fwhite*255);
                    TextureData[(x+y*TextureSidelength)*4+2]=(uint8_t)(fwhite*255);
                    TextureData[(x+y*TextureSidelength)*4+3]=(uint8_t)(fwhite*255);
                }
            }break;
            case projection_gen_mode_grid_color:{
                uint8_t white=grid_value((int32_t)(xproj_mm*px_256_per_mm),(int32_t)(yproj_mm*px_256_per_mm),256*1,256*2,0,(uint32_t)(px_256_per_mm*grid_linedist));
                float fwhite=1-(1-white/255.0f)/4.0f;
                uint8_t r=0;
                uint8_t g=0;
                uint8_t b=0;
                //fwhite=0;
                smooth_hsv2rgb(&r,&b,&g,atan2f(yproj_mm,xproj_mm)+M_PI,sqrt(xproj_mm*xproj_mm+yproj_mm*yproj_mm)/(sqrt(half_x_width*half_x_width+max_float(height_y_posit,-height_y_negat)*max_float(height_y_posit,-height_y_negat))),fwhite);
                if(ENDIANESS){
                    TextureData[(x+y*TextureSidelength)*4+0]=r;
                    TextureData[(x+y*TextureSidelength)*4+1]=g;
                    TextureData[(x+y*TextureSidelength)*4+2]=b;
                    TextureData[(x+y*TextureSidelength)*4+3]=0xff;    //No Trensparency
                }else{
                    TextureData[(x+y*TextureSidelength)*4+0]=0xff;    //No Trensparency
                    TextureData[(x+y*TextureSidelength)*4+1]=b;
                    TextureData[(x+y*TextureSidelength)*4+2]=g;
                    TextureData[(x+y*TextureSidelength)*4+3]=r;
                }
            }break;
            case projection_gen_mode_grid_pic:
                quadSampleTexture(&(TextureData[4*(x+y*TextureSidelength)]),optionalPicDataP,optionalPicSidelength,xproj_mm,yproj_mm,half_x_width,-half_x_width,height_y_posit,height_y_negat);
                break;
            }
        }
    }
    return;
}


void projection_gen_miniConeGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM){
    if(mode==projection_gen_mode_grid_pic){
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"In Pic mode, but no valid picture pointer");
            exit(1);
        }
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"Picturesidelength 0");
            exit(1);
        }
    }
    int32_t pixelsForRadius=(uint32_t)floor(r_cone*px_per_mm);
    uint32_t pixelsLbConerep_x=(uint32_t)floor(lb_conerep_x*px_per_mm);
    uint32_t pixelsLbConerep_y=(uint32_t)floor(lb_conerep_y*px_per_mm);
    uint32_t OffsetInText=(pixelsLbConerep_x+(int32_t)floor(scale_cone*pixelsForRadius)+TextureSidelength*((TextureSidelength-screenresy)+pixelsLbConerep_y+(int32_t)floor(scale_cone*pixelsForRadius)));
    for(int32_t y=-(int32_t)floor(scale_cone*pixelsForRadius);y<=(int32_t)floor(scale_cone*pixelsForRadius);y++){      //KEEP AS INT !!!
        for(int32_t x=-(int32_t)floor(scale_cone*pixelsForRadius);x<=(int32_t)floor(scale_cone*pixelsForRadius);x++){
            //dprintf(DBGT_ERROR,"%d,%d,%d,%d",x,y,OffsetInText,TextureSidelength);
            float xproj_mm=mm_per_px*x/scale_cone;
            float yproj_mm=mm_per_px*y/scale_cone;
            if(xproj_mm*xproj_mm+yproj_mm*yproj_mm>r_cone*r_cone){
                continue;   //Do not paint out of circle
            }

            switch(mode){
                case projection_gen_mode_grid_white:{
                    uint8_t white=grid_value(x*256,y*256,256*2,256*2,0,(uint32_t)(px_256_per_mm*scale_cone*grid_linedist));
                    float fwhite=1-(1-white/255.0f)/4.0f;
                    if(ENDIANESS){
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=0xff;    //No Trensparency
                    }else{
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=0xff;    //No Trensparency
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=(uint8_t)(fwhite*255);
                    }
                }break;
                case projection_gen_mode_grid_color:{
                    uint8_t white=grid_value(x*256,y*256,256*2,256*2,0,(uint32_t)(px_256_per_mm*scale_cone*grid_linedist));
                    float fwhite=1-(1-white/255.0f)/4.0f;
                    uint8_t r=0;
                    uint8_t g=0;
                    uint8_t b=0;
                    smooth_hsv2rgb(&r,&b,&g,atan2f(yproj_mm,xproj_mm)+M_PI,sqrtf(xproj_mm*xproj_mm+yproj_mm*yproj_mm)/r_cone,fwhite);
                    if(ENDIANESS){
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=r;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=g;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=b;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=0xff;    //No Trensparency
                    }else{
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=0xff;    //No Trensparency
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=b;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=g;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=r;
                    }
                }break;
                case projection_gen_mode_grid_pic:
                    quadSampleTexture(&(TextureData[4*(x+y*TextureSidelength+OffsetInText)]),optionalPicDataP,optionalPicSidelength,xproj_mm,yproj_mm,r_cone,-r_cone,r_cone,-r_cone);
                break;
            }
        }
    }
}

void projection_gen_miniCylGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM){
    if(mode==projection_gen_mode_grid_pic){
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"In Pic mode, but no valid picture pointer");
            exit(1);
        }
        if(!optionalPicDataP){
            dprintf(DBGT_ERROR,"Picturesidelength 0");
            exit(1);
        }
    }
    int32_t pixelsForLB_x=(int32_t)floor(lb_cylrep_x*px_per_mm);
    int32_t pixelsForLB_y=(int32_t)floor(lb_cylrep_y*px_per_mm);
    //Find border of perspective projection
    float intersect_tan_vec[3]={
        r_cyl*sqrtf(1-r_cyl*r_cyl/(r_cyl_obs*r_cyl_obs)),
        r_cyl*r_cyl/r_cyl_obs,
        h_cyl
    };
    float lowest_screen_vec2[3]={
        0.0,r_cyl,0.0
    };
    float half_x_width;
    float height_y_posit;
    float height_y_negat;
    cylinder_map_inter_to_plane(intersect_tan_vec,&half_x_width,&height_y_posit);
    cylinder_map_inter_to_plane(lowest_screen_vec2,0, &height_y_negat);


    /*float DT_distance_obs_intersect=sqrtf(intersect_tan_vec[0]*intersect_tan_vec[0]+(intersect_tan_vec[1]-r_cyl_obs)*(intersect_tan_vec[1]-r_cyl_obs));
    float DT_distance_screen_intersect=h_cyl/(h_cyl_obs-h_cyl)*DT_distance_obs_intersect;
    float tan_on_screen_vec2[2]={
        (DT_distance_obs_intersect+DT_distance_screen_intersect)*r_cyl/r_cyl_obs,       //+1.0mm is so that the project function still projects
        (DT_distance_obs_intersect+DT_distance_screen_intersect)*sqrtf(1-r_cyl*r_cyl/(r_cyl_obs*r_cyl_obs))-r_cyl_obs
    };

    if(!cylinder_project_s2o(&(tan_on_screen_vec2[0]),&(tan_on_screen_vec2[1]))){
        exit(42);
    }
    float lowest_screen_vec2[2]={
        0,-r_cyl+0.001f
    };
    if(!cylinder_project_s2o(&(lowest_screen_vec2[0]),&(lowest_screen_vec2[1]))){
        //exit(43);
    }*/
    int32_t OffsetInText=pixelsForLB_x+(int32_t)floor(px_per_mm*scale_cyl*half_x_width)+TextureSidelength*((TextureSidelength-screenresy)+pixelsForLB_y+(int32_t)floor(-px_per_mm*scale_cyl*height_y_negat));
    for(int32_t y=(int32_t)floor(px_per_mm*scale_cyl*height_y_negat);y<=(int32_t)floor(px_per_mm*scale_cyl*height_y_posit);y++){      //KEEP AS INT !!!           //TODO dont hardcode offset -0  ...
        for(int32_t x=(int32_t)floor(-px_per_mm*scale_cyl*half_x_width);x<=(int32_t)floor(px_per_mm*scale_cyl*half_x_width);x++){
            float xproj_mm=x*mm_per_px/scale_cyl;
            float yproj_mm=y*mm_per_px/scale_cyl;
            float tempx=xproj_mm;       //Make a copy so we can later use xproj_mm in quadsample without it beeing changed by cylinder_proj_o2s
            float tempy=yproj_mm;
            if(cylinder_project_o2s(&tempx,&tempy)){
                switch(mode){
                case projection_gen_mode_grid_white:{
                    uint8_t white=grid_value(x*256,y*256,256*2,256*2,0,(uint32_t)floor(px_256_per_mm*scale_cyl*grid_linedist));
                    float fwhite=1-(1-white/255.0f)/4.0f;
                    if(ENDIANESS){
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=0xff;    //No Trensparency
                    }else{
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=0xff;    //No Trensparency
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=(uint8_t)(fwhite*255);
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=(uint8_t)(fwhite*255);
                    }
                }break;
                case projection_gen_mode_grid_color:{
                    uint8_t white=grid_value(x*256,y*256,256*2,256*2,0,(uint32_t)floor(px_256_per_mm*scale_cyl*grid_linedist));
                    float fwhite=1-(1-white/255.0f)/4.0f;
                    uint8_t r=0;
                    uint8_t g=0;
                    uint8_t b=0;
                    //fwhite=0;
                    smooth_hsv2rgb(&r,&b,&g,atan2f(yproj_mm,xproj_mm)+M_PI,sqrt(xproj_mm*xproj_mm+yproj_mm*yproj_mm)/(sqrt(half_x_width*half_x_width+max_float(height_y_posit,-height_y_negat)*max_float(height_y_posit,-height_y_negat))),fwhite);
                    if(ENDIANESS){
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=r;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=g;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=b;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=0xff;    //No Trensparency
                    }else{
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+0]=0xff;    //No Trensparency
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+1]=b;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+2]=g;
                        TextureData[(x+y*TextureSidelength+OffsetInText)*4+3]=r;
                    }
                }break;
                case projection_gen_mode_grid_pic:
                    quadSampleTexture(&(TextureData[4*(x+y*TextureSidelength+OffsetInText)]),optionalPicDataP,optionalPicSidelength,xproj_mm,yproj_mm,half_x_width,-half_x_width,height_y_posit,height_y_negat);
                    break;
                }
            }
        }
    }
}

void quadSampleTexture(uint8_t* TextureOutp,uint8_t* PictureInp,uint32_t TextureSidelength,float relpos_x,float relpos_y,float max_x,float min_x, float max_y, float min_y){
    if(relpos_x<min_x||relpos_x>max_x){
        dprintf(DBGT_ERROR,"relpos_x out of range");
        return;
    }
    if(relpos_y<min_y||relpos_y>max_y){
        dprintf(DBGT_ERROR,"relpos_y out of range");
        return;
    }
    float delta_x=max_x-min_x;
    float delta_y=max_y-min_y;
    float texpx_per_mm=0;
    int delty_larger_than_deltx=0;
    if(delta_x>delta_y){
        texpx_per_mm=(TextureSidelength-1.02f)/delta_x;     //-1 because grid is inset by 0.5f
    }else{
        texpx_per_mm=(TextureSidelength-1.02f)/delta_y;
        delty_larger_than_deltx=1;
    }
    float tmpVec[2]={
        texpx_per_mm*(relpos_x-(1-delty_larger_than_deltx)*min_x-delty_larger_than_deltx*min_y)+0.01f,
        texpx_per_mm*(relpos_y-(1-delty_larger_than_deltx)*min_x-delty_larger_than_deltx*min_y)+0.01f
    };

    int32_t lowleftPxVec[2]={(int32_t)floor(tmpVec[0]),(int32_t)floor(tmpVec[1])};
    int32_t highleftPxVec[2]={(int32_t)floor(tmpVec[0]),(int32_t)ceil(tmpVec[1])};
    int32_t lowrightPxVec[2]={(int32_t)ceil(tmpVec[0]),(int32_t)floor(tmpVec[1])};
    int32_t highrightPxVec[2]={(int32_t)ceil(tmpVec[0]),(int32_t)ceil(tmpVec[1])};
    if(lowleftPxVec[0]<0||lowleftPxVec[1]<0){
        if(isnormal(relpos_x)){
            dprintf(DBGT_ERROR,"norm",relpos_x);
        }else{
            dprintf(DBGT_INFO,"not norm");
        }
        dprintf(DBGT_ERROR,"one texture vec below 0, got %f?",relpos_x);
        return;
    }
    if(highrightPxVec[0]>(TextureSidelength-1)||highrightPxVec[1]>(TextureSidelength-1)){
        dprintf(DBGT_ERROR,"one texture vec above TextureSidelength-1");
        return;
    }
    float rightPxMag=tmpVec[0]-floor(tmpVec[0]);
    float upPxMag=tmpVec[1]-floor(tmpVec[1]);
    //Mix vertical
    uint32_t loutputcolor=0;
    loutputcolor+=(uint32_t)((1-upPxMag)*PictureInp[4*(lowleftPxVec[0]+TextureSidelength*lowleftPxVec[1])]);
    loutputcolor+=((uint32_t)((1-upPxMag)*PictureInp[4*(lowleftPxVec[0]+TextureSidelength*lowleftPxVec[1])+1]))<<8;
    loutputcolor+=((uint32_t)((1-upPxMag)*PictureInp[4*(lowleftPxVec[0]+TextureSidelength*lowleftPxVec[1])+2]))<<16;
    loutputcolor+=((uint32_t)((1-upPxMag)*PictureInp[4*(lowleftPxVec[0]+TextureSidelength*lowleftPxVec[1])+3]))<<24;
    loutputcolor+=(uint32_t)((upPxMag)*PictureInp[4*(highleftPxVec[0]+TextureSidelength*highleftPxVec[1])]);
    loutputcolor+=((uint32_t)((upPxMag)*PictureInp[4*(highleftPxVec[0]+TextureSidelength*highleftPxVec[1])+1]))<<8;
    loutputcolor+=((uint32_t)((upPxMag)*PictureInp[4*(highleftPxVec[0]+TextureSidelength*highleftPxVec[1])+2]))<<16;
    loutputcolor+=((uint32_t)((upPxMag)*PictureInp[4*(highleftPxVec[0]+TextureSidelength*highleftPxVec[1])+3]))<<24;

    uint32_t routputcolor=0;
    routputcolor+=(uint32_t)((1-upPxMag)*PictureInp[4*(lowrightPxVec[0]+TextureSidelength*lowrightPxVec[1])]);
    routputcolor+=((uint32_t)((1-upPxMag)*PictureInp[4*(lowrightPxVec[0]+TextureSidelength*lowrightPxVec[1])+1]))<<8;
    routputcolor+=((uint32_t)((1-upPxMag)*PictureInp[4*(lowrightPxVec[0]+TextureSidelength*lowrightPxVec[1])+2]))<<16;
    routputcolor+=((uint32_t)((1-upPxMag)*PictureInp[4*(lowrightPxVec[0]+TextureSidelength*lowrightPxVec[1])+3]))<<24;
    routputcolor+=(uint32_t)((upPxMag)*PictureInp[4*(highrightPxVec[0]+TextureSidelength*highrightPxVec[1])]);
    routputcolor+=((uint32_t)((upPxMag)*PictureInp[4*(highrightPxVec[0]+TextureSidelength*highrightPxVec[1])+1]))<<8;
    routputcolor+=((uint32_t)((upPxMag)*PictureInp[4*(highrightPxVec[0]+TextureSidelength*highrightPxVec[1])+2]))<<16;
    routputcolor+=((uint32_t)((upPxMag)*PictureInp[4*(highrightPxVec[0]+TextureSidelength*highrightPxVec[1])+3]))<<24;
    //Mix horizontal
    TextureOutp[0]=0;
    TextureOutp[1]=0;
    TextureOutp[2]=0;
    TextureOutp[3]=0;
    //if(ENDIANESS){
    TextureOutp[0]+=(uint8_t)((1-rightPxMag)*(loutputcolor&0xff));
    TextureOutp[1]+=((uint8_t)((1-rightPxMag)*((loutputcolor>>8)&0xff)));
    TextureOutp[2]+=((uint8_t)((1-rightPxMag)*((loutputcolor>>16)&0xff)));
    TextureOutp[3]+=((uint8_t)((1-rightPxMag)*((loutputcolor>>24)&0xff)));
    TextureOutp[0]+=(uint8_t)((rightPxMag)*(routputcolor&0xff));
    TextureOutp[1]+=((uint8_t)((rightPxMag)*((routputcolor>>8)&0xff)));
    TextureOutp[2]+=((uint8_t)((rightPxMag)*((routputcolor>>16)&0xff)));
    TextureOutp[3]+=((uint8_t)((rightPxMag)*((routputcolor>>24)&0xff)));
    return;
}

void cylinder_map_inter_to_plane(float intersect_vec[3],float* x_screenp, float* y_screenp){
    float n[3]={0.0,r_cyl_obs-r_cyl,h_cyl_obs-h_cyl/2};
    float inter_minus_p_vec[3]={intersect_vec[0],intersect_vec[1]-r_cyl_obs,intersect_vec[2]-h_cyl_obs};
    float abs_sq_n=n[0]*n[0]+n[1]*n[1]+n[2]*n[2];
    if(x_screenp){
        *x_screenp=n[0]-abs_sq_n/(n[0]*inter_minus_p_vec[0]+n[1]*inter_minus_p_vec[1]+n[2]*inter_minus_p_vec[2])*(inter_minus_p_vec[0]);
    }
    float n_scalar_inter_minus_p=(n[0]*inter_minus_p_vec[0]+n[1]*inter_minus_p_vec[1]+n[2]*inter_minus_p_vec[2]);
    float projp_minus_centerp_vec[3]={
        0,
        n[1]-abs_sq_n/n_scalar_inter_minus_p*inter_minus_p_vec[1],
        n[2]-abs_sq_n/n_scalar_inter_minus_p*inter_minus_p_vec[2]
    };
    float abs_projp_minus_p=sqrtf(projp_minus_centerp_vec[1]*projp_minus_centerp_vec[1]+projp_minus_centerp_vec[2]*projp_minus_centerp_vec[2]);

    if(y_screenp){
        *y_screenp=abs_projp_minus_p*sign_float(projp_minus_centerp_vec[2]);
        //*y_screenp=-((n[1]/abs_sq_n-1/n_scalar_inter_minus_p*inter_minus_p_vec[1])*n[2]-(n[2]/abs_sq_n-1/n_scalar_inter_minus_p*inter_minus_p_vec[2])*n[1])*n[1];
    }
    return;
}

int cylinder_project_o2s(float* x_obsp,float* y_obsp){
    int sign_x_obs=sign_int(*x_obsp);
    float abs_x_obs=sign_x_obs*(*x_obsp);
    float n_vec[3]={0.0,r_cyl-r_cyl_obs,h_cyl/2-h_cyl_obs};
    float abs_n_vec=sqrtf(n_vec[0]*n_vec[0]+n_vec[1]*n_vec[1]+n_vec[2]*n_vec[2]);
    float plane_zero_vec[3]={0.0,r_cyl,h_cyl/2};
    float point_on_plane_vec[3]={       //Might give wrong value? is 0,r_cyl,h_cyl/2
        plane_zero_vec[0]+abs_x_obs,
        plane_zero_vec[1]+(*y_obsp)*n_vec[2]/abs_n_vec,
        plane_zero_vec[2]-(*y_obsp)*n_vec[1]/abs_n_vec
    };
    float a=(point_on_plane_vec[0]/(r_cyl_obs-point_on_plane_vec[1]))*(point_on_plane_vec[0]/(r_cyl_obs-point_on_plane_vec[1]))+1.0;
    float b=-point_on_plane_vec[0]*point_on_plane_vec[0]/(r_cyl_obs-point_on_plane_vec[1])*(1+point_on_plane_vec[1]/(r_cyl_obs-point_on_plane_vec[1]));
    float c=point_on_plane_vec[0]*point_on_plane_vec[0]*(1+point_on_plane_vec[1]/(r_cyl_obs-point_on_plane_vec[1]))*(1+point_on_plane_vec[1]/(r_cyl_obs-point_on_plane_vec[1]))-r_cyl*r_cyl;
    float intersect_vec[3];
    if(((b/(2*a))*(b/(2*a))-c/a)<0){
        return 0;
    }
    intersect_vec[1]=-b/(2*a)+sqrt((b/(2*a))*(b/(2*a))-c/a);
    intersect_vec[0]=sqrtf(r_cyl*r_cyl-intersect_vec[1]*intersect_vec[1]);//(r_cyl_obs-intersect_y)*x_obs/(r_cyl_obs-y_obs);
    //TD stands for top down, so distances are only on xy-plane

    float TDintersect_obs_vec[2]={
        intersect_vec[0],
        intersect_vec[1]-r_cyl_obs
    };
    float TDdistance_intersect_obs=sqrtf(TDintersect_obs_vec[0]*TDintersect_obs_vec[0]+TDintersect_obs_vec[1]*TDintersect_obs_vec[1]);
    float TDdistance_obs_p_on_p=sqrtf(point_on_plane_vec[0]*point_on_plane_vec[0]+(point_on_plane_vec[1]-r_cyl_obs)*(point_on_plane_vec[1]-r_cyl_obs));
    intersect_vec[2]=(point_on_plane_vec[2]-h_cyl_obs)*TDdistance_intersect_obs/TDdistance_obs_p_on_p+h_cyl_obs;
    //Detect invalid results
    if(intersect_vec[2]>h_cyl||intersect_vec[2]<0){
        return 0;
    }
    float TDdistance_screen_intersect=intersect_vec[2]*TDdistance_intersect_obs/(h_cyl_obs-intersect_vec[2]);
    float TDabssq_intersect_vec=(intersect_vec[0]*intersect_vec[0]+intersect_vec[1]*intersect_vec[1]);       //used as normal
    //mirror reflected ray
    float TDintersect_screen_vec[2]={
        TDintersect_obs_vec[0]-2*intersect_vec[0]/TDabssq_intersect_vec*(intersect_vec[0]*TDintersect_obs_vec[0]+intersect_vec[1]*TDintersect_obs_vec[1]),
        TDintersect_obs_vec[1]-2*intersect_vec[1]/TDabssq_intersect_vec*(intersect_vec[0]*TDintersect_obs_vec[0]+intersect_vec[1]*TDintersect_obs_vec[1])
    };
    //scale accordingly
    TDintersect_screen_vec[0]*=TDdistance_screen_intersect/TDdistance_intersect_obs;
    TDintersect_screen_vec[1]*=TDdistance_screen_intersect/TDdistance_intersect_obs;
    //dprintf(DBGT_ERROR,"O2S results, %f,%f",intersect_vec[0],intersect_vec[2]);
    *x_obsp=sign_x_obs*(intersect_vec[0]+TDintersect_screen_vec[0]);
    *y_obsp=-(intersect_vec[1]+TDintersect_screen_vec[1]);
    return 1;
}

int cylinder_project_s2o(float* x_screenp,float* y_screenp){
    int sign=sign_float(*x_screenp);
    float abs_x=(*x_screenp)*sign;
    *y_screenp=-(*y_screenp);
    float rad_screen_sq=(abs_x*abs_x+(*y_screenp)*(*y_screenp));
    if(*y_screenp<0&&abs_x<r_cyl){
        return 0;
    }
    float e=4*r_cyl_obs*r_cyl_obs*rad_screen_sq;
    float a=-(4*r_cyl_obs*rad_screen_sq+4*r_cyl_obs*r_cyl_obs*(*y_screenp))*r_cyl/e;
    float b=((r_cyl*r_cyl-4*r_cyl_obs*r_cyl_obs)*rad_screen_sq+r_cyl_obs*r_cyl_obs*r_cyl*r_cyl+2*r_cyl_obs*r_cyl*r_cyl*(*y_screenp))/e;
    float c=(2*r_cyl_obs*r_cyl*(abs_x*abs_x+2*(*y_screenp)*(*y_screenp)+2*r_cyl_obs*(*y_screenp)))/e;
    float d=(r_cyl_obs*r_cyl_obs*abs_x*abs_x-r_cyl*r_cyl*((*y_screenp)+r_cyl_obs)*((*y_screenp)+r_cyl_obs))/e;
    float complex qu_1=2*b*b*b-9*a*b*c+27*c*c+27*a*a*d-72*b*d;
    float complex qu_2=b*b-3*a*c+12*d;
    float complex qu_3=qu_1+csqrtf(-4*qu_2*qu_2*qu_2+qu_1*qu_1);
    float complex qu_4=cpowf(qu_3,1/3.0f);
    float complex qu_5=csqrtf(a*a/4-2*b/3+cpowf(2.0f,1/3.0f)*qu_2/3/qu_4+qu_4/cpowf(54.0f,1/3.0f));
    float complex qu_6_m=csqrtf(a*a/2-4*b/3-cpowf(2.0f,1/3.0f)*qu_2/3/qu_4-qu_4/cpowf(54.0f,1/3.0f)-(-a*a*a+4*a*b-8*c)/4/qu_5);
    float complex qu_6_p=csqrtf(a*a/2-4*b/3-cpowf(2.0f,1/3.0f)*qu_2/3/qu_4-qu_4/cpowf(54.0f,1/3.0f)+(-a*a*a+4*a*b-8*c)/4/qu_5);
    //float erg_1=creal(-a/4-qu_5/2-qu_6_m/2);
    //float erg_2=creal(-a/4-qu_5/2+qu_6_m/2);
    float erg_3=creal(-a/4+qu_5/2-qu_6_p/2);
    float erg_4=creal(-a/4+qu_5/2+qu_6_p/2);
    float erg=0;
    float diff_3_y_1=sqrt(1-erg_3*erg_3)*r_cyl_obs;
    float diff_3_x_1=erg_3*r_cyl_obs-r_cyl;
    float diff_3_y_2=erg_3*abs_x-sqrt(1-erg_3*erg_3)*(*y_screenp);
    float diff_3_x_2=sqrt(1-erg_3*erg_3)*abs_x+erg_3*(*y_screenp)-r_cyl;
    float diff_4_y_1=sqrt(1-erg_4*erg_4)*r_cyl_obs;
    float diff_4_x_1=erg_4*r_cyl_obs-r_cyl;
    float diff_4_y_2=erg_4*abs_x-sqrt(1-erg_4*erg_4)*(*y_screenp);
    float diff_4_x_2=sqrt(1-erg_4*erg_4)*abs_x+erg_4*(*y_screenp)-r_cyl;
    float diff_3=atan2(diff_3_y_1,diff_3_x_1)-atan2(diff_3_y_2,diff_3_x_2);
    float diff_4=atan2(diff_4_y_1,diff_4_x_1)-atan2(diff_4_y_2,diff_4_x_2);
    if(abs_x<2&&(*y_screenp)>r_cyl){
        erg=erg_4;
    }else if(fabs(diff_3)<0.007f){
        erg=erg_3;
    }else if(fabs(diff_4)<0.007f){
        erg=erg_4;
    }else{
        return 0;  //TODO test remove
        dprintf(DBGT_ERROR,"In cp_s2o not matchina any case");
    }
    //dprintf(DBGT_ERROR,"tang %f, %f",);
    float intersect_vec[3]={r_cyl*sqrtf(1-erg*erg),r_cyl*erg,0};
    if(sqrtf(r_cyl_obs*r_cyl_obs/(r_cyl*r_cyl)-1)+0.001f<(intersect_vec[0]/intersect_vec[1])){      //TODO fix?
        //dprintf(DBGT_INFO,"cyl proj s2o %f,%f",sqrtf(r_cyl_obs*r_cyl_obs/(r_cyl*r_cyl)-1),(intersect_vec[0]/intersect_vec[1]));
        return 0;
    }
    float dist_screen_intersect=sqrtf((abs_x-intersect_vec[0])*(abs_x-intersect_vec[0])+(*y_screenp-intersect_vec[1])*(*y_screenp-intersect_vec[1]));
    float dist_obs_intersect=sqrtf(intersect_vec[0]*intersect_vec[0]+(r_cyl_obs-intersect_vec[1])*(r_cyl_obs-intersect_vec[1]));
    intersect_vec[2]=dist_screen_intersect/(dist_obs_intersect+dist_screen_intersect)*h_cyl_obs;//height of intersect on cone from screen
    if(intersect_vec[2]>h_cyl+0.1f||intersect_vec[2]<0){
        return 0;
    }
    intersect_vec[0]*=sign;
    cylinder_map_inter_to_plane(intersect_vec,x_screenp,y_screenp);
    return 1;
}

float cone_project_o2s(float rad_obs){
    static float two_beta=FLT_MAX;
    if(two_beta==FLT_MAX){
        two_beta=2*atanf(r_cone/h_cone);
    }
    float alpha=3.14159f-atanf(h_cone_obs/rad_obs);
    float h_obs_d_tan=h_cone_obs/tanf(two_beta-alpha);
    return ((1+h_obs_d_tan/rad_obs)*((rad_obs*r_cone*(h_cone_obs-h_cone))/(h_cone_obs*r_cone-h_cone*rad_obs))-h_obs_d_tan)/rad_obs;
}

float cone_project_s2o(float rad_screen){
	static float cos_atan=FLT_MAX;
	if(cos_atan==FLT_MAX){      //Todo simplify cos(2atan())
        cos_atan=(1-(r_cone/h_cone)*(r_cone/h_cone))/(1+(r_cone/h_cone)*(r_cone/h_cone));//cos(2*atan(r_cone/h_cone));
	}
	static float sin_atan=FLT_MAX;
	if(sin_atan==FLT_MAX){
        sin_atan=2*(r_cone/h_cone)/(1+(r_cone/h_cone)*(r_cone/h_cone));//sin(2*atan(r_cone/height_cone));
	}
	float a=sin_atan*(r_cone*h_cone-r_cone*h_cone_obs-h_cone*rad_screen)+cos_atan*(-h_cone_obs*h_cone);
	float b=sin_atan*(h_cone_obs*(r_cone*rad_screen+h_cone_obs*h_cone))+cos_atan*(h_cone_obs*(-h_cone_obs*r_cone+2*r_cone*h_cone-rad_screen*h_cone));
	float c=h_cone_obs*h_cone_obs*(cos_atan*r_cone*rad_screen-sin_atan*r_cone*h_cone);
	float d1=-b/2/a;
	float d2=sqrt(b*b/4/a/a-c/a);
    return (d1-d2)/(rad_screen);
}

void set_point(uint8_t* TextureDatap, uint32_t TextureSidelength,uint32_t colorRGBA,int32_t posOnTextureX, int32_t posOnTextureY,int32_t penrad, uint32_t screenResX){
    for(int32_t xc=-penrad;xc<=(int32_t)penrad;xc++){
        for(int32_t yc=-penrad;yc<=(int32_t)penrad;yc++){
            if(posOnTextureX+xc>=0&&posOnTextureX+xc<screenResX){   //TODO add checks for other borders
                if(posOnTextureY+yc>=0&&posOnTextureY+yc<TextureSidelength){
                    if(yc*yc+xc*xc<=(penrad*penrad)){
                        for(uint8_t bytepos=0;bytepos<4;bytepos++){
                            TextureDatap[((posOnTextureX+xc)+(TextureSidelength*(posOnTextureY+yc)))*4+bytepos]=(colorRGBA&(0xff<<(8*bytepos)))>>(8*bytepos);       //TODO ENDIANESS
                        }
                    }else if(yc*yc+xc*xc<=((penrad+1)*(penrad+1))){
                        uint8_t magnitude_256=(255*penrad-(uint32_t)(255*sqrt((float)yc*yc+xc*xc)));
                        mixPixel(TextureDatap,((posOnTextureX+xc)+(TextureSidelength*(posOnTextureY+yc)))*4,(colorRGBA&0xffffff00)|magnitude_256);
                    }
                }
            }
        }
    }
}
