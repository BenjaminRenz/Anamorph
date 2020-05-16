#include "xmlReader/xmlReader.h"
#include "xmlReader/stringutils.h"
#include "debug/debug.h"
#include "gui/gui.h"
#include <stdlib.h>     //for exit
#include "gui/guiUserDefined.h" //to know which function id's have been asigned
#define hvdivnestdepth 20

uint32_t chkfkt_getActiveElements(struct xmlTreeElement* xmlELMNTtoTest){
    if(!compareEqualDynamicUTF32List(stringToUTF32Dynlist("element"),xmlELMNTtoTest->name)){
        return 0;
    }
    dprintf(DBGT_INFO,"found element");
    struct DynamicList* activeString=getValueFromKeyName(xmlELMNTtoTest->attributes,stringToUTF32Dynlist("active"));
    if(!activeString){
        dprintf(DBGT_INFO,"Found Element without active defined");
        return 0;
    }
    if(!compareEqualDynamicUTF32List(activeString,stringToUTF32Dynlist("1"))){
        //element is marked as inactive
        return 0;
    }
    return 1;
}

void shrinkBoarderAsprAndAlign(float* BoxLowerLeftXp,float* BoxLowerLeftYp,float* BoxUpperRightXp,float* BoxUpperRightYp,struct xmlTreeElement* CurrentXMLElement){
    //check if aspect ratio attribute has been defined
    struct DynamicList* aspRatVal=getValueFromKeyName(CurrentXMLElement->attributes,stringToUTF32Dynlist("aspr"));
    if(aspRatVal){
        struct DynamicList* alignVal=getValueFromKeyName(CurrentXMLElement->attributes,stringToUTF32Dynlist("align"));
        struct DynamicList* aspRatFloats=utf32dynlist_to_floats(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),aspRatVal);
        float CurrentElementAspectR=((float*)(aspRatFloats->items))[0];
        enum{match_res_alignatt_l=0,match_res_alignatt_t=1,match_res_alignatt_r=2,match_res_alignatt_b=3,match_res_alignatt_illegal=4};
        struct DynamicList* mw_alignattp=createMultiCharMatchList(5,
          createCharMatchList(2,'l','l'),
          createCharMatchList(2,'t','t'),
          createCharMatchList(2,'r','r'),
          createCharMatchList(2,'b','b'),
          createCharMatchList(2,0x0000,0xffff)
        );
        float deltax=(*BoxUpperRightXp)-(*BoxLowerLeftXp);
        float deltay=(*BoxUpperRightYp)-(*BoxLowerLeftYp);
        float scalerforSmallerSide=deltax/(CurrentElementAspectR*deltay);
        if(scalerforSmallerSide>1.0){
          //Element size is limited by it's height
          if(alignVal){
              //left or right aligned
              uint32_t matchIndex=0;
              getOffsetUntil(alignVal->items,alignVal->itemcnt,mw_alignattp,&matchIndex);
              if(matchIndex==match_res_alignatt_illegal){
                  dprintf(DBGT_ERROR,"Illegal character in alignment attribute");
                  exit(1);
                }else if(matchIndex==match_res_alignatt_l){
                  (*BoxUpperRightXp)=(*BoxLowerLeftXp)+deltax/scalerforSmallerSide;
              }else if(matchIndex==match_res_alignatt_r){
                  (*BoxLowerLeftXp)=(*BoxUpperRightXp)-deltax/scalerforSmallerSide;
              }else{
                  //ignoge t/b alignment, because bound by height
                  (*BoxUpperRightXp)=(*BoxLowerLeftXp)+(1+1/scalerforSmallerSide)*deltax/2;
                  (*BoxLowerLeftXp)=(*BoxLowerLeftXp)+(1-1/scalerforSmallerSide)*deltax/2;
              }
          }else{
              //centering
              (*BoxUpperRightXp)=(*BoxLowerLeftXp)+(1+1/scalerforSmallerSide)*deltax/2;
              (*BoxLowerLeftXp)=(*BoxLowerLeftXp)+(1-1/scalerforSmallerSide)*deltax/2;
          }
        }else{
          //element size is limited by it's width
          if(alignVal){
              //left or right aligned
              uint32_t matchIndex=0;
              getOffsetUntil(alignVal->items,alignVal->itemcnt,mw_alignattp,&matchIndex);
              if(matchIndex==match_res_alignatt_illegal){
                  dprintf(DBGT_ERROR,"Illegal character in alignment attribute");
                  exit(1);
              }else if(matchIndex==match_res_alignatt_b){
                  (*BoxUpperRightYp)=(*BoxLowerLeftYp)+deltay*scalerforSmallerSide;
              }else if(matchIndex==match_res_alignatt_t){
                  (*BoxLowerLeftYp)=(*BoxUpperRightYp)-deltay*scalerforSmallerSide;
              }else{
                  //ignoge t/b alignment, because bound by height
                  (*BoxUpperRightYp)=(*BoxLowerLeftYp)+(1+scalerforSmallerSide)*deltay/2;
                  (*BoxLowerLeftYp)=(*BoxLowerLeftYp)+(1-scalerforSmallerSide)*deltay/2;
              }
          }else{
              //centering
              (*BoxUpperRightYp)=(*BoxLowerLeftYp)+(1+scalerforSmallerSide)*deltay/2;
              (*BoxLowerLeftYp)=(*BoxLowerLeftYp)+(1-scalerforSmallerSide)*deltay/2;
          }
        }
        delete_DynList(mw_alignattp);
    }
}

struct DynamicList* genVTXforScene(struct xmlTreeElement* scene, int screenresx, int screenresy, uint32_t texturesize){
    dprintf(DBGT_INFO,"Try to find element with name layer");
    struct DynamicList* allLayers=getSubelementsWithName(stringToUTF32Dynlist("layer"),scene,1);
    struct DynamicList* AllLayersReturnDLp=0;
    for(int numLayer=0;numLayer<allLayers->itemcnt;numLayer++){
        struct DynamicList* vertAndUvsDLp=0;
        struct DynamicList* ColliderDLp=0;
        struct xmlTreeElement* CurrentLayer=((struct xmlTreeElement**)(allLayers->items))[numLayer];
        struct DynamicList* allActiveElements=getSubelementsWithCharacteristic(&chkfkt_getActiveElements,CurrentLayer,hvdivnestdepth);
        for(int numActiveElement=0;numActiveElement<allActiveElements->itemcnt;numActiveElement++){
            float BoxLowerLeftX=0.0f;
            float BoxLowerLeftY=0.0f;
            float BoxUpperRightX=(float)screenresx;
            float BoxUpperRightY=(float)screenresy;
            struct xmlTreeElement* CurrentElement=((struct xmlTreeElement**)(allActiveElements->items))[numActiveElement];
            struct xmlTreeElement* tmpXMLElementp=CurrentElement->parent; //used to walk upward in the xml tree over all v and hdiv tags
            while(tmpXMLElementp!=CurrentLayer){
                if(compareEqualDynamicUTF32List(tmpXMLElementp->name,stringToUTF32Dynlist("vdiv"))){
                    //Get all vdivs before vdiv which are children of the parent of vdiv
                    struct xmlTreeElement* tmpParentOfVdiv=tmpXMLElementp->parent;
                    double otherVdivSumPercDouble=0.0;
                    for(int currentChildIndex=0;tmpXMLElementp!=((struct xmlTreeElement**)(tmpParentOfVdiv->content->items))[currentChildIndex];currentChildIndex++){
                        struct DynamicList* otherPercDoubleVal=getValueFromKeyName((((struct xmlTreeElement**)(tmpParentOfVdiv->content->items))[currentChildIndex])->attributes,stringToUTF32Dynlist("perc"));
                        struct DynamicList* listOfOtherPercDouble=utf32dynlist_to_doubles(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),otherPercDoubleVal);
                        //Todo checks for nullpointer and so on
                        otherVdivSumPercDouble+=((double*)(listOfOtherPercDouble->items))[0];
                        delete_DynList(listOfOtherPercDouble);
                    }
                    //Get percentage of current vdiv
                    struct DynamicList* percDoubleVal=getValueFromKeyName(tmpXMLElementp->attributes,stringToUTF32Dynlist("perc"));
                    struct DynamicList* listOfCurPercDoubleDLp=utf32dynlist_to_doubles(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),percDoubleVal);
                    double currentVdivPercDouble=((double*)(listOfCurPercDoubleDLp->items))[0];
                    //Cleanup
                    delete_DynList(listOfCurPercDoubleDLp);
                    //Process the new resulting coordinates
                    float deltaY=BoxUpperRightY-BoxLowerLeftY;
                    BoxLowerLeftY+=otherVdivSumPercDouble*deltaY;
                    BoxUpperRightY-=(1.0f-(otherVdivSumPercDouble+currentVdivPercDouble))*deltaY;
                    //aspr and align
                    shrinkBoarderAsprAndAlign(&BoxLowerLeftX,&BoxLowerLeftY,&BoxUpperRightX,&BoxUpperRightY,tmpXMLElementp);
                }else if(compareEqualDynamicUTF32List(tmpXMLElementp->name,stringToUTF32Dynlist("hdiv"))){
                    //Get all hdivs before hdiv which are children of the parent of hdiv
                    struct xmlTreeElement* tmpParentOfVdiv=tmpXMLElementp->parent;
                    double otherHdivSumPercDouble=0.0;
                    for(int currentChildIndex=0;tmpXMLElementp!=((struct xmlTreeElement**)(tmpParentOfVdiv->content->items))[currentChildIndex];currentChildIndex++){
                        struct DynamicList* otherPercDoubleVal=getValueFromKeyName((((struct xmlTreeElement**)(tmpParentOfVdiv->content->items))[currentChildIndex])->attributes,stringToUTF32Dynlist("perc"));
                        struct DynamicList* listOfOtherPercDouble=utf32dynlist_to_doubles(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),otherPercDoubleVal);
                        //Todo checks for nullpointer and so on
                        otherHdivSumPercDouble+=((double*)(listOfOtherPercDouble->items))[0];
                        delete_DynList(listOfOtherPercDouble);
                    }
                    //Get percentage of current hdiv
                    struct DynamicList* percDoubleVal=getValueFromKeyName(tmpXMLElementp->attributes,stringToUTF32Dynlist("perc"));
                    struct DynamicList* listOfCurPercDoubleDLp=utf32dynlist_to_doubles(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(4,',',',','.','.'),percDoubleVal);
                    double currentHdivPercDouble=((double*)(listOfCurPercDoubleDLp->items))[0];
                    //Cleanup
                    delete_DynList(listOfCurPercDoubleDLp);
                    //Process the new resulting coordinates
                    float deltaX=BoxUpperRightX-BoxLowerLeftX;
                    BoxLowerLeftX+=otherHdivSumPercDouble*deltaX;
                    BoxUpperRightX-=(1.0f-(otherHdivSumPercDouble+currentHdivPercDouble))*deltaX;
                    //aspr and align
                    shrinkBoarderAsprAndAlign(&BoxLowerLeftX,&BoxLowerLeftY,&BoxUpperRightX,&BoxUpperRightY,tmpXMLElementp);
                }else{
                    dprintf(DBGT_ERROR,"Element was wrapped in something else than hdiv or vdiv");
                    return 0;
                }
                //Move up in tree
                tmpXMLElementp=tmpXMLElementp->parent;
            }
            //Parse attributes of current element
            dprintf(DBGT_INFO,"Generated coordinates %f,%f and %f,%f",BoxLowerLeftX,BoxLowerLeftY,BoxUpperRightX,BoxUpperRightY);
            struct DynamicList* zVal=getValueFromKeyName(CurrentElement->attributes,stringToUTF32Dynlist("z"));
            struct DynamicList* texcordVal=getValueFromKeyName(CurrentElement->attributes,stringToUTF32Dynlist("texcord"));
            //aspr and align
            shrinkBoarderAsprAndAlign(&BoxLowerLeftX,&BoxLowerLeftY,&BoxUpperRightX,&BoxUpperRightY,CurrentElement);
            //Parse collider attributes
            struct DynamicList* OnClickFPidVal=getValueFromKeyName(CurrentElement->attributes,stringToUTF32Dynlist("OnClickFPid"));
            struct DynamicList* OnHoldFPidVal=getValueFromKeyName(CurrentElement->attributes,stringToUTF32Dynlist("OnHoldFPid"));
            struct DynamicList* OnReleaseFPidVal=getValueFromKeyName(CurrentElement->attributes,stringToUTF32Dynlist("OnReleaseFPid"));
            //Calculate Collider
            struct Collider tempCollider;
            tempCollider.ColliderType=0;     //TODO Later for cicle collider
            tempCollider.colliderLL_x=BoxLowerLeftX;
            tempCollider.colliderLL_y=BoxLowerLeftY;
            tempCollider.colliderUR_x=BoxUpperRightX;
            tempCollider.colliderUR_y=BoxUpperRightY;
            if(OnClickFPidVal){
                struct DynamicList* OnClickID=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),OnClickFPidVal);
                if(!OnClickID){
                    dprintf(DBGT_INFO,"OnClick Id empty");
                    return 0;
                }
                if(((int*)(OnClickID->items))[0]>fparrayLength()){
                    dprintf(DBGT_ERROR,"OnClick Id references a functionId that is missing in the array");
                }
                tempCollider.onClickFPid=((int*)(OnClickID->items))[0];
            }else{
                tempCollider.onClickFPid=FP_CallbackNotSet;
            }
            if(OnHoldFPidVal){
                struct DynamicList* OnHoldID=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),OnHoldFPidVal);
                if(!OnHoldID){
                    dprintf(DBGT_INFO,"OnHold Id empty");
                    return 0;
                }
                 if(((int*)(OnHoldID->items))[0]>fparrayLength()){
                    dprintf(DBGT_ERROR,"OnClick Id references a functionId that is missing in the array");
                }
                tempCollider.onHoldFPid=((int*)(OnHoldID->items))[0];
            }else{
                tempCollider.onHoldFPid=FP_CallbackNotSet;
            }
            if(OnReleaseFPidVal){
                struct DynamicList* OnReleaseID=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),OnReleaseFPidVal);
                if(!OnReleaseID){
                    dprintf(DBGT_INFO,"OnRelease Id empty");
                    return 0;
                }
                 if(((int*)(OnReleaseID->items))[0]>fparrayLength()){
                    dprintf(DBGT_ERROR,"OnClick Id references a functionId that is missing in the array");
                }
                tempCollider.onReleaseFPid=((int*)(OnReleaseID->items))[0];
            }else{
                tempCollider.onReleaseFPid=FP_CallbackNotSet;
            }
            struct DynamicList* zfloatDLp=utf32dynlist_to_floats(createCharMatchList(2,' ',' '),createCharMatchList(2,'e','e'),createCharMatchList(2,'.','.'),zVal);


            tempCollider.collider_z=((float*)(zfloatDLp->items))[0];
            if(tempCollider.onClickFPid!=FP_CallbackNotSet||tempCollider.onHoldFPid!=FP_CallbackNotSet||tempCollider.onReleaseFPid!=FP_CallbackNotSet){
                //Do only append collider if it has at least one function attached to it
                ColliderDLp=append_DynamicList(ColliderDLp,&tempCollider,sizeof(struct Collider),ListType_Collider);
            }
            //map xy to glcoords (-1,1)
            dprintf(DBGT_INFO,"BBox1 %f,%f, %f,%f",BoxLowerLeftX,BoxLowerLeftY,BoxUpperRightX,BoxUpperRightY);
            dprintf(DBGT_INFO,"BBox2 %d,%d",screenresx,screenresy);
            BoxLowerLeftX=(BoxLowerLeftX/screenresx)*2-1;
            BoxLowerLeftY=(BoxLowerLeftY/screenresy)*2-1;
            BoxUpperRightX=(BoxUpperRightX/screenresx)*2-1;
            BoxUpperRightY=(BoxUpperRightY/screenresy)*2-1;

            dprintf(DBGT_INFO,"BBox %f,%f, %f,%f",BoxLowerLeftX,BoxLowerLeftY,BoxUpperRightX,BoxUpperRightY);
            //read uv
            if(!texcordVal){
                dprintf(DBGT_ERROR,"Missing texcoord");
                return 0;
            }
            struct DynamicList* textcoordDLp=utf32dynlist_to_floats(createCharMatchList(8,' ',' ','(','(',')',')',',',','),createCharMatchList(2,'e','e'),createCharMatchList(2,'.','.'),texcordVal);
            if(textcoordDLp->itemcnt!=4){
                dprintf(DBGT_ERROR,"Not exactly 4 texcoord found");
                return 0;
            }
            float uv_ll_x=((float*)(textcoordDLp->items))[0]/texturesize;
            float uv_ll_y=((float*)(textcoordDLp->items))[1]/texturesize;
            float uv_ur_x=((float*)(textcoordDLp->items))[2]/texturesize;
            float uv_ur_y=((float*)(textcoordDLp->items))[3]/texturesize;
            //process zVal
            if(!zVal){
                dprintf(DBGT_ERROR,"Missing Zval");
                return 0;
            }
            //Build triangles
            {
                //Vertex1_lowleft
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxLowerLeftX,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxLowerLeftY,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,(float*)zfloatDLp->items,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ll_x,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ll_y,sizeof(float),ListType_float);

                //Vertex2_lowright
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxUpperRightX,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxLowerLeftY,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,(float*)zfloatDLp->items,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ur_x,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ll_y,sizeof(float),ListType_float);

                //Vertex3_upleft
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxLowerLeftX,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxUpperRightY,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,(float*)zfloatDLp->items,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ll_x,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ur_y,sizeof(float),ListType_float);

                //Vertex2_lowright
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxUpperRightX,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxLowerLeftY,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,(float*)zfloatDLp->items,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ur_x,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ll_y,sizeof(float),ListType_float);

                //Vertex4_upright
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxUpperRightX,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxUpperRightY,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,(float*)zfloatDLp->items,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ur_x,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ur_y,sizeof(float),ListType_float);

                //Vertex3_upleft
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxLowerLeftX,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&BoxUpperRightY,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,(float*)zfloatDLp->items,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ll_x,sizeof(float),ListType_float);
                vertAndUvsDLp=append_DynamicList(vertAndUvsDLp,&uv_ur_y,sizeof(float),ListType_float);

            }
            delete_DynList(zfloatDLp);
        }

        //Create complete Layer info
        struct DynamicList* layer_id_valDLp=getValueFromKeyName(CurrentLayer->attributes,stringToUTF32Dynlist("id"));
        struct DynamicList* layeridDLp=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),layer_id_valDLp);
        if(!layeridDLp){
            dprintf(DBGT_ERROR,"Layer id missing");
            return 0;
        }
        struct DynamicList* layer_active_val=getValueFromKeyName(CurrentLayer->attributes,stringToUTF32Dynlist("active"));
        struct DynamicList* layer_active_int=utf32dynlist_to_ints(createCharMatchList(2,' ',' '),layer_active_val);
        if(!layer_active_int){
            dprintf(DBGT_ERROR,"Layer active attribut not specified");
            return 0;
        }
        if(((int*)(layer_active_int->items))[0]!=0&&((int*)(layer_active_int->items))[0]!=1){
            dprintf(DBGT_ERROR,"Layer active attribut not 0 or 1, was %d",((int*)(layer_active_int->items))[0]);
            return 0;
        }
        struct LayerCollection tmpLayerColl;
        tmpLayerColl.colliderDLp=ColliderDLp;
        tmpLayerColl.xyzuvDLp=vertAndUvsDLp;
        tmpLayerColl.id=((int*)(layeridDLp->items))[0];
        tmpLayerColl.active=((int*)(layer_active_int->items))[0];
        AllLayersReturnDLp=append_DynamicList(AllLayersReturnDLp,&tmpLayerColl,sizeof(struct LayerCollection),ListType_LayerCollection);
    }
    dprintf(DBGT_INFO,"Finished creation of vtx");
    return AllLayersReturnDLp;
}

