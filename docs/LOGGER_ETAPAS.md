# Etapas de Implantacao do Logger

## Etapa 0 - Base do .env

Status: concluida

Arquivos criados:
- `.env`
- `docs/LOGGER_ETAPAS.md`

Chaves definidas no `.env`:
- `LOG_LEVEL`
- `LOG_CONSOLE_LEVEL`
- `LOG_FILE_LEVEL`
- `LOG_TO_CONSOLE`
- `LOG_TO_FILE`
- `LOG_SPLIT_BY_LEVEL`
- `LOG_DIR`
- `LOG_FILE`
- `LOG_MAX_FILE_SIZE_MB`
- `LOG_MAX_FILES`

Escopo atual:
- uso exclusivo para configuracao do logger
- sem integrar banco, portas, IP ou outras configuracoes do servidor nesta fase

## Etapa 1 - Nucleo do logger

Status: concluida

Arquivos criados:
- `src/logger.hpp`
- `src/logger.cpp`

Escopo implementado:
- `LogLevel`
- `Logger::Config`
- singleton `Logger`
- fila interna de mensagens
- thread dedicada de escrita
- escrita em console
- escrita em arquivo
- split por nivel
- rotacao por tamanho
- macros `LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, `LOG_FATAL`

Pendente para proximas etapas:
- integracao no build
- integracao no startup/shutdown
- migracao dos primeiros pontos da source

## Etapa 2 - Leitura do .env

Status: concluida

Arquivos alterados:
- `src/logger.hpp`
- `src/logger.cpp`

Escopo implementado:
- `Logger::initializeFromEnv(...)`
- leitura simples de arquivo `.env`
- parse das chaves de log
- fallback interno quando chave estiver ausente
- comportamento seguro quando `.env` nao existir

Chaves atendidas:
- `LOG_LEVEL`
- `LOG_CONSOLE_LEVEL`
- `LOG_FILE_LEVEL`
- `LOG_TO_CONSOLE`
- `LOG_TO_FILE`
- `LOG_SPLIT_BY_LEVEL`
- `LOG_DIR`
- `LOG_FILE`
- `LOG_MAX_FILE_SIZE_MB`
- `LOG_MAX_FILES`

Pendente para proximas etapas:
- integracao no startup/shutdown
- migracao dos primeiros pontos da source

## Etapa 4 - Startup e shutdown

Status: concluida

Arquivos alterados:
- `src/otserv.cpp`
- `docs/LOGGER_ETAPAS.md`

Escopo implementado:
- inicializacao do logger no inicio de `startServer()`
- leitura do `.env` no bootstrap da source
- log de erro centralizado em `startupErrorMessage(...)`
- log do servidor online/offline
- log do fluxo de encerramento
- shutdown explicito do logger ao final do ciclo do servidor

Resultado desta etapa:
- o logger passa a existir desde o bootstrap do servidor
- falhas de startup agora entram no logger
- o encerramento fecha a thread do logger de forma controlada

## Etapa 5 - Primeira migracao de uso

Status: concluida

Arquivos alterados:
- `src/otserv.cpp`
- `docs/LOGGER_ETAPAS.md`

Escopo implementado:
- primeiros logs estruturados adicionados no bootstrap principal
- migracao inicial focada em pontos de alto valor para debug
- mantido o `std::cout` atual para nao alterar o fluxo visual do console nesta fase

Pontos migrados:
- carregamento de config
- carregamento da chave RSA
- conexao e preparacao inicial do banco
- carregamento de vocations
- carregamento de items OTB/XML
- carregamento dos sistemas e scripts Lua
- carregamento de monsters e outfits
- definicao do world type
- carregamento do mapa
- inicializacao do gamestate
- manutencao inicial de houses/market
- transicao final para servidor online

Resultado desta etapa:
- o logger ja comeca a gerar rastreio real da subida do servidor
- os logs ficam mais uteis para identificar em que fase o bootstrap falhou

## Etapa 3 - Build

Status: concluida

Arquivos alterados:
- `src/CMakeLists.txt`
- `vc17/theforgottenserver.vcxproj`
- `vc17/theforgottenserver.vcxproj.filters`

Escopo implementado:
- `logger.cpp` adicionado na lista de sources do build
- `logger.hpp` adicionado na lista de headers do build
- projeto `vc17` atualizado para reconhecer os novos arquivos
- filtros do Visual Studio atualizados para exibir os arquivos no grupo correto

Pendente para proximas etapas:
- integracao no startup/shutdown
- migracao dos primeiros pontos da source
