// Канал змін ролей -- НАШ, під нашим іменем мода.
//
// Був у ядрі (OZ_Rpc.RPC_ROLE_REQ/RPC_ROLE_RES, RegisterRoles, RoleRequest,
// RoleRespond) і поїхав сюди 2026-09-04. Ядро тримало реєстрацію й обидва
// кінці заради гри, якої в ньому немає: без цього мода той RPC не слухав
// ніхто, і половина конверта стояла мертвою.
//
// CF ключує RPC парою (ім'я мода, ім'я функції), тому пара
// ("OpenZone_Factions", "OZF_RoleReq") не може зіткнутись ні з ядровою, ні з
// чужою: збігтись мусили б ОБИДВА рядки.
//
// ЩО ТУТ НЕ ПОВТОРЮЄТЬСЯ З ЯДРА: різання довгого тіла на частини. Ядро ріже
// сторінкові й адмінські конверти, бо ті возять JSON у кілька кілобайт, а
// рушійний RPC псує рядки понад ~1024 байти (OZ_Const.RPC_STR_CHUNK, 900).
// Тут не возиться JSON узагалі -- операція, адреса цілі й один аргумент, --
// і жодне з трьох полів не наближається до межі. Єдине, що може вирости, --
// це `why` у відповіді, коли причину дає міст своїми словами; про це нижче.

class OZF_Rpc
{
    // ІМЕНА -- ВЛАСНІ, з префіксом мода. У ядрі ці ж функції звались
    // "OZ_RoleReq"/"OZ_RoleRes"; ядрове "OZ_RoleRes" ЛИШИЛОСЬ ЖИВИМ і возить
    // звістки КПК (обмін контактами), тож однакові імена під однаковим
    // іменем мода означали б два обробники на один ключ.
    static const string RPC_ROLE_REQ = "OZF_RoleReq";
    static const string RPC_ROLE_RES = "OZF_RoleRes";

    // Зареєстрована функція МУСИТЬ мати рівно цю форму -- її задає диспетчер
    // CF (Param4 + CallFunctionParams), ніде не оголошуючи явно:
    //
    //   void Ім'я(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    //
    // Чотири параметри в цьому порядку, void, ім'я збігається з рядком
    // посимвольно, метод НЕ статичний.
    static void RegisterServer(Class inst)
    {
        GetRPCManager().AddRPC(OZF_Const.MOD, RPC_ROLE_REQ, inst, SingleplayerExecutionType.Server);
    }

    static void RegisterClient(Class inst)
    {
        GetRPCManager().AddRPC(OZF_Const.MOD, RPC_ROLE_RES, inst, SingleplayerExecutionType.Client);
    }

    // Кого міняємо -- ІМ'ЯМ, а не uid.
    //
    // Чужого Steam64 клієнт не бачить НІКОЛИ -- це межа, яку тримає вся
    // сторінка контактів, і вона не робиться винятком заради зручності. Кому
    // належить ім'я, вирішує сервер, і серед кого шукати -- теж він.
    //
    // Актора не називаємо взагалі: він завжди береться з sender.
    //
    // guaranteed за замовчуванням FALSE. Усе, що тут надсилається, має
    // значення, тому true передається явно скрізь.
    static void RoleRequest(string op, string targetName, string arg)
    {
        Param3<string, string, string> p =
            new Param3<string, string, string>(op, targetName, arg);
        GetRPCManager().SendRPC(OZF_Const.MOD, RPC_ROLE_REQ, p, true);
    }

    // `why` -- або ключ таблиці рядків (STR_OZ_...), або ГОТОВИЙ ТЕКСТ від
    // моста. Друге тому, що причину відмови Discord знає лише він, і «бот не
    // може керувати цією роллю» набагато корисніше за наш код помилки. Той,
    // хто малює, розрізняє їх за префіксом (OZ_RoleNotice.Text).
    //
    // ТЕКСТ МОСТА -- ЄДИНЕ ТУТ, ЩО МОЖЕ ВИРОСТИ, і саме тому він приходить із
    // чужого боку вже підрізаним: міст кліпає свої рядки на тій самій межі
    // ~1024 байти, на якій рушійний RPC починає віддавати "String CORRUPTED".
    static void RoleRespond(PlayerIdentity to, string op, bool ok, string why)
    {
        Param3<string, bool, string> p = new Param3<string, bool, string>(op, ok, why);
        GetRPCManager().SendRPC(OZF_Const.MOD, RPC_ROLE_RES, p, true, to);
    }
}
