#ifndef STRINGUTILS_H_INCLUDED
#define STRINGUTILS_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
struct xmlTreeElement;
struct key_val_pair;

enum {  dynlisttype_xmlfile,
        dynlisttype_utf32chars,         //items of type uint32_t (utf32 characters not null terminated)
        dynlisttype_keyValuePairsp,      //items of type struct key_val_pait*
        dynlisttype_xmlELMNTCollectionp, //items of type struct xmlTreeElement*

        ListType_CharMatch,
        ListType_WordMatchp,
        ListType_MultiWordMatchp,
        ListType_MultiCharMatchp,

        ListType_int64,
        ListType_float,
        ListType_double,

        ListType_Collider,
        ListType_LayerCollection,
        ListType_TexturePointer
};

struct DynamicList{
    uint32_t type;
    uint32_t itemcnt;
    void* items; //Pointing to Sublist or Items
};

struct DynamicList* append_DynamicList(struct DynamicList* ListOrNullPtr,void* newElement,size_t sizeofListElements,uint32_t typeId);
struct DynamicList* create_DynamicList(size_t sizeofListElements,uint32_t NumOfNewElements,uint32_t typeId);
struct DynamicList* createCharMatchList(uint32_t argumentCount,...);
struct DynamicList* createWordMatchList(uint32_t argumentCount,...);
struct DynamicList* createMultiWordMatchList(uint32_t argumentCount,...);
struct DynamicList* createMultiCharMatchList(uint32_t argumentCount,...);
void delete_DynList(struct DynamicList* DynListPtr);
uint32_t utf8_to_utf32(uint8_t* inputString, uint32_t numberOfUTF8Chars, uint32_t* outputString);
size_t utf16_to_utf32(uint16_t* inputString, size_t numberOfUTF16Chars, uint32_t* outputString);
size_t utf32_cut_ASCII(uint32_t* inputString, uint32_t numberOfUTF32Chars, char* outputStringNullTer);
uint32_t utf32_to_utf8(uint32_t* inputString, uint32_t numberOfUTF32Chars,uint8_t* outputString);
uint32_t getOffsetUntil(uint32_t* fileInUtf32, uint32_t maxScanLength, struct DynamicList* MatchAgainst, uint32_t* optional_matchIndex);

struct DynamicList* getValueFromKeyName(struct DynamicList* attlist,struct DynamicList* nameDl);
struct DynamicList* getElementsWithCharacteristic(uint32_t (*checkfkt)(struct xmlTreeElement*),struct xmlTreeElement* xmlObj);
struct DynamicList* getSubelementsWithCharacteristic(uint32_t (*checkfkt)(struct xmlTreeElement*),struct xmlTreeElement* startElement,uint32_t maxDepth);
struct DynamicList* getSubelementsWithName(struct DynamicList* name,struct xmlTreeElement* startElement,uint32_t maxDepth);

struct DynamicList* utf32dynlist_to_floats(struct DynamicList* NumberSeperatorp,struct DynamicList* OrderOfMagp, struct DynamicList* DecimalSeperatorp,struct DynamicList* utf32StringInp);
struct DynamicList* utf32dynlist_to_doubles(struct DynamicList* NumberSeperatorp,struct DynamicList* OrderOfMagp, struct DynamicList* DecimalSeperatorp,struct DynamicList* utf32StringInp);
struct DynamicList* utf32dynlist_to_ints(struct DynamicList* NumberSeperatorp,struct DynamicList* utf32StringInp);
char* utf32dynlist_to_string(struct DynamicList* utf32dynlist);

#endif // STRINGUTILS_H_INCLUDED
