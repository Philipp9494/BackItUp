/**************************************************
 * FILENAME:        FileTree.h
 * PROJECT:         BackItUp
 *
 * AUTHOR:          Philipp Doblhofer
 * WEB:             www.gnp-tec.net
 * START DATE:      2014-Feb-01
 *
 **************************************************
 * DESCRIPTION:
 * FileTree class
 *************************************************/

#ifndef _FILETREE_H_
#define _FILETREE_H_

#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _FileTreeElement {
    char* Name;
    struct stat attr;

    struct _FileTreeElement *pNext;
    struct _FileTreeElement *pPrev;
} FileTreeElement;

class FileTree {
    private:
        FileTreeElement* pRoot;
    public:
        FileTree() { pRoot = NULL; }
        ~FileTree() { 
            while(pRoot != NULL) {
                free(pRoot->Name);
                pRoot = pRoot->pNext;
                free(pRoot->pPrev);
            }
        };

        bool addEntry(const char* name, struct stat attr) {
            FileTreeElement **pTmp;
            pTmp = &pRoot;
            if(*pTmp != NULL) {
                while((*pTmp)->pNext != NULL)
                    pTmp = &((*pTmp)->pNext);

                (*pTmp)->pNext = (FileTreeElement*)malloc(sizeof(FileTreeElement));
                if((*pTmp)->pNext == NULL)
                    return false;

                (*pTmp)->pNext->pNext = NULL;
                (*pTmp)->pNext->pPrev = (*pTmp);
                (*pTmp)->pNext->Name = strdup(name);
                if((*pTmp)->pNext->Name == NULL)
                    return false;
                (*pTmp)->pNext->attr = attr;
            } else {    
                (*pTmp) = (FileTreeElement*)malloc(sizeof(FileTreeElement));
                if((*pTmp) == NULL)
                    return false;

                (*pTmp)->pNext = NULL;
                (*pTmp)->pPrev = NULL;
                (*pTmp)->Name = strdup(name);
                if((*pTmp)->Name == NULL)
                    return false;
                (*pTmp)->attr = attr;
            }
            return true;
        }

        const char* serialize(FileTreeElement *pPtr) {
            char* ret = (char*)malloc(strlen(pPtr->Name) + sizeof(size_t) + sizeof(struct stat)+10);
            char* pTmp = ret;
            memcpy(ret, &(pPtr->attr), sizeof(struct stat));
            ret += sizeof(struct stat);
            *((size_t*)ret) = strlen(pPtr->Name);
            ret += sizeof(size_t);
            strcpy(ret, pPtr->Name);

            return pTmp;
        }

        const char* getNextSerializedElement() {
            static FileTreeElement *pTmp = pRoot;
            if(pTmp == NULL)
                return NULL;
            const char* s = serialize(pTmp);
            pTmp = pTmp->pNext;

            return s;
        }
};

#endif