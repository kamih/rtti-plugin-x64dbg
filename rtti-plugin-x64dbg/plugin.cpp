#include "plugin.h"
#include "Lib\ini.h"
#include "Lib\config.h"
#include "Lib\Rtti.h"
#include <set>

#define RTTI_COMMAND "rtti"
#define RTTI_PLUGIN_VERSION "2"

using namespace RTTI;

// Set of modules we know don't have RTTI (or/and are slow to scan)
std::set<std::string> gScanIgnoreModules;

// Scans all module .rdata sections for RTTI data
void ScanMemForRTTI()
{
	setLogFileName(L"rtti_log.txt");
	dputs("Start of module RTTI scan.");
	auto dbgFn = DbgFunctions();
	dbgFn->RefreshModuleList();
	dbgFn->MemUpdateMap();
	MEMMAP mm{0};
	DbgMemMap(&mm);
	for (int i = 0; i < mm.count; ++i) {
		guiStatusBarPrintf("[" PLUGIN_NAME "] Scanning for RTTI data: %d/%d\n", i, mm.count);
		GuiProcessEvents();
		auto &p = mm.page[i];
		// We're only interested in rdata sections
		if (p.mbi.Type != MEM_IMAGE || p.mbi.Protect != PAGE_READONLY || !strstr(p.info, "\".rdata\""))
			continue;
		const duint secBase = (duint)p.mbi.BaseAddress;
		const duint modBase = dbgFn->ModBaseFromAddr(secBase);
		if (!modBase) {
			dprintf("ERROR: couldn't get modbase from secBase: 0x%p!\n", secBase);
			continue;
		}
		// Skip system modules if option is set
		if (Config::settings.skip_system_modules && dbgFn->ModGetParty(modBase) == mod_system)
			continue;
		char modName[256] = {0};
		if (!dbgFn->ModNameFromAddr(secBase, modName, true)) {
			dprintf("ERROR: couldn't get mod name from secBase: 0x%p!\n", secBase);
			continue;
		}
		// If this is in our module ignore list, skip it
		if (gScanIgnoreModules.count(modName))
			continue;
		TypeInfo::scanSection(modName, modBase, secBase, p.mbi.RegionSize);
	}
	dputs("End of module RTTI scan. Log saved to: rtti_log.txt");
	closeLogFile();
}

// Checks the settings and auto-labels the enabled ones
bool AutoLabel(const TypeInfo &rtti)
{
	if (!rtti.valid())
		return false;

	if (Config::settings.auto_label_vftable)
	{
		char sz_vftable_label[MAX_COMMENT_SIZE] = "";

		// If there isn't a label already there
		if (!DbgGetLabelAt(rtti.getVFTablep(), SEG_DEFAULT, sz_vftable_label))
		{
			std::string label = rtti.getTypeName() + "::vftable";
			if (!DbgSetLabelAt(rtti.getVFTablep(), label.c_str()))
				return false;
		}
	}

	return true;
}

// Get the current window selection, aligns it to 4 byte boundaries and dumps it
void DumpRttiWindow(GUISELECTIONTYPE hWindow)
{
	if (!DbgIsDebugging())
	{
		dputs("You need to be debugging to use this command");
		return;
	}

	SELECTIONDATA sel;
	GuiSelectionGet(hWindow, &sel);
	duint alignedStart = sel.start - (sel.start % (sizeof duint));

	char cmd[256] = { 0 };
	sprintf_s(cmd, "%s %zX", RTTI_COMMAND, alignedStart);

	// Run the cbRttiCommand
	DbgCmdExec(cmd);
}

// 'rtti <addr>' command
static bool cbRttiCommand(int argc, char* argv[])
{
	if (argc > 2)
	{
		dprintf("Usage: rtti <address>\n");
		return false;
	}

	// command 'rtti' - Assume the selected bytes from the dump window
	if (argc == 1)
	{
		DumpRttiWindow(GUI_DUMP);
	}
	// command 'rtti <address>'
	else if (argc == 2)
	{
		duint addr = 0;
		int numFieldsAssigned = sscanf_s(argv[1], "%zx", &addr);

		if (numFieldsAssigned != 1)
		{
			dprintf("Usage: rtti <address>\n");
			return false;
		}
		TypeInfo rtti = TypeInfo::fromObjectThisAddr(addr, true);
		if (rtti.valid())
		{
			AutoLabel(rtti);
			auto &rttiInfo = rtti.getClassHierarchyString();
			dprintf("%s\n", rttiInfo.c_str());
		}
		else
			dprintf("No RTTI information found for address %p\n", addr);
	}

	return true;
}

PLUG_EXPORT void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
	switch(info->hEntry)
	{
	case MENU_AUTO_LABEL_VFTABLE:
		Config::settings.auto_label_vftable = !Config::settings.auto_label_vftable;
		Config::save();
		break;
	case MENU_SKIP_SYSTEM_MODULES:
		Config::settings.skip_system_modules = !Config::settings.skip_system_modules;
		Config::save();
		break;
	case MENU_DUMP_RTTI:
		DumpRttiWindow(GUI_DUMP);
		break;
	case MENU_ABOUT:
		MessageBoxA(GuiGetWindowHandle(), "RTTI plugin version v" RTTI_PLUGIN_VERSION "\n\nhttps://github.com/kamih/rtti-plugin-x64dbg", "About", 0);
		break;
	case MENU_SCAN:
		ScanMemForRTTI();
		break;
	default:
		break;
	}
}

//Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
	_plugin_registercommand(pluginHandle, RTTI_COMMAND, cbRttiCommand, true);
	return true; //Return false to cancel loading the plugin.
}

//Deinitialize your plugin data here.
void pluginStop()
{
	_plugin_unregistercommand(pluginHandle, RTTI_COMMAND);
}

//Do GUI/Menu related things here.
void pluginSetup()
{
	Config::setPath();
	Config::load();

	int optMenu = _plugin_menuadd(hMenu, "Options");
	_plugin_menuaddentry(optMenu, MENU_AUTO_LABEL_VFTABLE, "Auto-label vftable");
	_plugin_menuaddentry(optMenu, MENU_SKIP_SYSTEM_MODULES, "Skip system modules");

	_plugin_menuaddentry(hMenuDump, MENU_DUMP_RTTI, "&Dump Rtti");

	// About menu
	_plugin_menuaddentry(hMenu, MENU_ABOUT, "&About");

	// Scan menu
	_plugin_menuaddentry(hMenu, MENU_SCAN, "&Scan memory for RTTI");

	// Update the checked status
	_plugin_menuentrysetchecked(pluginHandle, MENU_AUTO_LABEL_VFTABLE, Config::settings.auto_label_vftable);
	_plugin_menuentrysetchecked(pluginHandle, MENU_SKIP_SYSTEM_MODULES, Config::settings.skip_system_modules);

	// Init module ignore set
	gScanIgnoreModules.insert("kernelbase.dll");
	gScanIgnoreModules.insert("kernel32.dll");
	gScanIgnoreModules.insert("ntdll.dll");
	gScanIgnoreModules.insert("cggl.dll");
	gScanIgnoreModules.insert("cg.dll");
	gScanIgnoreModules.insert("libmmd.dll");
	gScanIgnoreModules.insert("resampledmo.dll");
	gScanIgnoreModules.insert("dsound.dll");
	gScanIgnoreModules.insert("opengl32.dll");
	gScanIgnoreModules.insert("icm32.dll");
	gScanIgnoreModules.insert("sti.dll");
	gScanIgnoreModules.insert("msacm32.dll");
	gScanIgnoreModules.insert("c_g18030.dll");
	gScanIgnoreModules.insert("acgenral.dll");
	gScanIgnoreModules.insert("glu32.dll");
	gScanIgnoreModules.insert("iertutil.dll");
	gScanIgnoreModules.insert("wininet.dll");
	gScanIgnoreModules.insert("coreuicomponents.dll");
	gScanIgnoreModules.insert("dwrite.dll");
	gScanIgnoreModules.insert("d2d1.dll");
	gScanIgnoreModules.insert("windows.storage.dll");
	gScanIgnoreModules.insert("windows.ui.dll");
	gScanIgnoreModules.insert("windows.staterepositoryps.dll");
	gScanIgnoreModules.insert("windows.storage.search.dll");
	gScanIgnoreModules.insert("igc64.dll");
	gScanIgnoreModules.insert("nvwgf2umx.dll");
	gScanIgnoreModules.insert("nvoglv64.dll");
	gScanIgnoreModules.insert("shell32.dll");
	gScanIgnoreModules.insert("onecoreuapcommonproxystub.dll");
}
