#pragma once
#include <string>

namespace RTTI {;
namespace Config {;

struct settings_t
{
	bool auto_label_vftable;
	bool skip_system_modules;
};

// Load the configuration file with the settings of the plugin
void load();

// Save the configuration file for the settings of the plugin
void save();

// Set the config path to the current directory
void setPath();

extern std::string config_path;
extern settings_t settings;

};
};