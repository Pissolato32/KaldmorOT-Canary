# Plano: Separação Desenvolvimento vs Produção

## Visão Geral

Este documento define quais arquivos devem ser mantidos no repositório para cada ambiente.

---

## Arquivos Necessários para Produção

Após `git clone`, o servidor de produção precisa dos seguintes arquivos para rodar via Docker:

### Build e Compilação
```
src/                    # Código fonte C++ (Docker precisa compilar)
cmake/                  # Configurações CMake
CMakeLists.txt          # Build principal
CMakePresets.json       # Presets de build
vcpkg.json              # Dependências
recompile.sh            # Script de compilação
```

### Dados do Jogo
```
data/                   # Scripts Lua (core)
data-otservbr-global/  # Conteúdo do jogo (NPCs, monsters, quests)
data/XML/               # Configurações XML (outfits, vocations, etc.)
```

### Configuração
```
config.lua.dist         # Template de configuração
schema.sql              # Schema do banco de dados
```

### Docker
```
docker/                 # Dockerfiles e docker-compose
```

---

## Arquivos Ignoráveis (Desenvolvimento Only)

Estes arquivos **NÃO são necessários** para produção e podem ser ignorados:

| Pasta/Arquivo | Motivo |
|---------------|--------|
| `.github/` | CI/CD workflows (só usado em desenvolvimento) |
| `docs/` | Documentação |
| `plans/` | Planejamento |
| `tests/` | Testes unitários |
| `src/database/databasetasks.cpp` | Desenvolvimento apenas |
| `build*/` | Artefatos de build |
| `*.md` | Documentos (README, LICENSE, etc.) |
| `.git*` | Arquivos Git |
| `.clang-format` | Configurações de formatação |
| `.cmake-format` | Configurações CMake |
| `.luarc.json` | Configurações Lua |
| `Jenkinsfile` | CI Jenkins |
| `package.json` | NPM (não usado) |
| `GitVersion.yml` | Versionamento |

---

## Fluxo de Deploy Recomendado

### 1. Desenvolvimento (máquina local)
- Trabalhar com código fonte
- Testar mudanças
- Commitar no branch `fix/recompile-logic`

### 2. Merge para Main
- Fazer merge para `main`
- Push para GitHub

### 3. Produção (servidor Linux)
```bash
# Clonar apenas o necessário
git clone --depth 1 https://github.com/Usuario/KaldmorOT-Canary.git

# Entrar na pasta
cd KaldmorOT-Canary

# Criar config.lua a partir do template
cp config.lua.dist config.lua

# Build e start (Docker compila o código automaticamente)
cd docker
docker compose up --build -d
```

---

## .gitignore Sugerido

O .gitignore existente já cobre a maioria dos casos. Adicione estas seções:

```gitignore
# ============================================
# PRODUÇÃO - Arquivos necessários
# ============================================
# Estes arquivos são NECESSÁRIOS para produção:
# - src/, cmake/, CMakeLists.txt, vcpkg.json
# - data/, data-otservbr-global/, data/XML/
# - config.lua.dist, schema.sql
# - docker/

# ============================================
# DESENVOLVIMENTO ONLY - Ignorar em produção
# ============================================

# CI/CD (só desenvolvimento)
.github/
Jenkinsfile

# Documentação
docs/
plans/
*.md
!README.md  # Manter README

# Testes
tests/

# Build artifacts
build*/
cmake-build-*/
*.o
*.a
*.so
*.dll
canary
canary.old

# Configurações locais
.vscode/
.idea/
*.user
*.suo

# Dependências (vcpkg instala automaticamente)
vcpkg_installed/
```

---

## Resumo: O que fazer no Servidor de Produção

```bash
# 1. Clonar (git ignora tudo que não precisa)
git clone https://github.com/SeuUsuario/KaldmorOT-Canary.git

# 2. Configurar
cd KaldmorOT-Canary
cp config.lua.dist config.lua
# Editar config.lua com as configurações do servidor

# 3. Iniciar
cd docker
docker compose up --build -d
```

**Pronto!** O Docker vai:
1. Baixar dependências (vcpkg)
2. Compilar o código C++
3. Criar container com binário + dados
4. Subir o servidor