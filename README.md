<p align="center">
  <a href="https://www.qlcplus.org/">
    <img src="resources/icons/png/qlcplus.png" alt="QLC+ Logo" height="60" />
  </a>
</p>

<h1 align="center">Q Light Controller+</h1>
<p align="center"><em>Fork LEGACY (QLC+ 4 · QtWidgets)</em></p>
<p align="center">
  <strong>Controle de iluminação open-source para DMX, Art-Net, sACN e muito mais.</strong><br/>
  Esta fork adiciona um motor de efeitos RGB em LuaJIT e melhorias de fluxo ao vivo.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-4.14.5%20GIT-blue" alt="Versão da fork" />
  <img src="https://img.shields.io/badge/based%20on-mcallegari%2Fqlcplus-lightgrey" alt="Baseado no QLC+ upstream" />
  <img src="https://img.shields.io/badge/license-Apache%202.0-green" alt="Licença Apache 2.0" />
  <img src="https://img.shields.io/badge/LuaJIT-enabled-00007C" alt="Motor RGB em LuaJIT" />
  <img src="https://img.shields.io/badge/Qt-6%20%7C%205-41CD52" alt="Qt 5 e 6" />
</p>

<p align="center">
  <a href="https://github.com/EricNakamura/qlcplus-LEGACY">
    <img src="https://img.shields.io/badge/github-EricNakamura%2Fqlcplus--LEGACY-181717?logo=github" alt="Repositório da fork" /></a>
</p>

---

## Sobre esta fork

Esta é uma **fork pessoal** do [Q Light Controller Plus](https://github.com/mcallegari/qlcplus),
mantida em [EricNakamura/qlcplus-LEGACY](https://github.com/EricNakamura/qlcplus-LEGACY).

Ela parte do branch **QLC+ 4** (interface QtWidgets, branch `master`) e adiciona
recursos próprios focados em **desempenho dos efeitos** e no **uso ao vivo**.
A versão atual é **`4.14.5 GIT`**.

> O QLC+ original continua sendo a base e a referência. Para documentação geral
> de uso, consulte <https://docs.qlcplus.org/>. Esta fork **não** tem vínculo
> oficial com o projeto upstream.

## Recursos exclusivos da fork

| Recurso | Descrição |
|---|---|
| **Motor RGB em LuaJIT** | Efeitos da RGB Matrix são scripts `.lua` executados em LuaJIT, no lugar do motor JavaScript original. Muito mais rápido para matrizes grandes. Veja o [guia](docs/RGB-SCRIPTS-LUA.md). |
| **Engine RGB a 50 Hz com fase contínua** | O RGB Matrix avança a fase do efeito a cada tick (50 Hz) com multiplicador de velocidade, permitindo animações suaves e velocidade configurável em tempo real. |
| **Slider de transição global** | Slider opcional no Virtual Console que aplica um tempo de transição global a funções que não estão em flash. |
| **Monitor sempre no topo** | A janela do Monitor DMX pode ser fixada acima das demais janelas. |
| **Grade do Virtual Console configurável** | Tamanho e visibilidade da grade do Virtual Console ajustáveis. |
| **Fade-out no RGBMatrix** | Capacidade de fade-out restaurada para o RGBMatrix. |
| **Correções de bugs conhecidos** | Diversas correções de estabilidade e de comportamento em relação ao upstream. |
| **Auto-configuração Easy ArtNet** | O plug-in Art-Net reconhece e configura automaticamente (unicast) os dispositivos [Easy ArtNet Interface](https://github.com/EricNakamura/easy-artnet-interface). |
| **Build macOS modernizado** | Presets do CMake + `build.sh`, usando Qt 6 (Homebrew), Ninja e ccache. |

## Documentação

- **[Guia dos scripts RGB em Lua (LuaJIT)](docs/RGB-SCRIPTS-LUA.md)** — como criar,
  registrar, entender a fase contínua e depurar seus efeitos.
- Documentação oficial do QLC+: <https://docs.qlcplus.org/>
- Wiki (compilação e plataformas): <https://github.com/mcallegari/qlcplus/wiki>
- Convenções para agentes/contribuidores: [AGENTS.md](AGENTS.md)

### Protocolos suportados

MIDI, OSC, HID, DMX USB, Art-Net, E1.31 (sACN), OS2L e outros. Consulte a
[documentação de plug-ins](https://docs.qlcplus.org/v4/plugins).

### Projetos relacionados

- **[Easy ArtNet Interface](https://github.com/EricNakamura/easy-artnet-interface)** —
  interface Art-Net que esta fork reconhece e configura automaticamente
  (conexão unicast).

## Compilando (macOS / Qt 6)

**Dependências** (via [Homebrew](https://brew.sh/)): Qt 6, **LuaJIT**
(dependência obrigatória nesta fork), Ninja, ccache e CMake ≥ 3.25.

```bash
brew install qt ninja ccache luajit
```

> O código também suporta **Qt 5**, mas o fluxo abaixo assume Qt 6 do Homebrew.

**Nunca compile in-source** — o CMake falha de propósito. Use o helper ou os
presets:

```bash
# Build de Debug (testes + editor de fixtures) em ./build
./build.sh debug

# Outras opções: fast (sem testes/editor), release, package
./build.sh fast
./build.sh release
./build.sh package     # gera o bundle macOS (rode o install depois)
```

Ou manualmente, com os presets do CMake:

```bash
cmake --workflow --preset macos-debug
ninja -C build run      # executa o QLC+
```

### Testes

Os testes precisam dos recursos copiados para o diretório de build; rode sempre
pelo wrapper:

```bash
./unittest.sh ui        # QLC+ 4 (a interface padrão desta fork)
ninja -C build check    # equivalente
```

## Estrutura do repositório

| Diretório | Finalidade |
|---|---|
| `engine/` | Biblioteca central `qlcplusengine` (sem UI): `Doc`, `Fixture`, `Function`/`Scene`/`Chaser`/`EFX`/`RGBMatrix`, `MasterTimer`, `InputOutputMap`, `Universe`. |
| `engine/audio/` | Subsistema de áudio e plug-ins de decodificação. |
| `ui/` | Interface QtWidget do QLC+ 4 (`libqlcplusui`): `App`, `VirtualConsole`/`VCWidget`, `SimpleDesk`, gerentes. |
| `main/` | Ponto de entrada do executável do QLC+ 4. |
| `qmlui/` | Interface QML do QLC+ 5 (somente com `-Dqmlui=ON`). |
| `fixtureeditor/` | Editor de definições de fixture autônomo. |
| `plugins/` | Plug-ins de E/S e `plugins/interfaces/` (o contrato `QLCIOPlugin`). |
| `webaccess/` | Servidor HTTP embutido e interface web. |
| `hotplugmonitor/` | Detecção de hotplug de dispositivos multiplataforma. |
| `resources/` | Fixtures, gobos, perfis de entrada, scripts RGB, ícones, esquemas. |
| `platforms/` | Empacotamento por plataforma (`linux/`, `macos/`, `windows/`, `android/`, `ios/`). |

## Contribuindo

- Leia o [CONTRIBUTING.md](CONTRIBUTING.md) antes de enviar mudanças — em
  especial, mudanças no **engine** e em `VCWidget` devem ser discutidas antes.
- Suporte e canais de ajuda: [SUPPORT.md](SUPPORT.md).
- Este é um fork pessoal: contribuições ao QLC+ original devem ir para
  <https://github.com/mcallegari/qlcplus>.

## Licença

Licenciado sob a **Apache License 2.0**. Veja [COPYING](COPYING) para os detalhes.

Os avisos de copyright originais são preservados nos cabeçalhos de cada arquivo
e nos créditos abaixo.

---

## Créditos

A fork mantém integralmente os créditos do projeto upstream.

<p align="center">
Copyright © Heikki Junnila, Massimo Callegari
</p>

<details>
<summary>QLC+ 5</summary>

*   Eric Arnebäck (3D preview features)
*   Santiago Benejam Torres (Catalan translation)
*   Luis García Tornel (Spanish translation)
*   Nils Van Zuijlen, Jérôme Lebleu (French translation)
*   Felix Edelmann, Florian Edelmann (fixture definitions, German translation)
*   Jannis Achstetter (German translation)
*   Dai Suetake (Japanese translation)
*   Hannes Bossuyt (Dutch translation)
*   Aleksandr Gusarov (Russian translation)
*   Vadim Syniuhin (Ukrainian translation)
*   Mateusz Kędzierski + smaks6 (Polish translation)

</details>

<details>
<summary>QLC+ 4</summary>

*   Jano Svitok (bugfix, new features and improvements)
*   David Garyga (bugfix, new features and improvements)
*   Lukas Jähn (bugfix, new features)
*   Robert Box (fixtures review)
*   Thomas Achtner (ENTTEC wing improvements)
*   Joep Admiraal (MIDI SysEx init messages, Dutch translation)
*   Florian Euchner (FX5 USB DMX support)
*   Stefan Riemens (new features)
*   Bartosz Grabias (new features)
*   Simon Newton, Peter Newman (OLA plugin)
*   Janosch Frank (webaccess improvements)
*   Karri Kaksonen (DMX USB Eurolite USB DMX512 Pro support)
*   Stefan Krupop (HID DMXControl Projects e.V. Nodle U1 support)
*   Nathan Durnan (RGB scripts, new features)
*   Giorgio Rebecchi (new features)
*   Florian Edelmann (code cleanup, German translation)
*   Heiko Fanieng, Jannis Achstetter (German translation)
*   NiKoyes, Jérôme Lebleu, Olivier Humbert, Nils Van Zuijlen (French translation)
*   Raymond Van Laake (Dutch translation)
*   Luis García Tornel (Spanish translation)
*   Jan Lachman (Czech translation)
*   Nuno Almeida, Carlos Eduardo Porto de Oliveira (Portuguese translation)
*   Santiago Benejam Torres (Catalan translation)
*   Koichiro Saito, Dai Suetake (Japanese translation)
</details>

<details>
<summary>Q Light Controller</summary>

*   Stefan Krumm (Bugfixes, new features)
*   Christian Suehs (Bugfixes, new features)
*   Christopher Staite (Bugfixes)
*   Klaus Weidenbach (Bugfixes, German translation)
*   Lutz Hillebrand (uDMX plugin)
*   Matthew Jaggard (Velleman plugin)
*   Ptit Vachon (French translation)
</details>

---

<p align="center">
  <img src="https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++ badge" />
  <img src="https://img.shields.io/badge/Qt-%23217346.svg?style=for-the-badge&logo=Qt&logoColor=white" alt="Qt badge" />
  <img src="https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake badge" />
  <img src="https://img.shields.io/badge/LuaJIT-%2300007C.svg?style=for-the-badge&logo=lua&logoColor=white" alt="LuaJIT badge" />
</p>
