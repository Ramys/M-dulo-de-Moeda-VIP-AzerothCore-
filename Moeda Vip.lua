
-- Constantes
local VIP_COIN_PER_DAY = 86400 -- Cada moeda VIP equivale a 1 dia (em segundos)

-- Função para atualizar o status VIP do jogador
local function UpdateVIPStatus(event, player)
    local accountId = player:GetAccountId()
    local currentTime = os.time()

    -- Consulta o banco de dados para verificar o tempo VIP
    local query = CharDBQuery(string.format("SELECT vip_time FROM account_vip WHERE account_id = %d", accountId))

    if not query then
        -- Se não houver registro, o jogador não tem VIP
        return
    end

    local vipTime = query:GetUInt32(0)

    if vipTime > currentTime then
        -- O jogador ainda tem tempo VIP
        player:SendBroadcastMessage(string.format("Você possui VIP ativo até: %s.", os.date("%Y-%m-%d %H:%M:%S", vipTime)))
    else
        -- Remover privilégios VIP
        player:SendBroadcastMessage("Seu tempo VIP expirou.")
        CharDBExecute(string.format("DELETE FROM account_vip WHERE account_id = %d", accountId))
    end
end

-- Comando para adicionar moedas VIP
local function AddVIPCoins(event, player, _, args)
    local coins = tonumber(args)
    if not coins or coins <= 0 then
        player:SendBroadcastMessage("Uso: .addvipcoins <quantidade>")
        return false
    end

    local target = player:GetSelectedPlayer()
    if not target then
        player:SendBroadcastMessage("Nenhum jogador selecionado.")
        return false
    end

    AddVIPTime(target, coins * VIP_COIN_PER_DAY)
    player:SendBroadcastMessage(string.format("Adicionadas %d moedas VIP ao jogador %s.", coins, target:GetName()))
    return true
end

-- Comando para transferir moedas VIP
local function TransferVIPCoins(event, player, _, args)
    local playerName, coinsStr = strsplit(" ", args)
    local coins = tonumber(coinsStr)

    if not playerName or not coins or coins <= 0 then
        player:SendBroadcastMessage("Uso: .transfervipcoins <nome_do_jogador> <quantidade>")
        return false
    end

    local target = GetPlayerByName(playerName)
    if not target then
        player:SendBroadcastMessage(string.format("Jogador %s não encontrado.", playerName))
        return false
    end

    local sourceAccountId = player:GetAccountId()
    local targetAccountId = target:GetAccountId()

    -- Verificar se o jogador tem moedas suficientes
    local query = CharDBQuery(string.format("SELECT vip_time FROM account_vip WHERE account_id = %d", sourceAccountId))
    if not query then
        player:SendBroadcastMessage("Você não possui moedas VIP suficientes.")
        return false
    end

    local sourceVipTime = query:GetUInt32(0)
    local currentTime = os.time()

    local availableTime = sourceVipTime > currentTime and sourceVipTime - currentTime or 0

    if availableTime < coins * VIP_COIN_PER_DAY then
        player:SendBroadcastMessage("Você não possui moedas VIP suficientes.")
        return false
    end

    -- Remover tempo do remetente
    local newSourceTime = sourceVipTime - coins * VIP_COIN_PER_DAY
    CharDBExecute(string.format("UPDATE account_vip SET vip_time = %d WHERE account_id = %d", newSourceTime, sourceAccountId))

    -- Adicionar tempo ao destinatário
    AddVIPTime(target, coins * VIP_COIN_PER_DAY)

    player:SendBroadcastMessage(string.format("Transferidas %d moedas VIP para %s.", coins, target:GetName()))
    return true
end

-- Função para adicionar tempo VIP
local function AddVIPTime(player, seconds)
    local accountId = player:GetAccountId()
    local currentTime = os.time()

    local query = CharDBQuery(string.format("SELECT vip_time FROM account_vip WHERE account_id = %d", accountId))

    if query then
        local vipTime = query:GetUInt32(0)
        local newVipTime = vipTime > currentTime and vipTime + seconds or currentTime + seconds

        CharDBExecute(string.format("UPDATE account_vip SET vip_time = %d WHERE account_id = %d", newVipTime, accountId))
    else
        local newVipTime = currentTime + seconds
        CharDBExecute(string.format("INSERT INTO account_vip (account_id, vip_time) VALUES (%d, %d)", accountId, newVipTime))
    end
end

-- Registrar eventos e comandos
RegisterPlayerEvent(3, UpdateVIPStatus) -- Evento ao jogador fazer login
RegisterCommand("addvipcoins", 4, AddVIPCoins) -- Comando para adicionar moedas VIP
RegisterCommand("transfervipcoins", 3, TransferVIPCoins) -- Comando para transferir moedas VIP
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

- **Comandos VIP**: Restringir comandos como `.tele`, `.mail` e `.chat` apenas para jogadores com tempo VIP ativo. Você pode fazer isso verificando o tempo VIP antes de executar esses comandos.
- **Interface Gráfica**: Criar um sistema visual no jogo para exibir o tempo VIP restante usando mensagens ou interfaces personalizadas.
- **Eventos Especiais**: Distribuir moedas VIP gratuitamente durante eventos sazonais.

---

### 5. Como Implementar

1. **Coloque o Script no Diretório Eluna**:
   Salve o código acima em um arquivo `.lua` dentro do diretório `lua_scripts` do seu servidor AzerothCore.

2. **Reinicie o Servidor**:
   Reinicie o servidor para carregar o novo script.

3. **Teste os Comandos**:
   Conecte-se ao servidor e teste os comandos `.addvipcoins` e `.transfervipcoins`.

---

