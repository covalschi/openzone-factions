// Вкладка «Фракція» в КПК -- ОКРЕМИЙ pbo, як і вкладка рації.
//
// Жорстко вимагає обидва боки: без фракційної системи малювати нічого, без
// КПК немає де. Саме тому це склейка, а не частина котрогось із них: ані КПК,
// ані мод фракцій не мусять знати одне про одного, і кожен ставиться окремо.
//
// Сервер без цього pbo просто не має вкладки. Контакти, чат і карта при цьому
// працюють: вони питають ядро (OZ_Identity), а воно без мода фракцій чесно
// відповідає «фракції немає» -- і поділ на своїх і чужих зникає разом із нею,
// не ламаючи жодного екрана.

class CfgPatches
{
    class OpenZone_Factions_PDA
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
            "OpenZone_PDA"
        };
    };
};

class CfgMods
{
    class OpenZone_Factions_PDA
    {
        dir = "OpenZone_Factions_PDA";
        name = "OpenZone Factions PDA Page";
        author = "Zone Protocol";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            // НАШ ЗНАЧОК -- У НАШОМУ НАБОРІ (рішення власника 2026-09-09).
            // Картинка вкладки «Фракція» лежала в атласі КПК: мод малював
            // сторінку, а її значок ніс сусід, який про фракції не знає. Тепер
            // набір їде разом зі склейкою, тим самим способом, яким його везе
            // будь-який мод: рядок тут плюс .imageset поруч зі своїм _ca.paa
            // всередині pbo. Реєструє його ТА САМА конструкція, що й у КПК і
            // у VPPAdminTools, -- перелік файлів у defs мода.
            class imageSets { files[] = {"OpenZone_Factions_PDA/gui/imagesets/oz_factions_icons.imageset"}; };
            class gameScriptModule    { value = ""; files[] = {"OpenZone_Factions_PDA/scripts/3_Game"}; };
            class worldScriptModule   { value = ""; files[] = {"OpenZone_Factions_PDA/scripts/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"OpenZone_Factions_PDA/scripts/5_Mission"}; };
        };
    };
};
