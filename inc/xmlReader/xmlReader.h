#ifndef XMLREADER_H_INCLUDED
#define XMLREADER_H_INCLUDED
#include <stdio.h>
#include <xmlReader/stringutils.h>
struct xmlTreeElement{              //must be before function definition
    uint32_t type;                  //can be comment, pi, normal_tag or cdata
    struct DynamicList* name;       //is Name of tag or piTarget
    struct xmlTreeElement* parent;
    struct DynamicList* content;    //NULL or content in list form like utf32,or multiple elements
    struct DynamicList* attributes; //NULL or list of key,value pairs
};

struct key_val_pair{
    struct DynamicList* key;        //points to a DynamicList of type dynlisttype_utf32chars
    struct DynamicList* value;      //points to a DynamicList of type dynlisttype_utf32chars
};

enum {
    xmltype_comment,//Comment
    xmltype_pi,    //Processing instruction
    xmltype_tag,   //subtag
    xmltype_cdata, //or text
};



int readXML(FILE* xmlFile,struct xmlTreeElement** returnDocumentRoot);
int writeXML(FILE* xmlOutFile,struct xmlTreeElement* inputDocumentRoot);

void delete_DynList(struct DynamicList* DynListPtr);
struct DynamicList* append_DynamicList(struct DynamicList* ListOrNullPtr,void* newElement,size_t sizeofListElements,uint32_t typeId);
uint32_t compareEqualDynamicUTF32List(struct DynamicList* List1UTF32,struct DynamicList* List2UTF32);
struct DynamicList* stringToUTF32Dynlist(char* inputString);


#endif // XMLREADER_H_INCLUDED
