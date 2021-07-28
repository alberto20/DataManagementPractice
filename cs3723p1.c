/*
author: Adalberto Ramirez
programming assignment1 - data management

*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//#include "cs3723p1Driver.c"
#include "cs3723p1.h"

/*****************************************************************
function that when called either grabs the first node of the free list
if there is a node available or allocates a new node.
the function then sets its data into the node, along with 'A' for being allocated
and sets its allocated size and returns the pointer to the userdata
*****************************************************************/
void * userAllocate(StorageManager *pMgr, short shUserDataSize, short shNodeType, char sbUserData[], SMResult *psmResult)
{
    //printf("test");
	
	// pointers for nodes
    FreeNode* newHead;
    AllocNode* pHead;
    AllocNode* allocPHead;

    if(pMgr -> pFreeHeadM[shNodeType] == NULL)
    {
		pHead = utilAlloc(pMgr, shUserDataSize + 8);
       
		// set reference count
		pHead->shRefCount = 1;
       
		//set nodetype
		pHead->shNodeType = shNodeType;
       
		//set its allocated size shUserDataSize + pMgr->shPrefixSize
		pHead-> shAllocSize = shUserDataSize + pMgr->shPrefixSize;
       
		//set cAF
		pHead->cAF = 'A';
       
		//set sbData
		memcpy(pHead->sbData, sbUserData, shUserDataSize);
		//printf("test");
		//need to change to point to userData
		return pHead->sbData;
    }
    else
    {	
        //save first node into head
        FreeNode* temp  = pMgr->pFreeHeadM[shNodeType];

        //save second node to newHead
        newHead = temp->pNextFree;

        allocPHead = (AllocNode*) temp;

        //set new node as head of the list
        pMgr -> pFreeHeadM[shNodeType] = newHead;
		
		//set node to allocated
        allocPHead->cAF = 'A';
		
		//sets the reference count to 1
        allocPHead->shRefCount = 1;
		
		//uses memcopy to copy data into the node
		memcpy(allocPHead->sbData, sbUserData, shUserDataSize);
	
        //need to change to point to userData
        return allocPHead->sbData;
    }
	
	//sets the rc of psmResult to 0
	psmResult->rc = 0;
	stpcpy(psmResult->szErrorMessage, "Node was not allocated");
	
   return NULL;
}

/*****************************************************************
UserRemoveRef looks at the reference count and decrememnts and if the node 
reaches 0 it recurisively removes reference to any node it is pointing to if 
their count also reaches 0 and then uses memFree to place the node into a freenode list
*****************************************************************/
void userRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult)
{
	
    //printf("remove");
    AllocNode *pRef = (AllocNode*)(((char *) pUserData) - pMgr->shPrefixSize);

	// decrememnt ref count
    pRef->shRefCount--;
	
	short shNodeType = pRef->shNodeType;
	
	//test prints
	//printf("*****************************************************************\n");
	//printf("%d\n", shNodeType);
	//printf("%d\n", shRefCount);
	//printf("*****************************************************************\n");
	
	
	// if reference count is 0 and a allocated node
    if(pRef->shRefCount == 0 && pRef->cAF == 'A')
    {
		// set to free node cAf
		pRef->cAF = 'F';
		
		// loop for metaAttr data
        for(int i=pMgr->nodeTypeM[shNodeType].shBeginMetaAttr; shNodeType == pMgr->metaAttrM[i].shNodeType; i++ )
	    {
			// if datatype is pointer
			if(pMgr->metaAttrM[i].cDataType == 'P')
            {
				
                void **pNext = (void **)(((char *) pUserData) + pMgr->metaAttrM[i].shOffset);
				
				// if next pointer is not null recursive call userremoveref with location of next node
                if(*pNext != NULL)
                {
                    userRemoveRef(pMgr, *pNext, psmResult);
                }
            }
	    }
		//uses memfree to free up the node
        memFree(pMgr, pRef, psmResult);

    }
	//if refence 
    else if(pRef->shRefCount == 0 && pRef->cAF == 'F')
    {
        psmResult->rc = RC_CANT_FREE;
        strcpy(psmResult->szErrorMessage, "Attempted to free a node which isn't allocated");
    }
}
/*****************************************************************
iterates through the metaAttr array and looks if the attrName matches and 
looks to see if it is a pointer and not null and uses removeref to remove the count
and then replaces the previous pointer with the new pUserDataTo pointer
*****************************************************************/
void userAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, SMResult *psmResult)
{
	// meta data pointer and nodeType
    AllocNode *pMeta = (AllocNode*)(((char *) pUserDataFrom) - pMgr->shPrefixSize);
    short shNodeType = pMeta->shNodeType;
    
	// not needed
	//short pRefCount = pMeta->shRefCount;
    
	// loops the arrtraibute array
	for(int i=pMgr->nodeTypeM[shNodeType].shBeginMetaAttr; szAttrName == pMgr->metaAttrM[i].szAttrName; i++)
    {
		// if the attribute name matches
        if(strcmp(pMgr->metaAttrM[i].szAttrName, szAttrName))
		{
			// if data type is a pointer
			if(pMgr->metaAttrM[i].cDataType == 'P')
			{

				void **pNext = (void**)(((char *) pUserDataFrom) + pMgr->metaAttrM[i].shOffset);
				//if pointer location is not null
				if(pMgr->metaAttrM[i].shOffset != '\0')
				{
					//oid **pNext = (void**)(((char *) pUserDataFrom) + pMgr->metaAttrM[i].shOffset);
					// decrememnts refernce count
					userRemoveRef(pMgr, *pNext, psmResult);
					*pNext = pUserDataTo;
				}
				// if null it increments user refence count
				else
				{
					//void **pNext = (void**)(((char *) pUserDataFrom) + pMgr->metaAttrM[i].shOffset);
					userAddRef(pMgr, pUserDataFrom, psmResult);
					*pNext = (void**)pUserDataTo;
				}
				
			}
			// if not pointer sets psmResults with error message
			else
			{
				psmResult->rc = RC_ASSOC_ATTR_NOT_PTR;
				stpcpy(psmResult->szErrorMessage, "Attribute name for ASSOC not a pointer attribute");
			}
		}

    }
}

/*****************************************************************
when called the function points to the beginning of prefix data 
and points to the Reference count and incrememnts it by one
*****************************************************************/
void userAddRef(StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult)
{
    //printf("addRed");
    // sets pRef to point to the reference metadata
    AllocNode *pRef = (AllocNode*)(((char *) pUserDataTo) - pMgr->shPrefixSize);

    //incrememnts it by 1
    pRef->shRefCount++;
}

/*****************************************************************
When the function is called it checks to see if the node is a allocated node if yes
the node is set to be free and returns the node the freenode list of its type to the front
if it is not a allocated node it sets the psmResult and its error message
*****************************************************************/
void memFree(StorageManager *pMgr, AllocNode *pAlloc, SMResult *psmResult)
{
    //printf("mem");
    if(pAlloc->cAF == 'A')
    {
        //saves the node type into shNodetype variable
        short shNodeType = pAlloc->shNodeType;
		
		pAlloc->cAF = 'F';
        
        //saves pAlloc into newHead adn casts it pAlloc as a freeNode
        struct FreeNode* newHead = (FreeNode*) pAlloc;
        
        // sets the the pnext for the new head to point to first node in free node 
        newHead->pNextFree = pMgr->pFreeHeadM[shNodeType];
        
        // sets new head to front of node
        pMgr->pFreeHeadM[shNodeType] = newHead;
    }
    else
    {
        // sets psmResult to 902 for not a allocated node
        psmResult->rc = RC_CANT_FREE;
	strcpy(psmResult->szErrorMessage,"Attempted to free a node which isn't allocated");
    }
}
