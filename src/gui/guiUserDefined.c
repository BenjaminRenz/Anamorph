#include "gui/guiUserDefined.h"
#include "gui/gui.h"
#include "xmlReader/stringutils.h"
#include "debug/debug.h"

int (*functionpointerarray[])(struct DynamicList* AllLayersInfop,float x_perc, float y_perc)={
    &close_Menus,           //GuiFunction 0
    &open_Color_Menu,       //GuiFunction 1
    &set_color_blue,        //GuiFunction 2
    &set_color_green,       //GuiFunction 3
    &set_color_red,         //GuiFunction 4
    &set_color_black,       //GuiFunction 5
    &open_Penrad_Menu,      //GuiFunction 6
    &penrad_big,            //GuiFunction 7
    &penrad_med,            //GuiFunction 8
    &penrad_small,          //GuiFunction 9
    &clear_drawingplane,    //GuiFunction 10
    &open_MObject_Menu,     //GuiFunction 11
    &set_cylinder,          //GuiFunction 12
    &set_cone,              //GuiFunction 13
    &open_Grid_Menu,        //GuiFunction 14
    &set_grid_pic,          //GuiFunction 15
    &set_grid_color,        //GuiFunction 16
    &set_grid_white         //GuiFunction 17
};

int fparrayLength(void){
    return sizeof(functionpointerarray)/sizeof(*functionpointerarray);
}

int close_Menus(struct DynamicList* AllLayersInfop,float x, float y){
    //close all extended menus
    dprintf(DBGT_INFO,"FPID0, on %f,%f",x,y);
    //dprintf(DBGT_INFO,"%d",(((struct LayerCollection*)(AllLayersInfo->items))[0]).active);
    (((struct LayerCollection*)(AllLayersInfop->items))[4]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[5]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[9]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[10]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[14]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[15]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[19]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[20]).active=0;
    dprintf(DBGT_INFO,"color: %x",GlobalAppState.color);
    switch(GlobalAppState.color){
        case 0x000000ff:
            (((struct LayerCollection*)(AllLayersInfop->items))[0]).active=1;
        break;
        case 0x0000ffff:
            (((struct LayerCollection*)(AllLayersInfop->items))[3]).active=1;
        break;
        case 0x00ff00ff:
            (((struct LayerCollection*)(AllLayersInfop->items))[2]).active=1;
        break;
        case 0xff0000ff:
            (((struct LayerCollection*)(AllLayersInfop->items))[1]).active=1;
        break;
    }
    switch(GlobalAppState.penrad){
        case penradSmall:
            (((struct LayerCollection*)(AllLayersInfop->items))[6]).active=1;
        break;
        case penradMed:
            (((struct LayerCollection*)(AllLayersInfop->items))[7]).active=1;
        break;
        case penradBig:
            (((struct LayerCollection*)(AllLayersInfop->items))[8]).active=1;
        break;
    }
    switch(GlobalAppState.switch_cone_cyl){
        case switch_cone_cyl_is_cone:
            (((struct LayerCollection*)(AllLayersInfop->items))[13]).active=1;
        break;
        case switch_cone_cyl_is_cyl:
            (((struct LayerCollection*)(AllLayersInfop->items))[12]).active=1;
        break;
    }
    switch(GlobalAppState.switch_grid){
        case switch_grid_is_white:
            (((struct LayerCollection*)(AllLayersInfop->items))[16]).active=1;
        break;
        case switch_grid_is_color:
            (((struct LayerCollection*)(AllLayersInfop->items))[17]).active=1;
        break;
        case switch_grid_is_pic:
            (((struct LayerCollection*)(AllLayersInfop->items))[18]).active=1;
        break;
    }
    return return_update_gui_vtx;
}

int open_Color_Menu(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID1, on %f,%f",x,y);
    close_Menus(AllLayersInfop,x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[0]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[1]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[2]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[3]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[4]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[5]).active=1;
    return return_update_gui_vtx;
}

int set_color_black(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID2, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[0]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[4]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[5]).active=0;
    GlobalAppState.color=0x000000ff;
    return return_update_gui_vtx;
}

int set_color_red(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID3, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[1]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[4]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[5]).active=0;
    GlobalAppState.color=0xff0000ff;
    return return_update_gui_vtx;
}

int set_color_green(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID4, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[2]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[4]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[5]).active=0;
    GlobalAppState.color=0x00ff00ff;
    return return_update_gui_vtx;
}

int set_color_blue(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID5, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[3]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[4]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[5]).active=0;
    GlobalAppState.color=0x0000ffff;
    return return_update_gui_vtx;
}



int open_Penrad_Menu(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID6, on %f,%f",x,y);
    close_Menus(AllLayersInfop,x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[6]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[7]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[8]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[9]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[10]).active=1;
    return return_update_gui_vtx;
}

int penrad_small(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID7, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[6]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[9]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[10]).active=0;
    GlobalAppState.penrad=penradSmall;
    return return_update_gui_vtx;
}

int penrad_med(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID8, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[7]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[9]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[10]).active=0;
    GlobalAppState.penrad=penradMed;
    return return_update_gui_vtx;
}

int penrad_big(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID9, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[8]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[9]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[10]).active=0;
    GlobalAppState.penrad=penradBig;
    return return_update_gui_vtx;
}



int clear_drawingplane(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID10, on %f,%f",x,y);
    close_Menus(AllLayersInfop,x,y);
    GlobalAppState.clear_drawingplane=1;
    return return_update_gui_vtx;
}




int open_MObject_Menu(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID11, on %f,%f",x,y);
    close_Menus(AllLayersInfop,x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[12]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[13]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[14]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[15]).active=1;
    return return_update_gui_vtx;
}

int set_cylinder(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID12, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[12]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[14]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[15]).active=0;
    GlobalAppState.clear_drawingplane=1;
    GlobalAppState.switch_cone_cyl=switch_cone_cyl_to_cyl;
    return return_update_gui_vtx;
}

int set_cone(struct DynamicList* AllLayersInfop,float x, float y){
    dprintf(DBGT_INFO,"FPID13, on %f,%f",x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[13]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[14]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[15]).active=0;
    GlobalAppState.clear_drawingplane=1;
    GlobalAppState.switch_cone_cyl=switch_cone_cyl_to_cone;
    return return_update_gui_vtx;
}

int open_Grid_Menu(struct DynamicList* AllLayersInfop, float x, float y){
    close_Menus(AllLayersInfop,x,y);
    (((struct LayerCollection*)(AllLayersInfop->items))[16]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[17]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[18]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[19]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[20]).active=1;
    return return_update_gui_vtx;
}

int set_grid_white(struct DynamicList* AllLayersInfop, float x, float y){
    (((struct LayerCollection*)(AllLayersInfop->items))[16]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[19]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[20]).active=0;
    GlobalAppState.switch_grid=switch_grid_to_white;
    return return_update_gui_vtx;
}
int set_grid_color(struct DynamicList* AllLayersInfop, float x, float y){
    (((struct LayerCollection*)(AllLayersInfop->items))[17]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[19]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[20]).active=0;
    GlobalAppState.switch_grid=switch_grid_to_color;
    return return_update_gui_vtx;
}
int set_grid_pic(struct DynamicList* AllLayersInfop, float x, float y){
    (((struct LayerCollection*)(AllLayersInfop->items))[18]).active=1;
    (((struct LayerCollection*)(AllLayersInfop->items))[19]).active=0;
    (((struct LayerCollection*)(AllLayersInfop->items))[20]).active=0;
    GlobalAppState.switch_grid=switch_grid_to_pic;
    return return_update_gui_vtx;
}
