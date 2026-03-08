# Relatório de Análise Técnica - Kaldmor Canary OT

**Data da Análise:** 2026-03-08  
**Versão do Projeto:** Canary Server (OpenTibiaBR)  
**Escopo:** Análise completa do código-fonte, scripts, assets e configurações

---

## 1. Executive Summary

Este relatório apresenta uma análise de segurança e qualidade de código detalhada do projeto Kaldmor Canary OT. Após uma auditoria criteriosa das implementações iniciais, foram identificadas vulnerabilidades reais, bem como oportunidades de melhoria arquitetural e de boas práticas.

### Resumo de Prioridades:

| # | Prioridade | Problema Identificado |
|---|------------|-----------------------|
| 1 | **HIGH** | Conexão MySQL sem SSL habilitado |
| 2 | **MEDIUM** | Risco de SQL Injection via interpolação de string não validada |
| 3 | **MEDIUM** | Vazamento potencial de credenciais em arquivos temporários |
| 4 | **LOW** | Otimização de Índices no DB, uso de `goto` e inicialização de PRNG legado |

---

## 2. Assumptions (Premissas)

- O servidor **MySQL** é executado em `localhost` ou em uma rede local confiável (trusted network).
- Os **scripts Lua** (`datapack`) são considerados código confiável, mantido apenas pelos administradores.
- Apenas **game clients** legítimos devem interagir com as portas públicas do servidor.
- O servidor principal executa em um **ambiente confiável e restrito**, protegido por firewall.

---

## 3. Threat Model (Modelo de Ameaças)

### Superfície de Ataque (Attack Surface):
- **Input de Jogador**: Comandos de chat, talkactions, e interações em tempo real.
- **Camada de Scripts (Lua)**: Manipulação de eventos, actions, e animalevents.
- **Consultas ao Banco de Dados (Database Queries)**: Instruções SQL geradas dinamicamente via Lua e C++.

### Camada de Rede (Network Layer):
- **Game Protocol**: Tráfego de pacotes dos jogadores (TCP).
- **Login Protocol**: Fluxo de autenticação e lista de personagens (TCP).
- **MySQL Connection**: Comunicação com o banco de dados relacional.

### Principais Categorias de Risco:
1. **SQL Injection**: Modificação maliciosa de queries através de input não sanitizado.
2. **Privilege Escalation**: Uso indevido de comandos de GM (Gamemaster) ou sequestro de contas (Account Takeover).
3. **Denial of Service (DoS) / Server Crash**: Exceções não tratadas, null pointers e vazamentos de memória (Memory Leaks).
4. **Item Duplication**: Race conditions e falhas transacionais envolvendo movimentação de itens.

---

## 4. High Severity Issues (Problemas de Alta Gravidade)

### 4.1 SSL MySQL Desabilitado

| Atributo | Valor |
|----------|-------|
| **Severity** | HIGH |
| **File** | `src/database/database.cpp` |
| **Lines** | 47-52 |

**Problema:** A conexão com o banco de dados é estabelecida em texto claro (plaintext) devido à ausência da flag `CLIENT_SSL`. O código atual apenas configura opções opcionais, mas não impõe o uso de criptografia durante a chamada a `mysql_real_connect()`.

**Código Problemático:**
```cpp
// ❌ PROBLEMA: SSL não é ativado na conexão final
bool ssl_enabled = false;
mysql_options(handle, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &ssl_enabled);

// flag 0 = sem SSL
if (!mysql_real_connect(handle, host->c_str(), ..., 0)) {
```

**Solução Proposta (Patch):**
```cpp
// ✅ CORREÇÃO: Habilitar verificação e forçar o uso da flag CLIENT_SSL
bool verify = true;
mysql_options(handle, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verify);

if (!mysql_real_connect(
        handle,
        host->c_str(),
        user->c_str(),
        password->c_str(),
        database->c_str(),
        port,
        sock->c_str(),
        CLIENT_SSL)) {
    g_logger().error("MySQL Error Message: {}", mysql_error(handle));
    return false;
}
```

---

## 5. Medium Severity Issues (Problemas de Média Gravidade)

### 5.1 SQL Injection - Variável `accountId` em Interpolação de String

| Atributo | Valor |
|----------|-------|
| **Severity** | MEDIUM (Unsafe numeric interpolation) |
| **File** | `data/scripts/talkactions/gm/ban.lua` |
| **Lines** | 42, 49 |

**Problema:** O valor de `accountId` é concatenado diretamente na query SQL. Embora venha de uma fonte relativamente confiável (`player:getAccountId()`), omitir a validação/conversão de tipos ("type casting") é uma prática insegura que pode gerar vulnerabilidades futuras se a origem da variável mudar.

**Código Problemático:**
```lua
-- ⚠️ PROBLEMA: accountId concatenado diretamente sem conversão numérica
db.query("UPDATE `account_bans` SET `reason` = " .. db.escapeString(reason or "") .. 
         " WHERE `account_id` = " .. accountId)
```

**Solução Proposta (Patch):**
```lua
-- ✅ CORREÇÃO: Garantir que accountId é um número. Usar "or 0" evita erros de sintaxe Lua caso tonumber retorne nil.
local safeAccountId = tonumber(accountId) or 0

db.query("UPDATE `account_bans` SET `reason` = " .. db.escapeString(reason or "") .. 
         " WHERE `account_id` = " .. safeAccountId)
```

---

### 5.2 Credenciais Armazenadas em Arquivo Temporário Físico

| Atributo | Valor |
|----------|-------|
| **Severity** | MEDIUM |
| **File** | `src/database/database.cpp` |
| **Lines** | 79-101 |

**Problema:** O método que realiza chamadas por linha de comando armazena configurações sensíveis em um arquivo `.cnf` temporário. Se a execução falhar antes da instrução `remove()`, o arquivo permanecerá no disco físico expondo as credenciais.

**Código Problemático:**
```cpp
// ⚠️ PROBLEMA: Arquivo pode não ser excluído se ocorrer um early-return ou exceção
std::string tempConfigFile = "database_backup.cnf";
std::ofstream configFile(tempConfigFile);
// ... gravação de credenciais ...
int result = std::system(command.c_str());
std::filesystem::remove(tempConfigFile);
```

**Solução Proposta (Patch):**
Utilizar o padrão RAII (Resource Acquisition Is Initialization) para garantir a exclusão do arquivo quando o escopo terminar.

```cpp
// ✅ CORREÇÃO: Encapsular a deleção no destrutor para garantir a limpeza em qualquer cenário
class ConfigFileGuard {
    std::string path;
public:
    explicit ConfigFileGuard(std::string p) : path(std::move(p)) {}
    ~ConfigFileGuard() { 
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

std::string tempConfigFile = "database_backup.cnf";
ConfigFileGuard guard(tempConfigFile);
std::ofstream configFile(tempConfigFile);
// ... operações subsequentes ...
```

---

## 6. Low Severity Issues (Problemas de Baixa Gravidade)

### 6.1 Otimização de Índices de Banco de Dados

| Atributo | Valor |
|----------|-------|
| **Severity** | LOW (Performance / Scalability) |
| **File** | `schema.sql` |

**Sugestão:** A ausência de índices compostos em tabelas de alto acesso pode degradar a performance a longo prazo.
```sql
-- Adicionar índices para otimizar queries frequentes
CREATE INDEX idx_player_deaths_time ON player_deaths (player_id, time);
CREATE INDEX idx_guild_wars_status ON guild_wars (status, guild1, guild2);
```

---

### 6.2 Uso Excessivo de Declarações `goto`

| Atributo | Valor |
|----------|-------|
| **Severity** | LOW (Code Quality / Maintainability) |
| **File** | `src/database/database.cpp` |
| **Lines** | 323-331 |

**Problema:** O código utiliza labels e comandos `goto` para gerenciar tentativas de reconexão.
**Solução Proposta (Patch):** Substituir por loops convencionais para melhorar a legibilidade.

```cpp
int retries = 10;
while (retries > 0) {
    if (mysql_query(handle, query.data()) == 0) {
        break; // Sucesso
    }
    if (!isRecoverableError(mysql_errno(handle))) {
        return nullptr; // Erro fatal
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    retries--;
}
```

---

### 6.3 Inicialização de PRNG Previsível (`srand()`)

| Atributo | Valor |
|----------|-------|
| **Severity** | LOW (Cryptography / Randomness) |
| **File** | `src/canary_server.cpp` |
| **Lines** | 49 |

**Problema:** O uso de `srand(time(NULL))` é considerado criptograficamente fraco e obsoleto. O projeto já suporta modernas implementações (`std::random_device`).

**Solução Proposta (Patch):**
Remover a chamada legada:
```cpp
// ❌ Código a ser removido:
// srand(static_cast<unsigned int>(OTSYS_TIME()));
```

---

## 7. Falsos Positivos Avaliados

Estes itens foram investigados e confirmados como **SEGUROS**, não exigindo modificação imediata:

### 7.1 Uso de `db.escapeString()`
As seguintes rotinas utilizam `db.escapeString()` corretamente, mitigando eficazmente riscos de injeção em dados tipo string:
- ✅ `data/scripts/talkactions/gm/unban.lua`
- ✅ `data/scripts/talkactions/god/ip_ban.lua`
- ✅ `data/scripts/creaturescripts/player/death.lua`

### 7.2 Validação de Ponteiros
- ✅ `database.cpp:39`: Verificações e asserções aplicadas sobre `std::string` estão corretas e impedem falhas de segmentação (Segmentation Fault).

---

## 8. Recomendações de Security Hardening

Para solidificar a arquitetura de segurança, recomenda-se:

1. **Habilitar SSL Integrado**
   - Impor o uso de conexão criptografada (SSL/TLS) via configuração global, reforçando defesas mesmo em arquiteturas conteinerizadas (Docker).

2. **Princípio do Menor Privilégio no DB (Least Privilege)**
   - Omitir o uso do utilizador `root`. Criar perfis dedicados que possuam apenas privilégios transacionais (`SELECT`, `INSERT`, `UPDATE`, `DELETE`, `EXECUTE`).

3. **Isolamento de Credenciais de Backup**
   - Definir uma conta autônoma no banco de dados focada apenas na criação de dumps (`LOCK TABLES`, `SELECT`).

4. **Automação CI com Static Analysis Integrado**
   - Incluir ferramentas automatizadas como o CodeQL ou cppcheck no fluxo de integração (GitHub Actions), englobando a vasta biblioteca de scripts Lua.

5. **Auditoria de Consultas Falhas (Runtime Logging)**
   - Registar quaisquer tentativas de SQL com sintaxe inválida nos relatórios operacionais. Pode atuar como um Indicador de Comprometimento (IoC) precoce.

---

## 9. Trabalho de Avaliação Futuro

As sub-rotinas abaixo não foram enquadradas pelo escopo da auditoria atual e merecem futuras investigações:

- **Sistema de Trade (Trade System):** Verificação de duplicação em simultaneidade.
- **Logística de Containers:** Inserção cíclica, drop limits.
- **Protocolo de Rede:** Sanitarização total de RPCs oriundas de bad clients.
- **Heurísticas Anti-Cheat:** Resistência mecânica a macros e manipulação de coordonadas matemáticas.

---

## 10. Roadmap de Correções Sugerido

| Fase | Prioridade | Item / Objetivo | Previsão de Esforço |
|------|------------|-----------------|---------------------|
| 1 | **HIGH** | Forçar SSL em `mysql_real_connect` | Curto (1 Dia) |
| 2 | **MEDIUM** | Tratamento robusto e RAII para `tempConfigFile` | Curto (1 Dia) |
| 3 | **MEDIUM** | Revisar `tonumber()` nos módulos Lua de Banimento | Curto (1 Dia) |
| 4 | **LOW** | Refatoramentos gerais (goto, srand, db indexes) | Médio (2 Dias) |

---
**Fim do Relatório**
