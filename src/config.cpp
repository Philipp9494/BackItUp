/**************************************************
 * FILENAME:        config.cpp
 * PROJECT:         BackItUp
 *
 * AUTHOR:          Philipp Doblhofer
 * WEB:             www.gnp-tec.net
 * START DATE:      2013-Dec-29
 *
 **************************************************
 * DESCRIPTION:
 * This is the class, which reads the configuration
 * file for the backuper (XML). Afterwards it will
 * call the FIterator, which saves the files and
 * directories as set in the config file.
 *************************************************/

#include "../inc/config.h"
#include "../inc/fiterator.h"
#include "../inc/rfilehandler.h"
#include "../inc/cfilehandler.h"
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define ERRWR(x...)    { fprintf(stderr, x); return false; }

void Config::unload() {
    if(configfile != NULL)
        free(configfile);
    if(configversion != NULL)
        free(configversion);
    if(backup_dest != NULL)
        free(backup_dest);
    if(doc != NULL)
        xmlFreeDoc(doc);
    if(FH != NULL)
        delete FH;
}

void Config::reset() {
    configfile = NULL;
    configversion = NULL;
    backup_dest = NULL;
    doc = NULL;
    mode = MODE_UNSET;
    type = TYPE_UNSET;
    FH = NULL;

    while(directories.size() > 0) {
        void* buf = (void*)directories[directories.size()-1];
        if(buf != NULL)
            free(buf);
        directories.pop_back();
    }
    directories.clear();
}

bool Config::load(const char* file) {
    xmlNodePtr cur;

    unload();   
    reset();   

    if(file == NULL || *file == '\0')
        ERRWR("Invalid file pointer!\n\r");

    configfile = strdup(file);
    if(configfile == NULL) 
        ERRWR("Error allocating memory!\n\r");
    
    doc = xmlParseFile(file);
    if(doc == NULL) 
        ERRWR("Error parsing config file!\n\r");
        
    cur = xmlDocGetRootElement(doc);
    if(cur == NULL || strcmp((const char*)cur->name, "config") != 0)
        ERRWR("Error parsing config file!\n\r");

    // Get config version
    xmlChar* prop = xmlGetProp(cur, (const xmlChar*)"version");
    configversion = strdup((const char*)prop);
    if(configfile == NULL)
        ERRWR("Error allocating memory!\n\r");
    xmlFree(prop);


    // Is <quiet> set?
    if(xmlHasProp(cur, (const xmlChar*)"quiet") == NULL)
        log.addOutput(LogStdout, LogInfo, NULL, 0);

    #ifdef DEBUG
    printf("Config version %s\n\r", configversion);

    int lvl = 0;
    #endif

    // Now iterate trough each node
    while(cur!=NULL) {
        if(cur->type == XML_ELEMENT_NODE) {
            if(strcmp((const char*)cur->name, "config") == 0) {
                // Root node
                // NOP
            } else if(strcmp((const char*)cur->name, "directory") == 0) {
                if(strcmp((const char*)cur->parent->name, "directories") != 0) {
                    ERRWR("A <directory> node must be inside of the <directories> node!\n\r");
                }
                #ifdef DEBUG
                printf("> #%s#\n\r", xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                #endif

                directories.push_back((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
            } else if(strcmp((const char*)cur->name, "destination") == 0) {
                if(strcmp((const char*)cur->parent->name, "backup") != 0) {
                    ERRWR("The <destination> node must be inside of the <backup> node!\n\r");                
                }
                        
                if(backup_dest != NULL)
                    ERRWR("Only one backup destination is allowed!\n\r");
                backup_dest = (char*)malloc(strlen((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)) + 50);
                if(backup_dest == NULL) {
                    ERRWR("Error allocating memory!\n\r");
                }
                strcpy(backup_dest, (const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
            } else if(strcmp((const char*)cur->name, "mode") == 0) {
                if(strcmp((const char*)cur->parent->name, "backup") != 0) {
                    ERRWR("The <mode> node must be inside of the <backup> node!\n\r");                
                }                 
 
                if(mode != MODE_UNSET)
                    ERRWR("Only one <mode> node is allowed!\n\r");
                
                if(strcmp((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1), "full") == 0)
                    mode = MODE_FULL;
            } else if(strcmp((const char*)cur->name, "type") == 0) {
                if(strcmp((const char*)cur->parent->name, "backup") != 0) {
                    ERRWR("The <type> node must be inside of the <backup> node!\n\r");                
                }                 
 
                if(type != TYPE_UNSET)
                    ERRWR("Only one <type> node is allowed!\n\r");
                
                if(strcmp((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1), "uncompressed") == 0)
                    type = TYPE_UNCOMPRESSED;
                else if(strcmp((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1), "compressed") == 0)
                    type = TYPE_COMPRESSED;
            } else if(strcmp((const char*)cur->name, "log") == 0) {
                if(strcmp((const char*)cur->parent->name, "backup") != 0) {
                    ERRWR("The <log> node must be inside of the <backup> node!\n\r");                
                }                 
                
                log.addOutput(LogFile, LogInfo, (const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1), strlen((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
            }
    
            #ifdef DEBUG
            for(int i=0; i<lvl; i++)
                printf("  ");
            printf("%s\n\r", cur->name);
            #endif
        }

        // get next element
        if(cur->xmlChildrenNode != NULL) {
            #ifdef DEBUG
            lvl++;
            #endif
            cur=cur->xmlChildrenNode;
        } else if(cur->next != NULL) {
            cur=cur->next;
        } else {
            #ifdef DEBUG
            lvl--;
            #endif
            cur=cur->parent->next;
        }
    }

    if(type == TYPE_UNCOMPRESSED)
        FH = new RFileHandler();
    else if(type == TYPE_COMPRESSED)
        FH = new CFileHandler();
    else
        return false;

    return true;
}

bool Config::isValid() {
    if(backup_dest == NULL || strlen(backup_dest) == 0) {
        log.Log(LogError, "No backup destination found in config file!\n\r");
        return false;
    }

    if(mode == MODE_UNSET) {
        log.Log(LogError, "No valid backup-mode is choosen!\n\r");
        return false;
    }

    if(type == TYPE_UNSET) {
        log.Log(LogError, "No valid backup-type is choosen!\n\r");
        return false;
    }

    if(directories.size() == 0) {
        log.Log(LogError, "No directories are given!\n\r");
        return false;
    }

    log.Log(LogInfo, "Summary:\n\r\tConfigfile:\t\t%s (v%s)\n\r", configfile, configversion);
    log.Log(LogInfo, "\tBackup Destination:\t%s\n\r", backup_dest);
    log.Log(LogInfo, "\tMode:\t\t\t%s\n\r", mode == MODE_FULL ? "full" : "??");
    log.Log(LogInfo, "\tType:\t\t\t%s\n\r", type == TYPE_UNCOMPRESSED ? "uncompressed" : (type == TYPE_COMPRESSED ? "compressed" : "??"));

    log.Log(LogInfo, "\n\r");
    log.Log(LogInfo, "\tFolders to save:\n\r");
    for(unsigned int i=0; i<directories.size(); i++) {
        log.Log(LogInfo, "\t\t#%i\t%s\n\r", i, directories[i]);
    }

    return true;
}

void Config::backupDirectories() {
    if(mkdir(getBackupDestination(), 0755) != 0 && errno != EEXIST) {
        log.Log(LogError, "Couldn't create directory <%s>\n\r", getBackupDestination());   
        return ;
    }

    char buf[200];
    time_t t = time(0);
    struct tm* tmp = localtime(&t);
    strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", tmp);

    if(backup_dest[strlen(backup_dest)-1] != '/')
        strcat(backup_dest, "/");
    strcat(backup_dest, buf);

    if(type == TYPE_UNCOMPRESSED) {
        strcat(backup_dest, "/");
        if(mkdir(getBackupDestination(), 0755) != 0) {
            log.Log(LogError, "Couldn't create directory <%s>\n\r", getBackupDestination());   
        }
    } else if(type == TYPE_COMPRESSED) {
        strcat(backup_dest, ".tar.gz");
    }

    if(!FH->Init(this)) {
        log.Log(LogError, "Error initializing the filehandler!\n\r");
        return ;
    }

    for(unsigned int i=0; i<directories.size(); i++) {
        log.Log(LogInfo, "Backing up #%i\t%s\n\r", i, directories[i]);
        FIterator f(this, directories[i]);
    }
    FH->Finalize();
}
