#define ENDIANESS 0 //0=littleEND,1=bigEND
#include <stdlib.h>
#include <string.h> //FOR memset
#include <stdint.h> //FOR uint32_t
#include <math.h>
#include "debug/debug.h"
#include "xmlReader/xmlReader.h"
#include "xmlReader/stringutils.h"
#include "filereader/filereader.h"
#include "ProjectionGen/projectionGen.h"
#include "gui/gui.h"
#include "filereader/filereader.h"      //Temporary for texture sampling test
#include "gui/guiUserDefined.h"
#define M_PI 3.141592653589793

#define GLEW_STATIC
#include "glew/glew.h"
#include "glfw3/glfw3.h"
#include "glUtils/gl_utils.h"

#define recorde_deltat_multip 2


//Position of the minigrid's center in mm
#define r_cone  57.0f
#define h_cone  80.0f

//Internal Conversions
#define mm_per_px_256 (widthMM/(screenresx*256.0f))
#define px_256_per_mm ((screenresx*256.0f)/widthMM)
#define mm_per_px (widthMM/(float)screenresx)
#define px_per_mm (screenresx/(float)widthMM)

#define outerCone_rad_mm (h_cone*tanf(2*atanf(r_cone/h_cone)))     //MaxRadius of our screen projection of the cone


/*Note on texture unit and textures,
opengl features texture units which can have multiple textures bound to them
because shaders can only switch texture units an not textures directely a seperate texture unit is used for the background plane, the drawing plane and the ui
*/
/*Note on coordinate systems
we use opengl's coordinate system, where x is to the right, y is to the top and z is into the monitor
gl without transformation matrices only renders from -1 to 1 in all axis, keep that in mind when choosing gui depth coordinates
*/

struct GObject;

void glfw_error_callback(int error, const char* description);
void APIENTRY openglCallbackFunction(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
//Custom Graphic Object Functions
struct GObject* ui_backgroundplane_init(uint32_t screenresx, uint32_t screenresy);
void ui_backgroundplane_draw(struct GObject* IN_BGPlane);
void ui_drawingplane_draw(struct GObject* IN_DrawingPlane);
void ui_drawingplane_clear(struct GObject* IN_DrawingPlane, uint32_t screenResX, uint32_t screenResY);
struct GObject* ui_drawingplane_init(uint32_t screenresx,uint32_t screenresy);
void ui_drawingplane_write(struct GObject* IN_DrawingPlane, uint32_t penrad, uint32_t colorRGBA, uint32_t posOnTextureX, uint32_t posOnTextureY, uint32_t screenResX, uint32_t screenResY);
struct GObject* ui_generate_gui(char* xmlFilepath,uint32_t screenresx,uint32_t screenresy);
void ui_gui_draw(struct GObject* IN_GUI);
void updateGUIvtx(struct GObject* initGUIp);



int FindClickedElement(double touchscreenx,double touchscreeny);
int CurrentlyActiveCollider=(-1);
int CurrentlyActiveLayer=(-1);
struct DynamicList* AllLayerInfop;

uint32_t chfkt_name_layer(struct xmlTreeElement* TestXmlElement){
    static struct DynamicList* layer_utf32string=0;
    if(!layer_utf32string){
        layer_utf32string=stringToUTF32Dynlist("layer");
    }
    if(TestXmlElement->name&&compareEqualDynamicUTF32List(TestXmlElement->name,layer_utf32string)){
        return 1;
    }
    return 0;
}

uint32_t chkfkt_name_texture(struct xmlTreeElement* TestXmlElement){
    static struct DynamicList* layer_utf32string=0;
    if(!layer_utf32string){
        layer_utf32string=stringToUTF32Dynlist("texture");
    }
    if(TestXmlElement->name&&compareEqualDynamicUTF32List(TestXmlElement->name,layer_utf32string)&&TestXmlElement->attributes){
        return 1;
    }
    return 0;
}


//void guiInit
struct GObject{
    GLuint VertexArrayID;
    GLuint VertexBufferID;  //Not needed
    GLuint TextureBufferID;
    struct DynamicList* TexturePointerDLp;
    uint32_t TextureSidelength;
    uint32_t Vertices;
    GLuint ShaderID;
    GLuint TextureUnitIndex;
};

//Will return an xmlcollection list
/*struct xmlTreeElement* getChildrenWithName(uint32_t maxdepth, struct xmlTreeElement* parent, struct DynamicList* name){
    struct xmlTreeElement* currentElement=parent;
    while(1){
        for(uint32_t contentnum=0;contentnum<currentElement->content->itemcnt;contentnum++){
            if(((struct xmlTreeElement*)currentElement->content->items)[contentnum])
        }
    }
}*/

//GLOBALS
struct GObject* Canvasp;
uint32_t draw_enabled=0;
int widthMM=0;
int heightMM=0;
enum{drawModeActive,drawModeReload,drawModeInactive};
int drawMode=drawModeInactive;
struct xmlTreeElement* globalCommandBufferOutp=0;
double stopDeltatimeForLine=0.0;

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key==GLFW_KEY_SPACE&&action==GLFW_RELEASE){
        switch(drawMode){
        case drawModeActive:
            drawMode=drawModeReload;
            break;
        case drawModeInactive:
            globalCommandBufferOutp=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            globalCommandBufferOutp->name=stringToUTF32Dynlist("commands");
            globalCommandBufferOutp->content=0;
            globalCommandBufferOutp->attributes=0;
            globalCommandBufferOutp->type=xmltype_tag;
            globalCommandBufferOutp->parent=0;
            drawMode=drawModeActive;
            GlobalAppState.clear_drawingplane=1;
            stopDeltatimeForLine=glfwGetTime();     //for initial delay
            break;
        }
    }
}

void MouseButtonCallback(GLFWwindow* GLFW_window,int button, int action, int mods){
    if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS&&drawMode==drawModeActive){
        if(!draw_enabled){  //if this is the first point
            dprintf(DBGT_INFO,"first point");
            //set delay
            struct xmlTreeElement* delayTag=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            delayTag->name=stringToUTF32Dynlist("delay");
            delayTag->attributes=0;
            delayTag->parent=globalCommandBufferOutp;
            delayTag->content=0;
            delayTag->type=xmltype_tag;
            append_DynamicList(&globalCommandBufferOutp->content,&delayTag,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);

            //find out deltat
            double deltat=(glfwGetTime()-stopDeltatimeForLine);
            deltat*=1000;
            struct key_val_pair delay_ms_key_val;
            delay_ms_key_val.key=stringToUTF32Dynlist("ms");
            uint32_t requieredCharsForMs=snprintf(NULL,0,"%lf",deltat);
            struct DynamicList* TempFloatStringForMs=create_DynamicList(sizeof(uint32_t),requieredCharsForMs,dynlisttype_utf32chars);
            uint8_t* tempASCIIStringp=(uint8_t*)malloc((requieredCharsForMs+1)*sizeof(uint8_t));
            snprintf((char*)tempASCIIStringp,requieredCharsForMs+1,"%lf",deltat);
            utf8_to_utf32(tempASCIIStringp,requieredCharsForMs,TempFloatStringForMs->items);
            free(tempASCIIStringp);
            delay_ms_key_val.value=TempFloatStringForMs;
            //append to attributes
            append_DynamicList(&delayTag->attributes,&delay_ms_key_val,sizeof(struct key_val_pair),dynlisttype_keyValuePairsp);





            double xtouchpos=0.0;
            double ytouchpos=0.0;
            glfwGetCursorPos(GLFW_window,&xtouchpos,&ytouchpos);
            int xpos=(int32_t)xtouchpos;
            int ypos=(int32_t)ytouchpos;
            int screenresy=0;
            int screenresx=0;
            glfwGetWindowSize(GLFW_window,&screenresx,&screenresy);     //TODO make this global?
            int32_t mapped_x_pos=0;
            int32_t mapped_y_pos=0;
            ypos=screenresy-ypos;
            switch(mapPoints(&mapped_x_pos,&mapped_y_pos,xpos,ypos,screenresx,screenresy,widthMM,heightMM)){
            case mapPoints_return_from_o:
                xpos=mapped_x_pos;
                ypos=mapped_y_pos;
                //intended fallthrough, no break
            case mapPoints_return_from_s: ; //empty command to avoid "a label can only be part of a statement"
                //create new line xml element
                struct xmlTreeElement* newLineElementp=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
                newLineElementp->name=stringToUTF32Dynlist("line");
                newLineElementp->content=0;
                newLineElementp->attributes=0;
                newLineElementp->type=xmltype_tag;
                //append to our global command buffer for later output
                append_DynamicList(&(globalCommandBufferOutp->content),&newLineElementp,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
                newLineElementp->parent=globalCommandBufferOutp;

                xtouchpos=((double)xpos-screenresx/2)*mm_per_px/outerCone_rad_mm;
                ytouchpos=((double)ypos-screenresy/2)*mm_per_px/outerCone_rad_mm;
                uint32_t requieredCharsForXy=snprintf(NULL,0,"%lf %lf ",xtouchpos,ytouchpos);
                struct DynamicList* TempFloatString=create_DynamicList(sizeof(uint32_t),requieredCharsForXy,dynlisttype_utf32chars);
                uint8_t* tempASCIIStringp=(uint8_t*)malloc((requieredCharsForXy+1)*sizeof(uint8_t));                //+1 because zero termination
                snprintf((char*)tempASCIIStringp,requieredCharsForXy+1,"%lf %lf ",xtouchpos,ytouchpos);             //+1 because zero termination
                utf8_to_utf32(tempASCIIStringp,requieredCharsForXy,TempFloatString->items);
                free(tempASCIIStringp);

                stopDeltatimeForLine=glfwGetTime();

                struct key_val_pair line_key_valp;
                line_key_valp.key=stringToUTF32Dynlist("xycoord");
                line_key_valp.value=TempFloatString;
                append_DynamicList(&(newLineElementp->attributes),&line_key_valp,sizeof(struct key_val_pair),dynlisttype_keyValuePairsp);
                //dprintf(DBGT_ERROR,"Value as First: %s",utf32dynlist_to_string(TempFloatString));
                break;
            case mapPoints_return_could_not_map:
                exit(-1);
                break;
            }
            draw_enabled=1;
        }
    }else if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_RELEASE&&drawMode==drawModeActive){
        if(draw_enabled&&drawMode==drawModeActive){     //set last point in line
            dprintf(DBGT_INFO,"last point");
            double xtouchpos=0.0;
            double ytouchpos=0.0;
            glfwGetCursorPos(GLFW_window,&xtouchpos,&ytouchpos);
            int xpos=(int32_t)xtouchpos;
            int ypos=(int32_t)ytouchpos;
            int screenresy=0;
            int screenresx=0;
            glfwGetWindowSize(GLFW_window,&screenresx,&screenresy);     //TODO make this global?
            int32_t mapped_x_pos=0;
            int32_t mapped_y_pos=0;
            ypos=screenresy-ypos;
            switch(mapPoints(&mapped_x_pos,&mapped_y_pos,xpos,ypos,screenresx,screenresy,widthMM,heightMM)){
            case mapPoints_return_from_o:
                xpos=mapped_x_pos;
                ypos=mapped_y_pos;
                //intended fallthrough, no break
            case mapPoints_return_from_s:
                //transform touch position and mapped point into 1,1 coordinate system
                xtouchpos=((double)xpos-screenresx/2)*mm_per_px/outerCone_rad_mm;
                ytouchpos=((double)ypos-screenresy/2)*mm_per_px/outerCone_rad_mm;
                uint32_t requieredCharsForXy=snprintf(NULL,0,"%lf %lf ",xtouchpos,ytouchpos);
                struct DynamicList* TempFloatStringForXy=create_DynamicList(sizeof(uint32_t),requieredCharsForXy,dynlisttype_utf32chars);
                uint8_t* tempASCIIStringp=(uint8_t*)malloc((requieredCharsForXy+1)*sizeof(uint8_t));
                snprintf((char*)tempASCIIStringp,requieredCharsForXy+1,"%lf %lf ",xtouchpos,ytouchpos);
                utf8_to_utf32(tempASCIIStringp,requieredCharsForXy,TempFloatStringForXy->items);
                free(tempASCIIStringp);

                //close old line tag

                //find out deltat
                double deltat=(glfwGetTime()-stopDeltatimeForLine);
                deltat*=1000*recorde_deltat_multip;
                struct key_val_pair line_ms_key_val;
                line_ms_key_val.key=stringToUTF32Dynlist("ms");
                uint32_t requieredCharsForMs=snprintf(NULL,0,"%lf",deltat);
                struct DynamicList* TempFloatStringForMs=create_DynamicList(sizeof(uint32_t),requieredCharsForMs,dynlisttype_utf32chars);
                tempASCIIStringp=(uint8_t*)malloc((requieredCharsForMs+1)*sizeof(uint8_t));
                snprintf((char*)tempASCIIStringp,requieredCharsForMs+1,"%lf",deltat);
                utf8_to_utf32(tempASCIIStringp,requieredCharsForMs,TempFloatStringForMs->items);
                free(tempASCIIStringp);
                line_ms_key_val.value=TempFloatStringForMs;
                //append to attributes
                uint32_t index_of_last_line_elmnt=(globalCommandBufferOutp->content->itemcnt-1);
                struct xmlTreeElement* lastLineElementp=((struct xmlTreeElement**)globalCommandBufferOutp->content->items)[index_of_last_line_elmnt];
                append_DynamicList(&lastLineElementp->attributes,&line_ms_key_val,sizeof(struct key_val_pair),dynlisttype_keyValuePairsp);


                //append x2 and y2 to last line element
                //create the requiered space for the full set of x1 y1 x2 y2 coordinates
                struct key_val_pair oldLineOldKeyVal=((struct key_val_pair*)lastLineElementp->attributes->items)[0];        //xycoord should be the first element
                struct DynamicList* oldLineNewStringp=create_DynamicList(sizeof(uint32_t),oldLineOldKeyVal.value->itemcnt+requieredCharsForXy,dynlisttype_utf32chars);
                memcpy(oldLineNewStringp->items,oldLineOldKeyVal.value->items,sizeof(uint32_t)*(oldLineOldKeyVal.value->itemcnt));          //copy over previous string x1 y1
                memcpy(((uint32_t*)oldLineNewStringp->items)+oldLineOldKeyVal.value->itemcnt,TempFloatStringForXy->items,sizeof(uint32_t)*(requieredCharsForXy));  //copy new string x2 y2
                delete_DynList(oldLineOldKeyVal.value);    //delete old string
                ((struct key_val_pair*)lastLineElementp->attributes->items)[0].value=oldLineNewStringp;  //update reference from value

                break;
            case mapPoints_return_could_not_map:
                break;
            }
            draw_enabled=0;
        }
    }
    /*
    double xpos=0.0;
    double ypos=0.0;
    glfwGetCursorPos(GLFW_window,&xpos,&ypos);
    int screenresy=0;
    int screenresx=0;
    glfwGetWindowSize(GLFW_window,&screenresx,&screenresy);
    ypos=screenresy-ypos;
    if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS){
        if(FindClickedElement(xpos,ypos)){
            dprintf(DBGT_INFO,"Gui");
        }else{
            dprintf(DBGT_INFO,"Draw");
            (*functionpointerarray[0])(AllLayerInfop,0.0f,0.0f);
            updateGUIvtx(0);
            draw_enabled=1;
        }
        //TODO draw pixel

    }else if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_RELEASE){
        if(CurrentlyActiveCollider!=-1){
            struct DynamicList* AllColliderp=(((struct LayerCollection*)(AllLayerInfop->items))[CurrentlyActiveLayer]).colliderDLp;
            struct Collider CurrentCollider=((struct Collider*)(AllColliderp->items))[CurrentlyActiveCollider];
            float normalisedxCoord=(xpos-(int32_t)CurrentCollider.colliderLL_x)/((float)(CurrentCollider.colliderUR_x-CurrentCollider.colliderLL_x));
            float normalisedyCoord=(ypos-(int32_t)CurrentCollider.colliderLL_y)/((float)(CurrentCollider.colliderUR_y-CurrentCollider.colliderLL_y));
            if(CurrentCollider.onReleaseFPid!=-1){
                if((*functionpointerarray[CurrentCollider.onReleaseFPid])(AllLayerInfop,normalisedxCoord,normalisedyCoord)){
                    //need to update which layers are active
                    updateGUIvtx(0);
                }
            }
        }else{
            draw_enabled=0;
        }
        CurrentlyActiveCollider=-1;
        CurrentlyActiveLayer=0;
    }
    */
}


int FindClickedElement(double touchscreenx,double touchscreeny){
    dprintf(DBGT_ERROR,"GOT INTO FIND");
    int highestHitElementColliderIndex=(-1);
    int highestHitElementLayerIndex=0;
    float highestHitZsoFar=-1.0;
    float normalisedxCoord;
    float normalisedyCoord;
    for(uint32_t numLayer=0;numLayer<AllLayerInfop->itemcnt;numLayer++){
        struct LayerCollection CurrentLayer=((struct LayerCollection*)(AllLayerInfop->items))[numLayer];
        if(CurrentLayer.active){
            struct DynamicList* AllColliderp=CurrentLayer.colliderDLp;
            for(uint32_t numCollider=0;numCollider<AllColliderp->itemcnt;numCollider++){
                struct Collider CurrentCollider=((struct Collider*)(AllColliderp->items))[numCollider];
                if(CurrentCollider.colliderLL_x<touchscreenx&&CurrentCollider.colliderLL_y<touchscreeny&&
                   CurrentCollider.colliderUR_x>touchscreenx&&CurrentCollider.colliderUR_y>touchscreeny){
                    if(CurrentCollider.collider_z>highestHitZsoFar){
                        normalisedxCoord=(touchscreenx-(int32_t)CurrentCollider.colliderLL_x)/((float)(CurrentCollider.colliderUR_x-CurrentCollider.colliderLL_x));
                        normalisedyCoord=(touchscreeny-(int32_t)CurrentCollider.colliderLL_y)/((float)(CurrentCollider.colliderUR_y-CurrentCollider.colliderLL_y));
                        highestHitElementColliderIndex=numCollider;
                        highestHitElementLayerIndex=numLayer;
                        highestHitZsoFar=CurrentCollider.collider_z;
                    }
                }
            }
        }
    }
    if(highestHitElementColliderIndex!=(-1)){
        CurrentlyActiveCollider=highestHitElementColliderIndex;
        CurrentlyActiveLayer=highestHitElementLayerIndex;

        struct DynamicList* AllColliderp=(((struct LayerCollection*)(AllLayerInfop->items))[CurrentlyActiveLayer]).colliderDLp;
        struct Collider CurrentCollider=((struct Collider*)(AllColliderp->items))[CurrentlyActiveCollider];
        dprintf(DBGT_INFO,"%f",CurrentCollider.collider_z);
        if(CurrentCollider.onClickFPid!=-1){
            if((*functionpointerarray[CurrentCollider.onClickFPid])(AllLayerInfop,normalisedxCoord,normalisedyCoord)){
                //need to update which layers are active
                updateGUIvtx(0);
                dprintf(DBGT_INFO,"Update gui");
            }else{
                dprintf(DBGT_INFO,"Do not update vtx");
            }
        }else{
            (*functionpointerarray[0])(AllLayerInfop,normalisedxCoord,normalisedyCoord);    //close all gui menus
            updateGUIvtx(0);
        }
        return 1;   //any element was clicked
    }
    return 0; //no element was clicked
}

void CursorPosCallback(GLFWwindow* GLFW_window, double xpos, double ypos){
    if(draw_enabled&&drawMode==drawModeActive){     //set next point in line
        dprintf(DBGT_INFO,"mid point");
        double xtouchpos=0.0;
        double ytouchpos=0.0;
        glfwGetCursorPos(GLFW_window,&xtouchpos,&ytouchpos);
        int xpos=(int32_t)xtouchpos;
        int ypos=(int32_t)ytouchpos;
        int screenresy=0;
        int screenresx=0;
        glfwGetWindowSize(GLFW_window,&screenresx,&screenresy);     //TODO make this global?
        int32_t mapped_x_pos=0;
        int32_t mapped_y_pos=0;
        ypos=screenresy-ypos;
        switch(mapPoints(&mapped_x_pos,&mapped_y_pos,xpos,ypos,screenresx,screenresy,widthMM,heightMM)){
        case mapPoints_return_from_o:
            xpos=mapped_x_pos;
            ypos=mapped_y_pos;
            //intended fallthrough, no break
        case mapPoints_return_from_s:
            //transform touch position and mapped point into 1,1 coordinate system
            xtouchpos=((double)xpos-screenresx/2)*mm_per_px/outerCone_rad_mm;
            ytouchpos=((double)ypos-screenresy/2)*mm_per_px/outerCone_rad_mm;
            uint32_t requieredCharsForXy=snprintf(NULL,0,"%lf %lf ",xtouchpos,ytouchpos);
            struct DynamicList* TempFloatStringForXy=create_DynamicList(sizeof(uint32_t),requieredCharsForXy,dynlisttype_utf32chars);
            uint8_t* tempASCIIStringp=(uint8_t*)malloc((requieredCharsForXy+1)*sizeof(uint8_t));
            snprintf((char*)tempASCIIStringp,requieredCharsForXy+1,"%lf %lf ",xtouchpos,ytouchpos);
            utf8_to_utf32(tempASCIIStringp,requieredCharsForXy,TempFloatStringForXy->items);
            free(tempASCIIStringp);

            //close old line tag

            //find out deltat
            double deltat=(glfwGetTime()-stopDeltatimeForLine);
            deltat*=1000*recorde_deltat_multip;
            struct key_val_pair line_ms_key_val;
            line_ms_key_val.key=stringToUTF32Dynlist("ms");
            uint32_t requieredCharsForMs=snprintf(NULL,0,"%lf",deltat);
            struct DynamicList* TempFloatStringForMs=create_DynamicList(sizeof(uint32_t),requieredCharsForMs,dynlisttype_utf32chars);
            tempASCIIStringp=(uint8_t*)malloc((requieredCharsForMs+1)*sizeof(uint8_t));
            snprintf((char*)tempASCIIStringp,requieredCharsForMs+1,"%lf",deltat);
            utf8_to_utf32(tempASCIIStringp,requieredCharsForMs,TempFloatStringForMs->items);
            free(tempASCIIStringp);
            line_ms_key_val.value=TempFloatStringForMs;
            //append to attributes
            uint32_t index_of_last_line_elmnt=(globalCommandBufferOutp->content->itemcnt-1);
            struct xmlTreeElement* lastLineElementp=((struct xmlTreeElement**)globalCommandBufferOutp->content->items)[index_of_last_line_elmnt];
            append_DynamicList(&lastLineElementp->attributes,&line_ms_key_val,sizeof(struct key_val_pair),dynlisttype_keyValuePairsp);


            //append x2 and y2 to last line element
            //create the requiered space for the full set of x1 y1 x2 y2 coordinates
            struct key_val_pair oldLineOldKeyVal=(((struct key_val_pair*)lastLineElementp->attributes->items)[0]);        //xycoord should be the first element
            struct DynamicList* oldLineNewStringp=create_DynamicList(sizeof(uint32_t),oldLineOldKeyVal.value->itemcnt+requieredCharsForXy,dynlisttype_utf32chars);
            memcpy(oldLineNewStringp->items,oldLineOldKeyVal.value->items,sizeof(uint32_t)*(oldLineOldKeyVal.value->itemcnt));          //copy over previous string x1 y1
            memcpy(((uint32_t*)oldLineNewStringp->items)+(oldLineOldKeyVal.value->itemcnt),TempFloatStringForXy->items,sizeof(uint32_t)*(requieredCharsForXy));  //copy new string x2 y2
            delete_DynList(oldLineOldKeyVal.value);    //delete old string
            (((struct key_val_pair*)lastLineElementp->attributes->items)[0]).value=oldLineNewStringp;  //update reference from value





            //create new line xml element
            struct xmlTreeElement* newLineElementp=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            newLineElementp->name=stringToUTF32Dynlist("line");
            newLineElementp->content=0;
            newLineElementp->attributes=0;
            newLineElementp->type=xmltype_tag;
            //append to our global command buffer for later output
            append_DynamicList(&(globalCommandBufferOutp->content),&newLineElementp,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
            newLineElementp->parent=globalCommandBufferOutp;



            stopDeltatimeForLine=glfwGetTime();

            struct key_val_pair line_key_val;
            line_key_val.key=stringToUTF32Dynlist("xycoord");
            line_key_val.value=TempFloatStringForXy;
            append_DynamicList(&(newLineElementp->attributes),&line_key_val,sizeof(struct key_val_pair),dynlisttype_keyValuePairsp);

            break;
        case mapPoints_return_could_not_map:
            break;
        }


    }

    /*if(CurrentlyActiveCollider!=-1){
        struct DynamicList* AllColliderp=(((struct LayerCollection*)(AllLayerInfop->items))[CurrentlyActiveLayer]).colliderDLp;
        struct Collider CurrentCollider=((struct Collider*)(AllColliderp->items))[CurrentlyActiveCollider];
        float normalisedxCoord=(xpos-(int32_t)CurrentCollider.colliderLL_x)/((float)(CurrentCollider.colliderUR_x-CurrentCollider.colliderLL_x));
        float normalisedyCoord=(ypos-(int32_t)CurrentCollider.colliderLL_y)/((float)(CurrentCollider.colliderUR_y-CurrentCollider.colliderLL_y));
        if(CurrentCollider.onHoldFPid!=-1){
            if((*functionpointerarray[CurrentCollider.onHoldFPid])(AllLayerInfop,normalisedxCoord,normalisedyCoord)){
                //need to update which layers are active
                updateGUIvtx(0);
                dprintf(DBGT_INFO,"Update gui");
            }else{
                dprintf(DBGT_INFO,"Do not update vtx");
            }
        }
    }else{
        if(draw_enabled){
            if(xpos>0&&ypos>0){
                int screenresx=0;
                int screenresy=0;
                glfwGetWindowSize(GLFW_window,&screenresx,&screenresy);
                ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,(uint32_t)floor(xpos),Canvasp->TextureSidelength-(uint32_t)floor(ypos),screenresx,screenresy);
                if(draw_enabled==1){
                    draw_enabled=2;
                }
            }
        }
    }*/
}

void updateGUIvtx(struct GObject* initGUIp){
    static struct GObject* storedGUIp;   //will be initialized by first function call
    if(initGUIp){
        storedGUIp=initGUIp;
    }
    //Calculate needed storage space
    int numOfFloats=0;
    for(int CurrentLayerIndex=0;CurrentLayerIndex<AllLayerInfop->itemcnt;CurrentLayerIndex++){
        struct LayerCollection CurrentLayer=((struct LayerCollection*)(AllLayerInfop->items))[CurrentLayerIndex];
        if(CurrentLayer.active){
            numOfFloats+=CurrentLayer.xyzuvDLp->itemcnt;
        }
    }
    float* combinedVTXp=(float*)malloc(sizeof(float)*numOfFloats);
    //Merge Vertex Lists of individual vtx
    int currentNumOfFloatsInVtx=0;
    for(int CurrentLayerIndex=0;CurrentLayerIndex<AllLayerInfop->itemcnt;CurrentLayerIndex++){
        struct LayerCollection CurrentLayer=((struct LayerCollection*)(AllLayerInfop->items))[CurrentLayerIndex];
        if(CurrentLayer.active){
            memcpy(combinedVTXp+currentNumOfFloatsInVtx,CurrentLayer.xyzuvDLp->items,sizeof(float)*CurrentLayer.xyzuvDLp->itemcnt);
            currentNumOfFloatsInVtx+=CurrentLayer.xyzuvDLp->itemcnt;
        }
    }
    storedGUIp->Vertices=numOfFloats/5;
    glBindVertexArray(storedGUIp->VertexArrayID);
    glBindBuffer(GL_ARRAY_BUFFER,storedGUIp->VertexBufferID);
    glBufferData(GL_ARRAY_BUFFER,sizeof(float)*numOfFloats,(void*)combinedVTXp,GL_STATIC_DRAW);
    free(combinedVTXp);
    glBindVertexArray(0);
}

void processCommandXml(struct xmlTreeElement* CommandInput,int screenresx, int screenresy){
    static int next_cycle_is_first=1;
    static double time_command_start=0;
    static int32_t last_xpos_px=0;
    static int32_t last_ypos_px=0;
    static int32_t last_mapped_xpos_px=0;
    static int32_t last_mapped_ypos_px=0;
    static int command_number=0;
    if(next_cycle_is_first){        //init to current time when this function is first called
        time_command_start=glfwGetTime();
    }

    struct xmlTreeElement* currentCommand=((struct xmlTreeElement**)CommandInput->content->items)[command_number];
    double command_delay=0.0f;      //if we don't have a ms attribute assume delay 0ms
    if(currentCommand->attributes){
        struct DynamicList* delayValStringp=getValueFromKeyName(currentCommand->attributes,stringToUTF32Dynlist("ms"));
        if(delayValStringp){
            struct DynamicList* delayValDynlistp=utf32dynlist_to_doubles(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),delayValStringp);
            command_delay=((double*)delayValDynlistp->items)[0];
            delete_DynList(delayValDynlistp);
        }
    }



    if(command_delay*0.001>glfwGetTime()-time_command_start){     //delay not finished yet
        //process "continuous command"
        if(compareEqualDynamicUTF32List(stringToUTF32Dynlist("line"),currentCommand->name)){



            struct DynamicList* coordsp=utf32dynlist_to_floats(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),getValueFromKeyName(currentCommand->attributes,stringToUTF32Dynlist("xycoord")));

            float timemultiplyer=(glfwGetTime()-time_command_start)/(command_delay*0.001f);

            float interpolated_raw_x=((float*)coordsp->items)[0]+timemultiplyer*(((float*)coordsp->items)[2]-((float*)coordsp->items)[0]);
            float interpolated_raw_y=((float*)coordsp->items)[1]+timemultiplyer*(((float*)coordsp->items)[3]-((float*)coordsp->items)[1]);
            if(next_cycle_is_first){
                last_xpos_px=(int32_t)(((float*)coordsp->items)[0]*outerCone_rad_mm*px_per_mm)+screenresx/2;
                last_ypos_px=(int32_t)(((float*)coordsp->items)[1]*outerCone_rad_mm*px_per_mm)+screenresy/2;
                last_mapped_xpos_px=0;
                last_mapped_ypos_px=0;
                if(!mapPoints(&last_mapped_xpos_px,&last_mapped_ypos_px,last_xpos_px,last_ypos_px,screenresx,screenresy,widthMM,heightMM)){
                    dprintf(DBGT_ERROR,"Can't map point error!!!");
                }
            }
            delete_DynList(coordsp);
            int current_xpos_px=((int32_t)(interpolated_raw_x*outerCone_rad_mm*px_per_mm)+screenresx/2);
            int current_ypos_px=((int32_t)(interpolated_raw_y*outerCone_rad_mm*px_per_mm)+screenresy/2);

            //Mapping

            int32_t current_mapped_xpos_px=0;
            int32_t current_mapped_ypos_px=0;

            if(!mapPoints(&current_mapped_xpos_px,&current_mapped_ypos_px,last_xpos_px,last_ypos_px,screenresx,screenresy,widthMM,heightMM)){
                dprintf(DBGT_ERROR,"Can't map point error!!!");
            }

            if(next_cycle_is_first){//make sure first point lands on x1 y1
                //draw first point
                ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,last_xpos_px,Canvasp->TextureSidelength-screenresy+last_ypos_px,screenresx,screenresy);
                //draw first point in mapped space
                ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,last_mapped_xpos_px,Canvasp->TextureSidelength-screenresy+last_mapped_ypos_px,screenresx,screenresy);
            }
            //draw line
            connect_points(((uint8_t**)(Canvasp->TexturePointerDLp->items))[0],Canvasp->TextureSidelength,GlobalAppState.color,last_xpos_px,Canvasp->TextureSidelength-screenresy+last_ypos_px,GlobalAppState.penrad,current_xpos_px,Canvasp->TextureSidelength-screenresy+current_ypos_px,GlobalAppState.penrad);
            //draw line in mapped space
            connect_points(((uint8_t**)(Canvasp->TexturePointerDLp->items))[0],Canvasp->TextureSidelength,GlobalAppState.color,last_mapped_xpos_px,Canvasp->TextureSidelength-screenresy+last_mapped_ypos_px,GlobalAppState.penrad,current_mapped_xpos_px,Canvasp->TextureSidelength-screenresy+current_mapped_ypos_px,GlobalAppState.penrad);

            //draw second point
            ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,current_xpos_px,Canvasp->TextureSidelength-screenresy+current_ypos_px,screenresx,screenresy);
            //draw second point in mapped space
            ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,current_mapped_xpos_px,Canvasp->TextureSidelength-screenresy+current_mapped_ypos_px,screenresx,screenresy);

            //remember the last point and mapped point
            last_xpos_px=current_xpos_px;
            last_ypos_px=current_ypos_px;
            last_mapped_xpos_px=current_mapped_xpos_px;
            last_mapped_ypos_px=current_mapped_ypos_px;

        }
        if(next_cycle_is_first){
            next_cycle_is_first=0;
        }
    }else{      //delay finished
        time_command_start=glfwGetTime();
        next_cycle_is_first=1;
        //process end statement of last command
        if(compareEqualDynamicUTF32List(stringToUTF32Dynlist("delay"),currentCommand->name)){
            //Do nothing
        }else if(compareEqualDynamicUTF32List(stringToUTF32Dynlist("clear"),currentCommand->name)){
            GlobalAppState.clear_drawingplane=1;
        }else if(compareEqualDynamicUTF32List(stringToUTF32Dynlist("switchgrid"),currentCommand->name)){
            struct DynamicList* gridValStringp=getValueFromKeyName(currentCommand->attributes,stringToUTF32Dynlist("num"));
            struct DynamicList* gridValDynlistp=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),gridValStringp);
            GlobalAppState.switch_grid=((int*)gridValDynlistp->items)[0];
            if(GlobalAppState.switch_grid%2==0){
                dprintf(DBGT_ERROR,"Invalid num in switchgrid");
            }
            delete_DynList(gridValDynlistp);
        }else if(compareEqualDynamicUTF32List(stringToUTF32Dynlist("line"),currentCommand->name)){

            struct DynamicList* coordsp=utf32dynlist_to_floats(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),getValueFromKeyName(currentCommand->attributes,stringToUTF32Dynlist("xycoord")));

            float interpolated_raw_x=((float*)coordsp->items)[2];
            float interpolated_raw_y=((float*)coordsp->items)[3];

            delete_DynList(coordsp);

            int current_xpos_px=((int32_t)(interpolated_raw_x*outerCone_rad_mm*px_per_mm)+screenresx/2);
            int current_ypos_px=((int32_t)(interpolated_raw_y*outerCone_rad_mm*px_per_mm)+screenresy/2);

            //Mapping
            int32_t current_mapped_xpos_px=0;
            int32_t current_mapped_ypos_px=0;

            if(!mapPoints(&current_mapped_xpos_px,&current_mapped_ypos_px,last_xpos_px,last_ypos_px,screenresx,screenresy,widthMM,heightMM)){
                dprintf(DBGT_ERROR,"Can't map point error!!!");
            }
            //draw line
            connect_points(((uint8_t**)(Canvasp->TexturePointerDLp->items))[0],Canvasp->TextureSidelength,GlobalAppState.color,last_xpos_px,Canvasp->TextureSidelength-screenresy+last_ypos_px,GlobalAppState.penrad,current_xpos_px,Canvasp->TextureSidelength-screenresy+current_ypos_px,GlobalAppState.penrad);
            //draw line in mapped space
            connect_points(((uint8_t**)(Canvasp->TexturePointerDLp->items))[0],Canvasp->TextureSidelength,GlobalAppState.color,last_mapped_xpos_px,Canvasp->TextureSidelength-screenresy+last_mapped_ypos_px,GlobalAppState.penrad,current_mapped_xpos_px,Canvasp->TextureSidelength-screenresy+current_mapped_ypos_px,GlobalAppState.penrad);

            //draw second point
            ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,current_xpos_px,Canvasp->TextureSidelength-screenresy+current_ypos_px,screenresx,screenresy);
            //draw second point in mapped space
            ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,current_mapped_xpos_px,Canvasp->TextureSidelength-screenresy+current_mapped_ypos_px,screenresx,screenresy);

        }else{

        }
        if(++command_number>=CommandInput->content->itemcnt){ //when the last command has processed start again with the first
            command_number=0;
        }
    }
}

int main(void){

   //Corona, causes memory bug if before glfwInit()
    FILE* xmlFileCommandp=fopen("./res/commands.xml","rb");
    FILE* xmlFileCommandOutp=fopen("./res/commands_out.xml","wb");
    struct xmlTreeElement* Commandxmlrootp=0;
    readXML(xmlFileCommandp,&Commandxmlrootp);          //ERROR Corrupts heap
    fclose(xmlFileCommandp);
    struct xmlTreeElement* CommandInput=((struct xmlTreeElement**)Commandxmlrootp->content->items)[0];


    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    //window creation
    #define FULLSCREEN
    #ifdef FULLSCREEN
    const GLFWvidmode* vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    GLFWwindow* MainWindow = glfwCreateWindow(vidmode->width, vidmode->height, "Quantum Minigolf 2.0", glfwGetPrimaryMonitor(), NULL); //Fullscreen
    #else
    GLFWwindow* MainWindow = glfwCreateWindow(1920, 1080, "Pa1nt debug", NULL, NULL);   //Windowed hd ready

    #endif // FULLSCREEN

    if(!MainWindow) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(MainWindow);
    glfwSetMouseButtonCallback(MainWindow,MouseButtonCallback);
    glfwSetKeyCallback(MainWindow,KeyCallback);
    glfwSetCursorPosCallback(MainWindow,CursorPosCallback);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(GLEW_OK != err) {
        dprintf(DBGT_ERROR,"Error: glewInit() failed.");
    }
    dprintf(DBGT_INFO,"Window creation successful\n");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(openglCallbackFunction, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);   //Dont filter messages
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE);   //Dont filter messages
    //Set background color
    glClearColor(0.0f, 0.0f, 0.6f, 0.5f);
    //Enable z checking
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    //Get Windowsize
    int window_width=0;
    int window_height=0;
    glfwGetWindowSize(MainWindow,&window_width,&window_height);
    glfwGetMonitorPhysicalSize(glfwGetPrimaryMonitor(),&widthMM,&heightMM);
    widthMM*=2;         //monitor size pfusch TODO
    heightMM*=2;        //monitor size pfusch TODO
    GlobalAppState.color=0x00ff00ff;        //Corona
    GlobalAppState.penrad=4;
    GlobalAppState.clear_drawingplane=0;
    GlobalAppState.switch_cone_cyl=switch_cone_cyl_is_cone;
    GlobalAppState.switch_grid=switch_grid_is_white;
    //Initialize GUI
    int screenresx=0;
    int screenresy=0;
    glfwGetWindowSize(MainWindow,&screenresx,&screenresy);
    struct GObject* Guip=ui_generate_gui("guiLayout.xml",screenresx,screenresy);
    struct GObject* BGplanep=ui_backgroundplane_init(window_width,window_height);
    Canvasp=ui_drawingplane_init(window_width,window_height);

    //Create Default Shader
    BGplanep->ShaderID=glCreateProgram();
    glAttachShader(BGplanep->ShaderID,CompileShaderFromFile("./res/frag_default.glsl",GL_FRAGMENT_SHADER));
    glAttachShader(BGplanep->ShaderID,CompileShaderFromFile("./res/vertex_default.glsl",GL_VERTEX_SHADER));
    glLinkProgram(BGplanep->ShaderID);
    Canvasp->ShaderID=BGplanep->ShaderID;
    Guip->ShaderID=Canvasp->ShaderID;
    glDepthFunc(GL_GREATER);
    glClearDepth(-1.0f);    //Defaults to one and prevents the code from working
    //glDepthRange(1.0,-1.0);
    int LastTouchedPointx=-1;
    int LastTouchedPointy;
    int LastMappedPointx=0;
    int LastMappedPointy=0;
    /*ui_drawingplane_write(Canvasp,20,0x888888ff,100,4096-300,2160,1440);
        ui_drawingplane_write(Canvasp,100,0x888888ff,300,4096-300,2160,1440);
        connect_points(Canvasp->TextureData,Canvasp->TextureSidelength,255,255,0,100,4096-300,20,300,4096-300,100);
*/

    /*for(int i=0;i<8;i++){
        int dey=200*sin(M_PI/4*i);//+0.5);
        int dex=200*cos(M_PI/4*i);//+0.5);
        connect_pixels_aliased_oriented(Canvasp->TextureData,Canvasp->TextureSidelength,0,0,0,1400,4096-400,1400+dex,4096-(400+dey));
    }*/




    while(!glfwWindowShouldClose(MainWindow)){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //lets gl clear the depth and color buffer
        //DrawDrawingPlane
        glEnable(GL_BLEND);  //Transparency on
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        ui_backgroundplane_draw(BGplanep);
        ui_drawingplane_draw(Canvasp);
        //ui_gui_draw(Guip); //CORONA
        //Swap Buffers
        glFinish();
        glfwSwapBuffers(MainWindow);
        //Process Events
        glfwPollEvents();
        //processCommandXml(0);


        if(drawMode==drawModeReload){
            GlobalAppState.clear_drawingplane=1;
            drawMode=drawModeInactive;
            //create <clear/>
            struct xmlTreeElement* clearTag=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            clearTag->name=stringToUTF32Dynlist("clear");
            clearTag->attributes=0;
            clearTag->parent=globalCommandBufferOutp;
            clearTag->content=0;
            clearTag->type=xmltype_tag;
            append_DynamicList(&globalCommandBufferOutp->content,&clearTag,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);

            //create parent CommandBufferElement
            struct xmlTreeElement* XmlOuputElmntp=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            XmlOuputElmntp->name=0;
            XmlOuputElmntp->attributes=0;
            XmlOuputElmntp->content=create_DynamicList(sizeof(struct xmlTreeElement**),1,xmltype_tag);
            XmlOuputElmntp->parent=0;
            XmlOuputElmntp->type=xmltype_tag;
            ((struct xmlTreeElement**)XmlOuputElmntp->content->items)[0]=globalCommandBufferOutp;
            globalCommandBufferOutp->parent=XmlOuputElmntp;
            writeXML(xmlFileCommandOutp,XmlOuputElmntp);
            fclose(xmlFileCommandOutp);
            dprintf(DBGT_INFO,"Writing out file finished");
            exit(0);
        }else if(drawMode==drawModeInactive){
            processCommandXml(CommandInput,screenresx,screenresy);
        }

        if(GlobalAppState.clear_drawingplane){
            ui_drawingplane_clear(Canvasp,screenresx,screenresy);
            GlobalAppState.clear_drawingplane=0;
        }
        if(GlobalAppState.switch_cone_cyl==switch_cone_cyl_to_cone||GlobalAppState.switch_cone_cyl==switch_cone_cyl_to_cyl){
            glActiveTexture(GL_TEXTURE0+BGplanep->TextureUnitIndex);
            glBindTexture(GL_TEXTURE_2D,BGplanep->TextureBufferID);
            GlobalAppState.switch_cone_cyl-=2;
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,BGplanep->TextureSidelength,BGplanep->TextureSidelength,0, GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,((uint8_t**)(BGplanep->TexturePointerDLp->items))[GlobalAppState.switch_cone_cyl+GlobalAppState.switch_grid]);
        }
        if(GlobalAppState.switch_grid==switch_grid_to_white||GlobalAppState.switch_grid==switch_grid_to_color||GlobalAppState.switch_grid==switch_grid_to_pic){
            glActiveTexture(GL_TEXTURE0+BGplanep->TextureUnitIndex);
            glBindTexture(GL_TEXTURE_2D,BGplanep->TextureBufferID);
            GlobalAppState.switch_grid--;
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,BGplanep->TextureSidelength,BGplanep->TextureSidelength,0, GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,((uint8_t**)(BGplanep->TexturePointerDLp->items))[GlobalAppState.switch_cone_cyl+GlobalAppState.switch_grid]);
        }

        if(draw_enabled){
            double xpos=0;
            double ypos=0;
            glfwGetCursorPos(MainWindow,&xpos,&ypos);
            int screenresx=0;
            int screenresy=0;
            glfwGetWindowSize(MainWindow,&screenresx,&screenresy);
            uint32_t touchedpix_x=(uint32_t)floor(xpos);
            uint32_t touchedpix_y=(uint32_t)floor(ypos);
            if(xpos>0&&ypos>0&&ypos<=screenresy&&xpos<=screenresx){
                //Draw point
                //Format is abgr
                int32_t mappedpoint_pix_x=-1;
                int32_t mappedpoint_pix_y=-1;
                uint32_t could_map=mapPoints(&mappedpoint_pix_x,&mappedpoint_pix_y,touchedpix_x,screenresy-touchedpix_y,screenresx,screenresy,widthMM,heightMM);

                if(LastTouchedPointx!=-1){
                    //Draw non mapped lines
                    //DrawLine between points
                    connect_points(((uint8_t**)(Canvasp->TexturePointerDLp->items))[0],Canvasp->TextureSidelength,GlobalAppState.color,LastTouchedPointx,Canvasp->TextureSidelength-LastTouchedPointy,GlobalAppState.penrad,touchedpix_x,Canvasp->TextureSidelength-touchedpix_y,GlobalAppState.penrad);
                    ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,touchedpix_x,Canvasp->TextureSidelength-touchedpix_y,screenresx,screenresy);

                    dprintf(DBGT_INFO,"%d,%d,%d\n",LastTouchedPointx,LastTouchedPointy,touchedpix_x);
                    if((could_map)&&(LastMappedPointx!=-1)&&(LastMappedPointy!=-1)){
                        connect_points(((uint8_t**)(Canvasp->TexturePointerDLp->items))[0],Canvasp->TextureSidelength,GlobalAppState.color,mappedpoint_pix_x,Canvasp->TextureSidelength-screenresy+mappedpoint_pix_y,GlobalAppState.penrad,LastMappedPointx,Canvasp->TextureSidelength-screenresy+LastMappedPointy,GlobalAppState.penrad);
                        ui_drawingplane_write(Canvasp,GlobalAppState.penrad,GlobalAppState.color,mappedpoint_pix_x,Canvasp->TextureSidelength-screenresy+mappedpoint_pix_y,screenresx,screenresy);
                    }
                }

                LastMappedPointx=mappedpoint_pix_x;
                LastMappedPointy=mappedpoint_pix_y;
                LastTouchedPointx=touchedpix_x;
                LastTouchedPointy=touchedpix_y;
            }
        }else{
            LastTouchedPointx=-1;
        }
    }
    return 0;
}

struct GObject* ui_backgroundplane_init(uint32_t screenresx, uint32_t screenresy){
    struct GObject* genBackgroundPlane=(struct GObject*) malloc(sizeof(struct GObject));
    genBackgroundPlane->TextureUnitIndex=0;
    //Make the texture size to be a power of 2, so first find the bigger one of screenres
    uint32_t largestScreensize;
    if(screenresx<screenresy){
        largestScreensize=screenresy;
    }else{
        largestScreensize=screenresx;
    }
    //Find out wich one is the highest enabeled bit
    uint32_t bitshift=0;
    while(!(largestScreensize& 1<<(31-bitshift))  &&  bitshift<32){
        bitshift++;
    }
    genBackgroundPlane->TextureSidelength=1<<(32-bitshift);    //enable next higher bit, so texture is larger than screen area
    //Generate Coordinates for the triangles
    glGenVertexArrays(1,&(genBackgroundPlane->VertexArrayID));
    glBindVertexArray(genBackgroundPlane->VertexArrayID);
    glGenBuffers(1,&(genBackgroundPlane->VertexBufferID));
    glBindBuffer(GL_ARRAY_BUFFER,genBackgroundPlane->VertexBufferID);
    const GLfloat tempVertexData[]={
        -1.0f,1.0f-2*genBackgroundPlane->TextureSidelength/(float)screenresy, -0.5f,  //First Triangle
        -1.0f+2*genBackgroundPlane->TextureSidelength/(float)screenresx,1.0f-2*genBackgroundPlane->TextureSidelength/(float)screenresy, -0.5f,
        -1.0f, 1.0f, -0.5f,
        -1.0f, 1.0f, -0.5f,  //Second Triangle
        -1.0f+2*genBackgroundPlane->TextureSidelength/(float)screenresx,1.0f-2*genBackgroundPlane->TextureSidelength/(float)screenresy, -0.5f,
        -1.0f+2*genBackgroundPlane->TextureSidelength/(float)screenresx, 1.0f, -0.5f,

         //UV
         0.0f,0.0f,
         1.0f,0.0f,
         0.0f,1.0f,

         0.0f,1.0f,
         1.0f,0.0f,
         1.0f,1.0f

    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(tempVertexData),tempVertexData, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(float) * 18));
    glEnableVertexAttribArray(1);
    //Generate Texture

    genBackgroundPlane->TexturePointerDLp=0;

    //ui_drawingplane_fillwhite(genBackgroundPlane);
    //Asumes programm runs in fullscreen
    int textureNum=0;
    uint8_t* textureData=(uint8_t*)malloc(genBackgroundPlane->TextureSidelength*genBackgroundPlane->TextureSidelength*4*sizeof(uint8_t));
    append_DynamicList(&(genBackgroundPlane->TexturePointerDLp),&textureData,sizeof(uint8_t*),ListType_TexturePointer);
    projection_gen_largeConeGrid(projection_gen_mode_grid_white,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    projection_gen_miniConeGrid(projection_gen_mode_grid_white,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    textureNum++;

    textureData=(uint8_t*)malloc(genBackgroundPlane->TextureSidelength*genBackgroundPlane->TextureSidelength*4*sizeof(uint8_t));
    append_DynamicList(&(genBackgroundPlane->TexturePointerDLp),&textureData,sizeof(uint8_t*),ListType_TexturePointer);
    projection_gen_largeCylGrid(projection_gen_mode_grid_white,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    projection_gen_miniCylGrid(projection_gen_mode_grid_white,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    textureNum++;

    textureData=(uint8_t*)malloc(genBackgroundPlane->TextureSidelength*genBackgroundPlane->TextureSidelength*4*sizeof(uint8_t));
    append_DynamicList(&(genBackgroundPlane->TexturePointerDLp),&textureData,sizeof(uint8_t*),ListType_TexturePointer);
    projection_gen_largeConeGrid(projection_gen_mode_grid_color,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    projection_gen_miniConeGrid(projection_gen_mode_grid_color,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    textureNum++;

    textureData=(uint8_t*)malloc(genBackgroundPlane->TextureSidelength*genBackgroundPlane->TextureSidelength*4*sizeof(uint8_t));
    append_DynamicList(&(genBackgroundPlane->TexturePointerDLp),&textureData,sizeof(uint8_t*),ListType_TexturePointer);
    projection_gen_largeCylGrid(projection_gen_mode_grid_color,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    projection_gen_miniCylGrid(projection_gen_mode_grid_color,0,0,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
    textureNum++;

    FILE* xmlFilep=fopen("./res/images.xml","rb");
    struct xmlTreeElement* xmlrootp=0;


    readXML(xmlFilep,&xmlrootp);



    struct DynamicList* imagePathDLp=getSubelementsWithName(stringToUTF32Dynlist("image"),xmlrootp,3);
    for(int imagenum=0;imagenum<imagePathDLp->itemcnt;imagenum++){
        struct DynamicList* PathDLp=getValueFromKeyName((((struct xmlTreeElement**)(imagePathDLp->items))[imagenum])->attributes,stringToUTF32Dynlist("path"));
        struct DynamicList* PicSidelengthDLp=getValueFromKeyName((((struct xmlTreeElement**)(imagePathDLp->items))[imagenum])->attributes,stringToUTF32Dynlist("res"));
        if(!PathDLp){
            dprintf(DBGT_ERROR,"Image is missing path attribute");
            exit(1);
        }
        if(!PicSidelengthDLp){
            dprintf(DBGT_ERROR,"Image is missing res attribute");
            exit(1);
        }
        char* imagepathp=utf32dynlist_to_string(PathDLp);
        struct DynamicList* PicResDLp=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),PicSidelengthDLp);
        if(!PicResDLp){
            dprintf(DBGT_ERROR,"Resolution invalid");
            exit(1);
        }
        uint8_t* imagedatap=read_bmp(imagepathp);
        textureData=(uint8_t*)malloc(genBackgroundPlane->TextureSidelength*genBackgroundPlane->TextureSidelength*4*sizeof(uint8_t));
        append_DynamicList(&(genBackgroundPlane->TexturePointerDLp),&textureData,sizeof(uint8_t*),ListType_TexturePointer);
        projection_gen_largeConeGrid(projection_gen_mode_grid_pic,imagedatap,((int*)(PicResDLp->items))[0],((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
        projection_gen_miniConeGrid(projection_gen_mode_grid_pic,imagedatap,((int*)(PicResDLp->items))[0],((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
        dprintf(DBGT_INFO,"gentexNum %d",textureNum);
        textureNum++;

        textureData=(uint8_t*)malloc(genBackgroundPlane->TextureSidelength*genBackgroundPlane->TextureSidelength*4*sizeof(uint8_t));
        append_DynamicList(&(genBackgroundPlane->TexturePointerDLp),&textureData,sizeof(uint8_t*),ListType_TexturePointer);
        projection_gen_largeCylGrid(projection_gen_mode_grid_pic,imagedatap,((int*)(PicResDLp->items))[0],((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
        projection_gen_miniCylGrid(projection_gen_mode_grid_pic,imagedatap,((int*)(PicResDLp->items))[0],((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum],genBackgroundPlane->TextureSidelength,screenresx,screenresy,widthMM,heightMM);
        textureNum++;

        free(imagepathp);
        free(imagedatap);
    }


    glActiveTexture(GL_TEXTURE0+genBackgroundPlane->TextureUnitIndex);
    glGenTextures(1,&(genBackgroundPlane->TextureBufferID));
    glBindTexture(GL_TEXTURE_2D,genBackgroundPlane->TextureBufferID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    textureNum=GlobalAppState.switch_cone_cyl;
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,genBackgroundPlane->TextureSidelength,genBackgroundPlane->TextureSidelength,0, GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,((uint8_t**)(genBackgroundPlane->TexturePointerDLp->items))[textureNum]);
    //Unbind VertexArray
    glBindVertexArray(0);
    return genBackgroundPlane;
};

void ui_backgroundplane_draw(struct GObject* IN_BGPlane){
    glBindVertexArray(IN_BGPlane->VertexArrayID);
    //Set shader Variables
    //glActiveTexture(IN_BGPlane->TextureUnitIndex); //not needed, shader have access  to all texture units
    //glBindTexture(GL_TEXTURE_2D,IN_BGPlane->TextureBufferID);
    glUseProgram(IN_BGPlane->ShaderID);
    glUniform1i(glGetUniformLocation(IN_BGPlane->ShaderID, "texture0"),IN_BGPlane->TextureUnitIndex);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}


struct GObject* ui_drawingplane_init(uint32_t screenresx,uint32_t screenresy){
    struct GObject* genDrawPlane=(struct GObject*) malloc(sizeof(struct GObject));
    genDrawPlane->TextureUnitIndex=1;
    //Make the texture size to be a power of 2, so first find the bigger one of screenres
    uint32_t largestScreensize;
    if(screenresx<screenresy){
        largestScreensize=screenresy;
    }else{
        largestScreensize=screenresx;
    }
    //Find out wich one is the highest enabeled bit
    uint32_t bitshift=0;
    while(!(largestScreensize& 1<<(31-bitshift))  &&  bitshift<32){
        bitshift++;
    }
    genDrawPlane->TextureSidelength=1<<(32-bitshift);    //enable next higher bit, so texture is larger than screen area
    dprintf(DBGT_INFO,"%d",genDrawPlane->TextureSidelength);

    //Generate Coordinates for the triangles
    glGenVertexArrays(1,&(genDrawPlane->VertexArrayID));
    glBindVertexArray(genDrawPlane->VertexArrayID);
    glGenBuffers(1,&(genDrawPlane->VertexBufferID));
    glBindBuffer(GL_ARRAY_BUFFER,genDrawPlane->VertexBufferID);
    const GLfloat tempVertexData[]={
        -1.0f,1.0f-2*genDrawPlane->TextureSidelength/(float)screenresy, 0.0f,  //First Triangle
        -1.0f+2*genDrawPlane->TextureSidelength/(float)screenresx,1.0f-2*genDrawPlane->TextureSidelength/(float)screenresy, 0.0f,
        -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f,  //Second Triangle
        -1.0f+2*genDrawPlane->TextureSidelength/(float)screenresx,1.0f-2*genDrawPlane->TextureSidelength/(float)screenresy, 0.0f,
        -1.0f+2*genDrawPlane->TextureSidelength/(float)screenresx, 1.0f, 0.0f,

         //UV
         0.0f,0.0f,
         1.0f,0.0f,
         0.0f,1.0f,

         0.0f,1.0f,
         1.0f,0.0f,
         1.0f,1.0f

    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(tempVertexData),tempVertexData, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(float) * 18));
    glEnableVertexAttribArray(1);
    //Generate Texture

    genDrawPlane->TexturePointerDLp=0;
    uint8_t* textureData=(uint8_t*)malloc(genDrawPlane->TextureSidelength*genDrawPlane->TextureSidelength*4*sizeof(uint8_t));
    append_DynamicList(&(genDrawPlane->TexturePointerDLp),&textureData,sizeof(uint8_t*),ListType_TexturePointer);

    ui_drawingplane_clear(genDrawPlane,screenresx,screenresy);
    glActiveTexture(GL_TEXTURE0+genDrawPlane->TextureUnitIndex);   //Select Texture unit (glBindTexture now uses this texture unit)
    glGenTextures(1,&(genDrawPlane->TextureBufferID));
    glBindTexture(GL_TEXTURE_2D,genDrawPlane->TextureBufferID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,genDrawPlane->TextureSidelength,genDrawPlane->TextureSidelength,0, GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,((uint8_t**)(genDrawPlane->TexturePointerDLp->items))[0]);

    //Unbind VertexArray
    glBindVertexArray(0);
    return genDrawPlane;
}

void ui_drawingplane_draw(struct GObject* IN_DrawingPlane){
    glBindVertexArray(IN_DrawingPlane->VertexArrayID);
    //Set shader Variables
    //glActiveTexture(TEX0+IN_DrawingPlane->TextureUnitIndex);
    glUseProgram(IN_DrawingPlane->ShaderID);
    glUniform1i(glGetUniformLocation(IN_DrawingPlane->ShaderID, "texture0"),IN_DrawingPlane->TextureUnitIndex);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

void ui_drawingplane_clear(struct GObject* IN_DrawingPlane,uint32_t screenResX, uint32_t screenResY){
    memset(((uint8_t**)(IN_DrawingPlane->TexturePointerDLp->items))[0],0x00,4*sizeof(uint8_t)*IN_DrawingPlane->TextureSidelength*IN_DrawingPlane->TextureSidelength);
    ui_drawingplane_write(IN_DrawingPlane,0,0x00000000,0,0,screenResX,screenResY);
    //glActiveTexture(IN_DrawingPlane->VertexArrayID);
    //glBindTexture(GL_TEXTURE_2D,IN_DrawingPlane->TextureBufferID);
    //glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,IN_DrawingPlane->TextureSidelength,IN_DrawingPlane->TextureSidelength,0, GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,IN_DrawingPlane->TextureData);
}

void ui_drawingplane_write(struct GObject* IN_DrawingPlane, uint32_t penrad, uint32_t colorRGBA, uint32_t posOnTextureX, uint32_t posOnTextureY, uint32_t screenResX, uint32_t screenResY){
    set_point(((uint8_t**)(IN_DrawingPlane->TexturePointerDLp->items))[0],IN_DrawingPlane->TextureSidelength,colorRGBA,posOnTextureX,posOnTextureY,penrad,screenResX);
    glActiveTexture(GL_TEXTURE0+IN_DrawingPlane->TextureUnitIndex);   //Select Texture unit (glBindTexture now uses this texture unit)
    glBindTexture(GL_TEXTURE_2D,IN_DrawingPlane->TextureBufferID);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,IN_DrawingPlane->TextureSidelength,IN_DrawingPlane->TextureSidelength,0, GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,((uint8_t**)(IN_DrawingPlane->TexturePointerDLp->items))[0]);
}

struct GObject* ui_generate_gui(char* xmlFilepath,uint32_t screenresx,uint32_t screenresy){
    struct GObject* genGui=(struct GObject*)malloc(sizeof(struct GObject));
    struct xmlTreeElement* guixmlp=0;
    FILE* xmlFile=fopen("./res/guiLayout.xml","rb");
    printf("Parsing XML\n");
    readXML(xmlFile,&guixmlp);
    dprintf(DBGT_INFO,"Trying to get texture path1");
    struct DynamicList* texture=getSubelementsWithName(stringToUTF32Dynlist("texture"),guixmlp,1);
    if((!texture)||texture->itemcnt!=1){
        dprintf(DBGT_ERROR,"More or less than one texture specified in xml");
        return 0;
    }
    struct DynamicList* corresp_val=getValueFromKeyName(((struct DynamicList*)((struct xmlTreeElement**)texture->items)[0]->attributes),stringToUTF32Dynlist("path"));
    if(!corresp_val){
        dprintf(DBGT_ERROR,"Texture is missing a path attribute");
        return 0;
    }
    uint8_t* GUITexturep=read_bmp(utf32dynlist_to_string(corresp_val));
    if(!GUITexturep){
        dprintf(DBGT_ERROR,"gui.bmp is missing from disk");
        exit(1);
    }
    delete_DynList(corresp_val);
    corresp_val=getValueFromKeyName(((struct DynamicList*)((struct xmlTreeElement**)texture->items)[0]->attributes),stringToUTF32Dynlist("resolution"));
    struct DynamicList* listOfInts=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),corresp_val);
        //,createCharMatchList(4,'e','e','E','E'),createCharMatchList(4,',',',','.','.')
    if(!listOfInts){
        dprintf(DBGT_ERROR,"resolution attribute did not contain a valid int");
        return 0;
    }else if(listOfInts->itemcnt!=1){
        dprintf(DBGT_ERROR,"Got %d ints in resolution, but expected 1",listOfInts->itemcnt);
    }
    int TextureResolution=((int*)listOfInts->items)[0];
    dprintf(DBGT_INFO,"Texture Resolution is %d",TextureResolution);
    //generate, bind, and update texture
    genGui->TexturePointerDLp=0;
    append_DynamicList(&(genGui->TexturePointerDLp),&GUITexturep,sizeof(uint8_t*),ListType_TexturePointer);
    genGui->TextureSidelength=TextureResolution;
    genGui->TextureUnitIndex=2; //TODO don't hardcode?! for all of them, make function that, wenn called spits out a number and increments static
    glActiveTexture(GL_TEXTURE0+genGui->TextureUnitIndex);   //Select Texture unit (glBindTexture now uses this texture unit)
    glGenTextures(1,&(genGui->TextureBufferID));
    glBindTexture(GL_TEXTURE_2D,genGui->TextureBufferID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,genGui->TextureSidelength,genGui->TextureSidelength,0, GL_RGBA,GL_UNSIGNED_INT_8_8_8_8,((uint8_t**)(genGui->TexturePointerDLp->items))[0]);

    //Create Texture and Vertex coordinates for all gui elements
    glGenVertexArrays(1,&(genGui->VertexArrayID));
    glBindVertexArray(genGui->VertexArrayID);
    glGenBuffers(1,&(genGui->VertexBufferID));
    glBindBuffer(GL_ARRAY_BUFFER,genGui->VertexBufferID);
    struct DynamicList* scenelist=getSubelementsWithName(stringToUTF32Dynlist("scene"),guixmlp,1);
    if(!scenelist){
        dprintf(DBGT_ERROR,"No element with name scene found");
        return 0;
    }
    int numscene=0;
    for(;numscene<scenelist->itemcnt;numscene++){
        struct xmlTreeElement* currentScene=((struct xmlTreeElement**)(scenelist->items))[numscene];
        struct DynamicList* scene_id=getValueFromKeyName(currentScene->attributes,stringToUTF32Dynlist("id"));
        if(!scene_id){
            dprintf(DBGT_ERROR,"No scene id for scene number %d",numscene);
            return 0;
        }
        if(compareEqualDynamicUTF32List(scene_id,stringToUTF32Dynlist("0"))){
            break;
        }
    }
    if(numscene==scenelist->itemcnt){
        dprintf(DBGT_INFO,"No primary scene found");
        return 0;
    }
    struct DynamicList* AllLayerDLp=genVTXforScene(((struct xmlTreeElement**)(scenelist->items))[numscene],screenresx,screenresy,genGui->TextureSidelength);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (sizeof(float) * 5), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (sizeof(float) * 5), (GLvoid*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    AllLayerInfop=AllLayerDLp;
    glBindVertexArray(0);
    updateGUIvtx(genGui);
    updateGUIvtx(0);

    return genGui;
    //read_bmp();
}

void ui_gui_draw(struct GObject* IN_GUI){
    glBindVertexArray(IN_GUI->VertexArrayID);
    //Set shader Variables
    //glActiveTexture(IN_DrawingPlane->TextureUnitIndex);
    glUseProgram(IN_GUI->ShaderID);
    glUniform1i(glGetUniformLocation(IN_GUI->ShaderID, "texture0"),IN_GUI->TextureUnitIndex);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES,0,IN_GUI->Vertices);
    glBindVertexArray(0);
}

void glfw_error_callback(int error, const char* description) {
    dprintf(DBGT_ERROR,"GLFW: code %d, \n desc: %s",error,description);
}

void APIENTRY openglCallbackFunction(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    printf("Error in opengl occured!\n");
    printf("Message: %s\n", message);
    printf("type or error: ");
    switch(type) {
    case GL_DEBUG_TYPE_ERROR:
        printf("ERROR");
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        printf("DEPRECATED_BEHAVIOR");
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        printf("UNDEFINED_BEHAVIOR");
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        printf("PORTABILITY");
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        printf("PERFORMANCE");
        break;
    case GL_DEBUG_TYPE_OTHER:
        printf("OTHER");
        break;
    }
    printf("\nId:%d \n", id);
    printf("Severity:");
    switch(severity) {
    case GL_DEBUG_SEVERITY_LOW:
        printf("LOW");
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        printf("MEDIUM");
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        printf("HIGH");
        break;
    }
    printf("\nGLerror end\n");
}

