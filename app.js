// Counted Out Football: presentation layer.
// Reads the form, hands the values to the C++ game engine (compiled to
// WebAssembly with Embind), and displays what the engine returns.
// All scoring and winner logic lives in C++ (src/main.cpp).

import createGameModule from './game.js';

const TEAMS = ['t1', 't2'];

// Lineup slots: must match the field ids in index.html.
const SLOTS = [
  { id: 'qb',  position: 'QB' },
  { id: 'rb1', position: 'RB' },
  { id: 'rb2', position: 'RB' },
  { id: 'wr1', position: 'WR' },
  { id: 'wr2', position: 'WR' },
  { id: 'wr3', position: 'WR' },
];

const form = document.getElementById('matchup-form');
const calculateButton = document.getElementById('calculate');
const engineStatus = document.getElementById('engine-status');
const resultsPanel = document.getElementById('results');
const resultsBody = document.getElementById('results-body');

let engine = null;

// ---------- Field helpers ----------

function field(id) {
  return document.getElementById(id);
}

// Yard fields that a position doesn't use are absent from the page: count them as 0.
function numberOrZero(id) {
  const input = field(id);
  return input ? input.valueAsNumber : 0;
}

function readPlayer(team, slot) {
  const prefix = `${team}-${slot.id}`;
  const unranked = field(`${prefix}-unranked`).checked;

  return {
    name: field(`${prefix}-name`).value.trim(),
    position: slot.position,
    rank: unranked ? 0 : field(`${prefix}-rank`).valueAsNumber,
    unranked,
    passingYards: numberOrZero(`${prefix}-passing`),
    rushingYards: numberOrZero(`${prefix}-rushing`),
    receivingYards: numberOrZero(`${prefix}-receiving`),
    touchdowns: field(`${prefix}-touchdowns`).valueAsNumber,
    receptions: field(`${prefix}-receptions`).valueAsNumber,
  };
}

// ---------- Talking to the C++ engine ----------

// Builds the two C++ FantasyTeam objects, runs the Matchup, and returns plain
// JavaScript values. Every Embind object is deleted before returning.
function runMatchup() {
  const created = [];
  const track = (obj) => { created.push(obj); return obj; };

  try {
    const teams = TEAMS.map((team) => {
      const teamName = field(`${team}-team-name`).value.trim();
      const fantasyTeam = track(new engine.FantasyTeam(teamName));

      for (const slot of SLOTS) {
        const p = readPlayer(team, slot);
        const player = track(new engine.Player());

        if (!player.SetPlayer(p.name, p.position, p.rank, p.unranked) ||
            !player.SetStats(p.passingYards, p.rushingYards, p.receivingYards,
                             p.touchdowns, p.receptions) ||
            !fantasyTeam.AddPlayer(player)) {
          throw new Error(`The game engine rejected ${teamName}'s ${slot.id.toUpperCase()} entry. Check that player's fields.`);
        }
      }

      if (!fantasyTeam.IsRosterComplete()) {
        throw new Error(`${teamName} needs exactly 1 QB, 2 RB and 3 WR.`);
      }
      return fantasyTeam;
    });

    const matchup = track(new engine.Matchup(teams[0], teams[1]));

    return {
      teams: teams.map((t) => ({ name: t.GetTeamName(), score: t.CalculateTeamScore() })),
      winner: matchup.GetWinner(),          // 1, 2, or 0 for a tie
      message: matchup.GetResultMessage(),
    };
  } finally {
    // Matchup holds pointers to the teams, so delete in reverse creation order.
    for (const obj of created.reverse()) obj.delete();
  }
}

// ---------- Rendering ----------

function showResults(result) {
  resultsBody.replaceChildren();

  const table = document.createElement('table');
  table.className = 'result-table';
  const caption = document.createElement('caption');
  caption.className = 'visually-hidden';
  caption.textContent = 'Team scores';
  table.append(caption);

  result.teams.forEach((team, index) => {
    const row = table.insertRow();
    if (result.winner === index + 1) row.className = 'winner';

    const nameCell = document.createElement('th');
    nameCell.scope = 'row';
    nameCell.textContent = team.name;

    const scoreCell = document.createElement('td');
    scoreCell.textContent = `${team.score.toFixed(2)} pts`;

    row.append(nameCell, scoreCell);
  });

  const message = document.createElement('p');
  message.className = 'result-message';
  message.textContent = result.message;

  resultsBody.append(table, message);
}

function showError(text) {
  resultsBody.replaceChildren();
  const p = document.createElement('p');
  p.className = 'result-error';
  p.textContent = text;
  resultsBody.append(p);
}

function revealResults() {
  resultsPanel.focus({ preventScroll: true });
  resultsPanel.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

// ---------- Form behaviour ----------

// "Not ranked by ESPN" switches the rank box off (and out of validation).
function syncUnranked(checkbox) {
  const rankInput = field(checkbox.getAttribute('aria-controls'));
  rankInput.disabled = checkbox.checked;
  if (checkbox.checked) rankInput.value = '';
}

// Once a player's name is typed, the labels for that player's other fields use it,
// e.g. "Enter your RB1 rushing yards" becomes "Enter Jahmyr Gibbs rushing yards".
function syncSubject(nameInput) {
  const slotKey = nameInput.id.replace(/-name$/, '');
  const name = nameInput.value.trim();
  for (const span of document.querySelectorAll(`[data-subject-for="${slotKey}"]`)) {
    span.textContent = name || span.dataset.default;
  }
}

form.addEventListener('change', (event) => {
  if (event.target.classList.contains('unranked-toggle')) syncUnranked(event.target);
});

form.addEventListener('input', (event) => {
  if (event.target.id.endsWith('-name') && !event.target.id.includes('team')) {
    syncSubject(event.target);
  }
});

form.addEventListener('submit', (event) => {
  event.preventDefault();
  if (!engine) return;
  if (!form.reportValidity()) return;

  try {
    showResults(runMatchup());
  } catch (error) {
    showError(error.message);
  }
  revealResults();
});

// ---------- Start-up ----------

// Browsers may restore earlier form values on reload, so sync once at start.
document.querySelectorAll('.unranked-toggle').forEach(syncUnranked);
document.querySelectorAll('.player input[id$="-name"]').forEach(syncSubject);

createGameModule()
  .then((module) => {
    engine = module;
    calculateButton.disabled = false;
    engineStatus.textContent = 'Game engine ready.';
  })
  .catch((error) => {
    console.error(error);
    engineStatus.classList.add('error');
    engineStatus.textContent =
      'The game engine could not load. Serve this page over http(s) rather than opening the file directly.';
  });
