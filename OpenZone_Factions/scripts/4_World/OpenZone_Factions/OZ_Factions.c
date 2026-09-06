// Фракції -- служба ЯДРА, а не КПК.
//
// Живуть тут, бо фракціями користується не лише екран. Квестовий мод питає,
// чи свій перед ним; торговець -- чи продавати; ІІ -- чи стріляти; рація --
// кого пускати в канал. Якби таблиця лишалась у КПК, кожен із них тягнув би
// за собою мод ІНТЕРФЕЙСУ заради питання «хто в якій фракції». Тут її може
// спитати будь-хто, хто вже залежить від ядра.
//
// Ядро фракції НЕ ПРИДУМУЄ і нікого до них не приписує. Воно тримає таблицю
// (як звати, яким кольором малювати, хто кому ворог) і відповідає на питання
// «чия фракція». Хто до кого належить -- вирішує або чужий мод через
// постачальника, або ролі Discord, або адмін у файлі гравця.
//
// ПОРЯДОК СТАРШИНСТВА, і його треба знати, бо він визначає, хто кого
// перебиває:
//
//     1. постачальник (чужий мод)   -- знає найкраще, сказав слово -- воно й
//                                      буде
//     2. роль Discord               -- якщо акаунт прив'язаний і роль
//                                      відповідає фракції з таблиці
//     3. файл акаунта               -- те, що поставив адмін або квест
//
// Порожня фракція означає «одинак». Це не помилка й не збій -- це найчастіший
// стан у Зоні.

// Ставлення однієї фракції до іншої.
//
// Односторонньо в JSON, СИМЕТРИЧНО за замовчуванням у відповіді: якщо «Борг»
// оголосив ворожість «Волі», а «Воля» промовчала, вони все одно вороги.
// Односторонню ворожість можна задати явно -- просто напиши обидва рядки.
class OZ_FactionRelation
{
    string With;
    string Stand;
}

// Один запис таблиці.
//
// БЕЗ ІНІЦІАЛІЗАТОРІВ ПОЛІВ, навмисно: чужий мод має право успадкуватись від
// цього класу, а ініціалізатор поля в класі, від якого успадковуються, ламає
// нащадкам доступ до їхніх власних private-методів (виміряно 2026-08-26, див.
// скіл dayz-modding). Умовчання роздає Validate() -- він однаково мусить
// пройтись по записах, прочитаних із диска, де половини полів може не бути.
class OZ_Faction
{
    string Id;
    string DisplayName;

    // Коротка позначка для тісних місць: рядок контакту, підпис маячка.
    // Порожня -- малюй повну назву.
    string Short;

    // Колір рядком "R G B", 0..255. Не число ARGB: у JSON його читає ЛЮДИНА,
    // а 4278219546 не каже нікому нічого. ColorARGB() перетворює на те, що
    // розуміє віджет.
    string Color;

    // ПОЛІВ Joinable, Tags І Extra ТУТ БІЛЬШЕ НЕМАЄ (2026-09-06): їх
    // виставляли трьома місцями й не читав жоден рядок коду. Ключ, який
    // лишився в файлі адміна, JsonFileLoader мовчки пропускає.

    // Не показувати в переліках. Для службових фракцій: адміністрація,
    // сюжетні угруповання, які ще не мали з'явитись.
    bool Hidden;

    // БАЗОВА ІДЕНТИЧНІСТЬ, а не організація (рішення власника 2026-08-30).
    //
    // «Сталкери» -- це всі в Зоні: значок носить кожен, він не знімається при
    // вступі кудись іще, і в нього немає ні лідера, ні складу, ні внутрішніх
    // звань. Тому фракційний екран для такого запису НЕ ВІДКРИВАЄТЬСЯ: бачити
    // там поіменний список усіх сталкерів сервера і кнопку «вигнати» --
    // безглуздо й до того ж видало б людей, яких ніхто не питав.
    //
    // Для всього іншого це звичайна фракція: назва, колір, ставлення,
    // зона спавну працюють як завжди.
    bool BaseFaction;

    // Стеля складу САМЕ ЦІЄЇ фракції: скільки людей у ній може бути разом
    // (рахуються проекції ролей -- увесь відомий склад, не лише присутні).
    // 0 -- без межі. Рішення власника 2026-08-30: межа фракційна, не
    // серверна -- у «Долга» свій штат, у вчених свій.
    int MaxMembers;

    ref array<ref OZ_FactionRelation> Relations;
}

class OZ_FactionsConfig : OZ_ConfigBase
{
    ref array<ref OZ_Faction> Factions;

    override int LatestVersion()
    {
        return 4;
    }

    override void LoadDefaults()
    {
        Version  = LatestVersion();
        Factions = new array<ref OZ_Faction>();

        // Id -- КЛЮЧ ЗВ'ЯЗКУ між ботом і грою, і єдиний рядок, який обидві
        // сторони мусять знати посимвольно. Розбіжність тут не косметична:
        // синхронізація ролей просто не знайде нічого, мовчки.
        //
        // Назви -- УКРАЇНСЬКІ й ЖИВИМ ТЕКСТОМ, а не ключами stringtable.
        // Причина виміряна: у наборі мов рушія немає колонки ukrainian
        // (заголовок таблиці -- original, english, czech, german, russian,
        // polish, hungarian, italian, spanish, french, chinese, japanese,
        // portuguese, chinesesimp). Отже #STR_-ключ українською не
        // намалюється НІКОЛИ, і текст мусить їхати даними.
        //
        // Порожня фракція -- це «одинак», відсутність приналежності. Тому
        // окремого id `loner` НЕМАЄ: він означав би те саме, але поводився
        // інакше -- Stand("", x) дає NEUTRAL, а Stand("loner","loner") дав би
        // ALLY, тобто всі неприкаяні стали б союзниками одне одному. А от
        // `neutral` -- справжня організація: у неї є лідер.
        Add("ecolog",    "Вчені",      "ВЧН", "230 200  90");
        Add("duty",      "Долг",       "ДЛГ", "196  64  40");
        Add("freedom",   "Воля",       "ВОЛ", " 96 176  72");
        Add("mercenary", "Найманці",   "НАЙ", " 80 130 190");
        Add("neutral",   "Нейтрали",   "НЕЙ", "200 200 200");
        // «Нейтрали» -- органiзацiя з лiдером; вiльнi сталкери -- БАЗОВА
        // фракцiя (рiшення власника 2026-08-30): її носять усi, вона не
        // знiмається вступом кудись iще й не має нi лiдера, нi складу.
        // Порожнiй Id як був, так i лишається «одинаком» без приналежностi
        // взагалi -- це не те саме, що бути сталкером.
        Add("stalker",   "Сталкери",   "СТК", "216 192 112", true);
        Add("bandit",    "Бандити",    "БАН", "150 120  70");
        Add("clearsky",  "Чисте небо", "ЧН",  "120 190 200");
        Add("monolith",  "Моноліт",    "МНЛ", "170 150 220");
        Add("military",  "Військові",  "ВIЙ", "110 130  90");

        // Роздано НЕ повністю: тільки те, що в першоджерелі не обговорюється.
        // Решту хай ставить сервер -- його сюжету ми не знаємо.
        Relate("duty",     "freedom",  "hostile");
        Relate("bandit",   "neutral",  "hostile");
        Relate("monolith", "neutral",  "hostile");
        Relate("monolith", "duty",     "hostile");
        Relate("monolith", "freedom",  "hostile");
        Relate("monolith", "clearsky", "hostile");
        Relate("military", "bandit",   "hostile");
        Relate("military", "monolith", "hostile");
        Relate("ecolog",   "neutral",  "friendly");
        Relate("ecolog",   "clearsky", "friendly");
    }

    private void Add(string id, string name, string tag, string colour, bool baseFaction = false)
    {
        OZ_Faction f = new OZ_Faction();
        f.Id            = id;
        f.DisplayName   = name;
        f.Short         = tag;
        f.Color         = colour;
        f.Hidden        = false;
        f.BaseFaction   = baseFaction;
        f.Relations     = new array<ref OZ_FactionRelation>();
        Factions.Insert(f);
    }

    private void Relate(string from, string to, string stand)
    {
        for (int i = 0; i < Factions.Count(); i++)
        {
            if (Factions[i].Id != from)
                continue;

            OZ_FactionRelation r = new OZ_FactionRelation();
            r.With  = to;
            r.Stand = stand;
            Factions[i].Relations.Insert(r);
            return;
        }
    }

    // Справжня міграція, а не штамп версії.
    //
    // v1 не знав про Short і Relations. Файл
    // адміна з версією 1 треба ДОПОВНИТИ умовчаннями, а не перезаписати
    // нашими фракціями: там уже може стояти його власний список.
    override bool Migrate(int from)
    {
        if (from > LatestVersion())
            return false;

        if (!Factions)
            Factions = new array<ref OZ_Faction>();

        // v2 -> v3: два id перейменовані під набір, узгоджений із ботом.
        //
        // Перейменування id -- НЕ косметика: на нього посилаються поле
        // Faction у файлах гравців і рядки Relations. Тому міняємо і сам Id,
        // і кожне посилання на нього. Записи адміна, яких у новому наборі
        // немає (скажімо, `military`), лишаються НЕДОТОРКАНИМИ: його таблиця
        // -- його справа, а видалити чуже під час міграції означало б
        // втратити дані мовчки.
        if (from < 3)
        {
            Rename("ecologist", "ecolog",  "Ecologists", "Вчені");
            Rename("loner",     "neutral", "Loners",     "Нейтрали");
        }

        // v3 -> v4: з'явилась базова фракцiя. Позначаємо ЛИШЕ «сталкерiв» i
        // лише якщо запис у файлi є: чужi фракцiї адмiна це не чiпає, а
        // прапорець на них ставити нема за що.
        if (from < 4)
        {
            for (int i = 0; i < Factions.Count(); i++)
            {
                if (Factions[i].Id == "stalker")
                    Factions[i].BaseFaction = true;
            }
        }

        // Порожні поля роздасть Validate() -- він біжить одразу після
        // міграції й робить рівно цю роботу для будь-якого запису з диска.
        Version = LatestVersion();
        OZ_Log.Dbg("factions migrated from v" + from.ToString());
        return true;
    }

    // Перейменування тягне за собою ПІДПИС, інакше запис виходить із
    // міграції суперечливим сам собі.
    //
    // Було: Id стає "neutral", DisplayName лишається "Loners" -- тобто фракція
    // з новим іменем і старим поняттям на екрані. Це видно й зараз у файлі
    // стенду. Реєстр із бота перекриває підпис на льоту, тож помітно стає саме
    // тоді, коли моста немає, -- у найгіршу мить.
    //
    // Міняємо ЛИШЕ якщо адмін підпису не чіпав, тобто там досі стоїть старе
    // умовчання. Свій підпис -- його справа, і затирати його міграцією
    // означало б красти роботу.
    private void Rename(string oldId, string newId, string wasLabel, string nowLabel)
    {
        bool touched = false;

        for (int i = 0; i < Factions.Count(); i++)
        {
            if (Factions[i].Id == oldId)
            {
                Factions[i].Id = newId;

                if (Factions[i].DisplayName == wasLabel)
                    Factions[i].DisplayName = nowLabel;

                touched = true;
            }

            if (!Factions[i].Relations)
                continue;

            for (int j = 0; j < Factions[i].Relations.Count(); j++)
            {
                if (Factions[i].Relations[j].With != oldId)
                    continue;
                Factions[i].Relations[j].With = newId;
                touched = true;
            }
        }

        if (touched)
            OZ_Log.Info("factions: renamed \"" + oldId + "\" to \"" + newId + "\"");
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Factions)
            Factions = new array<ref OZ_Faction>();

        for (int i = 0; i < Factions.Count(); i++)
        {
            OZ_Faction f = Factions[i];

            if (f.Id == "")
            {
                OZ_Log.Warn("faction #" + i.ToString() + " has no Id - it will never match anybody");
                warnings++;
            }

            if (f.DisplayName == "")
                f.DisplayName = f.Id;

            if (f.Color == "")
                f.Color = "200 200 200";

            if (!f.Relations)
                f.Relations = new array<ref OZ_FactionRelation>();

            for (int j = 0; j < f.Relations.Count(); j++)
            {
                if (f.Relations[j].Stand == "")
                    f.Relations[j].Stand = OZ_FactionStand.NEUTRAL;

                if (f.Relations[j].With != "")
                    continue;

                string w = "faction \"" + f.Id;
                w += "\" declares a relation to nobody - it will never match";
                OZ_Log.Warn(w);
                warnings++;
            }
        }
    }
}

// Реєстр, як його присилає бот. НЕ те саме, що OZ_Faction: тут лише те, чим
// бот володіє, і жодного id ролі Discord -- гра його не потребує взагалі,
// бо бот перекладає ролі в слаги ще до того, як щось перетне провід.
class OZ_FactionRosterEntry
{
    string Id;
    string DisplayName;
    string Color;

    // The bot's ceiling on membership, 0 for none (TZ-4 R-C3.2). Travels so
    // the admin console can show and edit it; the game never enforces it --
    // the bot counts everybody, offline included, and the game cannot.
    int Limit;

    // Базова фракцiя: її носять усi й вона не є органiзацiєю. Див. довге
    // пояснення в полi OZ_Faction.BaseFaction.
    bool Base;
}

// Підпис однієї ролі, будь-якої з трьох осей. Той самий вигляд, що й у
// фракції, мінус колір: звання й мітки на екрані не фарбуються.
class OZ_RoleName
{
    string Id;
    string DisplayName;

    // Місце на драбині: більше -- старше. Має сенс лише для звань (і
    // сталкерських, і внутрішньофракційних); у міток і посад лишається
    // нулем, бо їх ніхто не шикує.
    //
    // Їде сюди, бо без порядку «підвищити» неможливе: гра знає слаги, але
    // хто з них вищий -- знає тільки реєстр бота.
    int Order;
}

class OZ_FactionRoster
{
    int Stamp;
    ref array<ref OZ_FactionRosterEntry> Factions;

    // Iншi осi -- лише пiдписи. Приїжджають тим самим конвертом, бо
    // змiнюються так само рiдко й з того самого джерела. FRanks --
    // внутрiфракцiйнi звання, id вигляду "duty:sergeant".
    ref array<ref OZ_RoleName> Ranks;
    ref array<ref OZ_RoleName> Traits;
    ref array<ref OZ_RoleName> Posts;
    ref array<ref OZ_RoleName> FRanks;

    // Factions REMOVED at the bot (TZ-2 section 15, R7.9). The merge adds and
    // renames and cannot infer an absence, so removals travel by name.
    ref array<string> Gone;
}

// Слова, якими описується ставлення. Рядками, бо їх читає адмін у JSON, і
// enum у файлі виглядав би числом.
class OZ_FactionStand
{
    static const string ALLY     = "ally";
    static const string FRIENDLY = "friendly";
    static const string NEUTRAL  = "neutral";
    // WARY тут більше немає: на це слово не дивився жоден рядок коду --
    // Stand() віддає оголошене як є, а перелік слів описує README.
    static const string HOSTILE  = "hostile";
}

// ДОГОВОРУ ДЛЯ ЧУЖОГО ПОСТАЧАЛЬНИКА ТУТ БІЛЬШЕ НЕМАЄ (2026-09-06).
//
// OZ_FactionProvider, Bind(), HasProvider() і гілка постачальника в OrgOf()
// пролежали без жодного викликача в усій серії. Порожній договір на майбутнє
// коштує не рядків, а неправди в старшинстві: шапка обіцяла три джерела, а
// джерел було два. З'явиться Expansion-подібний постачальник -- договір
// повернеться разом із модом, який його реалізує.

class OZ_Factions
{
    private static ref OZ_FactionsConfig s_Cfg;

    // The bot's membership ceilings by faction id, memory only, from the
    // roster. Not a field of OZ_Faction: that class is the admin's file, and
    // a number the bot owns has no business being written there.
    private static ref map<string, int> s_BotLimits;

    // Ролі Discord: uid -> id УГРУПОВАННЯ. Наповнює міст через прив'язку
    // акаунта; порожня мапа означає «синхронізації немає», а не «усі без
    // фракції».
    private static ref map<string, string> s_ByRole;

    // Те саме для БАЗОВОЇ осі. Окрема мапа, а не друге поле в одному записі,
    // рівно з тієї ж причини, з якої осей дві: вони приходять із різних
    // джерел і застарівають нарізно. Проекція може обнулити угруповання й не
    // сказати ані слова про базову -- і це нормальний стан, а не втрата.
    private static ref map<string, string> s_BaseByRole;

    // ДЗВІНКА OnChanged ТУТ БІЛЬШЕ НЕМАЄ (2026-09-06): на нього не підписався
    // ніхто в жодному репозиторії серії, а Invoke стояв у п'яти місцях --
    // тобто другий сигнал про зміну поруч із OZ_RoleNotify, у якого слухач є.

    // Ідемпотентна: хто перший покликав, той і завантажив.
    //
    // Порядок CF-модулів не гарантований, і на цьому стенді він УЖЕ підводив
    // -- рація відпрацювала раніше за КПК. Тому кличуть і ядро, і КПК, і будь
    // хто ще, кому таблиця потрібна раніше за нас.
    //
    // Окремої Reload() «для майбутньої гарячої перезагрузки» тут більше немає:
    // операції, яка її вмикала б, не написано, а один викликач у обгортки був
    // цей самий метод.
    static void ServerLoad()
    {
        if (s_Cfg)
            return;

        MigrateFileName();

        s_Cfg = new OZ_FactionsConfig();
        OZ_ConfigLoader<OZ_FactionsConfig>.Load(OZ_Const.PROFILE_DIR + "\\OZ_Factions.json", "factions", s_Cfg);

        Reindex();
    }

    // Таблиця за id. ПОШУК ЗА НЕЮ -- НАЙГАРЯЧІШИЙ ШЛЯХ УСЬОГО МОДА: кожне
    // питання «чия це людина» закінчується в Guarded -> IsBase -> Find, а
    // разом із ним туди ходять NameOf, ColorOf і ставлення. Лінійний обхід
    // таблиці з порівнянням РЯДКІВ коштував на кожен рядок контакту, на кожен
    // рядок складу й на кожен маячок.
    //
    // Мапа перебудовується там і тільки там, де таблиця міняється: після
    // завантаження й у ApplyRoster (єдині два місця, які пишуть s_Cfg.Factions).
    private static ref map<string, OZ_Faction> s_ById = new map<string, OZ_Faction>();

    private static void Reindex()
    {
        s_ById.Clear();
        if (!s_Cfg || !s_Cfg.Factions)
            return;
        for (int i = 0; i < s_Cfg.Factions.Count(); i++)
        {
            if (s_Cfg.Factions[i] && s_Cfg.Factions[i].Id != "")
                s_ById.Set(s_Cfg.Factions[i].Id, s_Cfg.Factions[i]);
        }
    }

    // Одноразове перейменування файла реєстру: до 2026-09-05 фракції писали
    // "OZ_Core_Factions.json" -- лишок з часів, коли таблиця ще жила в ядрі
    // (виділені звідти 2026-09-04). Тепер файл носить ім'я цього мода.
    //
    // МУСИТЬ СТОЯТИ ПЕРЕД Load. OZ_ConfigLoader на відсутньому шляху мовчки
    // пише умовчання (LoadDefaults) і одразу зберігає їх на диск -- спитай
    // він спершу новий шлях, стенд із самим лише старим файлом отримає ДЕСЯТЬ
    // дефолтних фракцій замість дев'яти адмінових, і в ту ж мить перезапису
    // старий файл перестане бути джерелом правди для будь-кого.
    //
    // Ознака разовості -- НАЯВНІСТЬ нового файла, а не окремий прапорець:
    // другий старт бачить його одразу й виходить першим рядком.
    private static void MigrateFileName()
    {
        string oldPath = OZ_Const.PROFILE_DIR + "\\OZ_Core_Factions.json";
        string newPath = OZ_Const.PROFILE_DIR + "\\OZ_Factions.json";

        if (FileExist(newPath))
            return;
        if (!FileExist(oldPath))
            return;

        string movedPath = oldPath + ".moved";

        if (!CopyFile(oldPath, newPath))
        {
            string failed = "factions: cannot copy " + oldPath + " to " + newPath;
            failed += " - the rename did not happen, the old file is untouched";
            OZ_Log.Error(failed);
            return;
        }

        // Копія під старим ім'ям лишається на диску (вимога міграції); якщо
        // саме вона не вдалася, старий файл лишаємо на місці й не видаляємо
        // його -- інакше вміст пережив би тільки під новим ім'ям, а обіцяний
        // .moved так і не з'явився б. Обидва виклики -- під $profile:, як
        // CopyFile/DeleteFile і вимагають.
        bool movedOk = CopyFile(oldPath, movedPath);
        if (!movedOk)
        {
            string warnMoved = "factions: cannot copy " + oldPath + " to " + movedPath;
            warnMoved += " - keeping " + oldPath + " in place instead of deleting it";
            OZ_Log.Warn(warnMoved);
        }

        if (movedOk)
        {
            // Видаляємо старий шлях лише тоді, коли копія під .moved вдалася.
            if (!DeleteFile(oldPath))
            {
                // Два файли реєстру лишаються на диску: newPath -- єдине
                // джерело правди відтепер, а вартовий на початку функції
                // (FileExist(newPath) вище) більше не чіпає oldPath на
                // наступних стартах -- він бачить лише наявність нового
                // файла, тож повторної спроби видалення не буде.
                string warnDelete = "factions: cannot delete " + oldPath + " after copying it to " + newPath;
                warnDelete += " - two registry files remain, " + newPath + " is the registry from now on";
                OZ_Log.Warn(warnDelete);
            }
        }

        string done = "factions: " + oldPath + " renamed to " + newPath;
        if (movedOk)
            done += " (old file kept as " + movedPath + ")";
        OZ_Log.Info(done);
    }

    static int Count()
    {
        if (!s_Cfg)
            return 0;
        if (!s_Cfg.Factions)
            return 0;
        return s_Cfg.Factions.Count();
    }

    // The bot's ceiling for a faction, 0 when none or unknown.
    static int BotLimitOf(string id)
    {
        if (!s_BotLimits || id == "")
            return 0;
        int n;
        if (s_BotLimits.Find(id, n))
            return n;
        return 0;
    }

    // ------------------------------------------------------------- таблиця

    static OZ_Faction Find(string id)
    {
        if (id == "")
            return null;

        OZ_Faction f;
        if (!s_ById.Find(id, f))
            return null;
        return f;
    }

    // Усі id. `includeHidden` -- для того, хто справді хоче всі: адмінського
    // інструменту чи експорту. Перелік для гравця має ходити без нього.
    static void Ids(out array<string> outIds, bool includeHidden = false)
    {
        if (!outIds)
            outIds = new array<string>();
        outIds.Clear();

        if (!s_Cfg)
            return;
        if (!s_Cfg.Factions)
            return;

        for (int i = 0; i < s_Cfg.Factions.Count(); i++)
        {
            if (s_Cfg.Factions[i].Hidden)
            {
                if (!includeHidden)
                    continue;
            }
            outIds.Insert(s_Cfg.Factions[i].Id);
        }
    }

    // Чи це БАЗОВА фракцiя -- та, яку носять усi (сталкери). Питати треба
    // всюди, де йдеться про фракцiю ЯК ПРО ОРГАНIЗАЦIЮ: склад, лiдер,
    // внутрiшнi звання, запрошення. Там, де йдеться про назву й колiр,
    // питати не треба -- базова фракцiя малюється, як усi.
    //
    // Незнайоме id -- НЕ базове: мовчазне «так» зачинило б фракцiйний
    // екран тим, кого ми просто ще не знаємо.
    static bool IsBase(string id)
    {
        OZ_Faction f = Find(id);
        if (!f)
            return false;
        return f.BaseFaction;
    }

    // Угруповання гравця, або порожньо. Вiднiмання «базова = вiдсутнiсть»
    // тут БІЛЬШЕ НЕМАЄ: осей тепер двi, i кожна приходить своїм шляхом
    // (ТЗ-1 §3). Лишилась тiльки сторожа нижче, в OrgOf.
    static string OrgOfUid(string uid)
    {
        return OrgOf(null, uid);
    }

    // Базова фракцiя гравця. Є в кожного, хто хоч раз заходив; порожньо
    // означає рiвно «ще не заходив», i саме на це дивиться перший вхід.
    //
    // Постачальника тут немає навмисно: його контракт вiддає органiзацiю.
    static string BaseOfUid(string uid)
    {
        if (uid == "")
            return "";

        if (s_BaseByRole)
        {
            string byRole;
            if (s_BaseByRole.Find(uid, byRole))
            {
                // Порожнiй рядок вiд моста базову НЕ знiмає (ТЗ-1 R5.4):
                // базову призначила гра, i Discord про неї не вирiшує.
                if (byRole != "")
                    return byRole;
            }
        }

        OZ_PlayerData d = OZ_PlayerStore.Load(uid);
        if (!d)
            return "";
        return d.BaseFaction;
    }

    // Людська назва. Незнайоме id повертаємо ЯК Є: чуже слово на екрані
    // чесніше за мовчазну підміну на «одинак».
    static string NameOf(string id)
    {
        if (id == "")
            return "";

        OZ_Faction f = Find(id);
        if (f)
            return f.DisplayName;
        return id;
    }

    static string ShortOf(string id)
    {
        OZ_Faction f = Find(id);
        if (!f)
            return id;
        if (f.Short != "")
            return f.Short;
        return f.DisplayName;
    }

    static string ColorOf(string id)
    {
        OZ_Faction f = Find(id);
        if (f)
            return f.Color;
        return "200 200 200";
    }

    // "R G B" -> ARGB для віджета.
    //
    // РОЗБИРАЄМО РАЗ НА КОЛІР, а не раз на виклик: кличеться це на кожен рядок
    // контакту й кожен рядок складу при кожному перемальовуванні, а Split
    // щоразу заводив масив і три рази перетворював рядок на число заради
    // трьох байтів, які не міняються.
    //
    // Ключ кеша -- САМ РЯДОК КОЛЬОРУ, а не id фракції: коли реєстр бота
    // перефарбує фракцію, ключ стане інший, і застарілого запису не буде за
    // визначенням. Скидати кеш нема потреби взагалі.
    private static ref map<string, int> s_Rgb = new map<string, int>();

    static int ColorARGB(string id, int alpha = 255)
    {
        string raw = ColorOf(id);

        int rgb;
        if (!s_Rgb.Find(raw, rgb))
        {
            rgb = ParseRgb(raw);
            s_Rgb.Set(raw, rgb);
        }

        return (rgb & 0x00FFFFFF) | (alpha << 24);
    }

    // Три канали з "196  64  40". Рахуємо ЗАПОВНЕНІ токени, а не позиції:
    // подвійні пробіли дають порожні. Канал поза 0..255 підрізаємо -- інакше
    // одруківка в файлі адміна (чи в реєстрі бота) перетікала б у сусідній
    // байт і фарбувала б рядок навмання, включно з прозорістю.
    private static int ParseRgb(string raw)
    {
        array<string> parts = new array<string>();
        raw.Split(" ", parts);

        array<int> c = new array<int>();
        for (int i = 0; i < parts.Count(); i++)
        {
            if (parts[i] == "")
                continue;
            c.Insert(Math.Clamp(parts[i].ToInt(), 0, 255));
        }

        if (c.Count() < 3)
            return ARGB(0, 200, 200, 200);

        return ARGB(0, c[0], c[1], c[2]);
    }

    // ---------------------------------------------------------- членство

    // Чиє УГРУПОВАННЯ. Старшинство описане в шапці файлу.
    static string OrgOf(PlayerBase player, string uid)
    {
        // `player` лишається в підписі: його вимагає контракт ядра
        // (OZ_IdentityService.OrgOfPlayer), а відповідь на нього однакова.
        if (uid == "")
            return "";

        if (s_ByRole)
        {
            string byRole;

            // НАЯВНІСТЬ запису -- це вже відповідь, і порожній рядок теж.
            //
            // Тут стояла перевірка `if (byRole != "")`, і вона тихо ламала
            // весь сенс розрізнення, яке боронить ForgetRole. Гравця вигнали
            // з Боргу в Discord, бот чесно сказав «ролі фракції не має», а гра
            // читала "duty" з його файла далі -- НАЗАВЖДИ. І це значення
            // годує AreHostile(), тобто рішення стріляти чи ні.
            //
            // Правило одне: запис Є -- Discord авторитетний, включно з
            // порожнім рядком. Запису НЕМАЄ -- ми не знаємо, і лише тоді
            // працює файл акаунта. Прибрати запис може тільки ForgetRole.
            if (s_ByRole.Find(uid, byRole))
                return Guarded(byRole);
        }

        OZ_PlayerData d = OZ_PlayerStore.Load(uid);
        if (!d)
            return "";
        return Guarded(d.OrgFaction);
    }

    // СТОРОЖА ОДНІЄЇ ІНВАРІАНТИ: базова фракція не буває угрупованням.
    //
    // Осі роз'їхались, але джерела лишились чужі -- гільдія, файл акаунта,
    // постачальник, -- і будь-яке з них може прислати слаг, позначений у
    // OZ_Factions.json як BaseFaction. Пропустити його означало б, що всі
    // «сталкери» раптом стали одним угрупованням і бачать одне одного
    // своїми: рівно те, що ТЗ-1 R3.2 забороняє про базову вісь.
    //
    // Мовчки, бо це стан налаштування, а не подія гри: місце, де про це
    // кажуть уголос, -- шапка адмінського розділу FACTIONS.
    private static string Guarded(string slug)
    {
        if (slug == "")
            return "";
        if (IsBase(slug))
            return "";
        return slug;
    }

    // SetOrgOf ТУТ БІЛЬШЕ НЕМАЄ: викликачів нуль, і це було записано як факт
    // ще в ТЗ-1 R5.3 -- угруповання роздає бот, гра його лише показує. Поле
    // OZ_PlayerData.OrgFaction лишається запасним шляхом, який ЧИТАЄ OrgOf.

    // Поставити БАЗОВУ фракцію. Кличе перший вхід, і більше ніхто.
    //
    // Вона не з Discord і не від постачальника: її призначила гра, і зняти
    // її може тільки заводський скид персонажа (ТЗ-1 R5.4). Тому тут немає
    // ані сторожі Guarded (навпаки: слаг МУСИТЬ бути базовим), ані оглядки
    // на старші джерела -- старших у цієї осі немає.
    static void SetBaseOf(string uid, string factionId)
    {
        if (!GetGame().IsServer())
            return;
        if (uid == "")
            return;

        OZ_PlayerData d = OZ_PlayerStore.Load(uid);
        if (!d)
            return;
        if (d.BaseFaction == factionId)
            return;

        d.BaseFaction = factionId;
        OZ_PlayerStore.MarkDirty(uid);
    }

    // Перша фракція з BaseFaction: true в порядку файлу, або порожньо.
    // Порядок файлу -- це і є відповідь «яку саме»: перевпорядкувати
    // OZ_Factions.json адмін уміє, а вигадувати йому ще одне поле «головна
    // базова» означало б два джерела правди про одне.
    static string FirstBaseId()
    {
        if (!s_Cfg || !s_Cfg.Factions)
            return "";

        for (int i = 0; i < s_Cfg.Factions.Count(); i++)
        {
            OZ_Faction f = s_Cfg.Factions[i];
            if (f && f.BaseFaction)
                return f.Id;
        }
        return "";
    }

    // Міст приніс ролі. Порожній id фракції = «ролі, що відповідає фракції,
    // у нього немає» -- тоді працює запасний шлях через файл акаунта.
    static void SetFromRole(string uid, string factionId)
    {
        if (!GetGame().IsServer())
            return;
        if (uid == "")
            return;

        if (!s_ByRole)
            s_ByRole = new map<string, string>();

        string had;
        s_ByRole.Find(uid, had);
        if (had == factionId)
            return;

        s_ByRole.Set(uid, factionId);
    }

    // Базова вісь із проекції. Порожній рядок тут НЕ знімає базову: запис
    // усе одно кладеться (щоб «міст про нього знає» лишалось правдою), але
    // читач у BaseOfUid порожній пропускає й іде до файла акаунта. Різниця
    // з угрупованням навмисна й записана в R5.4.
    static void SetBaseFromRole(string uid, string factionId)
    {
        if (!GetGame().IsServer())
            return;
        if (uid == "")
            return;

        if (!s_BaseByRole)
            s_BaseByRole = new map<string, string>();

        string had;
        s_BaseByRole.Find(uid, had);
        if (had == factionId)
            return;

        s_BaseByRole.Set(uid, factionId);
    }

    // Синхронізація ролей зникла (міст ліг, гравець відв'язав акаунт).
    // Прибрати ЗАПИС, а не поставити порожній рядок: порожній рядок означав
    // би «ролі немає», а нам треба «ми не знаємо».
    // Міст більше нічого про цього гравця не каже -- забуваємо ОБИДВІ осі.
    //
    // Для угруповання це важливо: запис зникає, і читач падає у файл акаунта
    // замість того, щоб вічно вірити останній почутій ролі. Для базової це
    // просто прибирання: її дім і так файл акаунта, а не проекція.
    static void ForgetRole(string uid)
    {
        bool had = false;

        if (s_ByRole && s_ByRole.Contains(uid))
        {
            s_ByRole.Remove(uid);
            had = true;
        }

        if (s_BaseByRole && s_BaseByRole.Contains(uid))
        {
            s_BaseByRole.Remove(uid);
            had = true;
        }


    }

    // --------------------------------------------------------- ставлення

    // Як `a` ставиться до `b`.
    //
    // Симетрично за замовчуванням: якщо `a` мовчить, дивимось, що сказав `b`.
    // Односторонню ворожість задають ЯВНО -- двома рядками в JSON.
    static string Stand(string a, string b)
    {
        if (a == "")
            return OZ_FactionStand.NEUTRAL;
        if (b == "")
            return OZ_FactionStand.NEUTRAL;
        if (a == b)
            return OZ_FactionStand.ALLY;

        string direct = Declared(a, b);
        if (direct != "")
            return direct;

        string mirrored = Declared(b, a);
        if (mirrored != "")
            return mirrored;

        return OZ_FactionStand.NEUTRAL;
    }

    static bool AreHostile(string a, string b)
    {
        return Stand(a, b) == OZ_FactionStand.HOSTILE;
    }

    static bool AreFriendly(string a, string b)
    {
        string s = Stand(a, b);
        if (s == OZ_FactionStand.ALLY)
            return true;
        return s == OZ_FactionStand.FRIENDLY;
    }

    private static string Declared(string from, string to)
    {
        OZ_Faction f = Find(from);
        if (!f)
            return "";
        if (!f.Relations)
            return "";

        for (int i = 0; i < f.Relations.Count(); i++)
        {
            if (f.Relations[i].With == to)
                return f.Relations[i].Stand;
        }
        return "";
    }

    // ------------------------------------------------------------- реєстр

    // Реєстр від бота: як фракція ЗВЕТЬСЯ і якого вона кольору.
    //
    // ДВА ДОМИ, і поділ між ними -- за тим, ХТО МОЖЕ це налаштувати, а не за
    // тим, хто читає:
    //
    //   бот  -- слаг, назва, колір. Він створює ролі в Discord, отже дізнається
    //           про них першим, і власник вимагав налаштовувати звідти.
    //   гра  -- Relations і Hidden. Правила симуляції цього сервера,
    //           яким у Discord немає де жити.
    //
    // Потік В ОДИН БІК. Бот може ДОДАТИ фракцію й переписати назву та колір;
    // видалити запис адміна або торкнутись його ставлень -- ніколи. Один
    // напрямок і один ключ зв'язку -- це і є весь механізм проти розходження.
    //
    // НЕ пишеться на диск. Мітка -- косметика, і при мертвому мості вона на
    // вісім секунд відкотиться до того, що в файлі адміна. Записувати чужу
    // назву в його файл означало б, що вона лишиться там і після того, як
    // бота приберуть.
    static void ApplyRoster(OZ_FactionRoster r)
    {
        if (!GetGame().IsServer())
            return;
        if (!r || !r.Factions)
            return;
        if (!s_Cfg)
            return;

        int added = 0;
        int renamed = 0;

        for (int i = 0; i < r.Factions.Count(); i++)
        {
            OZ_FactionRosterEntry e = r.Factions[i];
            if (e.Id == "")
                continue;

            OZ_Faction f = Find(e.Id);

            if (!f)
            {
                f = new OZ_Faction();
                f.Id            = e.Id;
                f.Hidden        = false;
                f.Relations     = new array<ref OZ_FactionRelation>();
                s_Cfg.Factions.Insert(f);
                // Одразу в покажчик: далі в цьому ж циклі Find() шукає й
                // щойно доданих -- інакше другий конверт того самого ростера
                // завів би фракцію вдруге.
                s_ById.Set(f.Id, f);
                added++;
            }

            if (e.DisplayName != "")
            {
                if (f.DisplayName != e.DisplayName)
                    renamed++;
                f.DisplayName = e.DisplayName;
            }

            if (e.Color != "")
                f.Color = e.Color;

            // Базовiсть -- слово РЕЄСТРУ: саме бот вирiшує, яка фракцiя
            // всезагальна, бо саме вiн вдягає її значок на кожного при
            // прив'язцi. Ставимо лише true: власний прапорець адмiна в
            // файлi реєстр не знiмає, бо реєстр про нього не знає.
            if (e.Base)
                f.BaseFaction = true;

            if (!s_BotLimits)
                s_BotLimits = new map<string, int>();
            s_BotLimits.Set(e.Id, e.Limit);
        }

        // REMOVALS BY NAME (TZ-2 section 15, R7.9). In memory only, like
        // everything the roster brings: the admin's file is not touched, and
        // the bot keeps saying Gone on every roster until the faction is
        // created again, so a restart cannot bring it back. The base never
        // goes -- nothing in the model survives that.
        int gone = 0;
        if (r.Gone && s_Cfg.Factions)
        {
            for (int g = 0; g < r.Gone.Count(); g++)
            {
                string slug = r.Gone[g];
                if (slug == "")
                    continue;
                for (int k = s_Cfg.Factions.Count() - 1; k >= 0; k--)
                {
                    OZ_Faction victim = s_Cfg.Factions[k];
                    if (!victim || victim.Id != slug)
                        continue;
                    if (victim.BaseFaction)
                        continue;
                    s_Cfg.Factions.RemoveOrdered(k);
                    s_ById.Remove(slug);
                    gone++;
                    OZ_Log.Info("factions: " + slug + " removed at the bot - dropped from the table");
                }
            }
        }

        string m = "factions: roster from the bridge, stamp " + r.Stamp.ToString();
        m += " (" + added.ToString() + " added, ";
        m += renamed.ToString() + " renamed, " + gone.ToString() + " removed)";

        // БАЗОВУ НАЗИВАЄМО ВГОЛОС. Її прапорець приходить лише з реєстру й
        // на диск не лягає (записи бота живуть у пам'ятi), тож без цього
        // рядка перевiрити, чи вiн доїхав, можна було б тiльки з гри --
        // саме та дiагностика, яку шукають о другiй ночi.
        string bases = "";
        for (int b = 0; b < s_Cfg.Factions.Count(); b++)
        {
            if (!s_Cfg.Factions[b].BaseFaction)
                continue;
            if (bases != "")
                bases += ",";
            bases += s_Cfg.Factions[b].Id;
        }
        if (bases == "")
            bases = "none";
        m += " base=" + bases;

        OZ_Log.Info(m);
    }

    // ExportJson ТУТ БІЛЬШЕ НЕМАЄ: писалась «для чужого мода, який захоче
    // перекинути таблицю цілком», і за весь час не покликав ніхто. Кому
    // знадобиться -- напише два рядки MakeData у себе.
}
