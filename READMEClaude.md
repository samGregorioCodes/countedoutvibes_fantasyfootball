# Counted Out Football

Two-team, head-to-head PPR fantasy matchup. The game logic is one C++ file
(`src/main.cpp`) that runs as a console program or, compiled with Emscripten,
as the WebAssembly engine behind the web page in `web/`.

## Scoring

| Stat | Points |
|---|---|
| Passing yards | 1 per 25 |
| Rushing yards | 1 per 10 |
| Receiving yards | 1 per 10 |
| Touchdown (any position) | 4 |
| Reception | 1 |
| Ranked outside ESPN's preseason top 100, or unranked | +5 |

Each team fields 1 QB, 2 RB and 3 WR. Team totals are rounded to hundredths
before comparison, so equal displayed scores are a tie.

## Project layout

```
src/main.cpp        C++ game logic + console game + Embind bindings
web/index.html      page markup (every input has an explicit label)
web/style.css       early-2000s fantasy portal styling
web/app.js          reads the form, calls the C++ engine, shows results
Makefile            native and Emscripten builds
.github/workflows/deploy.yml   builds and deploys dist/ to GitHub Pages
```

## Build

Console version (any C++17 compiler):

```sh
make native
./counted_out_football
```

Web version (requires [Emscripten](https://emscripten.org/docs/getting_started/downloads.html), tested with 7.0.0):

```sh
source /path/to/emsdk/emsdk_env.sh
make            # outputs dist/: index.html, style.css, app.js, game.js, game.wasm
make serve      # http://localhost:8000
```

The page must be served over http(s); opening `dist/index.html` directly from
disk won't load the WebAssembly module.

## How the page talks to C++

`src/main.cpp` exposes `Player`, `FantasyTeam` and `Matchup` to JavaScript
through an `EMSCRIPTEN_BINDINGS` block (compiled only for the web build).
`main()` is not run in the browser (`-sINVOKE_RUN=0`).

`web/app.js` loads the module (`import createGameModule from './game.js'`),
then on **Calculate Matchup**:

1. creates a `FantasyTeam` for each team name,
2. creates a `Player` per lineup slot, calls `SetPlayer` / `SetStats`
   (both return `false` for invalid data), and `AddPlayer`,
3. creates a `Matchup` and reads `CalculateTeamScore`, `GetWinner`
   and `GetResultMessage`,
4. deletes every Embind object.

## Deploy to GitHub Pages

1. Push this project to a GitHub repository with `main` as the default branch.
2. In the repository, go to **Settings → Pages** and set **Source** to **GitHub Actions**.
3. Push to `main` (or run the workflow manually from the **Actions** tab).

## Rankings

ESPN's preseason rankings are a reference link on the page; there is no API
integration. To point at a different season's list, change the ESPN link in
`web/index.html`.
