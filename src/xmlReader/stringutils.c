#include <xmlReader/stringutils.h>
#include <xmlReader/xmlReader.h>
#include <stdlib.h>     //for malloc
#include <string.h>     //for memcpy
#include <math.h>       //for pow
#include <debug/debug.h>
//Helper functions to analyze xml file and convert utf format
/*
readme
You create a char match list fist, if you want to match it to all characters between 'a' and 'b' aswell as between 'd' and 'z':
struct DynamicList* ExampleCharMatchList1=createCharMatchList(4,'a','b','d','z');
Use this to match only to the char 'a':
struct DynamicList* ExampleCharMatchList2=createCharMatchList(2,'a','a');

You can combine multiple CharMatchLists into a wordMatchList
struct DynamicList* ExampleWordMatchList=createWordMatchList(2,ExampleCharMatchList1,ExampleCharMatchList2);

and those wordMatchLists can be combined into MultiWordMatchLists which try to find multiple words simultaneousely.
For each position in the document it starts with the first entry in the MultiWordMatchList and checks if that one matches, if not it tries the second word and so on.
If there is not match for a given position it will increment the position by one and start again with the first WordMatchList.

Another possibility is to combine charMatchLists into MultiCharMatchLists which will be useful to find out if a char matches to ('a' to 'b', 'c' to 'z') or ('0' to '9')

All types can be passed to uint32_t getOffsetUntil(uint32_t* xmlInUtf32, uint32_t maxScanLength, struct DynamicList* MatchAgainst, uint32_t* optional_matchIndex);
It will return the offset of a match or if there is non the maxScanLength. The optional_matchIndex is useful for MultiWordMatchLists and MultiCharMatchLists to find out which one of the entries caused the match.

After that they should be freed with deleteDynList(uppermostListPointer); .

Warning! do not create intermediate objects, rather nest the ListTypes create functions like so:
createMultiCharMatchList(2,
    createCharMatchList(4,'a','b','d','z'),
    createCharMatchList(2,'0','9')
);
because otherwise the intermediate object's pointers will be invalid when you free the MultiCharMatchList.
*/




//type id is only requiered if the list has not been created yet
//only compatible with pointer type lists!!!!!!
struct DynamicList* append_DynamicList(struct DynamicList* ListOrNullPtr,void* newElementp,size_t sizeofListElements,uint32_t typeId){      //Destroys old pointer and returns new pointer, user needs to update all pointers referring to this object
    struct DynamicList* newDynList;
    if(ListOrNullPtr){//Does already exist, so increase storage space
        newDynList=realloc(ListOrNullPtr,sizeof(struct DynamicList)+sizeofListElements*(1+ListOrNullPtr->itemcnt));
        newDynList->itemcnt++;
    }else{
        newDynList=(struct DynamicList*)malloc(sizeof(struct DynamicList)+sizeofListElements);
        newDynList->itemcnt=1;
        newDynList->type=typeId;
    }
    //Fix dangling pointers
    newDynList->items=(&(newDynList[1]));
    void** lastElementInListp=(void**)((char*)newDynList->items+(sizeofListElements*(newDynList->itemcnt-1)));
    memcpy(lastElementInListp,newElementp,sizeofListElements);
    //(*lastElementInListp)=newElementp;
    return newDynList;
}

struct DynamicList* create_DynamicList(size_t sizeofListElements,uint32_t NumOfNewElements,uint32_t typeId){
    struct DynamicList* newDynList;
    newDynList=(struct DynamicList*) malloc(sizeof(struct DynamicList)+sizeofListElements*NumOfNewElements);
    newDynList->itemcnt=NumOfNewElements;
    newDynList->items=(&(newDynList[1]));
    newDynList->type=typeId;
    return newDynList;
};

struct DynamicList* createCharMatchList(uint32_t argumentCount,...){ //only pass uint32_t to this function!!, Matches from even character to next odd character
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=create_DynamicList(sizeof(uint32_t),argumentCount,ListType_CharMatch);
    for(uint32_t item=0;item<argumentCount;item++){
       ((uint32_t*)DynListPtr->items)[item]=va_arg(argp, uint32_t);
    }
    va_end(argp);
    return DynListPtr;
}

struct DynamicList* createWordMatchList(uint32_t argumentCount,...){ //in case you want to match a word with characters defined by the CharMatchLists like (abc acb bca bac cab cba)
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=create_DynamicList(sizeof(struct DynamicList*),argumentCount,ListType_WordMatchp);
    for(uint32_t item=0;item<argumentCount;item++){
       ((struct DynamicList**)DynListPtr->items)[item]=va_arg(argp, struct DynamicList*);
    }
    va_end(argp);
    return DynListPtr;
}

struct DynamicList* createMultiWordMatchList(uint32_t argumentCount,...){ //in case you want to match xml and XML but not xMl or XmL
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=create_DynamicList(sizeof(struct DynamicList*),argumentCount,ListType_MultiWordMatchp);
    for(uint32_t item=0;item<argumentCount;item++){
       ((struct DynamicList**)DynListPtr->items)[item]=va_arg(argp, struct DynamicList*);
    }
    va_end(argp);
    return DynListPtr;
}

struct DynamicList* createMultiCharMatchList(uint32_t argumentCount,...){
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=create_DynamicList(sizeof(struct DynamicList*),argumentCount,ListType_MultiCharMatchp);
    for(uint32_t item=0;item<argumentCount;item++){
       ((struct DynamicList**)DynListPtr->items)[item]=va_arg(argp, struct DynamicList*);
    }
    va_end(argp);
    return DynListPtr;
}

void delete_DynList(struct DynamicList* DynListPtr){
    switch(DynListPtr->type){
        case ListType_WordMatchp:
        case ListType_MultiWordMatchp:
        case ListType_MultiCharMatchp:
            for(uint32_t SubList=0; SubList<(DynListPtr->itemcnt); SubList++){
                delete_DynList(((struct DynamicList**)DynListPtr->items)[SubList]);
            }
        default:
            free(DynListPtr);
            break;
    }
}


uint32_t utf8_to_utf32(uint8_t* inputString, uint32_t numberOfUTF8Chars, uint32_t* outputString){
    uint32_t input_array_index=0;
    uint32_t output_array_index=0;
    dprintf(DBGT_INFO,"Info: Starting conversion of %d utf8 to unknown utf32 chars\n",numberOfUTF8Chars);
    while(input_array_index<numberOfUTF8Chars){
        if(inputString[input_array_index]>=0xF0){
            outputString[output_array_index++]=
                (((uint32_t)(inputString[input_array_index]  &0x07))<<(6*3))||
                (((uint32_t)(inputString[input_array_index+1]&0x3F))<<(6*2))||
                (((uint32_t)(inputString[input_array_index+2]&0x3F))<<(6*1))||
                (((uint32_t)(inputString[input_array_index+3]&0x3F))       );
                input_array_index+=4;
        }else if(inputString[input_array_index]>=0xE0){
            outputString[output_array_index++]=
                (((uint32_t)(inputString[input_array_index]  &0x0F))<<(6*2))||
                (((uint32_t)(inputString[input_array_index+1]&0x3F))<<(6*1))||
                (((uint32_t)(inputString[input_array_index+2]&0x3F))       );
                input_array_index+=3;
        }else if(inputString[input_array_index]>=0xC0){
            outputString[output_array_index++]=
                (((uint32_t)(inputString[input_array_index]  &0x1F))<<6)||
                (((uint32_t)(inputString[input_array_index+1]&0x3F))    );
            input_array_index+=2;
        }else{
            outputString[output_array_index++]=(uint32_t)(inputString[input_array_index++]&0x7F);
        }
    }
    printf("Info: Converted %d utf8 to %d utf32 chars\n",(unsigned int)input_array_index,(unsigned int)output_array_index);
    return output_array_index;
}

uint32_t utf32_to_utf8(uint32_t* inputString, uint32_t numberOfUTF32Chars,uint8_t* outputString){
    uint32_t output_array_index=0;
    for(uint32_t input_array_index=0;input_array_index<numberOfUTF32Chars;input_array_index++){
        if      (inputString[input_array_index]>=0x00010000){
            outputString[output_array_index]=  (((uint8_t)inputString[input_array_index]>>(3*6))&0x07)|0xf0;
            outputString[output_array_index+1]=(((uint8_t)inputString[input_array_index]>>(2*6))&0x3f)|0x80;
            outputString[output_array_index+2]=(((uint8_t)inputString[input_array_index]>>(1*6))&0x3f)|0x80;
            outputString[output_array_index+3]=(((uint8_t)inputString[input_array_index]>>(0*6))&0x3f)|0x80;
            output_array_index+=4;
        }else if(inputString[input_array_index]>=0x00000800){
            outputString[output_array_index]=(((uint8_t)inputString[input_array_index]  >>(2*6))&0x0f)|0xe0;
            outputString[output_array_index+1]=(((uint8_t)inputString[input_array_index]>>(1*6))&0x3f)|0x80;
            outputString[output_array_index+2]=(((uint8_t)inputString[input_array_index]>>(0*6))&0x3f)|0x80;
            output_array_index+=3;
        }else if(inputString[input_array_index]>=0x00000080){
            outputString[output_array_index]=(((uint8_t)inputString[input_array_index]  >>(1*6))&0x1f)|0xc0;
            outputString[output_array_index+1]=(((uint8_t)inputString[input_array_index]>>(0*6))&0x3f)|0x80;
            output_array_index+=2;
        }else{
            outputString[output_array_index]=(((uint8_t)inputString[input_array_index])         &0x7f);
            output_array_index+=1;
        }
    }
    printf("Info: Converted %d utf32 to %d utf8 chars\n",(unsigned int)numberOfUTF32Chars,(unsigned int)output_array_index);
    return output_array_index;
}

size_t utf16_to_utf32(uint16_t* inputString, size_t numberOfUTF16Chars, uint32_t* outputString){ //Make sure the supplied output buffer is able to hold at least
    size_t input_array_index=0;
    size_t output_array_index=0;
    while(input_array_index<numberOfUTF16Chars){
        if(inputString[input_array_index]>=0xD800){//detect double characters
            outputString[output_array_index++]=(((uint32_t)(inputString[input_array_index]&0x3FF))<<10)||(inputString[++input_array_index]);
        }else{
            outputString[output_array_index++]=(uint32_t)inputString[input_array_index++];
        }
    }
    printf("Info: Converted %d utf16 to %d utf32 chars\n",(unsigned int)input_array_index,(unsigned int)output_array_index);
    return output_array_index;
}

size_t utf32_cut_ASCII(uint32_t* inputString, uint32_t numberOfUTF32Chars, char* outputStringNullTer){
    uint32_t output_array_index=0;
    for(size_t input_array_index=0;input_array_index<numberOfUTF32Chars;input_array_index++){
        if(inputString[input_array_index]>0x7F){        //TODO check if 0x7f or 0x7f000000
            outputStringNullTer[output_array_index++]='?';
        }else{
            outputStringNullTer[output_array_index++]=inputString[input_array_index]&0x7F;
        }
    }
    outputStringNullTer[numberOfUTF32Chars]=0;
    return output_array_index;
}

/** \brief finds the offset to the first occurence of word(s) or character(s)
 *
 * \param inputfile pointer to file in utf32
 * \param how many characters the function is allowed to scan into the inputfile
 * \param Pointer to word/character list to match againt
 * \param if not null will return the offset in the struct DynamicList for wordS and characterS to know which one matched
 * \return offset of the first position of the word or offset to the character from the start of the inputfile in utf32 characters (4bytes)
 *
 */


//TODO urgent: avoid that word match scans so far into the file, that the word offset itself goes past the file
uint32_t getOffsetUntil(uint32_t* fileInUtf32, uint32_t maxScanLength, struct DynamicList* MatchAgainst, uint32_t* optional_matchIndex){
    switch(MatchAgainst->type){
        case ListType_MultiCharMatchp:
            for(uint32_t sPosOffset=0;sPosOffset<maxScanLength;sPosOffset++){
                for(uint32_t matchMultiCharArrayPos=0;matchMultiCharArrayPos<(MatchAgainst->itemcnt);matchMultiCharArrayPos++){
                    struct DynamicList* CharMatch=(((struct DynamicList**)MatchAgainst->items)[matchMultiCharArrayPos]);
                    for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(CharMatch->itemcnt);matchCharArrayPos+=2){
                        if((((uint32_t*)CharMatch->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset]) && (((uint32_t*)CharMatch->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset])){
                            if(optional_matchIndex){
                                (*optional_matchIndex)=matchMultiCharArrayPos;
                            }
                            return sPosOffset;
                        }
                    }
                }
            }
            return maxScanLength;
        break;
        case ListType_CharMatch:
            for(uint32_t sPosOffset=0;sPosOffset<maxScanLength;sPosOffset++){
                for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(MatchAgainst->itemcnt);matchCharArrayPos+=2){
                    if((((uint32_t*)MatchAgainst->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset]) && (((uint32_t*)MatchAgainst->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset])){
                        return sPosOffset;
                    }
                }
            }
            return maxScanLength; //no match found
        break;
        case ListType_WordMatchp: //multiple CharMatch lists
            for(uint32_t sPosOffset=0;sPosOffset<=(maxScanLength-(MatchAgainst->itemcnt));sPosOffset++){
                uint32_t wordPosOffset=0;
                for(;wordPosOffset<MatchAgainst->itemcnt;wordPosOffset++){
                    struct DynamicList* CharMatch=(((struct DynamicList**)MatchAgainst->items)[wordPosOffset]);
                    if(CharMatch->type!=ListType_CharMatch){
                        return UINT32_MAX;//type error
                    }
                    uint32_t MatchingRangeFlag=0;//TODO use KMP-algorithm instead (faster if multiple close matches)
                    for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(CharMatch->itemcnt);matchCharArrayPos+=2){
                        if((((uint32_t*)CharMatch->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset+wordPosOffset]) && (((uint32_t*)CharMatch->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset+wordPosOffset])){
                            MatchingRangeFlag=1; //if this character range did match set matchflag
                        }
                    }
                    if(!MatchingRangeFlag){
                        break; //as soon as we get a mismatch ignore the rest of the word and restart from the next position
                    }
                }
                if(wordPosOffset==(MatchAgainst->itemcnt)){ //all characters matched, we have found the word at the position
                    return sPosOffset; //return the index of the first character of the matching word
                }
            }
            return maxScanLength; //no match found
            break;
        case ListType_MultiWordMatchp:{
            uint32_t sPosOffset=0;
            for(;sPosOffset<maxScanLength;sPosOffset++){
                uint32_t wordIndex=0;
                for(;wordIndex<MatchAgainst->itemcnt;wordIndex++){
                    struct DynamicList* WordMatch=((struct DynamicList**)MatchAgainst->items)[wordIndex];
                    uint32_t wordPosOffset=0;
                    for(;wordPosOffset<WordMatch->itemcnt;wordPosOffset++){
                        if(wordPosOffset+sPosOffset>maxScanLength){
                            break; //word is extending beyond the scan area
                        }
                        struct DynamicList* CharMatch=((struct DynamicList**)WordMatch->items)[wordPosOffset];
                        uint32_t MatchingRangeFlag=0;
                        for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(CharMatch->itemcnt);matchCharArrayPos+=2){
                            if((((uint32_t*)CharMatch->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset+wordPosOffset]) && (((uint32_t*)CharMatch->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset+wordPosOffset])){
                                MatchingRangeFlag=1;
                            }
                        }
                        if(!MatchingRangeFlag){ //abort further search on one mismatched char
                            break;
                        }
                    }
                    if(wordPosOffset==WordMatch->itemcnt){ //the whole word matches (the inner loop did not terminate prematurely, full word scanned)
                        break;
                    }
                }
                if(wordIndex!=MatchAgainst->itemcnt){ //we got a match from the inner loop (the inner loop did terminate prematurely, word at wordIndex matched)
                    if(optional_matchIndex){
                        (*optional_matchIndex)=wordIndex;
                    }
                    return sPosOffset;
                }
            }
            if(sPosOffset==maxScanLength){ //nothing found, reached end of assigned scan range
                    return maxScanLength;
                }
        }
        break;

        default:
            printf("ERROR: Wrong list type for getOffsetUntil!\n");
            return UINT32_MAX;//type error
            break;
    }
    return 0;
}

struct DynamicList* getValueFromKeyName(struct DynamicList* attlist,struct DynamicList* nameDl){
    if(!attlist){
        dprintf(DBGT_ERROR,"Attlist was nullptr");
        return 0;
    }
    if(!attlist->itemcnt){
        dprintf(DBGT_ERROR,"Attlist has no attributes");
        return 0;
    }
    for(uint32_t index=0;index<attlist->itemcnt;index++){
        if(compareEqualDynamicUTF32List(((struct key_val_pair**)attlist->items)[index]->key,nameDl)){
            delete_DynList(nameDl);
            return ((struct key_val_pair**)attlist->items)[index]->value;
        }
    }
    delete_DynList(nameDl);
    return 0;
}

//0 for depth means search only childs of current element
struct DynamicList* getSubelementsWithCharacteristic(uint32_t (*checkfkt)(struct xmlTreeElement*),struct xmlTreeElement* startElementp,uint32_t maxDepth){
    struct DynamicList* returnDynList=0;
    struct xmlTreeElement* LastXMLTreeElement=startElementp->parent;
    struct xmlTreeElement* CurrentXMLTreeElement=startElementp;
    uint32_t currentDepth=0;
    uint32_t index=0;
    do{
        uint32_t itemcnt;
        if(CurrentXMLTreeElement->content==0){
            itemcnt=0;
        }else{
            itemcnt=CurrentXMLTreeElement->content->itemcnt;
        }
        if(CurrentXMLTreeElement->parent==LastXMLTreeElement){      //walk deeper
            for(;index<itemcnt;index++){
                if((*checkfkt)(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index])){
                    returnDynList=append_DynamicList(returnDynList,(void**)&(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]),sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
                }
                if(currentDepth<maxDepth){ //make sure our programm stops scanning for new elements if the maxdepth is reached
                    if(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]->type==xmltype_tag){  //One valid subelement found
                        LastXMLTreeElement=CurrentXMLTreeElement;
                        CurrentXMLTreeElement=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index];
                        index=0;
                        currentDepth++;
                        //TODO? This tests only elements of xmltype_tag
                        break;
                    }else{
                        dprintf(DBGT_ERROR,"Non standrad tag");
                    }
                }else{
                    dprintf(DBGT_INFO,"MaxDepthLimit reached");
                }
            }
        }else{      //go back upward
            for(index=0;index<itemcnt;index++){     //Try to find the index of the element we were in from it's parent
                if(LastXMLTreeElement==((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]){
                    if(itemcnt>1+index){  //Do we have more elements in the current Element
                        index++;
                        LastXMLTreeElement=CurrentXMLTreeElement->parent;
                        currentDepth--;
                        break;
                    }
                }
            }
        }
        if(index==itemcnt){
            //Turn around
            LastXMLTreeElement=CurrentXMLTreeElement;
            CurrentXMLTreeElement=CurrentXMLTreeElement->parent;
        }else{
            continue;
        }
    }while((CurrentXMLTreeElement!=0)&&(CurrentXMLTreeElement!=startElementp->parent));     //Todo error with abort condition when starting from subelement
    if(returnDynList){
        dprintf(DBGT_INFO,"Finished Search with %d matches\n",returnDynList->itemcnt);
    }else{
        dprintf(DBGT_INFO,"Did not found any match");
    }
    return returnDynList;
}

uint32_t nameCheckFkt(struct xmlTreeElement* nameInit_or_xmlElement){
    static struct DynamicList* nameDynlistUFT32=0;        //0 if nor initialized
    if(nameInit_or_xmlElement){
        if(!nameDynlistUFT32){//if function was not initialized with a name yet
            nameDynlistUFT32=(struct DynamicList*) nameInit_or_xmlElement;
            if(nameDynlistUFT32->type!=dynlisttype_utf32chars){
                return 43;
            }
            return 42;  //initialized name succesfully
        }else{
            if(compareEqualDynamicUTF32List(nameInit_or_xmlElement->name,nameDynlistUFT32)){
                return 1;
            }else{
                return 0;
            }
        }
    }else{
        nameDynlistUFT32=0; //Deinit
        return 42;
    }
}

struct DynamicList* getSubelementsWithName(struct DynamicList* namep,struct xmlTreeElement* startElementp,uint32_t maxDepth){
    nameCheckFkt(0);    //null deinitializes the function
    nameCheckFkt((struct xmlTreeElement*)namep);    //Hide the namep as an struct xmlTreeElement*, will be casted back in nameCheckFkt
    return getSubelementsWithCharacteristic(&nameCheckFkt,startElementp,maxDepth);
};


struct DynamicList* getElementsWithCharacteristic(uint32_t (*checkfkt)(struct xmlTreeElement*),struct xmlTreeElement* xmlObj){
    struct DynamicList* returnDynList=0;
    struct xmlTreeElement* LastXMLTreeElement=0;
    struct xmlTreeElement* CurrentXMLTreeElement=xmlObj;
    dprintf(DBGT_INFO,"zughjm");
    do{
        static uint32_t index=0;
        uint32_t itemcnt;
        if(CurrentXMLTreeElement->content==0){
            itemcnt=0;
        }else{
            itemcnt=CurrentXMLTreeElement->content->itemcnt;
        }
        if(CurrentXMLTreeElement->parent==LastXMLTreeElement){
            for(;index<itemcnt;index++){
                dprintf(DBGT_ERROR,"%d,%d",CurrentXMLTreeElement->content->type,dynlisttype_xmlELMNTCollectionp);
                dprintf(DBGT_INFO,"%p",(void*)(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[0]));
                if((*checkfkt)(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index])){
                    returnDynList=append_DynamicList(returnDynList,(void**)&(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]),sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
                }
                if(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]->type==xmltype_tag){  //One valid subelement found
                    LastXMLTreeElement=CurrentXMLTreeElement;
                    CurrentXMLTreeElement=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index];
                    index=0;
                    //This tests only elements of xmltype_tag
                    /*
                    if((*checkfkt)(CurrentXMLTreeElement)){
                        WRONG, SIZEOF SHOULD BE POINTER returnDynList=append_DynamicList(returnDynList,(void*)CurrentXMLTreeElement,sizeof(CurrentXMLTreeElement),dynlisttype_xmlELMNTCollection);
                    }
                    */
                    //TODO check element
                    break;
                }
            }
        }else{
            for(index=0;index<itemcnt;index++){
                if(LastXMLTreeElement==((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]){
                    if(itemcnt>1+index){  //Do we have more elements in the current Element
                        index++;
                        LastXMLTreeElement=CurrentXMLTreeElement->parent;
                        break;
                    }
                }
            }
        }
        if(index==itemcnt){
            //Turn around

            LastXMLTreeElement=CurrentXMLTreeElement;
            CurrentXMLTreeElement=CurrentXMLTreeElement->parent;
        }else{
            continue;
        }

    }while(CurrentXMLTreeElement!=xmlObj);
    dprintf(DBGT_INFO,"Finished Search with %d matches\n",returnDynList->itemcnt);
    return returnDynList;
}

struct DynamicList* utf32dynlist_to_ints(struct DynamicList* NumberSeperatorp,struct DynamicList* utf32StringInp){
    struct DynamicList* returnDynlistp=0;
    uint32_t offsetInString=0;
    enum{flag_in_digits=(1<<0),flag_minus_mantis=(1<<1)};
    int32_t flagreg=0;
    int64_t mantisVal=0;
    enum {
        match_res_nummatch_0=0,
        match_res_nummatch_1=1,
        match_res_nummatch_2=2,
        match_res_nummatch_3=3,
        match_res_nummatch_4=4,
        match_res_nummatch_5=5,
        match_res_nummatch_6=6,
        match_res_nummatch_7=7,
        match_res_nummatch_8=8,
        match_res_nummatch_9=9,
        match_res_nummatch_sign=10,
        match_res_nummatch_nsperator=11,
        match_res_nummatch_illegal=12
    };
    struct DynamicList* nummatch=createMultiCharMatchList(13,
        createCharMatchList(2,'0','0'),
        createCharMatchList(2,'1','1'),
        createCharMatchList(2,'2','2'),
        createCharMatchList(2,'3','3'),
        createCharMatchList(2,'4','4'),
        createCharMatchList(2,'5','5'),
        createCharMatchList(2,'6','6'),
        createCharMatchList(2,'7','7'),
        createCharMatchList(2,'8','8'),
        createCharMatchList(2,'9','9'),
        createCharMatchList(2,'-','-'),
        NumberSeperatorp,
        createCharMatchList(2,0x00,0xffff)
    );
    uint32_t matchIndex=0;
    while(offsetInString<utf32StringInp->itemcnt){
        //no offset because will match all possible chars
        getOffsetUntil(((uint32_t*)utf32StringInp->items)+offsetInString,1,nummatch,&matchIndex);
        if(matchIndex==match_res_nummatch_illegal){
            dprintf(DBGT_ERROR,"Illegal Character in inputstring");
            return 0;
        }else if(matchIndex==match_res_nummatch_sign){
            if(flagreg==0){
                flagreg|=flag_minus_mantis;
            }else{
                dprintf(DBGT_ERROR,"Unexpected Sign Char");
                return 0;
            }
        }else if(matchIndex==match_res_nummatch_nsperator){
            if(flagreg&flag_in_digits){
                if(flagreg&flag_minus_mantis){
                    mantisVal*=(-1);
                }
                returnDynlistp=append_DynamicList(returnDynlistp,&mantisVal,sizeof(double),ListType_double);
                flagreg=0;
                mantisVal=0;
            }
        }else{
            flagreg|=flag_in_digits;
            mantisVal*=10;
            mantisVal+=matchIndex;
        }
        offsetInString++;
    }

    if(flagreg&flag_in_digits){
        if(flagreg&flag_minus_mantis){
            mantisVal*=(-1);
        }
        returnDynlistp=append_DynamicList(returnDynlistp,&mantisVal,sizeof(double),ListType_double);
    }
    delete_DynList(nummatch);
    return returnDynlistp;
}


struct DynamicList* utf32dynlist_to_doubles(struct DynamicList* NumberSeperatorp,struct DynamicList* OrderOfMagp, struct DynamicList* DecimalSeperatorp,struct DynamicList* utf32StringInp){
    struct DynamicList* returnDynlistp=0;
    uint32_t offsetInString=0;
    double mantisVal=0;
    enum{flag_in_digits=(1<<0),flag_decplc=(1<<1),flag_in_decplcdigits=(1<<2),flag_orOfMag=(1<<3),flag_in_exp_digits=(1<<4),flag_minus_mantis=(1<<5),flag_minus_exp=(1<<6)};
    int32_t flagreg=0;
    int32_t NumOfDecplcDig=0;
    int32_t exponVal=0;
    enum {
        match_res_nummatch_0=0,
        match_res_nummatch_1=1,
        match_res_nummatch_2=2,
        match_res_nummatch_3=3,
        match_res_nummatch_4=4,
        match_res_nummatch_5=5,
        match_res_nummatch_6=6,
        match_res_nummatch_7=7,
        match_res_nummatch_8=8,
        match_res_nummatch_9=9,
        match_res_nummatch_sign=10,
        match_res_nummatch_nseperator=11,
        match_res_nummatch_dseperator=12,
        match_res_nummatch_OrderOfMag=13,
        match_res_nummatch_illegal=14
    };
    struct DynamicList* nummatch=createMultiCharMatchList(15,
        createCharMatchList(2,'0','0'),
        createCharMatchList(2,'1','1'),
        createCharMatchList(2,'2','2'),
        createCharMatchList(2,'3','3'),
        createCharMatchList(2,'4','4'),
        createCharMatchList(2,'5','5'),
        createCharMatchList(2,'6','6'),
        createCharMatchList(2,'7','7'),
        createCharMatchList(2,'8','8'),
        createCharMatchList(2,'9','9'),
        createCharMatchList(2,'-','-'),
        NumberSeperatorp,
        DecimalSeperatorp,
        OrderOfMagp,
        createCharMatchList(2,0x00,0xffff)
    );
    uint32_t matchIndex=0;
    while(offsetInString<utf32StringInp->itemcnt){
        //no offset because will match all possible chars
        getOffsetUntil(((uint32_t*)(utf32StringInp->items))+offsetInString,1,nummatch,&matchIndex);
        if(matchIndex==match_res_nummatch_illegal){
            dprintf(DBGT_ERROR,"Illegal Character in inputstring");
            return 0;
        }else if(matchIndex==match_res_nummatch_sign){
            if(flagreg&flag_orOfMag){
                flagreg|=flag_minus_exp;
            }else if(flagreg==0){
                flagreg|=flag_minus_mantis;
            }else{
                dprintf(DBGT_ERROR,"Unexpected Sign Char");
                return 0;
            }
        }else if(matchIndex==match_res_nummatch_nseperator){
            if(flagreg&flag_orOfMag){
                dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
                return 0;
            }
            if(flagreg&flag_decplc){
                dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
                return 0;
            }
            if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
                if(flagreg&flag_minus_mantis){
                    mantisVal*=(-1.0);
                }
                if(flagreg&flag_minus_exp){
                    exponVal*=(-1);
                }
                exponVal-=NumOfDecplcDig;
                mantisVal*=pow(10,exponVal);
                returnDynlistp=append_DynamicList(returnDynlistp,&mantisVal,sizeof(double),ListType_double);
                flagreg=0;
                NumOfDecplcDig=0;
                mantisVal=0;
                exponVal=0;
            }
        }else if(matchIndex==match_res_nummatch_dseperator){
            if(flagreg&flag_in_digits){
                flagreg|=flag_in_decplcdigits;         //enable in_decplcdigits flag
                flagreg&= ~(flag_in_digits);    //disable in_digits flag
            }else if(flagreg==0){               //to handle ".2"
                flagreg|=flag_in_decplcdigits;
                mantisVal=0;
            }else{
                if(flagreg&flag_in_exp_digits){
                    dprintf(DBGT_ERROR,"unexpected decDigSeparator in exponent");
                }else if(flagreg&flag_in_decplcdigits){
                    dprintf(DBGT_ERROR,"more than one decDigSeperator in mantisse");
                }
            }
        }else if(matchIndex==match_res_nummatch_OrderOfMag){
            if(flagreg&(flag_in_digits|flag_in_decplcdigits)){
                flagreg|=flag_orOfMag;
                flagreg&=~(flag_in_digits|flag_in_decplcdigits);    //clear flags
            }else{
                dprintf(DBGT_ERROR,"Unexpected Order of Magnitude Char");
                return 0;
            }
        }else{      //Digit from 0-9
            if(flagreg&flag_in_decplcdigits){
                NumOfDecplcDig++;
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_decplc){
                NumOfDecplcDig++;
                flagreg|=flag_in_decplcdigits;
                flagreg&=~(flag_decplc);
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_in_digits){
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_orOfMag){
                exponVal=matchIndex;
                flagreg|=flag_in_exp_digits;
                flagreg&=~(flag_orOfMag);
            }else if(flagreg&flag_in_exp_digits){
                exponVal*=10;
                exponVal+=matchIndex;
            }else{
                flagreg|=flag_in_digits;
                mantisVal=matchIndex;
            }
        }
        offsetInString++;
    }

    //finish parsing last double
    if(flagreg&flag_orOfMag){
        dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
        return 0;
    }
    if(flagreg&flag_decplc){
        dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
        return 0;
    }
    if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
        if(flagreg&flag_minus_mantis){
            mantisVal*=(-1.0);
        }
        if(flagreg&flag_minus_exp){
            exponVal*=(-1);
        }
        exponVal-=NumOfDecplcDig;
        mantisVal*=pow(10,exponVal);
        returnDynlistp=append_DynamicList(returnDynlistp,&mantisVal,sizeof(double),ListType_double);
    }
    delete_DynList(nummatch);       //TODO fix for all return types
    return returnDynlistp;
}


struct DynamicList* utf32dynlist_to_floats(struct DynamicList* NumberSeperatorp,struct DynamicList* OrderOfMagp, struct DynamicList* DecimalSeperatorp,struct DynamicList* utf32StringInp){
    struct DynamicList* returnDynlistp=0;
    uint32_t offsetInString=0;
    float mantisVal=0;
    enum{flag_in_digits=(1<<0),flag_decplc=(1<<1),flag_in_decplcdigits=(1<<2),flag_orOfMag=(1<<3),flag_in_exp_digits=(1<<4),flag_minus_mantis=(1<<5),flag_minus_exp=(1<<6)};
    int32_t flagreg=0;
    int32_t NumOfDecplcDig=0;
    int32_t exponVal=0;
    enum {
        match_res_nummatch_0=0,
        match_res_nummatch_1=1,
        match_res_nummatch_2=2,
        match_res_nummatch_3=3,
        match_res_nummatch_4=4,
        match_res_nummatch_5=5,
        match_res_nummatch_6=6,
        match_res_nummatch_7=7,
        match_res_nummatch_8=8,
        match_res_nummatch_9=9,
        match_res_nummatch_sign=10,
        match_res_nummatch_nseperator=11,
        match_res_nummatch_dseperator=12,
        match_res_nummatch_OrderOfMag=13,
        match_res_nummatch_illegal=14
    };
    struct DynamicList* nummatch=createMultiCharMatchList(15,
        createCharMatchList(2,'0','0'),
        createCharMatchList(2,'1','1'),
        createCharMatchList(2,'2','2'),
        createCharMatchList(2,'3','3'),
        createCharMatchList(2,'4','4'),
        createCharMatchList(2,'5','5'),
        createCharMatchList(2,'6','6'),
        createCharMatchList(2,'7','7'),
        createCharMatchList(2,'8','8'),
        createCharMatchList(2,'9','9'),
        createCharMatchList(2,'-','-'),
        NumberSeperatorp,
        DecimalSeperatorp,
        OrderOfMagp,
        createCharMatchList(2,0x00,0xffff)
    );
    uint32_t matchIndex=0;
    while(offsetInString<utf32StringInp->itemcnt){
        //no offset because will match all possible chars
        getOffsetUntil(((uint32_t*)(utf32StringInp->items))+offsetInString,1,nummatch,&matchIndex);
        if(matchIndex==match_res_nummatch_illegal){
            dprintf(DBGT_ERROR,"Illegal Character in inputstring");
            return 0;
        }else if(matchIndex==match_res_nummatch_sign){
            if(flagreg&flag_orOfMag){
                flagreg|=flag_minus_exp;
            }else if(flagreg==0){
                flagreg|=flag_minus_mantis;
            }else{
                dprintf(DBGT_ERROR,"Unexpected Sign Char");
                return 0;
            }
        }else if(matchIndex==match_res_nummatch_nseperator){
            if(flagreg&flag_orOfMag){
                dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
                return 0;
            }
            if(flagreg&flag_decplc){
                dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
                return 0;
            }
            if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
                if(flagreg&flag_minus_mantis){
                    mantisVal*=(-1.0);
                }
                if(flagreg&flag_minus_exp){
                    exponVal*=(-1);
                }
                exponVal-=NumOfDecplcDig;
                mantisVal*=pow(10,exponVal);
                returnDynlistp=append_DynamicList(returnDynlistp,&mantisVal,sizeof(float),ListType_float);
                flagreg=0;
                NumOfDecplcDig=0;
                mantisVal=0;
                exponVal=0;
            }
        }else if(matchIndex==match_res_nummatch_dseperator){
            if(flagreg&flag_in_digits){
                flagreg|=flag_in_decplcdigits;         //enable in_decplcdigits flag
                flagreg&= ~(flag_in_digits);    //disable in_digits flag
            }else if(flagreg==0){               //to handle ".2"
                flagreg|=flag_in_decplcdigits;
                mantisVal=0;
            }else{
                if(flagreg&flag_in_exp_digits){
                    dprintf(DBGT_ERROR,"unexpected decDigSeparator in exponent");
                }else if(flagreg&flag_in_decplcdigits){
                    dprintf(DBGT_ERROR,"more than one decDigSeperator in mantisse");
                }
            }
        }else if(matchIndex==match_res_nummatch_OrderOfMag){
            if(flagreg&(flag_in_digits|flag_in_decplcdigits)){
                flagreg|=flag_orOfMag;
                flagreg&=~(flag_in_digits|flag_in_decplcdigits);    //clear flags
            }else{
                dprintf(DBGT_ERROR,"Unexpected Order of Magnitude Char");
                return 0;
            }
        }else{      //Digit from 0-9
            if(flagreg&flag_in_decplcdigits){
                NumOfDecplcDig++;
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_decplc){
                NumOfDecplcDig++;
                flagreg|=flag_in_decplcdigits;
                flagreg&=~(flag_decplc);
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_in_digits){
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_orOfMag){
                exponVal=matchIndex;
                flagreg|=flag_in_exp_digits;
                flagreg&=~(flag_orOfMag);
            }else if(flagreg&flag_in_exp_digits){
                exponVal*=10;
                exponVal+=matchIndex;
            }else{
                flagreg|=flag_in_digits;
                mantisVal=matchIndex;
            }
        }
        offsetInString++;
    }

    //finish parsing last float
    if(flagreg&flag_orOfMag){
        dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
        return 0;
    }
    if(flagreg&flag_decplc){
        dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
        return 0;
    }
    if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
        if(flagreg&flag_minus_mantis){
            mantisVal*=(-1.0);
        }
        if(flagreg&flag_minus_exp){
            exponVal*=(-1);
        }
        exponVal-=NumOfDecplcDig;
        mantisVal*=pow(10,exponVal);
        returnDynlistp=append_DynamicList(returnDynlistp,&mantisVal,sizeof(float),ListType_float);
    }
    delete_DynList(nummatch);       //TODO fix for all return types
    return returnDynlistp;
}
