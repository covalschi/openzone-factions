// Сторінка «Фракція»: хто ми, хто в нас, і фракційні дії. Все, що тут
// відбувається, йде через OZ_RoleOps (лідерські дії -- RoleRequest прямо з
// клієнта, як і раніше); сторінка лише ЗБИРАЄ стан в одну відповідь.
//
// Оновлюється САМА: ядро дзвонить у OZ_RoleNotify на кожну зміну проекції,
// а модуль розносить "push" усім онлайн -- і цій сторінці, і контактам.

class OZ_PdaHandlerFaction : OZ_PageHandler
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok    = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        if (op == "state")
            return State(sender, ok, error);

        return "";
    }

    // ВКЛАДКИ НЕМАЄ ЗОВСІМ, поки гравець не в угрупованні.
    //
    // Рішення власника 2026-08-30, дослівно: «"сталкери" не должні біть во
    // вкладке "Factions" в пда, сделай флаг "basefaction" для сталкеров, все
    // "basefaction" не активируют єту вкладку».
    //
    // Половина цього жила тут із самого початку -- прапорець BaseFaction і
    // OZ_Factions.IsBase(), -- а воріт не було: склейка реєструвала сторінку
    // безумовно, і IsBase для видимості вкладки не читав ніхто. Ось вони.
    //
    // Не «екран, який каже, що фракції немає»: екран без ростера все одно
    // лишається вкладкою, якій власник сказав не бути. Вступив в угруповання
    // -- вкладка прийшла; вийшов -- пішла. Хто ти в Зоні, показують КОНТАКТИ.
    override bool VisibleFor(string uid)
    {
        if (uid == "")
            return false;
        return OZ_Factions.OrgOfUid(uid) != "";
    }

    private string State(PlayerIdentity sender, out bool ok, out string error)
    {
        string acting = sender.GetPlainId();
        string uid    = acting;

        // Акаунт називає ПРИСТРІЙ -- та сама доктрина, що в розмов.
        OZ_PDA_Base dev = OZ_PdaLookup.HeldBy(sender);
        if (dev && dev.OZ_SessionUid() != "")
            uid = dev.OZ_SessionUid();

        // ЧИТАЄМО ПРИСТРОЄМ, ДІЄМО СОБОЮ -- і поки це розходилось, чужий КПК
        // видавав свого хазяїна (2026-09-06).
        //
        // Стан збирався для сесії ПРИЛАДУ, а кожен OZF_RoleReq виконується від
        // імені ВІДПРАВНИКА. Той, хто підняв чужий КПК, бачив хазяйське
        // запрошення, поіменний список його друзів поза фракцією і лідерські
        // кнопки -- усі до одної безсилі, бо міст питав про лідерство того, хто
        // тисне. Тобто екран обіцяв владу, якої немає, і водночас показував те,
        // чого показувати не мав.
        //
        // Склад і назва фракції лишаються приладовими: це ЧИТАННЯ, і доктрина
        // «акаунт називає пристрій» саме про нього. Дії й особисте -- за тим,
        // хто справді натисне.
        bool mine = uid == acting;

        OZ_FactionState st = new OZ_FactionState();

        // БАЗОВА фракція тут не рахується за фракцію. «Сталкери» -- це всі
        // в Зоні, а не організація: складу в неї немає, лідера немає, i
        // поіменний перелік усіх сталкерів сервера на цьому екрані був би
        // не лише безглуздий, а й видав би людей, яких ніхто не питав.
        // Екран каже чесне «фракції немає» -- те саме, що одинакові.
        string slug = OZ_Factions.OrgOfUid(uid);
        st.Org = slug;

        // Запрошення чекає на ЛЮДИНУ, а не на пристрій: приймає його той, хто
        // тисне, і кнопка «прийняти» шле accept від його імені.
        if (mine)
        {
            OZ_FactionInvite inv = OZ_FactionInvites.Pending(acting);
            if (inv)
            {
                st.InviteFaction = OZ_Factions.NameOf(inv.Faction);
                st.InviteFrom    = inv.FromName;
            }
        }

        if (slug == "")
        {
            ok = true;
            error = "";
            return Serialise(st, ok, error);
        }

        st.FactionName = OZ_Factions.NameOf(slug);
        st.Color       = OZ_Factions.ColorARGB(slug);
        // ЗВАННЯ В ШАПЦІ -- ТОГО, ХТО ТИСНЕ. Сталкерське звання особисте й
        // фракції не належить: у чужому КПК чесно стоїть звання того, хто
        // його підняв, а не хазяїна. Порожньо, поки міст про нього мовчить.
        st.MyRank      = OZ_RoleNames.Of(OZ_Roles.RankOf(acting));

        // ЧЛЕНСТВО -- ТЕЖ ЙОГО, і саме воно вмикає «піти з фракції».
        //
        // Кнопка стояла на приладовій фракції (`st.Org != ""`), тобто на
        // фракції ХАЗЯЇНА: одинак, який підняв чужий КПК, бачив «покинути
        // Найманців» і підтвердження «втратиш звання <хазяїна>», а натиск слав
        // faction.clear ВІД СЕБЕ -- бо кожен OZF_RoleReq виконується від імені
        // відправника. Міст чесно чистив порожню фракцію того, хто тисне, і
        // відповідав «Готово».
        //
        // Два рядки, а не `mine && ...`: складене «і», присвоєне ПОЛЮ
        // об'єкта, дає не той результат (зміряно, див. MeLeader нижче).
        st.MeMember = false;
        if (mine)
            st.MeMember = OZ_Factions.OrgOfUid(acting) != "";

        // Лідерські кнопки -- лише тому, чиє лідерство міст і перевірятиме.
        //
        // ДВА РЯДКИ, А НЕ `mine && OZ_Roles.IsLeader(acting)`, і це не стиль.
        // Зміряно на стенді 2026-09-06: при `mine=true` і `IsLeader=true`
        // (обидва надруковані в лог тим самим викликом рядком нижче) поле
        // діставало FALSE. Логічне «і» з локальним bool ліворуч і статичним
        // викликом праворуч, присвоєне ПОЛЮ об'єкта, дає не той результат;
        // окремий `if` дає правильний.
        st.MeLeader = false;
        if (mine)
            st.MeLeader = OZ_Roles.IsLeader(acting);

        // Драбина фракції -- щоб лідер міг підвищувати й знижувати, не
        // набираючи слагів: клієнт бере сусідню сходинку сам.
        OZ_Roles.FRankLadder(slug, st.RankIds, st.RankNames);

        // Члени: кеш проекцій за цей запуск плюс усі онлайн із цією
        // фракцією -- і я сам. Повного вічного списку сервер не має, і
        // чесніше показати відоме, ніж вигадувати.
        array<string> uids = new array<string>();
        OZ_Roles.OrgMembers(slug, uids);

        // ХТО В ЗОНІ -- ОДНИМ ПРОХОДОМ. Присутність кожного члена питали в
        // OZ_ChatWho.Online, а той перебирає GetPlayers() наново: на екран, що
        // сам оновлюється раз на п'ять секунд і на кожен push, це коштувало
        // O(склад * присутні). Присутніх ми тут і так перебираємо -- лишається
        // запам'ятати їх мапою.
        map<string, bool> online = new map<string, bool>();

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int pi = 0; pi < players.Count(); pi++)
        {
            if (!players[pi])
                continue;
            PlayerIdentity oid = players[pi].GetIdentity();
            if (!oid)
                continue;
            string ou = oid.GetPlainId();
            online.Set(ou, true);
            if (OZ_Factions.OrgOfUid(ou) == slug && uids.Find(ou) == -1)
                uids.Insert(ou);
        }
        if (uids.Find(uid) == -1)
            uids.Insert(uid);

        for (int i = 0; i < uids.Count(); i++)
        {
            // Peek, а не Load: відсутньому члену Load завів би файл і тримав
            // би його в кеші до кінця запуску (див. OZ_PlayerStore).
            OZ_PlayerData md = OZ_PlayerStore.Peek(uids[i]);
            if (!md || md.Name == "")
                continue;   // безіменний кеш нікому нічого не скаже

            OZ_FactionMember m = new OZ_FactionMember();
            m.Name   = md.Name;
            m.Key    = OZ_Names.KeyOf(OZ_PlayerStore.KeyOf(uids[i]));
            m.Rank    = OZ_RoleNames.Of(OZ_Roles.RankOf(uids[i]));
            m.FRankId = OZ_Roles.FRankOf(uids[i]);
            if (m.FRankId != "")
                m.FRank = OZ_RoleNames.Of(slug + ":" + m.FRankId);
            m.Leader = OZ_Roles.IsLeader(uids[i]);
            m.Online = online.Contains(uids[i]);
            // «ЦЕ Я» -- про того, хто тисне. Клієнт бере з цього рядка своє
            // фракційне звання для попередження «що саме буде втрачено», тож
            // на чужому КПК позначка мусить або стояти на власному рядку
            // носія (він у тій самій фракції), або не стояти ніде.
            m.Me     = uids[i] == acting;
            st.Members.Insert(m);
        }

        // КАНДИДАТІВ НА ЗАПРОШЕННЯ ТУТ БІЛЬШЕ НЕМАЄ (2026-09-06).
        //
        // Поле Candidates наповнювалось кожен опит стану -- прохід по друзях
        // лідера з читанням файла кожного, -- і жоден екран його не читав
        // ЖОДНОГО разу: кличуть того, кого вибрано в лівій половині вкладки, у
        // контактах (OZ_PdaPageFaction.ContactPick). Тобто сервер щоп'ять
        // секунд збирав і слав проводом поіменний список чужих друзів, який
        // клієнт викидав.
        return Serialise(st, ok, error);
    }

    private string Serialise(OZ_FactionState st, out bool ok, out string error)
    {
        string outJson;
        string err;
        if (!JsonFileLoader<OZ_FactionState>.MakeData(st, outJson, err, false))
        {
            ok = false;
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return outJson;
    }
}

// Розголос «ролі змінились»: обидві сторінки, яким не байдуже, чують
// push і перечитують стан самі.
//
// ОДНА РОЗСИЛКА НА ЧЕРГУ ЗМІН, а не одна на кожну зміну (2026-09-06).
//
// Розсилка коштує два гарантовані RPC КОЖНОМУ присутньому, тобто O(P) на
// зміну. Проекції приїжджають ПАЧКАМИ: одне доставлення опиту моста несе
// стільки конвертів роду "roles", скільки їх назбиралось, а `Fresh` після
// перепідключення моста -- по конверту на КОЖНОГО прив'язаного гравця. Виходило
// O(N*P): тридцять гравців у Зоні на перепідключенні моста давали дев'ятсот
// розсилок, тобто тисячу вісімсот гарантованих RPC в один кадр, і кожен клієнт
// відповідав на них тисячею запитів стану.
//
// Пачка стискається в один прапорець і ОДИН CallLater(0): черга виконує його
// в наступному тіку, коли всі конверти пачки вже застосовані. Кожен присутній
// дістає рівно один push на пачку, хоч би скільки проекцій у ній змінилось --
// а зміст push'а й так порожній, це просто «перечитай стан».
//
// Втрачається лише uid, і він тут ніколи не був потрібен: push іде ВСІМ, бо
// склад чужої фракції й рядок контакту змінюються від чужої ролі так само, як
// від власної.
class OZ_PdaRolePush
{
    private static bool s_Pending = false;

    static void Changed(string uid)
    {
        if (s_Pending)
            return;

        s_Pending = true;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OZ_PdaRolePush.Flush, 0, false);
    }

    static void Flush()
    {
        s_Pending = false;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            if (!players[i])
                continue;
            PlayerIdentity id = players[i].GetIdentity();
            if (!id)
                continue;
            OZ_Rpc.Respond(id, OZFP_Const.PAGE_FACTION, "push", true, "", "");
            OZ_Rpc.Respond(id, OZ_PdaConst.PAGE_CONTACTS, "push", true, "", "");
        }
    }
}
