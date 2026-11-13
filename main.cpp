
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <limits>
#include <cmath>
#include <cctype>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif

using namespace std;


// ==========================================
// SECTION 1: CLASS DECLARATIONS
// ==========================================

// Game logic. These classes do no console or browser input/output, so the
// same code is used by the console game and by the WebAssembly build.

class Player {
private:
    string name;
    string position;
    int espnRank;
    bool unranked;
    double passingYards;
    double rushingYards;
    double receivingYards;
    int touchdowns;
    int receptions;

public:
    Player();

    bool SetPlayer(string playerName, string playerPosition,
                   int ranking, bool isUnranked);

    bool SetStats(double passYards, double rushYards, double recYards,
                  int touchdownCount, int receptionCount);

    void EnterStats();

    double CalculatePoints() const;
    bool GetsRankBonus() const;

    string GetName() const;
    string GetPosition() const;
};


class FantasyTeam {
private:
    string teamName;
    vector<Player> roster;

    int CountPosition(string position) const;

public:
    FantasyTeam(string name);

    bool AddPlayer(const Player& player);
    void EnterRoster();
    void EnterPlayerStats();
    bool IsRosterComplete() const;
    double CalculateTeamScore() const;
    void PrintResults() const;
    string GetTeamName() const;
};


class Matchup {
private:
    const FantasyTeam* teamOne;
    const FantasyTeam* teamTwo;

public:
    Matchup(const FantasyTeam& firstTeam, const FantasyTeam& secondTeam);

    int GetWinner() const;
    string GetResultMessage() const;
    void DetermineWinner() const;
};


// Console presentation only: reads and validates keyboard input.
// Not used by the WebAssembly build.

class ConsoleInput {
public:
    static string ReadText(string prompt);
    static int ReadInt(string prompt, int minimum);
    static double ReadDouble(string prompt);
    static bool ReadYesNo(string prompt);
};


// ==========================================
// SECTION 2: CLASS DEFINITIONS
// ==========================================

// SCORING RULES (PPR)
const double PASSING_YARDS_PER_POINT = 25.0;
const double RUSHING_YARDS_PER_POINT = 10.0;
const double RECEIVING_YARDS_PER_POINT = 10.0;
const double POINTS_PER_TOUCHDOWN = 4.0;
const double POINTS_PER_RECEPTION = 1.0;
const int TOP_RANK_CUTOFF = 100;
const double COUNTED_OUT_BONUS = 5.0;

const int QB_SLOTS = 1;
const int RB_SLOTS = 2;
const int WR_SLOTS = 3;


// PLAYER DEFINITIONS

Player::Player() {
    name = "";
    position = "";
    espnRank = 0;
    unranked = false;
    passingYards = 0.0;
    rushingYards = 0.0;
    receivingYards = 0.0;
    touchdowns = 0;
    receptions = 0;
}

bool Player::SetPlayer(string playerName, string playerPosition,
                       int ranking, bool isUnranked) {
    bool hasName = playerName.find_first_not_of(" \t") != string::npos;
    bool validPosition = playerPosition == "QB" || playerPosition == "RB" ||
                         playerPosition == "WR";
    bool validRank = isUnranked || ranking >= 1;

    if (!hasName || !validPosition || !validRank) {
        return false;
    }

    name = playerName;
    position = playerPosition;
    unranked = isUnranked;
    espnRank = isUnranked ? 0 : ranking;
    return true;
}

bool Player::SetStats(double passYards, double rushYards, double recYards,
                      int touchdownCount, int receptionCount) {
    if (!isfinite(passYards) || !isfinite(rushYards) || !isfinite(recYards) ||
        touchdownCount < 0 || receptionCount < 0) {
        return false;
    }

    passingYards = passYards;
    rushingYards = rushYards;
    receivingYards = recYards;
    touchdowns = touchdownCount;
    receptions = receptionCount;
    return true;
}

void Player::EnterStats() {
    double passYards = 0.0;
    double rushYards = 0.0;
    double recYards = 0.0;

    if (position == "QB") {
        passYards = ConsoleInput::ReadDouble("Enter " + name + " passing yards: ");
        rushYards = ConsoleInput::ReadDouble("Enter " + name + " rushing yards: ");
    }
    else if (position == "RB") {
        rushYards = ConsoleInput::ReadDouble("Enter " + name + " rushing yards: ");
        recYards = ConsoleInput::ReadDouble("Enter " + name + " receiving yards: ");
    }
    else {
        recYards = ConsoleInput::ReadDouble("Enter " + name + " receiving yards: ");
        rushYards = ConsoleInput::ReadDouble("Enter " + name + " rushing yards: ");
    }

    int touchdownCount = ConsoleInput::ReadInt("Enter " + name + " touchdowns: ", 0);
    int receptionCount = ConsoleInput::ReadInt("Enter " + name + " receptions: ", 0);

    SetStats(passYards, rushYards, recYards, touchdownCount, receptionCount);
}

double Player::CalculatePoints() const {
    double points = 0.0;

    points += passingYards / PASSING_YARDS_PER_POINT;
    points += rushingYards / RUSHING_YARDS_PER_POINT;
    points += receivingYards / RECEIVING_YARDS_PER_POINT;
    points += touchdowns * POINTS_PER_TOUCHDOWN;
    points += receptions * POINTS_PER_RECEPTION;

    if (GetsRankBonus()) {
        points += COUNTED_OUT_BONUS;
    }

    return points;
}

bool Player::GetsRankBonus() const {
    return unranked || espnRank > TOP_RANK_CUTOFF;
}

string Player::GetName() const {
    return name;
}

string Player::GetPosition() const {
    return position;
}


// FANTASY TEAM DEFINITIONS

FantasyTeam::FantasyTeam(string name) {
    teamName = name;
}

int FantasyTeam::CountPosition(string position) const {
    int count = 0;

    for (unsigned int i = 0; i < roster.size(); ++i) {
        if (roster.at(i).GetPosition() == position) {
            ++count;
        }
    }

    return count;
}

bool FantasyTeam::AddPlayer(const Player& player) {
    string position = player.GetPosition();
    int slots = 0;

    if (position == "QB") {
        slots = QB_SLOTS;
    }
    else if (position == "RB") {
        slots = RB_SLOTS;
    }
    else if (position == "WR") {
        slots = WR_SLOTS;
    }

    if (CountPosition(position) >= slots) {
        return false;
    }

    roster.push_back(player);
    return true;
}

void FantasyTeam::EnterRoster() {
    string positions[] = { "QB", "RB", "WR" };
    int slots[] = { QB_SLOTS, RB_SLOTS, WR_SLOTS };

    for (int p = 0; p < 3; ++p) {
        for (int i = 1; i <= slots[p]; ++i) {
            string slotName = positions[p];
            if (slots[p] > 1) {
                slotName += to_string(i);
            }

            string playerName = ConsoleInput::ReadText("\nEnter your " + slotName + ": ");

            bool isUnranked = ConsoleInput::ReadYesNo(
                "Is your " + slotName + " unranked by ESPN (outside the top " +
                to_string(TOP_RANK_CUTOFF) + ")? (y/n): ");

            int ranking = 0;
            if (!isUnranked) {
                ranking = ConsoleInput::ReadInt(
                    "Enter your " + slotName + " ESPN overall rank: ", 1);
            }

            Player player;
            player.SetPlayer(playerName, positions[p], ranking, isUnranked);
            AddPlayer(player);
        }
    }
}

void FantasyTeam::EnterPlayerStats() {
    cout << "\n--- Enter statistics for " << teamName << " ---\n";

    for (unsigned int i = 0; i < roster.size(); ++i) {
        cout << "\nPlayer: " << roster.at(i).GetName()
             << " (" << roster.at(i).GetPosition() << ")" << endl;
        roster.at(i).EnterStats();
    }
}

bool FantasyTeam::IsRosterComplete() const {
    return CountPosition("QB") == QB_SLOTS &&
           CountPosition("RB") == RB_SLOTS &&
           CountPosition("WR") == WR_SLOTS;
}

// Team score is rounded to hundredths so the score shown is the score compared.
double FantasyTeam::CalculateTeamScore() const {
    double total = 0.0;

    for (unsigned int i = 0; i < roster.size(); ++i) {
        total += roster.at(i).CalculatePoints();
    }

    return round(total * 100.0) / 100.0;
}

void FantasyTeam::PrintResults() const {
    ostringstream score;
    score << fixed << setprecision(2) << CalculateTeamScore();

    cout << "\n" << teamName << ": " << score.str() << " points\n";
}

string FantasyTeam::GetTeamName() const {
    return teamName;
}


// MATCHUP DEFINITIONS

Matchup::Matchup(const FantasyTeam& firstTeam,
                 const FantasyTeam& secondTeam) {
    teamOne = &firstTeam;
    teamTwo = &secondTeam;
}

// Returns 1 if team one wins, 2 if team two wins, 0 for a tie.
int Matchup::GetWinner() const {
    long long scoreOne = llround(teamOne->CalculateTeamScore() * 100.0);
    long long scoreTwo = llround(teamTwo->CalculateTeamScore() * 100.0);

    if (scoreOne > scoreTwo) {
        return 1;
    }
    else if (scoreTwo > scoreOne) {
        return 2;
    }

    return 0;
}

string Matchup::GetResultMessage() const {
    int winner = GetWinner();

    if (winner == 1) {
        return teamOne->GetTeamName() + " wins!";
    }
    else if (winner == 2) {
        return teamTwo->GetTeamName() + " wins!";
    }

    return "It's a tie!";
}

void Matchup::DetermineWinner() const {
    cout << "\n========== MATCHUP RESULTS ==========\n";

    teamOne->PrintResults();
    teamTwo->PrintResults();

    cout << "\n" << GetResultMessage() << "\n";
}


// CONSOLE INPUT DEFINITIONS

string ConsoleInput::ReadText(string prompt) {
    string text;

    while (true) {
        cout << prompt;
        if (!getline(cin >> ws, text)) {
            cout << "\nInput ended.\n";
            exit(1);
        }
        if (text.find_first_not_of(" \t") != string::npos) {
            return text;
        }
        cout << "Please enter a name.\n";
    }
}

int ConsoleInput::ReadInt(string prompt, int minimum) {
    int value;

    while (true) {
        cout << prompt;
        if (cin >> value && value >= minimum) {
            return value;
        }
        if (cin.eof()) {
            cout << "\nInput ended.\n";
            exit(1);
        }
        cout << "Please enter a whole number of at least " << minimum << ".\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

double ConsoleInput::ReadDouble(string prompt) {
    double value;

    while (true) {
        cout << prompt;
        if (cin >> value && isfinite(value)) {
            return value;
        }
        if (cin.eof()) {
            cout << "\nInput ended.\n";
            exit(1);
        }
        cout << "Please enter a number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

bool ConsoleInput::ReadYesNo(string prompt) {
    while (true) {
        string answer = ReadText(prompt);
        char first = static_cast<char>(tolower(answer.at(0)));

        if (first == 'y') {
            return true;
        }
        if (first == 'n') {
            return false;
        }
        cout << "Please enter y or n.\n";
    }
}


// WEBASSEMBLY BINDINGS (browser build only)
// Exposes the game-logic classes to JavaScript. main() is not run in the
// browser; the web page calls these functions directly.

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(counted_out_football) {
    emscripten::class_<Player>("Player")
        .constructor<>()
        .function("SetPlayer", &Player::SetPlayer)
        .function("SetStats", &Player::SetStats)
        .function("CalculatePoints", &Player::CalculatePoints)
        .function("GetName", &Player::GetName)
        .function("GetPosition", &Player::GetPosition);

    emscripten::class_<FantasyTeam>("FantasyTeam")
        .constructor<string>()
        .function("AddPlayer", &FantasyTeam::AddPlayer)
        .function("IsRosterComplete", &FantasyTeam::IsRosterComplete)
        .function("CalculateTeamScore", &FantasyTeam::CalculateTeamScore)
        .function("GetTeamName", &FantasyTeam::GetTeamName);

    emscripten::class_<Matchup>("Matchup")
        .constructor<const FantasyTeam&, const FantasyTeam&>()
        .function("GetWinner", &Matchup::GetWinner)
        .function("GetResultMessage", &Matchup::GetResultMessage);
}
#endif


// ==========================================
// SECTION 3: MAIN GAME
// ==========================================

int main() {

    cout << "================================\n";
    cout << "       COUNTED OUT FOOTBALL\n";
    cout << "================================\n";

    string teamNameOne = ConsoleInput::ReadText("\nEnter Team One Name: ");
    string teamNameTwo = ConsoleInput::ReadText("Enter Team Two Name: ");

    FantasyTeam teamOne(teamNameOne);
    FantasyTeam teamTwo(teamNameTwo);

    cout << "\n========== TEAM ONE ==========\n";
    teamOne.EnterRoster();

    cout << "\n========== TEAM TWO ==========\n";
    teamTwo.EnterRoster();

    teamOne.EnterPlayerStats();
    teamTwo.EnterPlayerStats();

    Matchup game(teamOne, teamTwo);
    game.DetermineWinner();

    return 0;
}
