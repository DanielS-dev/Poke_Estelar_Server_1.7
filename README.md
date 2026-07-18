### Poke Estelar Server
Servidor Poketibia/OTServer em C++ baseado em The Forgotten Server (TFS) 1.7, usando o projeto Poke Fans como base inicial e cliente/protocolo 13.10.

Este repositorio e usado para estudo e evolucao de uma source de servidor: organizacao de codigo C++, refatoracao por responsabilidades, integracao com Lua/XML, banco de dados e build no Windows com Visual Studio e vcpkg.

## Sobre o Projeto
O Poke Estelar Server e uma source de servidor Poketibia derivada de uma base TFS 1.7, com suporte ao cliente/protocolo 13.10. O objetivo principal e estudar, manter e modernizar a estrutura do servidor sem misturar dependencias externas com o codigo-fonte.

## Principais areas de estudo:

Estrutura de um servidor TFS/Poketibia.
Manutencao e modernizacao de codigo C++.
Integracao com scripts Lua e arquivos XML.
Persistencia com banco de dados MySQL/MariaDB.
Organizacao da source por dominios e responsabilidades.
Gerenciamento de dependencias com vcpkg.

## Roadmap de Migracao para Poketibia

1. Sistema base de Pokemon
Captura, summon/recall, troca de pokebola, box/CP, persistencia e validacao de tiles.

2. Sistema de moves e combate
Moves por especie, cooldowns, barra de skills, tipagem, dano, status e passivas.

3. Sistema de evolucao e forms
Evolucao por level/item/condicao, mega evolucao, regionais, shiny e transformacoes.

4. Sistema de progressao
Level do Pokemon, experiencia, boost, happiness/love, addons e held items.

5. Sistema de mobilidade Pokemon
Ride, fly, surf, dive e teleportes/regras de movimentacao por mapa.

6. Sistema de clãs
Escolha de clan, progressao por clan, bonus, quests e restricoes especificas.

7. Sistema de utilidades Pokemon
TM/HM, move relearn, bless/revive, healer, stones, vitaminas e itens especiais.

8. Sistema de interface
Janela/status do Pokemon, HP, nome, level, effects, icons e opcodes do cliente.

9. Sistema de NPCs e mundo
NPCs de captura, trade, evolucao, clan, box e quests especificas de Poketibia.

10. Ajustes de source TFS 1.7
Hooks, bindings Lua, protocolo e persistencia extra para suportar todos os sistemas acima.
