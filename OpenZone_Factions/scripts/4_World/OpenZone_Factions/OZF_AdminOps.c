// Адмінська сторінка фракційної системи: ростер і пермадес.
//
// ОКРЕМИЙ РОЗДІЛ, а не операції ядрового. Ядро лишає собі редактор конфігів
// і спавни -- те, що є в нього завжди; ростер із рангами, званнями, рисами
// й призначенням фракцій має сенс рівно там, де стоїть цей мод. Сервер без
// нього не мусить бачити в консолі половину кнопок, які нічого не роблять.
//
// Межа безпеки НЕ ТУТ, і це зміна 2026-09-01 (ТЗ-5 §C3): права перевіряє
// диспетчер ядра в OZ_AdminReq, першим рядком, один раз на всі розділи всіх
// модів. Друга перевірка тут виглядала б обережною, а насправді робила б
// правило розсипаним по модах -- і мовчазний розділ без неї виглядав би так
// само, як розділ, що чесно відмовив.

// OZF_Const ПОЇХАВ У 3_Game (2026-09-04): на нього дивиться і власний канал
// ролей, який мусить лежати нижче за всіх своїх викликачів, і обидві склейки
// в 5_Mission. Тут лишились самі операції розділу.

class OZ_AdminRosterRow
{
    string Name    = "";
    string Uid     = "";
    // ОБИДВІ осі, і саме для адміна це важливо найбільше: він єдиний, хто
    // бачить різницю між «одинак-легенда» і «не заходив жодного разу», а
    // одне поле їх не розрізняє.
    string Base    = "";
    string Org     = "";
    string DName   = "";
    string Traits  = "";
    string Rank    = "";
    string FRank   = "";
    bool   Leader  = false;
    // У Зоні зараз. Ростер тепер перелічує й відсутніх (ТЗ-4 R-C4.2), і
    // консоль мусить їх розрізняти: відсутнього не покличеш до слова.
    bool   Online  = false;
}

class OZ_AdminRoster
{
    ref array<ref OZ_AdminRosterRow> Rows;
    ref array<string> Factions;

    // Каталоги з реєстру бота: адмiну треба з чого вибирати. FRanks --
    // внутрiфракцiйнi звання, id вигляду "duty:sergeant".
    ref array<string> Traits;
    ref array<string> Ranks;
    ref array<string> FRanks;

    // The faction editor's view of each entry in Factions, same order
    // (TZ-2 section 15, R7.8): what it is called, the bot's ceiling, and
    // whether it has a leader post.
    ref array<string> FacLabels;
    ref array<int>    FacLimits;
    ref array<bool>   FacLeaders;

    // ЧОМУ СПИСОК НЕПОВНИЙ, коли він неповний. Порожньо -- ростер повний.
    //
    // Мовчазне усічення читалось як «на сервері стільки людей»: міст відмовив
    // або відповів нерозбірливо, ростер падав до самих присутніх, і admin
    // отримував ok=true зі списком, коротшим за правду, без жодної ознаки. Це
    // НЕ помилка операції -- показати те, що є, правильно, -- тому їде окремим
    // полем, а не в error.
    string Partial = "";

    void OZ_AdminRoster()
    {
        Rows       = new array<ref OZ_AdminRosterRow>();
        Factions   = new array<string>();
        Traits     = new array<string>();
        Ranks      = new array<string>();
        FRanks     = new array<string>();
        FacLabels  = new array<string>();
        FacLimits  = new array<int>();
        FacLeaders = new array<bool>();
    }
}

// The faction editor's letter (TZ-2 section 15, R7.8): the VPP pane fills
// Slug, Label, Limit and HasLeader; the SERVER stamps Admin before the
// letter goes to the bridge, so a client cannot claim it.
class OZF_FactionEdit
{
    string Slug      = "";
    string Label     = "";
    int    Limit     = 0;
    bool   HasLeader = false;
    bool   Admin     = false;
}

class OZF_FactionRemoveAsk
{
    string Slug  = "";
    bool   Admin = false;
}

// A plain yes or no from the bridge, handed on to the admin who asked.
class OZF_AckReply : OZ_BridgeReply
{
    protected string m_AdminUid;
    protected string m_Op;
    protected string m_What;

    void OZF_AckReply(string adminUid, string op, string what)
    {
        m_AdminUid = adminUid;
        m_Op       = op;
        m_What     = what;
    }

    override void OnBody(string json)
    {
        PlayerIdentity to = OZ_Link.Online(m_AdminUid);
        if (!to)
            return;

        OZ_BridgeAck ack;
        string err;
        if (!JsonFileLoader<OZ_BridgeAck>.LoadData(json, ack, err) || !ack)
        {
            OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, false, "", "STR_OZ_ERR_INTERNAL");
            return;
        }

        if (!ack.Ok)
        {
            OZ_Log.Warn("admin: bridge refused " + m_What + ": " + ack.Why);
            OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, false, "", ack.Why);
            return;
        }

        OZ_Log.Info("admin: " + m_What + " accepted by the bridge");
        OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, true, "{}", "");
    }

    override void OnFail(int code)
    {
        PlayerIdentity to = OZ_Link.Online(m_AdminUid);
        if (!to)
            return;
        OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, false, "", "STR_OZ_ERR_NO_BRIDGE");
    }
}

class OZ_AdminWipeAsk
{
    string Uid = "";

    // «Ігрову половину вже зроблено». Ставить ГРА, коли пермадес почався в
    // нiй: тодi мiст робить лише своє й не шле поштовх назад. Порожнє (з
    // команди бота) означає протилежне -- мiст мусить розбудити гру.
    bool FromGame = false;
}

// Вiдповiдь моста на команду: вдалося чи нi, i чому.
class OZ_BridgeAck
{
    bool   Ok  = false;
    string Why = "";
}

// ЧИЙ ВАЙП УЖЕ В ДОРОЗІ.
//
// Пермадес не ідемпотентний, і саме тому подвійне натискання коштує дорого:
// обидва листи доїжджають до `ack.Ok`, обидві відповіді кличуть
// OZ_PlayerWipe.Local, і покоління стрибає через два -- а Freeze між ними
// записує друге, вже порожнє життя окремим файлом могили. Консоль тримає
// підтвердження (WIPE двічі), але воно захищає від випадкового кліку, а не
// від другого кліку по озброєній кнопці, поки міст думає над першим.
//
// Ключ -- ЦІЛЬ, а не адмін: два адміни на одного небіжчика -- та сама шкода.
// Знімається в OZ_AdminWipeReply обома дорогами, і третьої в нього немає:
// мовчання рушія приходить в OnQuiet, а той за замовчуванням кличе OnFail.
class OZ_WipeInFlight
{
    private static ref map<string, bool> s_By = new map<string, bool>();

    static bool Busy(string uid)
    {
        return s_By.Contains(uid);
    }

    static void Begin(string uid)
    {
        s_By.Set(uid, true);
    }

    static void Done(string uid)
    {
        if (s_By.Contains(uid))
            s_By.Remove(uid);
    }
}

// Мiст вiдповiв на вайп -- ТУТ І ВІДБУВАЄТЬСЯ ІГРОВА ПОЛОВИНА.
//
// Порядок перевернуто 2026-09-06, і це не косметика. Гра робила своє
// ПЕРШОЮ -- заморожувала покоління, запечатувала КПК, стирала друзів, -- а
// потім питала міст. Відмова Discord (бот не може керувати роллю, тред не
// віддається, гільдія недоступна) лишала гравця розполовиненим: у Зоні мертвий,
// у Discord живий. Порада «повтори команду» не рятувала: повтор заморожував ЩЕ
// ОДНЕ, уже порожнє покоління, і кожна спроба додавала небіжчику життя.
//
// Тепер гру чіпає лише `ack.Ok`. Мовчання моста -- OnFail -- не чіпає нічого
// взагалі, а нерозбірлива відповідь так само нічого: половину вайпу неможливо
// відрізнити від бага, а «не сталося нічого» адмін бачить і повторює безпечно.
//
// FromGame у листі лишається true: бот не мусить штовхати нам "wipe" назад --
// свою половину ми зробимо самі, з цієї відповіді.
class OZ_AdminWipeReply : OZ_BridgeReply
{
    protected string m_AdminUid;
    protected string m_Op;
    protected string m_Uid;

    void OZ_AdminWipeReply(string adminUid, string op, string uid)
    {
        m_AdminUid = adminUid;
        m_Op       = op;
        m_Uid      = uid;
    }

    override void OnBody(string json)
    {
        OZ_WipeInFlight.Done(m_Uid);

        PlayerIdentity to = OZ_Link.Online(m_AdminUid);

        OZ_BridgeAck ack;
        string err;
        if (!JsonFileLoader<OZ_BridgeAck>.LoadData(json, ack, err) || !ack)
        {
            OZ_Log.Warn("admin: unreadable answer to the wipe of " + m_Uid + ": " + err + " - nothing was changed in the game");
            if (to)
                OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, false, "", "STR_OZ_ERR_INTERNAL");
            return;
        }

        if (!ack.Ok)
        {
            OZ_Log.Warn("admin: bridge refused the wipe: " + ack.Why + " - nothing was changed in the game");
            if (to)
                OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, false, "", ack.Why);
            return;
        }

        // Адмін міг вийти, поки міст думав, -- на долю персонажа це не
        // впливає. Спершу робимо, потім розповідаємо тому, хто ще слухає.
        OZ_PlayerWipe.Local(m_Uid);

        if (to)
            OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, true, "{}", "");
    }

    override void OnFail(int code)
    {
        OZ_WipeInFlight.Done(m_Uid);

        OZ_Log.Warn("admin: the wipe of " + m_Uid + " never reached the bridge - nothing was changed in the game");

        PlayerIdentity to = OZ_Link.Online(m_AdminUid);
        if (!to)
            return;
        OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, false, "", "STR_OZ_ERR_NO_BRIDGE");
    }
}

// ІГРОВА ПОЛОВИНА ПЕРМАДЕСУ, окремо від того, хто її попросив.
//
// Просять двоє: адмінська консоль у грі (і тоді вона ж кличе міст) і сам
// МІСТ -- коли пермадес запустили командою бота, а не з гри. Без цього
// класу друга дорога робила лише половину справи: ролі в Discord
// скидались, а КПК небіжчика лишались живими, бо гра про смерть не чула.
class OZ_PlayerWipe
{
    static void Local(string uid)
    {
        if (!GetGame().IsServer())
            return;
        if (uid == "")
            return;

        // СПЕРШУ ЗАМОРОЗКА, потiм чистка. Старе життя лягає в окремий файл
        // цiлим -- з контактами, нотатками й усiм, що в ньому було, -- i
        // лише пiсля цього живий запис стає новим персонажем.
        OZ_PlayerStore.Freeze(uid);

        OZ_PlayerData d = OZ_PlayerStore.Load(uid);
        d.SessionEpoch = d.SessionEpoch + 1;

        if (d.Friends)
            d.Friends.Clear();
        if (d.FriendReq)
            d.FriendReq.Clear();
        if (d.NpcContacts)
            d.NpcContacts.Clear();

        // СПАДКОВИХ ПОЛІВ ТУТ БІЛЬШЕ НЕМАЄ (2026-09-06). Chats, TransponderTo,
        // TransponderMode і PresenceHidden ядро тримало лише заради цього
        // прибирання: жоден екран їх не читає, а два з них має заміну нижче
        // (TransponderSet, HiddenFrom*). Поки ми їх чіпали, ядро не могло їх
        // прибрати -- імені, якого немає, Enforce не пробачає.
        //
        // Нові поля ТЗ-4 §A скидаються пермадесом (R-A1.3, R-A3): нове
        // життя не успадковує ані мовчання, ані маячка старого.
        if (d.TransponderSet)
            d.TransponderSet.Clear();
        d.HiddenFromZone     = false;
        d.HiddenFromContacts = false;
        d.BaseFaction     = "";
        d.OrgFaction      = "";
        d.SeenBase        = "";
        d.SeenOrg         = "";
        d.SeenRank        = "";
        d.SeenFRank       = "";
        if (d.SeenPosts)
            d.SeenPosts.Clear();
        if (d.SeenTraits)
            d.SeenTraits.Clear();

        OZ_PlayerStore.Flush(uid);

        // Точка спавну ОСОБИСТА. ClearPersonal чесно скаже «не було» -- нам
        // однаково, головне, що пiсля вайпу її немає.
        //
        // Одноразової точки бiльше не буває: механiзм пiшов iз ядра
        // 2026-09-06 за вiдсутнiстю продюсера в усiй серiї, i виклик
        // ClearNextSpawn скасовував те, чого нiхто не ставив.
        OZ_Spawns.ClearPersonal(uid);

        // ЧУЖІ ЗАПИСНИКИ НЕ ЧІПАЄМО, і це рішення власника 2026-08-30.
        //
        // Викреслити небiжчика з чужих контактiв означало б РОЗПОВIСТИ про
        // його смерть: рядок, який зник, читається однозначно. КПК не
        // повiдомляє про смерть -- нiколи. Запис лишається на мiсцi
        // замороженим, з датою останньої появи в Зонi, i чи людина загинула,
        // чи просто не заходить, з нього не видно.
        //
        // Нове життя того самого акаунта -- ОКРЕМИЙ запис (uid#покоління),
        // якого нi в кого ще немає: знайомитись доведеться наново.

        // Проекцiю ролей забуваємо: мiст пришле нову, вже новачкову.
        OZ_Roles.Forget(uid);

        OZ_Log.Info("player " + uid + " wiped: generation frozen, devices sealed");
    }
}

// Поштовх «цього гравця стерли» -- від моста. Приходить, коли пермадес
// запустили командою бота: гра робить свою половину тут.
class OZ_WipeSink : OZ_BridgeSink
{
    override void Deliver(string json)
    {
        OZ_AdminWipeAsk a;
        string err;
        if (!JsonFileLoader<OZ_AdminWipeAsk>.LoadData(json, a, err) || !a)
        {
            OZ_Log.Warn("wipe: unreadable push from the bridge: " + err);
            return;
        }

        OZ_PlayerWipe.Local(a.Uid);
    }
}

// Проекції з бази бота -- усі, хто прив'язав акаунт, присутні чи ні.
class OZ_RosterViews
{
    bool   Ok  = false;
    string Why = "";
    ref array<ref OZ_RoleView> Rows;

    void OZ_RosterViews()
    {
        Rows = new array<ref OZ_RoleView>();
    }
}

// Мiст вiдповiв на запит ростера -- збираємо його й вiддаємо адмiновi.
// Мовчання чи вiдмова моста НЕ лишають екран порожнiм: тодi ростер такий,
// як був до цього, -- лише присутнi, з кешу проекцiй.
class OZF_RosterReply : OZ_BridgeReply
{
    protected string m_AdminUid;
    protected string m_Op;

    void OZF_RosterReply(string adminUid, string op)
    {
        m_AdminUid = adminUid;
        m_Op       = op;
    }

    override void OnBody(string json)
    {
        PlayerIdentity to = OZ_Link.Online(m_AdminUid);
        if (!to)
            return;

        OZ_RosterViews v;
        string err;
        if (!JsonFileLoader<OZ_RosterViews>.LoadData(json, v, err) || !v)
        {
            OZ_Log.Warn("admin: roster from the bridge is unreadable: " + err);
            Send(to, null, "the bridge answer did not parse - only the players in the Zone are listed");
            return;
        }

        if (!v.Ok)
        {
            OZ_Log.Warn("admin: bridge refused the roster: " + v.Why);
            Send(to, null, "the bridge refused the roster (" + v.Why + ") - only the players in the Zone are listed");
            return;
        }

        Send(to, v.Rows, "");
    }

    override void OnFail(int code)
    {
        PlayerIdentity to = OZ_Link.Online(m_AdminUid);
        if (!to)
            return;
        Send(to, null, "the bridge did not answer - only the players in the Zone are listed");
    }

    private void Send(PlayerIdentity to, array<ref OZ_RoleView> rows, string partial)
    {
        bool ok;
        string error;
        string body = OZF_AdminSection.BuildRoster(rows, partial, ok, error);
        OZ_Rpc.AdminRespond(to, OZF_Const.SECTION, m_Op, ok, body, error);
    }
}

class OZF_AdminSection : OZ_AdminSection
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        // Прав тут не питаємо: їх спитав диспетчер ядра, першим рядком, до
        // розбору операції. Межа безпеки одна на всі розділи всіх модів.

        if (op == "roster")
            return Roster(op, sender, ok, error);

        // Пермадес. Iм'я живе в операцiї з тiєї ж причини, що й у cfg_*:
        // uid короткий, але правило одне на всi адмiнськi операцiї.
        if (op.IndexOf("player_wipe:") == 0)
            return PlayerWipe(op.Substring(12, op.Length() - 12), op, sender, ok, error);

        // The faction editor (TZ-2 section 15, R7.8): both go straight to the
        // bot's tables, and the roster comes back on the next poll.
        if (op == "faction_upsert")
            return FactionUpsert(json, op, sender, ok, error);
        if (op.IndexOf("faction_remove:") == 0)
            return FactionRemove(op.Substring(15, op.Length() - 15), op, sender, ok, error);

        error = "STR_OZ_ERR_UNKNOWN_OP";
        return "";
    }

    // Create or change a faction at the bot. Nothing is written here: the
    // table in memory follows the roster the bot sends back, so the game
    // never holds a faction the bot does not.
    private string FactionUpsert(string json, string op, PlayerIdentity sender, out bool ok, out string error)
    {
        OZF_FactionEdit e;
        string jerr;
        if (!JsonFileLoader<OZF_FactionEdit>.LoadData(json, e, jerr) || !e || e.Slug == "")
        {
            error = "STR_OZ_ERR_NO_TARGET";
            return "";
        }

        if (!OZ_BridgeClient.Alive())
        {
            error = "STR_OZ_ERR_NO_BRIDGE";
            return "";
        }

        // The server vouches for its console. The bridge trusts the shared
        // secret, and the flag rides inside the letter the secret signs.
        e.Admin = true;

        string letter;
        if (!JsonFileLoader<OZF_FactionEdit>.MakeData(e, letter, jerr, false))
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_Log.Info("admin: faction " + e.Slug + " saved by " + sender.GetPlainId());
        OZ_BridgeClient.Call("v1/factions/upsert", letter, new OZF_AckReply(sender.GetPlainId(), op, "faction " + e.Slug));

        ok    = false;
        error = OZ_Const.DEFER;
        return "";
    }

    private string FactionRemove(string slug, string op, PlayerIdentity sender, out bool ok, out string error)
    {
        if (slug == "")
        {
            error = "STR_OZ_ERR_NO_TARGET";
            return "";
        }

        if (!OZ_BridgeClient.Alive())
        {
            error = "STR_OZ_ERR_NO_BRIDGE";
            return "";
        }

        OZF_FactionRemoveAsk r = new OZF_FactionRemoveAsk();
        r.Slug  = slug;
        r.Admin = true;

        string letter;
        string jerr;
        if (!JsonFileLoader<OZF_FactionRemoveAsk>.MakeData(r, letter, jerr, false))
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_Log.Info("admin: faction " + slug + " removed by " + sender.GetPlainId());
        OZ_BridgeClient.Call("v1/factions/remove", letter, new OZF_AckReply(sender.GetPlainId(), op, "removing faction " + slug));

        ok    = false;
        error = OZ_Const.DEFER;
        return "";
    }

    // «Чистий аркуш»: персонаж помер назавжди, ГРАВЕЦЬ лишається.
        //
        // Ігрова половина -- тут i одразу: епоха сесiй +1 (всi його КПК
        // замерзають назавжди -- сесiю вiдкриває лише безхазяйний пристрiй,
        // а цi назавжди лишаються зайнятими мертвою сесiєю), друзi, запити,
        // групи, транспондер, особиста точка спавну -- геть. Прив'язка Discord
        // ЛИШАЄТЬСЯ: гравець той самий, це персонаж новий.
        //
        // Половина моста (вихiд iз приватних тредiв, скидання ролей до
        // новачка) їде викликом v1/player/wipe, i вiдповiдь клiєнтовi -- ТIЛЬКИ
        // пiсля неї: адмiн мусить знати, що вайп пройшов ЦIЛКОМ, а не наполовину.
        //
        // ІГРОВОЇ ПОЛОВИНИ ТУТ БІЛЬШЕ НЕМАЄ -- вона в OZ_AdminWipeReply, під
        // `ack.Ok`. Причина довга й лежить там.
        private string PlayerWipe(string uid, string op, PlayerIdentity sender, out bool ok, out string error)
        {
            if (uid == "")
            {
                error = "STR_OZ_ERR_NO_TARGET";
                return "";
            }

            // Мiст питаємо ПЕРШИМ: якщо його немає, не робимо НIЧОГО. Половина
            // вайпу гiрша за жодного -- замерзлi КПК при живих тредах виглядали
            // б як баг, а не як смерть.
            if (!OZ_BridgeClient.Alive())
            {
                error = "STR_OZ_ERR_NO_BRIDGE";
                return "";
            }

            // Другий натиск, поки перший у дорозі, -- відмова, а не другий
            // пермадес. Причина довга й лежить в OZ_WipeInFlight.
            if (OZ_WipeInFlight.Busy(uid))
            {
                error = "STR_OZ_ERR_SLOW_DOWN";
                return "";
            }

            OZ_AdminWipeAsk a = new OZ_AdminWipeAsk();
            a.Uid = uid;
            // Гру вiдпрацюємо самi, з вiдповiдi, -- хай мiст не шле поштовх назад.
            a.FromGame = true;

            string letter;
            string jerr;
            if (!JsonFileLoader<OZ_AdminWipeAsk>.MakeData(a, letter, jerr, false))
            {
                error = "STR_OZ_ERR_INTERNAL";
                return "";
            }

            OZ_Log.Info("admin: player " + uid + " wipe asked by " + sender.GetPlainId());
            OZ_WipeInFlight.Begin(uid);
            OZ_BridgeClient.Call("v1/player/wipe", letter, new OZ_AdminWipeReply(sender.GetPlainId(), op, uid));

            // Вiдповiдь пiде з OZ_AdminWipeReply, коли мiст вiдпишеться.
            ok    = false;
            error = OZ_Const.DEFER;
            return "";
        }

    // Ростер -- З БАЗИ БОТА, i в ньому є вiдсутнi (ТЗ-4 R-C4.2). Досi вiн
    // перелiчував лише тих, хто в Зонi, бо кеш проекцiй живе поки гравець
    // пiдключений, -- i вiдсутнього не можна було нi вайпнути, нi призначити.
    // Мiст знає кожного, хто прив'язав акаунт; присутнiх без прив'язки
    // додаємо самi. Вiдповiдь iде з OZF_RosterReply; без моста -- одразу,
    // як ранiше.
    private string Roster(string op, PlayerIdentity sender, out bool ok, out string error)
        {
            if (!OZ_BridgeClient.Alive())
                return BuildRoster(null, "no link to the bridge - only the players in the Zone are listed", ok, error);

            OZ_BridgeClient.Call("v1/roles/roster", "{}", new OZF_RosterReply(sender.GetPlainId(), op));

            ok    = false;
            error = OZ_Const.DEFER;
            return "";
        }

    // Рядки ростера: спершу проекцiї моста (присутнi, потiм вiдсутнi), далi
    // присутнi, яких мiст не знає. Iм'я вiдсутнього -- з його файла гравця;
    // коли й там порожньо -- iм'я в Discord, а на крайнiй випадок uid.
    static string BuildRoster(array<ref OZ_RoleView> views, string partial, out bool ok, out string error)
        {
            ok = false;

            OZ_AdminRoster r = new OZ_AdminRoster();
            r.Partial = partial;

            // ПРИХОВАНІ ФРАКЦІЇ ТЕЖ. Прапорець Hidden ховає службові фракції
            // від ГРАВЦЯ; консоль -- єдиний екран, який мусить бачити все, і
            // до 2026-09-06 параметр `includeHidden` не передавав true ніде,
            // тобто адмін не міг ні призначити приховану фракцію, ні побачити,
            // що вона взагалі є.
            OZ_Factions.Ids(r.Factions, true);

            // The editor's columns, one per faction id above.
            for (int fi = 0; fi < r.Factions.Count(); fi++)
            {
                string fslug = r.Factions[fi];
                string flabel = fslug;
                OZ_Faction fdef = OZ_Factions.Find(fslug);
                if (fdef && fdef.DisplayName != "")
                    flabel = fdef.DisplayName;
                r.FacLabels.Insert(flabel);
                r.FacLimits.Insert(OZ_Factions.BotLimitOf(fslug));
                r.FacLeaders.Insert(OZ_RoleNames.Known(fslug + ":leader"));
            }

            OZ_Roles.TraitIds(r.Traits);
            OZ_Roles.RankIds(r.Ranks);
            OZ_Roles.FRankIds(r.FRanks);

            // ХТО В ЗОНІ -- ОДИН РАЗ, МАПОЮ (2026-09-06).
            //
            // Було: OZ_Link.Online(uid) на КОЖНУ проекцію, а він щоразу
            // перебирає GetPlayers() -- і все це двічі, бо присутніх і
            // відсутніх збирали двома проходами по всьому списку. Плюс
            // seen.Find по масиву, тобто ще квадрат. На гільдії в кілька сотень
            // прив'язаних акаунтів ростер коштував сотні тисяч порівнянь
            // РЯДКІВ на один натиск кнопки в консолі.
            //
            // Порядок рядків не змінився: присутні за порядком проекцій, далі
            // відсутні за тим самим порядком, далі присутні, яких міст не знає.
            map<string, PlayerIdentity> here = new map<string, PlayerIdentity>();

            array<Man> players = new array<Man>();
            GetGame().GetPlayers(players);

            for (int p = 0; p < players.Count(); p++)
            {
                if (!players[p])
                    continue;
                PlayerIdentity pid = players[p].GetIdentity();
                if (!pid)
                    continue;
                here.Set(pid.GetPlainId(), pid);
            }

            map<string, bool> seen = new map<string, bool>();

            if (views)
            {
                array<ref OZ_AdminRosterRow> away = new array<ref OZ_AdminRosterRow>();

                for (int v = 0; v < views.Count(); v++)
                {
                    OZ_RoleView view = views[v];
                    if (!view || view.Uid == "")
                        continue;
                    if (seen.Contains(view.Uid))
                        continue;
                    seen.Set(view.Uid, true);

                    PlayerIdentity on = null;
                    here.Find(view.Uid, on);

                    OZ_AdminRosterRow row = RowOf(view.Uid, view, on);
                    if (on)
                        r.Rows.Insert(row);
                    else
                        away.Insert(row);
                }

                for (int a = 0; a < away.Count(); a++)
                    r.Rows.Insert(away[a]);
            }

            for (int i = 0; i < players.Count(); i++)
            {
                if (!players[i])
                    continue;
                PlayerIdentity id = players[i].GetIdentity();
                if (!id)
                    continue;
                if (seen.Contains(id.GetPlainId()))
                    continue;

                r.Rows.Insert(RowOf(id.GetPlainId(), null, id));
            }

            string outJson;
            string err;
            if (!JsonFileLoader<OZ_AdminRoster>.MakeData(r, outJson, err, false))
            {
                error = "STR_OZ_ERR_INTERNAL";
                return "";
            }

            ok = true;
            return outJson;
        }

    // ОДИН рядок ростера, обома дорогами: з проекції моста (`v`) і без неї.
    // Раніше ті самі десять полів заповнювались двома окремими блоками, і
    // вони вже встигли розійтись -- «в Зоні» другий блок ставив завжди, хоч
    // перший на це дивився.
    //
    // Iм'я вiдсутнього -- з його файла ЧЕРЕЗ Peek: Load завiв би файл кожному
    // акаунту гiльдiї, який на цьому серверi нiколи не був, i тримав би їх усi
    // в кешi до кiнця запуску. Немає файла -- iм'я в Discord, а на крайнiй
    // випадок uid.
    private static OZ_AdminRosterRow RowOf(string uid, OZ_RoleView v, PlayerIdentity on)
        {
            OZ_AdminRosterRow row = new OZ_AdminRosterRow();
            row.Uid    = uid;
            row.Online = on != null;

            if (on)
            {
                row.Name = on.GetName();
            }
            else
            {
                OZ_PlayerData pd = OZ_PlayerStore.Peek(uid);
                if (pd)
                    row.Name = pd.Name;
            }

            if (v)
            {
                if (row.Name == "")
                    row.Name = v.DName;
                row.Base   = v.Base;
                row.Org    = v.Org;
                row.DName  = v.DName;
                row.Traits = OZ_Roles.TraitsLine(v);
                row.Rank   = v.Rank;
                row.FRank  = OZ_Roles.ViewFRank(v);
                row.Leader = OZ_Roles.ViewIsLeader(v);
            }
            else
            {
                // Мiст про нього не казав -- питаємо власнi служби, у яких є
                // запасний шлях через файл акаунта. Iменi в Discord у нього
                // немає й бути не може: воно приїжджає рiвно тим рядком
                // ростера, якого для цього гравця мiст не прислав.
                row.Base   = OZ_Factions.BaseOfUid(uid);
                row.Org    = OZ_Factions.OrgOfUid(uid);
                row.Traits = OZ_Roles.TraitsLineOf(uid);
                row.Rank   = OZ_Roles.RankOf(uid);
                row.FRank  = OZ_Roles.FRankOf(uid);
                row.Leader = OZ_Roles.IsLeader(uid);
            }

            if (row.Name == "")
                row.Name = uid;

            return row;
        }
}
