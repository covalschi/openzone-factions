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
// а в кожній фракції окремо (OZ_Core_Factions.json, поле MaxMembers): у
// кожного угруповання свій штат, і редагується він через адмінську консоль.
class OZF_FactionLimits
{
    // Скільки живе запрошення у фракцію. Довше -- і гравець приймає
    // запрошення від лідера, який давно передумав.
    int InviteTtlSeconds = 120;
}

// ЧУЖИЙ ФАЙЛ НАШИМИ ОЧИМА: конфіг ядра, з якого нас цікавить один розділ.
//
// Клас-оглядач оголошує ЛИШЕ те, що читає. Решту ключів (а там і секрет моста)
// JsonFileLoader мовчки пропускає -- зміряно, див. Inherit нижче, -- і саме
// тому читати чужий файл так безпечно: чого немає в класі, того немає й у
// пам'яті.
class OZF_LegacyCoreSettings
{
    ref OZF_FactionLimits Faction;
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

    static OZF_Settings Get()
    {
        return s_Inst;
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

        if (!Faction)
            Faction = new OZF_FactionLimits();

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

        // Чи файл узагалі новий -- питаємо ДО завантаження: далі його вже
        // створить сам загрузчик, і відповідь стане неправдою.
        bool first = !FileExist(OZF_Const.SETTINGS);

        s_Inst = new OZF_Settings();
        OZ_ConfigLoader<OZF_Settings>.Load(OZF_Const.SETTINGS, "Factions settings", s_Inst);

        if (first)
            Inherit();
    }

    // РАЗ І ТІЛЬКИ РАЗ: забрати старий розділ "Faction" із конфігу ядра.
    //
    // Навіщо взагалі. До 2026-09-04 межі фракцій жили в OZ_Core_Settings.json,
    // і на живому сервері там стоїть число, яке адмін виставив свідомо.
    // Просто перестати його читати означало б мовчки повернути умовчання --
    // тобто зміну поведінки, про яку ніхто не дізнається, поки не помітить,
    // що запрошення живуть не стільки, скільки домовлялись.
    //
    // Чому за наявністю НАШОГО файла, а не за прапорцем «мігрували». Файла
    // немає -- ми тут уперше, і брати чуже число безпечно. Файл є -- він
    // головний, хоч би що лишалось у ядрі: адмін, який виправив НАШ файл, не
    // мусить щоразу боротись зі старим числом сусіда.
    //
    // Ключ "Faction" з конфігу ядра НЕ ПРИБИРАЄМО. Той файл не наш, ядро його
    // перезаписує саме, а зайвий розділ JsonFileLoader мовчки пропускає --
    // тобто шкоди від нього нуль, а лізти чужим файлом у чужу теку заради
    // косметики -- це якраз той спосіб втратити секрет моста, від якого ядро
    // й бережеться.
    //
    // ЩО ТУТ ЗМІРЯНО (стенд, 2026-09-04, script_2026-09-04_19-26-38.log --
    // два засіяні файли, у першому розділ "Faction" стоїть ПОСЕРЕД чужих
    // ключів, у другому його немає зовсім):
    //
    //   PROBE A-section-present: loadfile=ok alloc=yes ttl=300
    //   PROBE B-section-absent:  loadfile=ok alloc=yes ttl=0
    //
    //   1. ВІДОМИЙ КЛЮЧ, ЩО СТОЇТЬ ЗА НЕВІДОМИМИ, ЧИТАЄТЬСЯ. Клас-оглядач з
    //      одним полем `ref OZF_FactionLimits Faction` дістав з чужого файла
    //      свої 300 -- отже, окремий розбір тексту тут не потрібен, і ~78
    //      рядків ручного різання по фігурних дужках (FactionSection/ReadAll)
    //      цей файл більше не носить. Попередня спроба провалилась не через
    //      розбір, а через п.3: розділу в живому файлі вже не було.
    //   2. ВІДСУТНЄ `ref`-ПОЛЕ ПОВЕРТАЄТЬСЯ СТВОРЕНИМ І ОБНУЛЕНИМ. Не null:
    //      об'єкт є, а ініціалізатор поля (InviteTtlSeconds = 120) НЕ
    //      застосовано -- всередині нуль. Тому `if (!old.Faction)` не може
    //      відповісти «а чи прочитали», і ознакою «не прочитано» служить сам
    //      НУЛЬ: у файлі його не буває, Validate не пускає нижче десяти.
    //
    // І ДРУГЕ МІСЦЕ, ДЕ ШУКАТИ, -- РЕЗЕРВНА КОПІЯ:
    //
    //   3. ядро перечитує свій конфіг і ПЕРЕЗАПИСУЄ його щоразу, коли Validate
    //      щось поправив, -- і в перезаписі розділу "Faction" уже немає, бо
    //      поля з таким іменем у ядрі більше немає;
    //   4. порядок модулів CF не гарантований, тобто ми не можемо стати
    //      попереду ядра й прочитати файл до того.
    //
    // Але перед тим перезаписом ядро КЛАДЕ КОПІЮ -- OZ_ConfigLoader.Save
    // робить її сам, під іменем тега (OZ_Const.SETTINGS_BAK).
    //
    // ЯКЕ ВІКНО ЦЕ ДАЄ НАСПРАВДІ -- чесно, бо від цього залежить порада адміну:
    //
    //   * сервер, у якого Validate ядра щось лає кожен старт (найзвичайніший
    //     випадок -- міст по http: «Bridge.Url is not https»), має рівно ОДИН
    //     старт. На першому ядро перезаписує живий файл без розділу; на
    //     другому воно ж копіює цей уже безрозділовий файл ПОВЕРХ копії, і
    //     розділу не лишається ніде. Тому мод фракцій треба піднімати на
    //     першому ж старті після оновлення ядра;
    //   * сервер, у якого Validate ядра мовчить, свій конфіг не переписує
    //     взагалі -- там розділ і вікно живуть скільки завгодно.
    //
    // КОЖЕН ВИХІД КАЖЕ, ЧОМУ ВИЙШЛИ, І ВИХІД «НЕМАЄ ЗВІДКИ» -- ГОЛОСНО.
    // Міграція, яка мовчки не спрацювала, виглядає точно як міграція, якій не
    // було чого переносити, а різниця між ними для адміна в тому, чи стоїть у
    // нього те число, яке він ставив. Dbg тут не годиться: на бойовому сервері
    // DebugMode вимкнено, і єдиний наслідок, заради якого вся ця машинерія
    // існує, не лишив би в лозі жодного рядка.
    private static void Inherit()
    {
        int ttl = OldTtl(OZ_Const.SETTINGS);
        if (ttl <= 0)
            ttl = OldTtl(OZ_Const.SETTINGS_BAK);

        if (ttl <= 0)
        {
            string none = "factions settings: no Faction section in the core config or its backup, InviteTtlSeconds stands at the default ";
            none += s_Inst.Faction.InviteTtlSeconds.ToString();
            none += " - if this server ran another number, set it by hand in ";
            none += OZF_Const.SETTINGS;
            OZ_Log.Warn(none);
            return;
        }

        s_Inst.Faction.InviteTtlSeconds = ttl;

        int warnings;
        s_Inst.Validate(warnings);
        OZ_ConfigLoader<OZF_Settings>.Save(OZF_Const.SETTINGS, "Factions settings", s_Inst);

        string line = "factions settings: inherited Faction.InviteTtlSeconds=";
        line += s_Inst.Faction.InviteTtlSeconds.ToString();
        line += " from the core config - edit it in OZ_Factions_Settings.json from now on";
        OZ_Log.Info(line);
    }

    // Скільки секунд стояло в розділі "Faction" ЦЬОГО файла. Нуль означає
    // «нічого не взяли» -- і байдуже, чи файла немає, чи він не розібрався,
    // чи розділу в ньому не було: діяти в усіх трьох випадках однаково.
    //
    // У лог їде ЛИШЕ це число й ім'я файла. Вміст чужого конфігу не друкуємо
    // ніколи: у ньому секрет моста.
    private static int OldTtl(string path)
    {
        if (!FileExist(path))
            return 0;

        OZF_LegacyCoreSettings old = new OZF_LegacyCoreSettings();

        string err;
        if (!JsonFileLoader<OZF_LegacyCoreSettings>.LoadFile(path, old, err))
        {
            OZ_Log.Warn("factions settings: cannot read " + path + " for the old Faction section: " + err);
            return 0;
        }

        if (!old.Faction)
            return 0;

        string got = "factions settings: ";
        got += path;
        got += " gives Faction.InviteTtlSeconds=";
        got += old.Faction.InviteTtlSeconds.ToString();
        OZ_Log.Dbg(got);

        return old.Faction.InviteTtlSeconds;
    }
}
