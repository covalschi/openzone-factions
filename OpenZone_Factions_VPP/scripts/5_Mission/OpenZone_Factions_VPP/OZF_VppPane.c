// Панель «FACTIONS» адмiнського вiкна OpenZone -- окремий pbo.
//
// Чiпляється вкладкою до вiкна ядра через modded class, рiвно як це робить
// панель рацiї: ядро про фракцiї не знає й знати не мусить.
//
// НАВIЩО ЦЕ ТУТ, А НЕ В ЯДРI. Фракцiйна система виїхала з ядра окремим модом
// 2026-09-01 (рiшення власника: у ядрi лишаються служби, а не гра). Панель --
// її обличчя: ростер, ранги, звання, риси, призначення фракцiй i пермадес.
// Лишити її в ядрi означало б, що ядрова вкладка не збереться без мода
// фракцiй -- тобто рiвно та залежнiсть, яку ми знiмали.
//
// Гарди: NO_GUI -- сервер компiлює Mission без UI; AVPPAdminTools i
// OpenZone_VPP -- iмена класiв CfgMods (їх авто-дефайнить рушiй).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // ------------------------------------------------- фракцiї i гравцi
        //
        // Джерело фракцiй -- РОСТЕР, а не Factions.json: файла в реєстрi
        // конфiгiв бiльше немає (фракцiї народжує лише бот), а для призначення
        // досить слагiв.
        protected ref array<string> m_Factions;
        protected ref array<int> m_FacRowIdx;   // рядок списку -> iндекс у m_Factions
        protected int m_FacPicked = -1;

        // Редактор фракцiй (ТЗ-2 §15, R7.8): що показує форма i чи зведено
        // курок. Обидвi дiї назовнi -- SAVE i REMOVE -- у два натискання,
        // як i все у вкладцi.
        protected ref array<string> m_FacLabels;
        protected ref array<int>    m_FacLimits;
        protected ref array<bool>   m_FacLeaders;
        protected bool   m_FacLeaderOn   = false;
        protected bool   m_FacSaveArmed  = false;
        protected bool   m_FacDelArmed   = false;
        protected string m_FacArmedSlug  = "";

        // Ростер цiлком: картцi гравця треба все, а не лише iм'я з uid-ом.
        //
        // РЯДКАМИ, А НЕ ДЕСЯТЬМА ПАРАЛЕЛЬНИМИ МАСИВАМИ (2026-09-06). Конверт
        // приїжджає рядками; десять масивів були ручним розбиранням рядка на
        // стовпчики, після якого кожне звертання мусило пам'ятати, що індекс
        // у всіх десяти той самий. Тепер тримаємо те, що приїхало.
        protected ref array<ref OZ_AdminRosterRow> m_Rows;
        protected int m_RosterPicked = -1;

        // Каталоги з реєстру бота, i що зараз пiд курсором циклерiв. FRanks --
        // повнi id "duty:sergeant"; циклер показує лише драбину фракцiї
        // ВИБРАНОГО гравця, тому iндекс живе окремо й скидається з вибором.
        protected ref array<string> m_Traits;
        protected int m_TraitAt = 0;
        protected ref array<string> m_Ranks;
        protected int m_RankAt = 0;
        protected ref array<string> m_FRanks;
        protected int m_FRankAt = 0;

        // Пермадес: пiдтвердження другим натисканням, як i всюди у вкладцi
        // (модальнi вiкна у VPP -- пастка).
        protected bool m_WipeArmed = false;
        protected string m_WipeUid = "";

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;

        m_Factions  = new array<string>();
        m_FacRowIdx = new array<int>();
        m_FacLabels  = new array<string>();
        m_FacLimits  = new array<int>();
        m_FacLeaders = new array<bool>();
        m_Rows    = new array<ref OZ_AdminRosterRow>();
        m_Traits  = new array<string>();
        m_Ranks   = new array<string>();
        m_FRanks  = new array<string>();

        Widget pane = GetGame().GetWorkspace().CreateWidgets("OpenZone_Factions_VPP/gui/layouts/ozf_vpp_pane.layout", M_SUB_WIDGET);
        if (!pane)
        {
            OZ_Log.Error("factions vpp pane: layout failed to load");
            return;
        }

        RegisterPane("factions", "FACTIONS", pane, "FacHint");

        // ВЛАСНИЙ слухач вiдповiдей, а не гiлка в ядровому: ростер i пермадес
        // -- нашi операцiї, i ядро про них бiльше не знає.
        OZ_ClientState.AdminWatch().Insert(this.OnFactionResponse);
        OZ_Notice.OnAnswer.Insert(this.OnRoleAnswer);
    }

    void ~OZ_VppAdminMenu()
    {
        OZ_ClientState.AdminWatch().Remove(this.OnFactionResponse);
        OZ_Notice.OnAnswer.Remove(this.OnRoleAnswer);

        // Відкладені перепити теж знімаємо: до п'яти CallLater на 9,5 с
        // лишались у черзі після закриття вікна й стріляли по знищеному меню.
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.AskRoster);
    }

    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);

        if (id == "factions")
            AskRoster();

        // Циклеру фракцiй у панелi спавнiв потрiбнi слаги, а їх дає ростер.
        // Ядро бiльше не просить його само -- воно й не знає, що вiн є.
        if (id == "spawns" && m_Factions.Count() == 0)
            AskRoster();
    }

    void OnFactionResponse(string section, string op, bool ok, string json, string error)
    {
        // ОДИН розділ, і той наш. Раніше тут стояло «"admin" АБО "factions"»
        // -- і саме та поблажливість ховала помилку: клієнт слав ростер на
        // "admin", де про нього не чули, а перевірка на прийомі мовчки
        // приймала б обидві адреси, тож ніщо ніде не сварилось.
        if (section != OZF_Const.SECTION)
            return;

        if (!ok)
        {
            if (op.IndexOf("player_wipe:") == 0)
                m_WipeArmed = false;
            if (op == "faction_upsert")
                m_FacSaveArmed = false;
            if (op.IndexOf("faction_remove:") == 0)
                m_FacDelArmed = false;
            Hint("#" + error);
            return;
        }

                if (op == "faction_upsert")
                {
                    m_FacSaveArmed = false;
                    Hint("faction saved at the bot - the roster follows within seconds");
                    RefreshSoon();
                    return;
                }

                if (op.IndexOf("faction_remove:") == 0)
                {
                    m_FacDelArmed = false;
                    m_FacPicked = -1;
                    ClearFacForm();
                    Hint("faction removed at the bot - its members are plain stalkers again");
                    RefreshSoon();
                    return;
                }

                if (op == "roster")
                {
                    OZ_AdminRoster r = new OZ_AdminRoster();
                    string rerr;
                    // Копія до першого ж перемальовування: BuildRoster
                    // читає вісім списків упереміш зі своїми виділеннями.
                    if (JsonFileLoader<OZ_AdminRoster>.LoadData(json, r, rerr) && r)
                        BuildRoster(r.Copy());
                    return;
                }

                if (op.IndexOf("player_wipe:") == 0)
                {
                    m_WipeArmed = false;
                    Hint("wiped: the character starts over as a novice stalker");
                    RefreshSoon();
                    return;
                }
    }

    void OnRoleAnswer(string op, bool ok, string why)
        {
            if (!IsOpen())
                return;

            string line = op;
            if (ok)
                line += ": done";
            else
                line += ": " + Widget.TranslateString("#" + why);
            Hint(line);

            // СПАВНОВИХ ГІЛОК ТУТ БІЛЬШЕ НЕМАЄ. Спавни їдуть адмінським
            // конвертом у ядрову панель, і відповідь на них ловить саме вона
            // (OZ_VppMenu.OnSpawnAnswer). Поки підписаним був лише цей pbo,
            // сервер без мода фракцій не бачив ані підказки, ані помилки, ані
            // оновлення списку -- успіх був невідрізненний від відмови.

            // Ролi їдуть через Discord: перепитуємо ростер трохи згодом, i ще
            // раз пiзнiше -- проекцiя вертається не миттєво.
            RefreshSoon();
        }

    // ПЕРЕПИТАТИ РОСТЕР ПІСЛЯ ДІЇ -- і рівно двічі, хоч би скільки дій
        // натиснули поспіль.
        //
        // Пара CallLater була вписана в чотирьох місцях, і жодне з них не
        // знімало попередню: десяток кліків підряд ставив у чергу два десятки
        // однакових запитів ростера, кожен -- дзвінок до моста. Remove перед
        // постановкою лишає в черзі рівно два, від ОСТАННЬОЇ дії. Пізній
        // потрібен: реєстр приїжджає наступним опитом моста, до восьми секунд.
        protected void RefreshSoon()
        {
            ScriptCallQueue q = GetGame().GetCallQueue(CALL_CATEGORY_GUI);
            q.Remove(this.AskRoster);
            q.CallLater(this.AskRoster, 2500, false);
            q.CallLater(this.AskRoster, 9500, false);
        }

    protected void AskRoster()
        {
            if (IsOpen())
                Ask(OZF_Const.SECTION, "roster", "{}");
        }

    // ---------------------------------------------------------- фракцiї

        protected void BuildRoster(OZ_AdminRoster r)
        {
            m_Repaint = true;

            // Слаги фракцiй i каталоги -- з того самого конверта.
            m_Factions.Clear();
            m_FacLabels.Clear();
            m_FacLimits.Clear();
            m_FacLeaders.Clear();
            if (r.Factions)
            {
                for (int fi = 0; fi < r.Factions.Count(); fi++)
                {
                    m_Factions.Insert(r.Factions[fi]);

                    string flabel = r.Factions[fi];
                    if (r.FacLabels && fi < r.FacLabels.Count() && r.FacLabels[fi] != "")
                        flabel = r.FacLabels[fi];
                    m_FacLabels.Insert(flabel);

                    int flimit = 0;
                    if (r.FacLimits && fi < r.FacLimits.Count())
                        flimit = r.FacLimits[fi];
                    m_FacLimits.Insert(flimit);

                    bool fleader = false;
                    if (r.FacLeaders && fi < r.FacLeaders.Count())
                        fleader = r.FacLeaders[fi];
                    m_FacLeaders.Insert(fleader);
                }
            }
            if (m_FacPicked >= m_Factions.Count())
                m_FacPicked = -1;
            if (m_FacPicked >= 0)
                FillFacForm(m_FacPicked);

            // ЦИКЛЕР ПАНЕЛІ СПАВНІВ. Ядро тримає для нього шов
            // (OZ_VppFactionSlugs), і до 2026-09-01 його Set() не кликав ніхто
            // в усьому дереві: перелік завжди був порожній, SpawnSlugAt() завжди
            // повертав "-", а кнопка перемикання рахувала (n+1) % 1 == 0. Тобто
            // адмін міг завести й очистити рівно ОДНУ зону -- запасну.
            //
            // Перелік іде сюди ЦІЛКОМ, разом із базовими фракціями: зона
            // базової фракції -- окрема ступінь сходів спавну (ТЗ-5 §A1), і
            // ставити її треба тим самим циклером.
            OZ_VppFactionSlugs.Set(m_Factions);

            m_Traits.Clear();
            if (r.Traits)
            {
                for (int ti = 0; ti < r.Traits.Count(); ti++)
                    m_Traits.Insert(r.Traits[ti]);
            }
            if (m_TraitAt >= m_Traits.Count())
                m_TraitAt = 0;

            m_Ranks.Clear();
            if (r.Ranks)
            {
                for (int ri = 0; ri < r.Ranks.Count(); ri++)
                    m_Ranks.Insert(r.Ranks[ri]);
            }
            if (m_RankAt >= m_Ranks.Count())
                m_RankAt = 0;

            m_FRanks.Clear();
            if (r.FRanks)
            {
                for (int qi = 0; qi < r.FRanks.Count(); qi++)
                    m_FRanks.Insert(r.FRanks[qi]);
            }

            RebuildFacList();
            PaintSpawnCycler();
            PaintTraitCycler();
            PaintRankCycler();

            // Вибiр переживає оновлення: ростер перечитується сам пiсля кожної
            // операцiї, i губити вiд цього видiлення -- значить клацати гравця
            // заново пiсля кожної кнопки.
            string keepUid = "";
            if (m_RosterPicked >= 0 && m_RosterPicked < m_Rows.Count())
                keepUid = m_Rows[m_RosterPicked].Uid;

            // РЯДКИ -- СВОЇ, А НЕ ЗАГРУЗЧИКОВІ. Панель тримає їх між
            // оновленнями (2,5 і 9,5 секунди), а адмін клацає по них іще
            // пізніше: картка гравця й префікс драбини читаються з Org та
            // Uid хвилинами після розбору. Причина довга й лежить в
            // OZ_RoleView.Copy -- ідіома в репозиторії одна.
            m_Rows = new array<ref OZ_AdminRosterRow>();
            if (r.Rows)
            {
                for (int ri2 = 0; ri2 < r.Rows.Count(); ri2++)
                {
                    if (r.Rows[ri2])
                        m_Rows.Insert(r.Rows[ri2].Copy());
                }
            }
            m_RosterPicked = -1;
            m_WipeArmed = false;

            if (keepUid != "")
            {
                for (int i = 0; i < m_Rows.Count(); i++)
                {
                    if (m_Rows[i] && m_Rows[i].Uid == keepUid)
                    {
                        m_RosterPicked = i;
                        break;
                    }
                }
            }

            RepaintRoster();
            m_Repaint = false;
            FillPlayerCard();
            PaintFRankCycler();

            // ЧОМУ СПИСОК НЕПОВНИЙ -- останнім словом, поверх «збережено».
            // Мовчазне усічення читалось як повний ростер.
            if (r.Partial != "")
                Hint(r.Partial);
        }

    // Список гравцiв: компактний рядок, видiлений позначено стрiлкою --
        // пiдсвiтка рядка листбокса непомiтна, i це вже коштувало плутанини.
        protected void RepaintRoster()
        {
            TextListboxWidget lb = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("FacRoster"));
            if (!lb)
                return;

            lb.ClearItems();

            for (int i = 0; i < m_Rows.Count(); i++)
            {
                OZ_AdminRosterRow row = m_Rows[i];
                if (!row)
                    continue;

                string line = "";
                if (i == m_RosterPicked)
                    line = "> ";

                line += row.Name;
                // Вiдсутнi теж у списку (ТЗ-4 R-C4.2) -- i позначенi: їх можна
                // вайпнути чи призначити, але не покликати до слова.
                if (!row.Online)
                    line += " (offline)";

                // УГРУПОВАННЯ, а коли його немає -- базова в дужках. Без
                // другої половини одинак і той, хто не заходив жодного разу,
                // виглядали б однаково порожньо, а це різні люди: у першого
                // є звання й трейти, у другого немає нічого.
                if (row.Org != "")
                {
                    line += "  --  " + row.Org;
                    if (row.Leader)
                        line += " [L]";
                }
                else if (row.Base != "")
                {
                    line += "  --  (" + row.Base + ")";
                }

                lb.AddItem(line, NULL, 0);

                if (i == m_RosterPicked)
                    lb.SelectRow(i);
            }
        }

    // Картка гравця: кожен факт своїм рядком, бо в один вони не влазять.
        protected void FillPlayerCard()
        {
            TextWidget nameT    = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InfoName"));
            TextWidget discordT = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InfoDiscord"));
            TextWidget steamT   = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InfoSteam"));
            TextWidget facT     = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InfoFaction"));
            TextWidget rankT    = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InfoRank"));
            TextWidget traitsT  = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InfoTraits"));

            if (m_RosterPicked < 0 || m_RosterPicked >= m_Rows.Count() || !m_Rows[m_RosterPicked])
            {
                if (nameT)
                    nameT.SetText("pick a player in the roster below");
                if (discordT)
                    discordT.SetText("");
                if (steamT)
                    steamT.SetText("");
                if (facT)
                    facT.SetText("");
                if (rankT)
                    rankT.SetText("");
                if (traitsT)
                    traitsT.SetText("");
                return;
            }

            OZ_AdminRosterRow row = m_Rows[m_RosterPicked];

            if (nameT)
            {
                string shown = row.Name;
                if (!row.Online)
                    shown += "  (offline)";
                nameT.SetText(shown);
            }

            if (discordT)
            {
                string dn = row.DName;
                if (dn == "")
                    dn = "-";
                discordT.SetText(dn);
            }

            if (steamT)
                steamT.SetText(row.Uid);

            if (facT)
            {
                // ОБИДВІ осі в одному рядку картки: базова спершу, потім
                // угруповання зі званням і лідерством. Тире там, де осі
                // немає, -- порожнє місце й «нема угруповання» читаються
                // однаково, а це різні відповіді.
                string fac = row.Base;
                if (fac == "")
                    fac = "-";

                if (row.Org != "")
                {
                    fac += "  /  " + row.Org;
                    if (row.FRank != "")
                        fac += "  ^" + row.FRank;
                    if (row.Leader)
                        fac += "  [leader]";
                }
                facT.SetText(fac);
            }

            if (rankT)
            {
                string rk = row.Rank;
                if (rk == "")
                    rk = "-";
                rankT.SetText(rk);
            }

            if (traitsT)
            {
                string tr = row.Traits;
                if (tr == "")
                    tr = "-";
                traitsT.SetText(tr);
            }
        }

    protected void RebuildFacList()
        {
            TextListboxWidget lb = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("FacCfgList"));
            if (!lb)
                return;

            string filter = "";
            EditBoxWidget se = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("FacSearch"));
            if (se)
            {
                filter = se.GetText();
                filter.ToLower();
            }

            lb.ClearItems();
            m_FacRowIdx.Clear();

            for (int i = 0; i < m_Factions.Count(); i++)
            {
                string line = m_Factions[i];
                if (i < m_FacLabels.Count() && m_FacLabels[i] != "" && m_FacLabels[i] != m_Factions[i])
                    line += "  --  " + m_FacLabels[i];

                if (filter != "")
                {
                    string probe = line;
                    probe.ToLower();
                    if (probe.IndexOf(filter) == -1)
                        continue;
                }

                // Видiлення позначаємо стрiлкою: пiдсвiтка рядка листбокса
                // непомiтна, i хто видiлений -- було не зрозумiло.
                if (i == m_FacPicked)
                    line = "> " + line;

                int row = lb.AddItem(line, NULL, 0);
                m_FacRowIdx.Insert(i);

                if (i == m_FacPicked)
                    lb.SelectRow(row);
            }
        }

    // ---------------------------------------------------- редактор фракцiй

        // Форма показує вибрану фракцiю; NEW чистить її для нової.
        protected void FillFacForm(int idx)
        {
            if (idx < 0 || idx >= m_Factions.Count())
            {
                ClearFacForm();
                return;
            }
            SetEdit("FacSlug", m_Factions[idx]);
            SetEdit("FacLabel", m_FacLabels[idx]);
            SetEdit("FacLimit", m_FacLimits[idx].ToString());
            m_FacLeaderOn = m_FacLeaders[idx];
            m_FacSaveArmed = false;
            m_FacDelArmed  = false;
            PaintFacLeader();
        }

        protected void ClearFacForm()
        {
            SetEdit("FacSlug", "");
            SetEdit("FacLabel", "");
            SetEdit("FacLimit", "0");
            m_FacLeaderOn  = true;
            m_FacSaveArmed = false;
            m_FacDelArmed  = false;
            PaintFacLeader();
        }

        protected void PaintFacLeader()
        {
            TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnFacLeaderText"));
            if (!t)
                return;
            if (m_FacLeaderOn)
                t.SetText("LEADER POST: yes");
            else
                t.SetText("LEADER POST: no");
        }

        protected string FormSlug()
        {
            string slug = GetEdit("FacSlug");
            slug.Trim();
            slug.ToLower();
            return slug;
        }

    // Цiль дiй з гравцем: точна адреса uid з ростера. Сервер приймає таку
        // форму лише вiд адмiна -- тезки й самопризначення перестають бути
        // проблемою.
        protected string PickedPlayer()
        {
            if (m_RosterPicked < 0 || m_RosterPicked >= m_Rows.Count() || !m_Rows[m_RosterPicked])
                return "";
            return m_Rows[m_RosterPicked].Uid;
        }

    protected void PaintTraitCycler()
        {
            TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTraitText"));
            if (!t)
                return;

            if (m_Traits.Count() == 0)
            {
                t.SetText("trait: none known yet");
                return;
            }
            t.SetText("trait: " + m_Traits[m_TraitAt]);
        }

    protected void PaintRankCycler()
        {
            TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnRankText"));
            if (!t)
                return;

            if (m_Ranks.Count() == 0)
            {
                t.SetText("rank: none known yet");
                return;
            }
            t.SetText("rank: " + m_Ranks[m_RankAt]);
        }

    // Драбина УГРУПОВАННЯ вибраного гравця, голими слагами. Порожньо --
        // гравець не вибраний, поза угрупованням, або звань там не завели.
        protected void FRankOptions(array<string> outBare)
        {
            if (m_RosterPicked < 0 || m_RosterPicked >= m_Rows.Count() || !m_Rows[m_RosterPicked])
                return;

            // Драбина належить УГРУПОВАННЮ: у базової фракції внутрішніх
            // звань немає й бути не може -- вона в усіх однакова.
            string prefix = m_Rows[m_RosterPicked].Org + ":";
            if (prefix == ":")
                return;

            for (int i = 0; i < m_FRanks.Count(); i++)
            {
                if (m_FRanks[i].IndexOf(prefix) == 0)
                    outBare.Insert(m_FRanks[i].Substring(prefix.Length(), m_FRanks[i].Length() - prefix.Length()));
            }
        }

    protected void PaintFRankCycler()
        {
            TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnFRankText"));
            if (!t)
                return;

            array<string> opts = new array<string>();
            FRankOptions(opts);

            if (opts.Count() == 0)
            {
                t.SetText("faction rank: none here");
                return;
            }

            if (m_FRankAt >= opts.Count())
                m_FRankAt = 0;
            t.SetText("faction rank: " + opts[m_FRankAt]);
        }

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (!M_SUB_WIDGET || m_Repaint || !w)
            return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);

        string nm = w.GetName();

                if (nm == "FacCfgList")
                {
                    if (row >= 0 && row < m_FacRowIdx.Count())
                    {
                        m_FacPicked = m_FacRowIdx[row];
                        m_Repaint = true;
                        RebuildFacList();
                        m_Repaint = false;
                        FillFacForm(m_FacPicked);
                        Hint("faction: " + m_Factions[m_FacPicked] + " - edit the form and press SAVE twice, or ASSIGN the picked player");
                    }
                    return true;
                }

                if (nm == "FacRoster")
                {
                    if (row >= 0 && row < m_Rows.Count())
                    {
                        m_RosterPicked = row;
                        m_WipeArmed = false;
                        m_FRankAt = 0;
                        m_Repaint = true;
                        RepaintRoster();
                        m_Repaint = false;
                        FillPlayerCard();
                        PaintFRankCycler();
                    }
                    return true;
                }

        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
                if (w && w.GetName() == "FacSearch")
                {
                    RebuildFacList();
                    return true;
                }

                if (w && (w.GetName() == "FacSlug" || w.GetName() == "FacLabel" || w.GetName() == "FacLimit"))
                {
                    m_FacSaveArmed = false;
                    m_FacDelArmed  = false;
                    return true;
                }

        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w || !M_SUB_WIDGET)
            return super.OnClick(w, x, y, button);

        string nm = w.GetName();

                if (nm == "BtnAssign" || nm == "BtnClearFac" || nm == "BtnLead")
                {
                    string uid = PickedPlayer();
                    if (uid == "")
                    {
                        Hint("pick a player on the list first");
                        return true;
                    }
                    string player = "uid:" + uid;

                    if (nm == "BtnAssign")
                    {
                        if (m_FacPicked < 0 || m_FacPicked >= m_Factions.Count())
                        {
                            Hint("pick a faction on the left first");
                            return true;
                        }
                        OZF_Rpc.RoleRequest(OZ_RoleOp.FACTION_SET, player, m_Factions[m_FacPicked]);
                    }
                    else if (nm == "BtnClearFac")
                    {
                        OZF_Rpc.RoleRequest(OZ_RoleOp.FACTION_CLEAR, player, "");
                    }
                    else
                    {
                        // Консоль ставить лiдера НАПРЯМУ. leader.transfer тут не
                        // годиться: вiн -- акт лiдера й вимагає, щоб актор сам
                        // тримав пост (змiряно: nobody to hand it over from).
                        OZF_Rpc.RoleRequest(OZ_RoleOp.LEADER_SET, player, "");
                    }
                    return true;
                }

                if (nm == "BtnRank")
                {
                    if (m_Ranks.Count() > 0)
                        m_RankAt = (m_RankAt + 1) % m_Ranks.Count();
                    PaintRankCycler();
                    return true;
                }

                if (nm == "BtnFRank")
                {
                    array<string> copts = new array<string>();
                    FRankOptions(copts);
                    if (copts.Count() > 0)
                        m_FRankAt = (m_FRankAt + 1) % copts.Count();
                    PaintFRankCycler();
                    return true;
                }

                if (nm == "BtnFRankSet" || nm == "BtnFRankClear")
                {
                    string fuid = PickedPlayer();
                    if (fuid == "")
                    {
                        Hint("pick a player on the list first");
                        return true;
                    }

                    if (nm == "BtnFRankSet")
                    {
                        array<string> fopts = new array<string>();
                        FRankOptions(fopts);
                        if (fopts.Count() == 0)
                        {
                            Hint("his faction has no ranks - add them via the bot");
                            return true;
                        }
                        if (m_FRankAt >= fopts.Count())
                            m_FRankAt = 0;
                        OZF_Rpc.RoleRequest(OZ_RoleOp.FRANK_SET, "uid:" + fuid, fopts[m_FRankAt]);
                    }
                    else
                    {
                        OZF_Rpc.RoleRequest(OZ_RoleOp.FRANK_SET, "uid:" + fuid, "");
                    }
                    return true;
                }

                if (nm == "BtnRankSet" || nm == "BtnRankClear")
                {
                    string ruid = PickedPlayer();
                    if (ruid == "")
                    {
                        Hint("pick a player on the list first");
                        return true;
                    }

                    if (nm == "BtnRankSet")
                    {
                        if (m_Ranks.Count() == 0)
                        {
                            Hint("no ranks in the bot registry yet");
                            return true;
                        }
                        OZF_Rpc.RoleRequest(OZ_RoleOp.RANK_SET, "uid:" + ruid, m_Ranks[m_RankAt]);
                    }
                    else
                    {
                        // Порожнiй аргумент -- зняти звання зовсiм: так читає його
                        // мiст (rank.set без arg знiмає всi).
                        OZF_Rpc.RoleRequest(OZ_RoleOp.RANK_SET, "uid:" + ruid, "");
                    }
                    return true;
                }

                if (nm == "BtnTrait")
                {
                    if (m_Traits.Count() > 0)
                        m_TraitAt = (m_TraitAt + 1) % m_Traits.Count();
                    PaintTraitCycler();
                    return true;
                }

                if (nm == "BtnTraitAdd" || nm == "BtnTraitDel")
                {
                    string tuid = PickedPlayer();
                    if (tuid == "")
                    {
                        Hint("pick a player on the list first");
                        return true;
                    }
                    if (m_Traits.Count() == 0)
                    {
                        Hint("no traits in the bot registry yet");
                        return true;
                    }

                    if (nm == "BtnTraitAdd")
                        OZF_Rpc.RoleRequest(OZ_RoleOp.TRAIT_ADD, "uid:" + tuid, m_Traits[m_TraitAt]);
                    else
                        OZF_Rpc.RoleRequest(OZ_RoleOp.TRAIT_REMOVE, "uid:" + tuid, m_Traits[m_TraitAt]);
                    return true;
                }

                if (nm == "BtnPSpawn" || nm == "BtnPSpawnClear")
                {
                    string suid = PickedPlayer();
                    if (suid == "")
                    {
                        Hint("pick a player on the list first");
                        return true;
                    }

                    if (nm == "BtnPSpawn")
                    {
                        string arg = suid;
                        string rad = GetEdit("PSpRadius");
                        if (rad != "")
                            arg += " " + rad;
                        Ask(OZ_AdminSect.SPAWNS, OZ_SpawnOp.UID_HERE, arg);
                    }
                    else
                    {
                        Ask(OZ_AdminSect.SPAWNS, OZ_SpawnOp.UID_CLEAR, suid);
                    }
                    return true;
                }

                if (nm == "BtnWipe")
                {
                    string wuid = PickedPlayer();
                    if (wuid == "")
                    {
                        Hint("pick a player on the list first");
                        return true;
                    }

                    // Пiдтвердження другим натисканням ПО ТОМУ Ж гравцевi: змiна
                    // вибору скидає зброю, iнакше пiдтвердженням для одного стало
                    // б натискання, зроблене для iншого.
                    if (!m_WipeArmed || m_WipeUid != wuid)
                    {
                        m_WipeArmed = true;
                        m_WipeUid = wuid;
                        Hint("press WIPE again to erase " + m_Rows[m_RosterPicked].Name + " forever");
                        return true;
                    }

                    m_WipeArmed = false;
                    Ask(OZF_Const.SECTION, "player_wipe:" + wuid, "{}");
                    Hint("wiping...");
                    return true;
                }

                if (nm == "BtnRoster")
                {
                    Ask(OZF_Const.SECTION, "roster", "{}");
                    return true;
                }

                // ---- редактор фракцiй (ТЗ-2 §15, R7.8)

                if (nm == "BtnFacNew")
                {
                    m_FacPicked = -1;
                    m_Repaint = true;
                    RebuildFacList();
                    m_Repaint = false;
                    ClearFacForm();
                    Hint("new faction: slug (lowercase, 2-24), name, limit (0 = none), leader post - then SAVE twice");
                    return true;
                }

                if (nm == "BtnFacLeader")
                {
                    m_FacLeaderOn = !m_FacLeaderOn;
                    m_FacSaveArmed = false;
                    PaintFacLeader();
                    return true;
                }

                if (nm == "BtnFacSave")
                {
                    string sslug = FormSlug();
                    if (sslug == "")
                    {
                        Hint("type a slug first - a short lowercase word");
                        return true;
                    }

                    // Другим натисканням ПО ТОМУ Ж слагу: змiна форми скидає
                    // курок, iнакше пiдтвердженням для однiєї фракцiї стало б
                    // натискання, зроблене для iншої.
                    if (!m_FacSaveArmed || m_FacArmedSlug != sslug)
                    {
                        m_FacSaveArmed = true;
                        m_FacDelArmed  = false;
                        m_FacArmedSlug = sslug;
                        if (m_Factions.Find(sslug) == -1)
                            Hint("press SAVE again to CREATE faction " + sslug + " at the bot (and its Discord role when the roles mirror is on)");
                        else
                            Hint("press SAVE again to CHANGE faction " + sslug + " at the bot");
                        return true;
                    }
                    m_FacSaveArmed = false;

                    OZF_FactionEdit e = new OZF_FactionEdit();
                    e.Slug      = sslug;
                    e.Label     = GetEdit("FacLabel");
                    e.Label.Trim();
                    string limitText = GetEdit("FacLimit");
                    e.Limit     = limitText.ToInt();
                    e.HasLeader = m_FacLeaderOn;

                    string letter;
                    string jerr;
                    if (!JsonFileLoader<OZF_FactionEdit>.MakeData(e, letter, jerr, false))
                    {
                        Hint("cannot build the request: " + jerr);
                        return true;
                    }
                    Ask(OZF_Const.SECTION, "faction_upsert", letter);
                    Hint("saving faction " + sslug + "...");
                    return true;
                }

                if (nm == "BtnFacDel")
                {
                    string dslug = FormSlug();
                    if (dslug == "")
                    {
                        Hint("pick a faction on the left first");
                        return true;
                    }
                    if (m_Factions.Find(dslug) == -1)
                    {
                        Hint("no such faction: " + dslug);
                        return true;
                    }

                    if (!m_FacDelArmed || m_FacArmedSlug != dslug)
                    {
                        m_FacDelArmed  = true;
                        m_FacSaveArmed = false;
                        m_FacArmedSlug = dslug;
                        Hint("press REMOVE again to delete faction " + dslug + " at the bot - its members become plain stalkers");
                        return true;
                    }
                    m_FacDelArmed = false;

                    Ask(OZF_Const.SECTION, "faction_remove:" + dslug, "{}");
                    Hint("removing faction " + dslug + "...");
                    return true;
                }

        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
