#include "config.h"
#include "ini.h"
#include "..\pluginmain.h"

namespace RTTI {;
namespace Config {;

std::string config_path;
settings_t settings;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setPath()
{
	char szCurrentDirectory[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
	strcat_s(szCurrentDirectory, "\\");

	config_path = szCurrentDirectory + std::string("Rtti.ini");
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void load()
{
	IniManager iniReader(config_path);
	settings.auto_label_vftable = iniReader.readBoolean("settings", "auto_label_vftable", false);
	settings.skip_system_modules = iniReader.readBoolean("settings", "skip_system_modules", true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void save() 
{
	IniManager iniWriter(config_path);
	iniWriter.writeBoolean("settings", "auto_label_vftable", settings.auto_label_vftable);
	iniWriter.writeBoolean("settings", "skip_system_modules", settings.skip_system_modules);

	_plugin_menuentrysetchecked(pluginHandle, MENU_AUTO_LABEL_VFTABLE, settings.auto_label_vftable);
	_plugin_menuentrysetchecked(pluginHandle, MENU_SKIP_SYSTEM_MODULES, settings.skip_system_modules);
}

};
};