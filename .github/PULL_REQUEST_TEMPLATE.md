# Description

Please include a summary of the change and which issue is fixed. Please also include relevant motivation and context. List any dependencies that are required for this change.

## Behaviour
### **Actual**

Descreva o comportamento ATUAL (antes da mudança). Exemplo: "O servidor não carregava scripts na ordem correta"

### **Expected**

Descreva o comportamento ESPERADO (depois da mudança). Exemplo: "Scripts carregam na ordem: core.lua → libs → scripts"

### Fixes #issuenumber

## Type of change

Please delete options that are not relevant.

  - [ ] Bug fix (non-breaking change which fixes an issue)
  - [ ] New feature (non-breaking change which adds functionality)
  - [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
  - [ ] This change requires a documentation update

## How Has This Been Tested

Please describe the tests that you ran to verify your changes. Provide instructions so we can reproduce. Please also list any relevant details for your test configuration

  - [ ] Test A
  - [ ] Test B

**Test Configuration**:

  - Server Version:
  - Client:
  - Operating System:

## Checklist

  - [ ] My code follows the style guidelines of this project
  - [ ] I have performed a self-review of my own code
  - [ ] I checked the PR checks reports
  - [ ] I have commented my code, particularly in hard-to-understand areas
  - [ ] I have made corresponding changes to the documentation
  - [ ] My changes generate no new warnings
  - [ ] I have added tests that prove my fix is effective or that my feature works

---

<!-- 
INSTRUÇÕES PARA LLM AO PREENCHER ESTE TEMPLATE:

1. Description: Resuma a mudança em 1-2 frases (não use placeholders)
2. Behaviour:
   - Actual: Descreva o comportamento ANTES da mudança (não deixe como "Do this...")
   - Expected: Descreva o comportamento DEPOIS da mudança
3. Type of change: Selecione apenas 1 opção
4. Checklist: Marque todas que se aplicam

Exemplo de como preencher:
- Description: "Corrige ordem de carregamento de scripts na inicialização do servidor"
- Actual: "Scripts carregavam sem seguir ordem específica, causando erros de dependência"
- Expected: "Scripts agora carregam na ordem: core.lua primeiro, depois libs, depois scripts"
-->
