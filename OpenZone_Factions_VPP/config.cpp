// Панель «FACTIONS» адмінського вікна OpenZone -- ОКРЕМИЙ pbo.
//
// requiredAddons нижче -- ЖОРСТКА залежність, тобто блокуюче вікно ще до
// завантаження, а не тихий пропуск. Тут це саме те, що треба: панель без
// фракцій нічого не малює, а без ядрового вікна їй нема до чого чіплятись.
//
// Сервер без цього pbo бачить у вкладці OpenZone тільки те, що належить
// ядру, -- спавни й редактор конфігів. Це не збіднена версія, а чесна: усе
// решта й справді не встановлене.
//
// DZM_VPPAdminToolsScripts -- клас CfgPatches скриптового pbo самого VPP.
// #ifdef у коді гардить на AVPPAdminTools: рушій авто-дефайнить імена класів
// CfgMods, а не CfgPatches (зміряно 2026-07-31 на 1.29 diag).

class CfgPatches
{
    class OpenZone_Factions_VPP
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "OpenZone_Core",
            "OpenZone_Factions",
            "OpenZone_VPP",
            "DZM_VPPAdminToolsScripts"
        };
    };
};

class CfgMods
{
    class OpenZone_Factions_VPP
    {
        dir = "OpenZone_Factions_VPP";
        name = "OpenZone Factions VPP Pane";
        author = "Zone Protocol";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Mission"};

        class defs
        {
            class missionScriptModule { value = ""; files[] = {"OpenZone_Factions_VPP/scripts/5_Mission"}; };
        };
    };
};
