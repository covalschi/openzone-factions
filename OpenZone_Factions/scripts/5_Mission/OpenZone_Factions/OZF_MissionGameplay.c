// Клієнтська точка входу мода фракцій: одна реєстрація і нічого більше.
//
// Дзеркало ядрового OZ_MissionGameplay і з тієї ж причини: приймач живе в
// 5_Mission, а модуль CF -- у 4_World, і покликати звідти сюди не можна.
//
// На виділеному сервері MissionGameplay не створюється взагалі (там
// MissionServer), тож цей код туди просто не потрапляє -- і не мусить:
// відповідь ролей приймає клієнт, серверу приймати нічого.

modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();

        OZF_Rpc.RegisterClient(OZF_ClientState.Instance());
    }
}
