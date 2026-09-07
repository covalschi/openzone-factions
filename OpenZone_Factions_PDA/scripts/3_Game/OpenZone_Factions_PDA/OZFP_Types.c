// Форма, якою сторінка «Фракція» говорить із сервером.
//
// ЖИВЕ В СКЛЕЙЦІ, а не в КПК. Сторінка фракції -- це фракційна система, яку
// власник виніс із ядра окремим модом 2026-08-31; КПК про неї більше не знає
// нічого. Сервер без мода фракцій просто не має цієї вкладки, так само як не
// має вкладки рації без склейки з рацією.

// --- сторінка «Фракція» ---

class OZ_FactionMember
{
    string Name = "";
    // АДРЕСА для лідерських дій (ТЗ-4 R-C4.1): непрозорий ключ персонажа,
    // той самий, що й у рядках контактів (OZ_Names.KeyOf від "<uid>#<gen>").
    // Steam64 клієнтові не їде, як і раніше. Ім'я лишається підписом: на
    // тезках воно двозначне.
    string Key  = "";
    // Сталкерське звання -- особисте, поза фракцією.
    string Rank = "";
    // Внутрішньофракційне: підпис для екрана і слаг для арифметики
    // «наступне вище». Порожньо -- звання немає.
    string FRank   = "";
    string FRankId = "";
    bool Leader = false;
    bool Online = false;
    bool Me     = false;

    OZ_FactionMember Copy()
    {
        OZ_FactionMember c = new OZ_FactionMember();
        c.Name    = Name;
        c.Key     = Key;
        c.Rank    = Rank;
        c.FRank   = FRank;
        c.FRankId = FRankId;
        c.Leader  = Leader;
        c.Online  = Online;
        c.Me      = Me;
        return c;
    }
}

class OZ_FactionState
{
    // Порожній slug -- одинак: сторінка чесно каже, що фракції немає.
    // УГРУПОВАННЯ гравця, або порожньо. Базова фракція сюди не потрапляє
    // ніколи: у неї немає ані складу, ані лідера, і екран узагалі не про неї.
    string Org         = "";
    string FactionName = "";
    int    Color       = 0;
    // ТРИ ПОЛЯ ПРО ТОГО, ХТО ТИСНЕ, а не про хазяїна приладу.
    //
    // Склад і назва фракції читаються ПРИЛАДОМ (доктрина «акаунт називає
    // пристрій»), але звання в шапці, позначка «це я» в рядку складу й
    // кнопка «піти» -- це вже про людину з КПК у руках, бо саме її uid
    // поїде в OZF_RoleReq. Поки MeMember не було, одинак із чужим КПК
    // діставав кнопку «покинути <фракцію хазяїна>», а натиск слав
    // faction.clear на його власну, якої немає.
    string MyRank      = "";
    bool   MeMember    = false;
    bool   MeLeader    = false;

    // Запрошення, що чекає САМЕ на мене.
    string InviteFaction = "";
    string InviteFrom    = "";

    ref array<ref OZ_FactionMember> Members;

    // ПОЛЯ Candidates ТУТ БІЛЬШЕ НЕМАЄ (2026-09-06): сервер його наповнював,
    // клієнт не читав ніколи. Кличуть того, кого вибрано в контактах.

    // Драбина ЦІЄЇ фракції, знизу вгору: слаги й підписи поруч. Порожня --
    // звань у фракції не заводили, і кнопки підвищення нема сенсу малювати.
    ref array<string> RankIds;
    ref array<string> RankNames;

    void OZ_FactionState()
    {
        Members    = new array<ref OZ_FactionMember>();
        RankIds    = new array<string>();
        RankNames  = new array<string>();
    }

    // Копія, яку збудував скрипт. Сторінка тримає стан МІЖ ОПИТАМИ -- п'ять
    // секунд і довше, -- і перемальовує з нього шапку, склад і тексти
    // підтверджень, тобто читає ті самі поля хвилинами після розбору.
    // Причина довга й лежить в OZ_RoleView.Copy (OpenZone_Factions);
    // ідіома в репозиторії одна.
    OZ_FactionState Copy()
    {
        OZ_FactionState c = new OZ_FactionState();
        c.Org           = Org;
        c.FactionName   = FactionName;
        c.Color         = Color;
        c.MyRank        = MyRank;
        c.MeMember      = MeMember;
        c.MeLeader      = MeLeader;
        c.InviteFaction = InviteFaction;
        c.InviteFrom    = InviteFrom;

        if (Members)
        {
            for (int i = 0; i < Members.Count(); i++)
            {
                if (Members[i])
                    c.Members.Insert(Members[i].Copy());
            }
        }
        if (RankIds)
        {
            for (int j = 0; j < RankIds.Count(); j++)
                c.RankIds.Insert(RankIds[j]);
        }
        if (RankNames)
        {
            for (int k = 0; k < RankNames.Count(); k++)
                c.RankNames.Insert(RankNames[k]);
        }

        return c;
    }
}
