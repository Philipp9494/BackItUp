/**************************************************
 * FILENAME:        main.cpp
 * PROJECT:         BackItUp
 *
 * AUTHOR:          Philipp Doblhofer
 * WEB:             www.gnp-tec.net
 * START DATE:      2013-Dec-29
 *
 **************************************************
 * DESCRIPTION:
 * Main program, which initiates the process of
 * the backup. (Read config, start copying, ...)
 *************************************************/

#include <stdio.h>
#include "../inc/BackItUp.h"

int main(int argc, char** argv) {
    BackItUp b(argc, argv);
}
