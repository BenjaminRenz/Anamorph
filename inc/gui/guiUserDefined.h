#ifndef GUIUSERDEFINED_H_INCLUDED
#define GUIUSERDEFINED_H_INCLUDED
#include <stdint.h>

struct DynamicList;
enum{return_update_gui_vtx=1,return_dont_update_gui_vtx=0};

enum{penradSmall=4,penradMed=6,penradBig=7};
enum{switch_cone_cyl_is_cone=0,switch_cone_cyl_is_cyl=1,switch_cone_cyl_to_cone=2,switch_cone_cyl_to_cyl=3};                                 //Don't change, logic in main depends on these exact values
enum{switch_grid_is_white=0,switch_grid_is_color=2,switch_grid_is_pic=4,switch_grid_to_white=1,switch_grid_to_color=3,switch_grid_to_pic=5}; //Don't change, logic in main depends on these exact values
struct ApplicationState{
    uint32_t color;
    int penrad;
    int clear_drawingplane;
    int switch_cone_cyl;
    int switch_grid;
};

struct ApplicationState GlobalAppState;

//if return!=0, refresh collider and vertices
int close_Menus(struct DynamicList* AllLayersInfop,float x, float y);
int open_Color_Menu(struct DynamicList* AllLayersInfop,float x, float y);
int set_color_blue(struct DynamicList* AllLayersInfop,float x, float y);
int set_color_green(struct DynamicList* AllLayersInfop,float x, float y);
int set_color_red(struct DynamicList* AllLayersInfop,float x, float y);
int set_color_black(struct DynamicList* AllLayersInfop,float x, float y);
int open_Penrad_Menu(struct DynamicList* AllLayersInfop,float x, float y);
int penrad_big(struct DynamicList* AllLayersInfop,float x, float y);
int penrad_med(struct DynamicList* AllLayersInfop,float x, float y);
int penrad_small(struct DynamicList* AllLayersInfop,float x, float y);
int clear_drawingplane(struct DynamicList* AllLayersInfop,float x, float y);
int open_MObject_Menu(struct DynamicList* AllLayersInfop,float x, float y);
int set_cylinder(struct DynamicList* AllLayersInfop,float x, float y);
int set_cone(struct DynamicList* AllLayersInfop,float x, float y);
int open_Grid_Menu(struct DynamicList* AllLayersInfop,float x, float y);
int set_grid_white(struct DynamicList* AllLayersInfop,float x, float y);
int set_grid_color(struct DynamicList* AllLayersInfop,float x, float y);
int set_grid_pic(struct DynamicList* AllLayersInfop,float x, float y);


extern int (*functionpointerarray[])(struct DynamicList* AllLayersInfop,float x_perc, float y_perc);
int fparrayLength(void);

#endif // GUIUSERDEFINED_H_INCLUDED
