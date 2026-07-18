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
