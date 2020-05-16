#ifndef GUI_H_INCLUDED
#define GUI_H_INCLUDED
#include <stdint.h>


#define FP_CallbackNotSet -1
struct xmlTreeElement;
struct DynamicList* genVTXforScene(struct xmlTreeElement* scene, int screenresx, int screenresy, uint32_t texturesize);

struct LayerCollection{
    int id;
    int active;
    struct DynamicList* xyzuvDLp;
    struct DynamicList* colliderDLp;       //List of type Listtype_Collider
};

struct Collider{
    uint32_t ColliderType;
    float colliderLL_x;
    float colliderLL_y;
    float colliderUR_x;
    float colliderUR_y;
    float collider_z;
    int32_t onClickFPid;
    int32_t onHoldFPid;
    int32_t onReleaseFPid;
};



#endif // GUI_H_INCLUDED
