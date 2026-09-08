// Налаштування мода фракцій -- ВЛАСНИЙ файл, а не розділ у конфігу ядра.
//
// $profile:OpenZone\OZ_Factions_Settings.json, через ту саму конфіг-службу
// ядра, якою користуються КПК (OZ_PDA_Profiles.json, OZ_PDA_Tuning.json),
// рація (OZ_Radio_Settings.json) і наш же файл спорядження
// (OZ_Factions_Loadouts.json): OZ_ConfigBase + OZ_ConfigLoader<T>.
//
// Головне правило конфіг-служби діє й тут: КОНФІГ НІКОЛИ НЕ Є ПРИЧИНОЮ НЕ
// ЗАВАНТАЖИТИСЬ. Зіпсований файл їде в карантин, на його місце стають
// умовчання, сервер піднімається.
//
// СЕРВЕРНИЙ. Клієнтові звідси не потрібне жодне число: строк життя
// запрошення перевіряє той, хто запрошення й тримає.

// Фракційні межі -- рішення сервера, не Discord. Стеля складу живе НЕ тут,
// а в кожній фракції окремо (OZ_Factions.json, поле MaxMembers): у
// кожного угруповання свій штат, і редагується він через адмінську консоль.
class OZF_FactionLimits
{
    // Скільки живе запрошення у фракцію. Довше -- і гравець приймає
    // запрошення від лідера, який давно передумав.
    int InviteTtlSeconds = 120;

    // Копія в об'єкт, який зробив скрипт (шапка OZ_ConfigBase ядра, зміряно
    // 2026-09-06). s_Inst живе весь запуск сервера, і це число читає кожне
    // запрошення -- через години після розбору файла.
    OZF_FactionLimits Copy()
    {
        OZF_FactionLimits c = new OZF_FactionLimits();
        c.InviteTtlSeconds = InviteTtlSeconds;
        return c;
    }
}

class OZF_Settings : OZ_ConfigBase
{
    // Межі строку життя запрошення -- КОНСТАНТАМИ, бо обидві мають причину.
    //
    // Низ: під десятьма секундами запрошення не встигає доїхати до людини.
    // Верх: строк іде в int МІЛІСЕКУНДАМИ -- OZ_FactionInvites рахує
    // ExpiresAt = GetGame().GetTime() + InviteTtlSeconds * 1000, -- тож уже
    // близько 2 147 483 секунд int32 переповнюється, ExpiresAt стає від'ємним
    // і КОЖНЕ запрошення протухає в ту саму мить, коли його видали. Це відмова,
    // яку з логу не пояснити ніяк. Доба -- це вже не строк, а «назавжди», тому
    // стеля стоїть на три порядки нижче за переповнення: сенсу над нею немає,
    // а запас є.
    static const int INVITE_TTL_MIN = 10;
    static const int INVITE_TTL_MAX = 86400;

    ref OZF_FactionLimits Faction;

    private static ref OZF_Settings s_Inst;

    // ЧИ МОЖНА ПИСАТИ ПОВЕРХ ФАЙЛА. Відповідь дає сам загрузчик, і мовчазне
    // «так» коштувало б адмінського файла: `false` означає «на диску лежить
    // єдиний примірник, якого ми не зрозуміли і не змогли винести в карантин».
    //
    // Той самий взірець, що в ядрі (OZ_Settings.Writable, OZ_Spawns.s_Writable).
    private static bool s_Writable = true;

    static OZF_Settings Get()
    {
        return s_Inst;
    }

    static bool Writable()
    {
        return s_Writable;
    }

    override int LatestVersion()
    {
        return 1;
    }

    // Виставляє КОЖНЕ поле: кличеться і на порожньому об'єкті, і поверх
    // напівпрочитаного після невдалого розбору.
    override void LoadDefaults()
    {
        Version = LatestVersion();
        Faction = new OZF_FactionLimits();
    }

    // Кожне зауваження -- окремий Warning. Завантаження НЕ валиться.
    override void Validate(out int warnings)
    {
        warnings = 0;

        // ВКЛАДЕНЕ -- У СТВОРЕНЕ СКРИПТОМ, до першого ж рядка попередження.
        // Розділу може не бути у файлі зовсім: тоді серіалізатор віддає його
        // створеним і обнуленим, а не null (шапка OZ_ConfigBase ядра), і
        // саме тут це й зміряли -- зонд A/B цього файла.
        if (!Faction)
            Faction = new OZF_FactionLimits();
        else
            Faction = Faction.Copy();

        // СПРАВЖНЄ УМОВЧАННЯ ПІСЛЯ КОПІЇ. Ключа, якого у файлі немає, копія
        // переносить нулем, а не сто двадцяткою з ініціалізатора; нуль
        // секунд запрошення не означає нічого, тож віддаємо задокументоване
        // число мовчки, а не через нижній кламп, який назвав би його
        // десяткою.
        if (Faction.InviteTtlSeconds == 0)
            Faction.InviteTtlSeconds = 120;

        if (Faction.InviteTtlSeconds < INVITE_TTL_MIN)
        {
            OZ_Log.Warn("Faction.InviteTtlSeconds under 10 s cannot be read in time, clamped to 10");
            Faction.InviteTtlSeconds = INVITE_TTL_MIN;
            warnings++;
        }

        if (Faction.InviteTtlSeconds > INVITE_TTL_MAX)
        {
            OZ_Log.Warn("Faction.InviteTtlSeconds over a day overflows the invitation clock, clamped to 86400");
            Faction.InviteTtlSeconds = INVITE_TTL_MAX;
            warnings++;
        }
    }

    static void ServerLoad()
    {
        OZ_Json.EnsureTree();

        s_Inst = new OZF_Settings();
        s_Writable = OZ_ConfigLoader<OZF_Settings>.Load(OZF_Const.SETTINGS, "Factions settings", s_Inst);
    }
}
