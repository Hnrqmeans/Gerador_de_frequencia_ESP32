# Configuracao remota do gerador PWM

## Instrucao para IA e manutencao deste documento

Ao trabalhar neste projeto, a IA deve tratar este arquivo como a especificacao viva da configuracao remota.

Regras obrigatorias:

1. Antes de sugerir ou implementar uma alteracao, conferir o codigo atual, os nomes reais das funcoes, os comandos implementados e a estrutura das pastas.
2. Manter este documento atualizado conforme o projeto realmente esta, sem registrar como implementado algo que ainda e apenas planejado.
3. Depois de cada implementacao, atualizar as tabelas, exemplos, limites, comandos e o status das funcionalidades afetadas.
4. Diferenciar claramente as secoes `Implementado`, `Planejado` e `Futuro`.
5. Preservar este modelo organizado por comandos, modos, parametros, persistencia e interfaces.
6. Melhorar a clareza do documento sempre que houver uma melhoria segura, sem remover informacao tecnica relevante.
7. Registrar nomes de comandos, unidades, limites e valores persistidos com a mesma nomenclatura usada no firmware.
8. Validar o firmware depois das alteracoes e registrar qualquer limitacao ou comportamento ainda nao implementado.

Esta regra permanece valida para futuras interfaces serial, aplicativo Windows, Wi-Fi e botoes fisicos.

Este documento define o que pode ser alterado no gerador PWM por comando serial, por um futuro aplicativo para computador ou por botoes fisicos.

A comunicacao atual usa a porta serial USB do ESP32. No futuro, o mesmo modelo de comandos pode ser reutilizado por Wi-Fi.

## Comandos implementados

O protocolo atual possui estes comandos:

| Funcao | Comando | Faixa ou opcoes | Exemplo |
|---|---|---:|---|
| Frequencia de todos os canais | `F<frequencia>` | `0.5` a `150000 Hz` | `F1000` |
| Frequencia de um canal | `F<canal>:<frequencia>` | Canal `0` a `15` | `F3:1500` |
| Duty cycle de todos os canais | `D<percentual>` | `0` a `100%` | `D50` |
| Duty cycle de um canal | `D<canal>:<percentual>` | Canal `0` a `15` | `D3:25` |
| Selecionar modo configuravel | `M1` | Modo `Configurable` | `M1` |
| Selecionar perfil de teste | `M2` | Modo `TestProfile` | `M2` |
| Configurar perfil de teste | `P<media>:<sigma>:<intervalo>` | Media valida, sigma >= `0`, intervalo >= `50 ms` | `P12:3:500` |
| Consultar estado | `S` | Nao se aplica | `S` |
| Salvar configuracoes | `SAVE` | Linha exata | `SAVE` |

### Regra de aplicacao

- Sem indice de canal, o valor e aplicado a todos os canais.
- Com indice de canal, o valor e aplicado somente ao canal informado.
- O canal valido vai de `0` a `15`.
- A frequencia valida vai de `0.5` a `150000 Hz`.
- O duty cycle valido vai de `0` a `100%`.
- Depois de alterar frequencia, duty, modo ou perfil, os valores ficam somente em RAM e sao marcados como pendentes.
- Somente a linha exata `SAVE` grava os valores na NVS do ESP32, usada pelo projeto como EEPROM simulada.
- O comando `S` apenas informa o modo, o perfil, os valores e se existem alteracoes pendentes.
- Enviar `S`, um comando invalido ou repetir `SAVE` sem alteracoes nao grava nada.
- Se o ESP32 reiniciar antes de `SAVE`, as alteracoes pendentes sao perdidas e a ultima configuracao salva sera carregada.

### Limites definidos no firmware

Os limites abaixo estao em `include/config/config.h`:

- Frequencia minima: `0.5 Hz`.
- Frequencia maxima: `150000 Hz`.
- Duty padrao: `5%`.
- Frequencia padrao: `1 Hz`.
- Quantidade de canais: `16`.
- Indices dos canais: `0` a `15`.

## Modo de frequencia

O firmware possui dois modos conceituais:

```cpp
enum class FrequencyMode : uint8_t
{
  Configurable = 1,
  TestProfile = 2
};
```

### `Configurable`

- A frequencia definida pelo usuario e aplicada aos canais.
- Os comandos `F...` alteram a frequencia configurada.
- A frequencia e persistida na EEPROM simulada.
- E o modo adequado para controle por aplicativo ou botoes.

### `TestProfile`

- A frequencia e gerada automaticamente para cada canal.
- O perfil usa uma distribuicao normal.
- Media atual: `12 Hz`.
- Sigma atual: `3 Hz`.
- Atualizacao atual: a cada `500 ms`.
- Esses valores estao definidos em `include/config/config.h`.

### Comandos de modo

O modo deve ser selecionado por dois comandos simples:

| Comando | Modo | Nome interno | Comportamento |
|---|---|---|---|
| `M1` | Configuravel | `Configurable` | Usa a frequencia definida pelo usuario |
| `M2` | Perfil de teste | `TestProfile` | Gera frequencias automaticamente |

Exemplos:

```text
M1
M2
```

Ao receber `M1` ou `M2`, o firmware:

1. Validar o comando.
2. Alterar o modo em memoria.
3. Aplicar imediatamente o novo comportamento.
4. Salvar o modo na EEPROM simulada.
5. Confirmar a alteracao na resposta serial.

### O modo pode ser salvo na EEPROM?

Sim. O modo e armazenado como um valor pequeno:

```cpp
enum class FrequencyMode : uint8_t
{
  Configurable = 1,
  TestProfile = 2
};
```

Na EEPROM simulada, deve ser persistido o valor correspondente a `1` ou `2`. No proximo boot, o ESP32 carrega esse valor e inicia no ultimo modo selecionado.

O modo nao depende mais de uma constante de compilacao como:

```cpp
constexpr FrequencyMode configuredFrequencyMode = FrequencyMode::Configurable;
```

Ele agora e um estado de execucao em `PwmGenerator`, carregado da EEPROM simulada. Assim, `M1` e `M2` alteram o comportamento sem recompilar ou reenviar o firmware.

## Parametros por modo

Os comandos `F` e `D` continuam sendo os comandos principais. O significado da frequencia aplicada depende do modo atual.

### Modo 1: `Configurable`

No modo `Configurable`, o usuario controla diretamente:

| Parametro | Comando | Escopo |
|---|---|---|
| Frequencia | `F<frequencia>` ou `F<canal>:<frequencia>` | Todos ou um canal |
| Duty cycle | `D<percentual>` ou `D<canal>:<percentual>` | Todos ou um canal |

Exemplos:

```text
M1
F1000
D50
F3:1500
D3:25
S
```

O modo `Configurable` deve manter na EEPROM simulada:

- Frequencia configurada de cada canal.
- Duty cycle configurado de cada canal.
- Modo atual, com valor `1`.

### Modo 2: `TestProfile`

No modo `TestProfile`, a frequencia de cada canal e gerada automaticamente. O duty cycle continua sendo configuravel.

| Parametro | Pode ser alterado? | Observacao |
|---|---|---|
| Modo | Sim, usando `M1` ou `M2` | `M2` ativa o perfil |
| Frequencia manual de referencia | Sim, usando `F` | Fica salva para o proximo `M1`, mas nao substitui a frequencia gerada |
| Duty cycle | Sim, usando `D` | Pode ser aplicado a todos ou a um canal |
| Sigma | Sim, usando `P` | Controla a variacao da frequencia |
| Media | Sim, usando `P` | Define o centro da distribuicao |
| Intervalo de atualizacao | Sim, usando `P` | Define quando gerar novos valores |

O comportamento esperado no modo `TestProfile` e:

- `F...` salva a frequencia manual, mas nao altera diretamente a frequencia gerada.
- `D...` continua alterando o duty cycle.
- `S` mostra a frequencia atual gerada e os parametros do perfil.
- `M1` retorna ao ultimo conjunto de frequencias manuais salvo.
- `M2` inicia ou continua a geracao automatica.

O modo `TestProfile` deve manter na EEPROM simulada:

- Media do perfil.
- Sigma do perfil.
- Intervalo de atualizacao.
- Duty cycle de cada canal.
- Frequencias manuais usadas quando voltar para `Configurable`.
- Modo atual, com valor `2`.

## Configuracoes do perfil de teste

Estas configuracoes sao relevantes para aplicativo, Wi-Fi ou botoes futuros:

| Configuracao | Nome sugerido | Unidade | Funcao |
|---|---|---:|---|
| Media do perfil | `testProfileMeanFrequencyHz` | Hz | Define o centro da distribuicao |
| Variacao do perfil | `testProfileSigmaHz` | Hz | Define quanto a frequencia pode variar |
| Intervalo de atualizacao | `testProfileUpdateIntervalMs` | ms | Define a velocidade de renovacao do perfil |
| Frequencia minima permitida | `minFrequencyHz` | Hz | Impede valores invalidos |
| Frequencia maxima permitida | `maxFrequencyHz` | Hz | Impede valores invalidos |

Esses parametros ja podem ser configurados por `P<media>:<sigma>:<intervalo>`. O aplicativo e os botoes fisicos podem usar a mesma operacao futuramente.

Regras recomendadas:

- `meanFrequencyHz` deve respeitar os limites de frequencia.
- `sigmaHz` deve ser maior ou igual a zero.
- `updateIntervalMs` deve ter um limite minimo para evitar atualizacoes excessivas.
- O perfil deve impedir divisao por zero quando uma frequencia gerada for zero.
- Ao sair de `TestProfile`, deve ser aplicada a ultima configuracao manual salva.

## Configuracao por canal

O aplicativo deve permitir selecionar:

- Um canal especifico.
- Todos os canais.
- Um grupo de canais, futuramente.
- O mesmo valor para todos.
- Valores independentes por canal.

Modelo recomendado para o aplicativo:

```json
{
  "target": "all",
  "frequencyHz": 1000.0,
  "dutyPercent": 50
}
```

Para um canal especifico:

```json
{
  "target": 3,
  "frequencyHz": 1500.0,
  "dutyPercent": 25
}
```

Equivalentes seriais:

```text
F1000
D50
F3:1500
D3:25
```

## Estado que o aplicativo deve consultar

O comando de status futuro deve retornar:

- Modo atual: `Configurable` ou `TestProfile`.
- Frequencia configurada de cada canal.
- Frequencia realmente aplicada de cada canal.
- Duty cycle de cada canal.
- Canal selecionado, quando houver uma interface de controle local.
- Media, sigma e intervalo do perfil de teste.
- Estado da EEPROM simulada.
- Estado da conexao Wi-Fi, quando implementada.
- Versao do firmware.

Exemplo de resposta planejada:

```json
{
  "mode": "configurable",
  "channels": [
    {
      "index": 0,
      "frequencyHz": 1000.0,
      "actualFrequencyHz": 1000.0,
      "dutyPercent": 50
    }
  ],
  "profile": {
    "meanFrequencyHz": 12.0,
    "sigmaHz": 3.0,
    "updateIntervalMs": 500
  }
}
```

## Botoes fisicos futuros

Uma interface fisica pode reutilizar as mesmas operacoes do aplicativo.

### Controles sugeridos

| Controle | Acao |
|---|---|
| Botao `MODE` | Alterna entre `Configurable` e `TestProfile` |
| Botao `CHANNEL` | Seleciona o canal atual |
| Botao `ALL` | Seleciona todos os canais |
| Botao `FREQUENCY` | Entra no ajuste de frequencia |
| Botao `DUTY` | Entra no ajuste de duty |
| Botao `PLUS` | Aumenta o valor selecionado |
| Botao `MINUS` | Diminui o valor selecionado |
| Botao `SAVE` | Salva a configuracao na EEPROM simulada |
| Botao `RESET` | Retorna aos valores padrao, com confirmacao |

### Comportamento recomendado

1. `CHANNEL` seleciona um canal de `0` a `15`.
2. `ALL` aplica a alteracao a todos os canais.
3. `PLUS` e `MINUS` alteram o valor conforme a escala selecionada.
4. Um toque curto altera pouco o valor.
5. Um toque longo altera rapidamente o valor.
6. `SAVE` confirma a persistencia.
7. Apos um periodo sem interacao, a interface retorna para a tela de status.

### Escalas de ajuste sugeridas

| Parametro | Toque curto | Toque longo |
|---|---:|---:|
| Frequencia abaixo de `10 Hz` | `0.1 Hz` | `1 Hz` |
| Frequencia entre `10 Hz` e `1000 Hz` | `1 Hz` | `10 Hz` |
| Frequencia acima de `1000 Hz` | `10 Hz` | `100 Hz` |
| Duty cycle | `1%` | `5%` |
| Sigma | `0.1 Hz` | `1 Hz` |
| Intervalo | `50 ms` | `100 ms` |

## Aplicativo para computador

O aplicativo portatil para Windows pode enviar os mesmos comandos pela USB serial.

Exemplos de uso planejados:

```text
pwm-control.exe --port COM5 --status
pwm-control.exe --port COM5 --command M1
pwm-control.exe --port COM5 --command M2
pwm-control.exe --port COM5 --all --frequency 1000
pwm-control.exe --port COM5 --all --duty 50
pwm-control.exe --port COM5 --channel 3 --frequency 1500
pwm-control.exe --port COM5 --channel 3 --duty 25
```

O aplicativo deve sempre:

- Mostrar a porta serial selecionada.
- Permitir selecionar um canal ou todos.
- Validar limites antes de enviar.
- Mostrar a resposta do ESP32.
- Diferenciar valor configurado de valor realmente aplicado.
- Indicar se a alteracao esta pendente ou foi salva.
- Enviar `SAVE` somente por uma acao explicita do usuario.
- Solicitar confirmacao antes de restaurar padroes.

## EEPROM simulada

A classe `SimulatedEeprom` usa `Preferences/NVS` do ESP32. Ela nao e uma EEPROM fisica dedicada.

Configuracoes persistidas atualmente:

- Modo de frequencia.
- Frequencia por canal.
- Duty cycle por canal.
- Media do perfil de teste.
- Sigma do perfil de teste.
- Intervalo de atualizacao do perfil.

Grupos de canais e preferencias da interface permanecem futuros.

Operacoes planejadas:

```text
SAVE
LOAD
RESET_DEFAULTS
```

`SAVE` e a unica operacao de gravacao disponivel no protocolo atual. `LOAD` e `RESET_DEFAULTS` permanecem planejados.

O comando `RESET_DEFAULTS` deve exigir confirmacao para evitar apagar uma configuracao acidentalmente.

## Regras de seguranca e validacao

- Rejeitar frequencias fora de `0.5` a `150000 Hz`.
- Rejeitar duty fora de `0` a `100%`.
- Rejeitar canal fora de `0` a `15`.
- Rejeitar sigma negativo.
- Rejeitar intervalo de atualizacao muito baixo.
- Nunca dividir por zero ao calcular temporizacao.
- Nao aplicar configuracao parcialmente invalida.
- Responder com erro legivel e manter a configuracao anterior.
- Salvar na EEPROM simulada somente depois de validar todos os valores.
- Se a EEPROM simulada falhar, informar o erro sem interromper o PWM atual.

## Ordem recomendada de implementacao

1. Modo de execucao, perfil e persistencia na EEPROM simulada: implementado.
2. Comandos `M1`, `M2`, `P`, `F`, `D`, `S` e `SAVE`: implementado.
3. Criar uma resposta de status estruturada para aplicativo: futuro.
4. Criar uma camada independente de transporte para serial e Wi-Fi: futuro.
5. Criar o aplicativo portatil para Windows: futuro.
6. Adicionar botoes fisicos usando as mesmas funcoes de controle: futuro.

## Principio de arquitetura

O aplicativo, o Wi-Fi, a serial e os botoes fisicos devem chamar as mesmas funcoes de configuracao. Nenhuma dessas interfaces deve alterar diretamente os calculos ou os pinos do PWM.

```text
Interface serial
Interface Wi-Fi
Interface de botoes
        |
        v
Controlador de configuracao
        |
        +--> PwmGenerator
        +--> SimulatedEeprom
        +--> PwmCalculations
```

Assim, adicionar uma nova interface nao exige duplicar as regras de validacao, persistencia ou controle dos canais.
