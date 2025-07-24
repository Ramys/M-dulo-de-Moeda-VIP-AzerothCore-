#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "AccountMgr.h"

// Constantes
#define VIP_COIN_PER_DAY 1 // Cada moeda VIP equivale a 1 dia (em segundos: 86400)

class VIPCurrencySystem : public PlayerScript
{
public:
    VIPCurrencySystem() : PlayerScript("VIPCurrencySystem") {}

    // Chamado ao jogador fazer login
    void OnLogin(Player* player) override
    {
        UpdateVIPStatus(player);
    }

    // Função para atualizar o status VIP do jogador
    void UpdateVIPStatus(Player* player)
    {
        uint32 accountId = player->GetSession()->GetAccountId();
        uint32 currentTime = time(nullptr);

        QueryResult result = CharacterDatabase.PQuery("SELECT vip_time FROM account_vip WHERE account_id = %u", accountId);

        if (!result)
        {
            // Se não houver registro, o jogador não tem VIP
            return;
        }

        Field* fields = result->Fetch();
        uint32 vipTime = fields[0].GetUInt32();

        if (vipTime > currentTime)
        {
            // O jogador ainda tem tempo VIP
            ChatHandler(player->GetSession()).SendSysMessage("Você possui VIP ativo até: " + std::to_string(vipTime));
        }
        else
        {
            // Remover privilégios VIP
            ChatHandler(player->GetSession()).SendSysMessage("Seu tempo VIP expirou.");
            CharacterDatabase.PExecute("DELETE FROM account_vip WHERE account_id = %u", accountId);
        }
    }

    // Comando para adicionar moedas VIP
    static bool HandleAddVIPCoins(ChatHandler* handler, const char* args)
    {
        if (!*args)
        {
            handler->SendSysMessage("Uso: .addvipcoins <quantidade>");
            return false;
        }

        uint32 coins = atoi(args);
        if (coins <= 0)
        {
            handler->SendSysMessage("A quantidade de moedas deve ser maior que zero.");
            return false;
        }

        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendSysMessage("Nenhum jogador selecionado.");
            return false;
        }

        AddVIPTime(target, coins * VIP_COIN_PER_DAY * 86400); // Converter dias em segundos
        handler->PSendSysMessage("Adicionadas %u moedas VIP ao jogador %s.", coins, target->GetName().c_str());
        return true;
    }

    // Comando para transferir moedas VIP
    static bool HandleTransferVIPCoins(ChatHandler* handler, const char* args)
    {
        if (!*args)
        {
            handler->SendSysMessage("Uso: .transfervipcoins <nome_do_jogador> <quantidade>");
            return false;
        }

        char* playerNameStr = strtok((char*)args, " ");
        char* coinsStr = strtok(nullptr, " ");

        if (!playerNameStr || !coinsStr)
        {
            handler->SendSysMessage("Uso: .transfervipcoins <nome_do_jogador> <quantidade>");
            return false;
        }

        std::string playerName = playerNameStr;
        uint32 coins = atoi(coinsStr);

        if (coins <= 0)
        {
            handler->SendSysMessage("A quantidade de moedas deve ser maior que zero.");
            return false;
        }

        Player* source = handler->GetSession()->GetPlayer();
        Player* target = ObjectAccessor::FindPlayerByName(playerName);

        if (!target)
        {
            handler->PSendSysMessage("Jogador %s não encontrado.", playerName.c_str());
            return false;
        }

        uint32 sourceAccountId = source->GetSession()->GetAccountId();
        uint32 targetAccountId = target->GetSession()->GetAccountId();

        QueryResult result = CharacterDatabase.PQuery("SELECT vip_time FROM account_vip WHERE account_id = %u", sourceAccountId);
        if (!result)
        {
            handler->SendSysMessage("Você não possui moedas VIP suficientes.");
            return false;
        }

        Field* fields = result->Fetch();
        uint32 sourceVipTime = fields[0].GetUInt32();
        uint32 currentTime = time(nullptr);

        uint32 availableTime = sourceVipTime > currentTime ? sourceVipTime - currentTime : 0;

        if (availableTime < coins * VIP_COIN_PER_DAY * 86400)
        {
            handler->SendSysMessage("Você não possui moedas VIP suficientes.");
            return false;
        }

        // Remover tempo do remetente
        uint32 newSourceTime = sourceVipTime - coins * VIP_COIN_PER_DAY * 86400;
        CharacterDatabase.PExecute("UPDATE account_vip SET vip_time = %u WHERE account_id = %u", newSourceTime, sourceAccountId);

        // Adicionar tempo ao destinatário
        AddVIPTime(target, coins * VIP_COIN_PER_DAY * 86400);

        handler->PSendSysMessage("Transferidas %u moedas VIP para %s.", coins, target->GetName().c_str());
        return true;
    }

private:
    // Função para adicionar tempo VIP
    static void AddVIPTime(Player* player, uint32 seconds)
    {
        uint32 accountId = player->GetSession()->GetAccountId();
        uint32 currentTime = time(nullptr);

        QueryResult result = CharacterDatabase.PQuery("SELECT vip_time FROM account_vip WHERE account_id = %u", accountId);

        if (result)
        {
            Field* fields = result->Fetch();
            uint32 vipTime = fields[0].GetUInt32();
            uint32 newVipTime = vipTime > currentTime ? vipTime + seconds : currentTime + seconds;

            CharacterDatabase.PExecute("UPDATE account_vip SET vip_time = %u WHERE account_id = %u", newVipTime, accountId);
        }
        else
        {
            uint32 newVipTime = currentTime + seconds;
            CharacterDatabase.PExecute("INSERT INTO account_vip (account_id, vip_time) VALUES (%u, %u)", accountId, newVipTime);
        }
    }
};

// Registro dos comandos
void AddVIPCurrencyCommands()
{
    new VIPCurrencySystem();

    // Registrar os comandos
    new ChatCommandBuilder("addvipcoins", SEC_ADMINISTRATOR, true, &VIPCurrencySystem::HandleAddVIPCoins, "");
    new ChatCommandBuilder("transfervipcoins", SEC_PLAYER, true, &VIPCurrencySystem::HandleTransferVIPCoins, "");
}
```

---

### 2. Configuração no Banco de Dados

Crie uma nova tabela no banco de dados `auth` para armazenar o tempo VIP:

```sql
CREATE TABLE `account_vip` (
    `account_id` INT(10) UNSIGNED NOT NULL,
    `vip_time` INT(10) UNSIGNED NOT NULL,
    PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
```

- `account_id`: ID da conta do jogador.
- `vip_time`: Tempo VIP em timestamp UNIX.

---

### 3. Como Funciona

1. **Comando `.addvipcoins`**: Permite que um administrador adicione moedas VIP a um jogador. Cada moeda adiciona 1 dia (86400 segundos) ao tempo VIP.
   ```
   Uso: .addvipcoins <quantidade>
   ```

2. **Comando `.transfervipcoins`**: Permite que um jogador transfira suas moedas VIP para outro jogador.
   ```
   Uso: .transfervipcoins <nome_do_jogador> <quantidade>
   ```

3. **Tempo VIP Acumulável**: O tempo VIP é acumulado automaticamente quando novas moedas são adicionadas.

4. **Notificações**: Jogadores recebem notificações sobre o status de seu tempo VIP ao fazer login.

---

### 4. Personalizações Possíveis

- **Comandos VIP**: Restringir comandos como `.tele`, `.mail` e `.chat` apenas para jogadores com tempo VIP ativo.
- **Interface Gráfica**: Criar um sistema visual no jogo para exibir o tempo VIP restante.
- **Eventos Especiais**: Distribuir moedas VIP gratuitamente durante eventos sazonais.

---

