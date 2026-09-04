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

// Фракцiйнi межi -- рiшення сервера, не Discord. Стеля складу живе НЕ тут,
// а в кожнiй фракцiї окремо (OZ_Core_Factions.json, поле MaxMembers): у
// кожного угруповання свiй штат, i редагується вiн через адмiнську консоль.
class OZF_FactionLimits
{
    // Скiльки живе запрошення у фракцiю. Довше -- i гравець приймає
    // запрошення вiд лiдера, який давно передумав.
    int InviteTtlSeconds = 120;
}

class OZF_Settings : OZ_ConfigBase
{
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

        if (Faction.InviteTtlSeconds < 10)
        {
            OZ_Log.Warn("Faction.InviteTtlSeconds under 10 s cannot be read in time, clamped to 10");
            Faction.InviteTtlSeconds = 10;
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
    // ЧОМУ РІЖЕМО РОЗДІЛ ТЕКСТОМ, А НЕ ЧИТАЄМО ФАЙЛ КЛАСОМ-ОГЛЯДАЧЕМ.
    //
    // Перша спроба була саме така: клас з одним полем `ref OZF_FactionLimits
    // Faction` і JsonFileLoader<T>.LoadFile на чужому файлі. Зміряно на стенді
    // 2026-09-04 -- НЕ ПРАЦЮЄ І НЕ КАЖЕ ПРО ЦЕ: LoadFile повернув успіх,
    // об'єкт Faction створився (тобто перевірка «а чи він є» пройшла), а число
    // всередині лишилось нулем, і 120 адміна тихо стали десяткою після клампа.
    // Ядро той самий файл читає без жодної скарги -- зайвий ключ воно
    // пропускає, -- тож пропуск НЕВІДОМОГО ключа й пропуск ВІДОМОГО, що стоїть
    // за невідомими, -- різні речі, і покладатись на другу не можна.
    //
    // Тому читачеві дістається документ, у якому він знає кожен ключ: сам
    // розділ, вирізаний за фігурними дужками. Вкладених об'єктів у ньому не
    // було ніколи (одне ціле число), тож перша ж закривна дужка -- його.
    //
    // І ДРУГЕ МІСЦЕ, ДЕ ШУКАТИ, -- РЕЗЕРВНА КОПІЯ. Зміряно на стенді
    // 2026-09-04, і без цього міграція не спрацювала жодного разу:
    //
    //   1. ядро перечитує свій конфіг і ПЕРЕЗАПИСУЄ його щоразу, коли
    //      Validate щось поправив (а на будь-якому стенді з http-мостом
    //      Validate сварить кожен старт), -- і в перезаписі розділу "Faction"
    //      вже немає, бо поля з таким іменем у ядрі більше немає;
    //   2. порядок модулів CF не гарантований, тобто ми не можемо стати
    //      попереду ядра й прочитати файл до того.
    //
    // Але перед тим перезаписом ядро КЛАДЕ КОПІЮ -- OZ_ConfigLoader.Save
    // робить її сам, під іменем тега. Тож на першому ж старті після оновлення
    // розділ є або в живому файлі (якщо ми встигли раніше), або в копії (якщо
    // ні). Дивимось в обидва -- і перегони перестають бути перегонами.
    //
    // КОЖЕН ВИХІД КАЖЕ, ЧОМУ ВИЙШЛИ. Міграція, яка мовчки не спрацювала,
    // виглядає точно як міграція, якій не було чого переносити, -- а різниця
    // між ними для адміна в тому, чи стоїть у нього те число, яке він ставив.
    private static void Inherit()
    {
        string body = FactionSection(OZ_Const.SETTINGS);
        if (body == "")
            body = FactionSection(OZ_Const.BACKUP_DIR + "\\Settings.bak.json");

        if (body == "")
        {
            OZ_Log.Dbg("factions settings: neither the core config nor its backup carries a Faction section, defaults stand");
            return;
        }

        // НУЛЬ ЯК ОЗНАКА «НЕ ПРОЧИТАНО». Умовчання поля -- 120, і лишити його
        // означало б не відрізнити прочитану сто двадцятку від непрочитаного
        // нічого. Нуль такою відповіддю бути не може: Validate не пускає нижче
        // десяти, тобто у файлі його не буває.
        OZF_FactionLimits old = new OZF_FactionLimits();
        old.InviteTtlSeconds = 0;

        string err;
        if (!JsonFileLoader<OZF_FactionLimits>.LoadData(body, old, err))
        {
            OZ_Log.Warn("factions settings: the old Faction section of the core config is unreadable, keeping defaults: " + err);
            return;
        }

        if (old.InviteTtlSeconds <= 0)
        {
            OZ_Log.Warn("factions settings: the old Faction section gave no InviteTtlSeconds, keeping the default");
            return;
        }

        s_Inst.Faction.InviteTtlSeconds = old.InviteTtlSeconds;

        int warnings;
        s_Inst.Validate(warnings);
        OZ_ConfigLoader<OZF_Settings>.Save(OZF_Const.SETTINGS, "Factions settings", s_Inst);

        string line = "factions settings: inherited Faction.InviteTtlSeconds=";
        line += s_Inst.Faction.InviteTtlSeconds.ToString();
        line += " from the core config - edit it in OZ_Factions_Settings.json from now on";
        OZ_Log.Info(line);
    }

    // Розділ "Faction" цього файла як самостійний JSON -- або порожньо.
    private static string FactionSection(string path)
    {
        string all = ReadAll(path);
        if (all == "")
            return "";

        int at = all.IndexOf("\"Faction\"");
        int ob = -1;
        int cb = -1;

        if (at >= 0)
            ob = all.IndexOfFrom(at, "{");
        if (ob >= 0)
            cb = all.IndexOfFrom(ob, "}");

        if (cb < 0)
            return "";

        return all.Substring(ob, cb - ob + 1);
    }

    // Увесь файл одним рядком. Пробіл між рядками навмисний: FGets віддає
    // рядок без переводу, а склеєні впритул два рядки могли б зліпити два
    // токени в один.
    private static string ReadAll(string path)
    {
        if (!FileExist(path))
            return "";

        FileHandle fh = OpenFile(path, FileMode.READ);
        if (!fh)
            return "";

        string all  = "";
        string line = "";

        // МЕЖА ЦИКЛУ, а не `while (true)` з довірою до FGets. Рушій обіцяє -1
        // на кінці файла, але в тій самій довідці поруч стоїть «кінець файла
        // -- ПОРОЖНІЙ РЯДОК», а нуль наш вихід не спиняє (нуль -- це просто
        // порожній рядок посеред файла). Дві обіцянки, які суперечать одна
        // одній, у циклі без стелі означають підвішений сервер; стеля коштує
        // один int.
        int guard = 0;
        while (guard < 8192)
        {
            if (FGets(fh, line) < 0)
                break;

            all += line;
            all += " ";
            guard++;
        }

        CloseFile(fh);

        string read = "factions settings: read ";
        read += all.Length().ToString();
        read += " bytes of ";
        read += path;
        OZ_Log.Dbg(read);

        return all;
    }
}
