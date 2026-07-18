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
- `LOG_CAPTURE_STD_STREAMS`
- `LOG_CONSOLE_COMPACT`
- `LOG_CONSOLE_SHOW_CATEGORY`
- `LOG_FILE_INCLUDE_TIMESTAMP`
- `LOG_FILE_INCLUDE_SOURCE`
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
- `LOG_CAPTURE_STD_STREAMS`
- `LOG_CONSOLE_COMPACT`
- `LOG_CONSOLE_SHOW_CATEGORY`
- `LOG_FILE_INCLUDE_TIMESTAMP`
- `LOG_FILE_INCLUDE_SOURCE`
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
- `src/main.cpp`
- `src/configmanager.cpp`
- `docs/LOGGER_ETAPAS.md`

Escopo implementado:
- primeiros logs estruturados adicionados no bootstrap principal
- expansao do logger para a entrada do processo em `main.cpp`
- expansao do logger para carga e avisos em `configmanager.cpp`
- migracao inicial focada em pontos de alto valor para debug
- mantido o `std::cout` atual para nao alterar o fluxo visual do console nesta fase

Pontos migrados:
- inicio e encerramento do processo
- processamento de argumentos principais
- carregamento de config
- erro ao carregar `config.lua`
- avisos de acesso invalido no `ConfigManager`
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
- o logger agora cobre desde a entrada do processo ate a carga de configuracao
- os logs ficam mais uteis para identificar em que fase o bootstrap falhou

## Etapa 6 - Segunda migracao de uso

Status: concluida

Arquivos alterados:
- `.env`
- `src/logger.hpp`
- `src/logger.cpp`
- `src/http/http.cpp`
- `src/http/listener.cpp`
- `src/items.cpp`
- `docs/LOGGER_ETAPAS.md`

Escopo implementado:
- captura automatica de `std::cout`, `std::cerr` e `std::clog`
- redirecionamento dos logs legados da source inteira para o logger
- preservacao da saida no console sem recursao interna
- flush e restauracao segura dos streams no shutdown
- migracao dos pontos fora de stream padrao que ainda usavam `fmt::print`

Cobertura desta etapa:
- praticamente todos os arquivos que ainda escreviam em console via streams padrao passam a entrar no logger
- `http` passou a usar logger diretamente
- `items.cpp` passou a usar logger diretamente no aviso de direcao invalida

Nova chave de ambiente:
- `LOG_CAPTURE_STD_STREAMS`

Refino aplicado depois da etapa:
- console agora pode ficar compacto e mais limpo
- arquivo agora pode manter timestamp e origem do log
- a separacao entre `LOG_CONSOLE_LEVEL` e `LOG_FILE_LEVEL` continua ativa, mas com formatos diferentes
- a source passou a usar streams do proprio logger (`LOG_STDOUT`, `LOG_STDERR`, `LOG_STDLOG`) no lugar de `std::cout`, `std::cerr` e `std::clog`
- `LOG_CAPTURE_STD_STREAMS` pode permanecer desligado quando toda a source estiver migrada

Resultado desta etapa:
- a source inteira passa a gerar rastreio centralizado sem precisar migracao manual arquivo por arquivo
- os logs antigos continuam aparecendo no console e agora tambem entram no sistema de arquivos do logger

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
