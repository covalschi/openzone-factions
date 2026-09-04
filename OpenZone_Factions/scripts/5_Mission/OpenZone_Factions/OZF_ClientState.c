// Клієнтський приймач нашого каналу ролей.
//
// ЧОМУ 5_Mission, А НЕ МОДУЛЬ CF. Модуль живе в 4_World, а спільна точка
// збору відповідей -- OZ_RoleNotice -- у 5_Mission ядра, і видимості знизу
// вгору в Enforce немає: 3_Game -> 4_World -> 5_Mission і тільки так. Рівно
// з цієї причини ядро тримає свій OZ_ClientState там само.
//
// НІЧОГО НЕ ВИРІШУЄ. Розібрати конверт і віддати тому, хто малює, -- усе.
// Малює сторінка «Фракція» в КПК і панель FACTIONS в адмінці, і жодна з них
// не знає, що відповідь приїхала саме нашим каналом, а не ядровим.
//
// На виділеному сервері цього коду немає взагалі: там MissionServer, і
// MissionGameplay не створюється.

class OZF_ClientState
{
    private static ref OZF_ClientState s_Inst;

    static OZF_ClientState Instance()
    {
        if (!s_Inst)
            s_Inst = new OZF_ClientState();
        return s_Inst;
    }

    // Обробник CF: ім'я збігається з рядком у AddRPC посимвольно, чотири
    // параметри в цьому порядку, void, НЕ статичний.
    void OZF_RoleRes(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param3<string, bool, string> data;
        if (!ctx.Read(data))
            return;

        OZ_RoleNotice.Take(data.param1, data.param2, data.param3);
    }
}
