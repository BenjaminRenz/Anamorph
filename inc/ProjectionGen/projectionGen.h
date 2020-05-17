#ifndef PROJECTIONGEN_H_INCLUDED
#define PROJECTIONGEN_H_INCLUDED

enum{mapPoints_return_could_not_map=0,mapPoints_return_from_s=1,mapPoints_return_from_o=2};     //point was in in _from_x and mappes to another point
uint8_t mapPoints(int32_t* mapped_xpos, int32_t* mapped_ypos, int32_t touch_xpos, int32_t touch_ypos,uint32_t screenresx, uint32_t screenresy,uint32_t widthMM, uint32_t heightMM);

float cone_project_s2o(float rad_screen);
uint8_t grid_value(int32_t x_256,int32_t y_256,uint32_t gradient_256, uint32_t linewidth_px_256, uint32_t grid_center_offset_px_256, uint32_t grid_distance_px_256);
float cone_project_o2s(float rad_obs);  //Projection from the observers view of the reflection onto the screen



enum{
    projection_gen_mode_grid_white,
    projection_gen_mode_grid_color,
    projection_gen_mode_grid_pic
};



void projection_gen_largeConeGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM);
void projection_gen_miniConeGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM);
void projection_gen_largeCylGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM);
void projection_gen_miniCylGrid(int mode,uint8_t* optionalPicDataP,uint32_t optionalPicSidelength,uint8_t* TextureData,uint32_t TextureSidelength,uint32_t screenresx, uint32_t screenresy,int widthMM, int heightMM);
void connect_points(uint8_t* TextureDatap,uint32_t TextureSidelength,uint32_t rgba,int32_t p1x,int32_t p1y,int32_t r1,int32_t p2x,int32_t p2y,int32_t r2);
void set_point(uint8_t* TextureDatap, uint32_t TextureSidelength,uint32_t colorRGBA,int32_t posOnTextureX, int32_t posOnTextureY,int32_t penrad, uint32_t screenResX);
#endif // PROJECTIONGEN_H_INCLUDED
