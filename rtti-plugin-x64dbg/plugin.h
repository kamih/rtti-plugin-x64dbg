#pragma once

#include "pluginmain.h"

//plugin data
#define PLUGIN_NAME "Rtti-plugin-x64"
#define PLUGIN_VERSION 2

//functions
bool pluginInit(PLUG_INITSTRUCT* initStruct);
void pluginStop();
void pluginSetup();
