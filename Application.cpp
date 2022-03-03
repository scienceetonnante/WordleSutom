#include <cmath>
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static constexpr int MAX_NUMBER_OF_WORDS = 4096;


using namespace std;

static constexpr char ASCII_0 = '0';
static constexpr char ASCII_A = 'A';
static constexpr char ASCII_Z = 'Z';

// constexpr function to compute powers of 3
static constexpr int pow3(int k)
{
    int result = 1;

    for (int i = 0 ; i < k ; ++i)
    {
        result *= 3;
    }

    return result;
}

// Fast version of pow() for powers of 3
constexpr int pow(int x, int y)
{
    if (x != 3 || y > 12)
    {
        int result = 1;

        for (int i = 0 ; i < y ; ++i)
        {
            result *= x;
        }

        return result;
    }
    else
    {
        constexpr array<int, 13> cache {
            pow3(0),
            pow3(1),
            pow3(2),
            pow3(3),
            pow3(4),
            pow3(5),
            pow3(6),
            pow3(7),
            pow3(8),
            pow3(9),
            pow3(10),
            pow3(11),
            pow3(12),
        };

        return cache[y];
    }
}


// Produces the pattern for a given tentative, assuming the truth
// Pattern is coded as an integer equal to sum_k a_k 3^k  where a_k=0,1,2 are the color obtained (0:gray,1:yellow,2:green)
// So this the number represents its "base 3" coding
int ComputePattern(const string &tentative, string truth)
{
    vector<int> result(tentative.size(), 0);

    for(size_t  k = 0; k < tentative.size(); k++)
    {
        if(tentative[k]==truth[k])
        {
            result[k] = 2;        // Green coded by 2
            truth[k] = '-';
        }
    }

    for(size_t k = 0; k < tentative.size(); k++)
    {
        if (result[k] != 0) continue;

        bool fnd = false;
        for(size_t k2 = 0; k2 < tentative.size(); k2++)
        {
            // If found elsewhere and that elsewhere is not already green
            if(tentative[k] == truth[k2])
            {
                fnd = true;
                truth[k2] = '-';
                break;
            }
        }
        if(fnd) result[k] = 1;    // Yellow coded by 1

    }

    int res = 0;

    for(size_t k = 0; k < tentative.size(); k++)
    {
        res += result[k] * pow(3,k);
    }

    return res;
}


// Convert a string of 0,1,2 into the pattern code (warning, nothing checked)
int StringToPattern(const string &s)
{
    int res = 0;
    for(size_t k=0; k < s.size();k++)
    {
        char c = s[k];              // Ascii code
        int a = c - ASCII_0;        // 0,1 or 2
        res += a * pow(3,k);
    }
    return res;
}


// The Unicode colored squares associated to a given pattern
string PatternToStringOfSquares(int pattern, size_t K)
{
    string res;

    int current = pattern;
    for(size_t k=0; k < K;k++)
    {
        int a = current%3;
        res += a==2 ? "\U0001F7E9" : (a==1 ? "\U0001F7E8" : "\u2B1B");
        current = current/3;
    }
    return res;
}


// ==================================================================================================================

/* A class to contain the current state of the game : words that have been played, and patterns obtained */
class GameState
{
    private:
        size_t K;
        struct Step
        {
            Step(string word, int p) :
                played_word(std::move(word)),
                pattern(p)
            {}
            string played_word;
            int pattern;
        };
        vector<Step> steps;
        vector<int> green_mask;             // Redundant information for speed : the list of green letters (-1 if not known, 0 for A, 1 for B etc)

    public:
        // Base constructor
        explicit GameState(size_t K_) : K(K_), green_mask(K, -1)
        {
        }

        // Constructor with mask
        GameState(size_t K_, const string &mask) : K(K_), green_mask(K, -1)
        {
            for(size_t k=0; k < K; k++)
            {
                char c_mask = mask[k];
                if(c_mask >= ASCII_A && c_mask <= ASCII_Z)  // if the mask specifies a letter
                {
                    green_mask[k] = c_mask - ASCII_A;
                }
            }
        }

        // Copy constructor
        GameState(const GameState &that) = default;


        size_t GetWordSize() const {return K;}


        // Update the state by giving a word and its associated obtained pattern (we don't check size, warning)        
        void Update(const string &word, int pattern)
        {
            steps.emplace_back(word, pattern);

            // Decode pattern to register green letters
            int current = pattern;
            for(size_t k=0; k < K; k++)
            {
                int a = current%3;
                if(a==2)    // if the letter was good and well placed
                {
                    char c = word[k];
                    green_mask[k] = (c-ASCII_A);   // 0 for letter A
                }
                current = current/3;
            }
        }


        // Whether a word is compatible with current state of the game
        // Compatible : it could be the solution ie, it could have produced that sequence.

        bool isCompatible (const string &candidate_truth, bool check_only_last_step) const
        {
            // First check the green mask to save time
            for(size_t k=0; k < K; k++)
            {
                if(green_mask[k] != -1)
                {
                    char c = candidate_truth[k];
                    if(c-ASCII_A != green_mask[k]) return false;
                }

            }

            // Then check each of the previous steps of the game, to see whether that candidate truth word could have produced that series of patterns
            // We check in reverse assuming the later patterns carry more constraints (with option to check only that one if we know other are satisied)
            for(int i = steps.size() -1 ; i >=0 ; i--)
            {
                const string &word = steps[i].played_word;
                int pattern = steps[i].pattern;
                if(ComputePattern(word,candidate_truth) != pattern) return false;   // not compatible == That candidate_truth would not have produced that observed pattern

                if(i==steps.size() -1 && check_only_last_step) return true;
            }
            return true;
        }        
       

        int NbOfCompatibleWords(const vector<string> &words) const
        {            
            int cnt = 0;
            for(const auto& word : words)
            {        
                if(isCompatible(word,false))
                {
                    cnt++;
                }
            }
            return cnt;
        }

};


// ==================================================================================================================

double ComputeEntropy(const GameState &initial_state, const string &word, const vector<string> &possible_solutions)
{
    double entropy = 0;
    size_t K = initial_state.GetWordSize();

    // For each pattern we could get, compute expected entropy
    for(int pattern = 0; pattern < pow(3,K); pattern++)
    {
        // If we got that pattern from that word, in which state would we be
        GameState state(initial_state);
        state.Update(word,pattern);

        // In that case, count how many would be compatible as being the ground truth
        int cnt = 0;            
        for(const auto& candidate_word : possible_solutions)
        {
            cnt += state.isCompatible(candidate_word,true);     // we are checking previously possible solutions, so we look only at last step
        }
        double p = static_cast<double>(cnt) / static_cast<double>(possible_solutions.size());
        if(p>0) 
        {
            entropy += - p * log(p) / log(2);
        }
    }

    return entropy;        
}


string ComputeBestChoice(const GameState& initial_state, const vector<string> &words)
{
    vector<string> candidate_pool = words;

    // Build the list of remaining possible solutions at this stage
    vector<string> possible_solutions;
    for(const auto& word : words)
    {
        if(initial_state.isCompatible(word,false))
        {
            possible_solutions.push_back(word);
        }
    }

    // If only one, we are done
    if(possible_solutions.size() == 1) return possible_solutions[0];

    // If less than 10 : display
    cout << "Number of possible solutions " << possible_solutions.size() << " :";
    if(possible_solutions.size() < 10)
    {
        for(auto & possible_solution : possible_solutions)
        {
            cout << possible_solution << ",";
        }
    }
    cout << endl;

    // If less than 3, we limit our choice to the possible solutions, so we try to "shoot to kill"
    if(possible_solutions.size() < 4)
    {
        candidate_pool = possible_solutions;
    }

    // Now find the word with the maximum entropy    
    string best_choice;
    float best_entropy = -1;
    for(int iw = 0; iw < candidate_pool.size(); iw++)
    {   
        // if(iw%100==0) cout << "N°"<<iw<<endl;     // verbose
        string word = candidate_pool[iw];        

        float entropy = ComputeEntropy(initial_state, word, possible_solutions);
        if(entropy > best_entropy)
        {
            best_entropy = entropy;
            best_choice = word;    
            cout << "New best option (n°" << iw << ") : " << best_choice << " : " << best_entropy << " bits" << endl;        
        }  
        
    }    
    return best_choice;
}




// ==================================================================================================================

// Simple tests
// ============

void PrintTest(const string &truth, const string &word)
{
    cout << "(" << truth << ")" << " " << word << " " << PatternToStringOfSquares(ComputePattern(word,truth),word.size()) << endl;
}

void BasicRuleTest()
{
    PrintTest("ABCDE","AXXXX");     // 🟩⬛⬛⬛⬛ one good
    PrintTest("ABCDE","XAXXX");     // ⬛🟨⬛⬛⬛ one misplaced
    PrintTest("ABCDE","AEXXX");     // 🟩🟨⬛⬛⬛ one good one misplaced

    PrintTest("ABCDE","AAXXX");     // 🟩⬛⬛⬛⬛ one good once
    PrintTest("ABCDE","XAAXX");     // ⬛🟨⬛⬛⬛ double misplacement of the same letter

    PrintTest("AABCD","AXAXX");     // 🟩⬛🟨⬛⬛ same letter twice one good, a second copy misplaced
    PrintTest("AABCD","AAXXX");     // 🟩🟩⬛⬛⬛
    PrintTest("AABCD","AAXXA");     // 🟩🟩⬛⬛⬛
    PrintTest("AAACD","AAXXA");     // 🟩🟩⬛⬛🟨 there is a third copy somewhere
}


// ==================================================================================================================

// Load words with a given length. File are assumed to be like "data/mots_5.txt"
vector<string> LoadWords(size_t K, size_t N)
{
    // Read file dictionnary of words
    vector<string> words;
    const string filename = "data/mots_" + to_string(K) + ".txt";
    ifstream file(filename);

    if (!file.is_open())
    {
        cerr << "Failed to open file: " << filename << endl;
        // A bit extreme, but if we are here the program won't work and will crash soon emough.
        abort();
    }

    string line;
    
    while(getline(file,line) && words.size() < N)
    {
        // Convert to CAPS : after we assume there is nothing elese than A-Z
        for_each(line.begin(), line.end(), [](char & c)
        {
            c = ::toupper(c);
        });

        if(find(words.begin(), words.end(), line) == words.end() )
        {
            words.push_back(line);            
        }            
    }
    return words;
}


// Load words matching a certain mask. 
// E.g if mask = "F......" : length=7 and starts with F.
vector<string> LoadWordsWithMask(size_t N, const string &mask)
{
    size_t K = mask.size();
    vector<string> words = LoadWords(K,N);
    vector<string> res;
    int cnt = 0;
    for(auto& word : words)
    {
        bool word_ok = true;
        for(size_t k=0; k < K; k++)
        {
            char c_mask = mask[k];
            if(c_mask >= ASCII_A && c_mask <= ASCII_Z)  // if the mask specifies a letter, check the word satisfies it
            {
                char c_word = word[k];
                if(c_word != c_mask)
                {
                    word_ok =false;
                    break;
                }
            }
        }
        if(word_ok) 
        {
            res.push_back(word);
            //cout << cnt << " " << word << endl;
            cnt++;
        }
    }
    return res;
}


// =================================================================================================

// Automatically plays a game with a given solution "ground_truth" and a given initial_mask

int AutomaticPlay(const vector<string> & words, const string &ground_truth, const string &initial_mask)
{    
    cout << "\n*** NEW GAME Truth=" << ground_truth << endl;
    
    size_t K = words[0].size();
    GameState state(K,initial_mask);
    int nb_compat = state.NbOfCompatibleWords(words);
    cout << "Nb of compatible words : " << nb_compat << " Entropy=" << log(nb_compat)/log(2) << '\n';

    const int MAX_STEPS = 6;
    for(int s = 0; s < MAX_STEPS; s++)
    {        
        string proposal;

        // If first steps Use known best words for opening
        if(s==0)
        {
            if(initial_mask == ".....") proposal = "TARIE";
            if(initial_mask == "......") proposal = "SORTIE";
        }

        if(proposal.empty()) proposal = ComputeBestChoice(state, words);
        cout << '\n' << proposal;

        int pattern = ComputePattern(proposal, ground_truth);
        cout << " " << PatternToStringOfSquares(pattern,state.GetWordSize()) << " ";

        if(proposal == ground_truth) 
        {
            cout << "SOLVED IN " << (s+1) << " STEPS" << endl;
            return s+1;            
        }
        
        double old_entropy = log(state.NbOfCompatibleWords(words))/log(2);
        state.Update(proposal, pattern);
        double new_entropy = log(state.NbOfCompatibleWords(words))/log(2);

        cout << "Entropy gain = " << (old_entropy-new_entropy);
        cout << " Nb of compatible words : " << state.NbOfCompatibleWords(words) << " New entropy=" << log(state.NbOfCompatibleWords(words))/log(2) << " ";            
    }
    return MAX_STEPS;
}


int AutoWordle(const string &ground_truth)
{    
    size_t K = ground_truth.size();
    vector<string> words = LoadWords(K,MAX_NUMBER_OF_WORDS);
    string initial_mask = string(K, '.');

    int score = AutomaticPlay(words, ground_truth, initial_mask);

    return score;
}

// Automatically plays a "SUTOM" like game, with first letter given
int AutoSutom(const string &ground_truth)
{    
    size_t K = ground_truth.size();
    string initial_mask = ground_truth[0] + string(K - 1, '.'); // First letter
    vector<string> words = LoadWordsWithMask(100000,initial_mask);

    int score = AutomaticPlay(words, ground_truth, initial_mask);

    return score;
}


// =================================================================================================

void FindBestOpening(int K)
{
    vector<string> words = LoadWords(K, MAX_NUMBER_OF_WORDS);
    GameState initial_state(K);
    ComputeBestChoice(initial_state,words);
}


// A series of random tests to compute average performance
void ComputeAveragePerformance(int K, int NB_TESTS)
{
    vector<string> words = LoadWords(K, MAX_NUMBER_OF_WORDS);
    string initial_mask = string(K, '.');

    std::random_device rd;                          // obtain a random number from hardware
    std::mt19937 gen(rd());                         // seed the generator
    std::uniform_int_distribution<> distr(0, 1000); // define the range

    double avg = 0;
    for(int i = 0; i < NB_TESTS; i++)
    {  
        int n = distr(gen);
        string truth = words[n];                        // choose a word
        int s = AutomaticPlay(words, truth, initial_mask);            // get performance
        avg = ((avg * static_cast<double>(i) + s))/(static_cast<double>(i+1));        // update average
        cout << "*** CURRENT AVERAGE = " << avg << " (" << i + 1 << " tests)\n" << endl;
    }
}

// A series of random tests to compute average performance
void ComputeAverageSutomPerformance(int K, int NB_TESTS)
{
    std::random_device rd;                          // obtain a random number from hardware
    std::mt19937 gen(rd());                         // seed the generator
    vector<string> words = LoadWords(K, MAX_NUMBER_OF_WORDS);
    std::uniform_int_distribution<> distr(0, 1000); // define the range

    double avg = 0;
    for(int i = 0; i < NB_TESTS; i++)
    {  
        int n = distr(gen);
        
        string truth = words[n];  // choose a word
        int s = AutoSutom(truth);            // get performance

        avg = ((avg * static_cast<double>(i) + s))/(static_cast<double>(i+1));        // update average
        cout << "*** CURRENT AVERAGE = " << avg << " (" << i + 1 << " tests)" << endl << '\n';
    }
}



// Play an interactive game : enter what you get and the algo will suggest next word
void RealInteractiveGame()
{
    string initial_mask;
    cout << "Enter initial mask:";
    cin >> initial_mask;

    size_t K = initial_mask.size();

    vector<string> words = LoadWordsWithMask(100000,initial_mask);

    GameState state(K,initial_mask);

    for(int s = 0; s < 6; s++)
    {      
        string proposal;
        
        // If first steps Use known best words for opening
        if(s==0)
        {
            if(initial_mask == ".....") proposal = "TARIE";
            if(initial_mask == "......") proposal = "SORTIE";
        }
        
        if(proposal.empty()) proposal = ComputeBestChoice(state, words);
        
        cout << "Suggestion : " << proposal << '\n';
        cout << "Choix :";
        string choice;
        cin >> choice;
        cout << "Resultat obtenu :";            // expect a string like 21002 for green/yellow/gray/gray/green
        string result;
        cin >> result;               
        state.Update(choice, StringToPattern(result));        
    }
}


// =================================================================================================

int main()
{
    // Below a few things you can do

    BasicRuleTest();

    auto clock = chrono::steady_clock();
    const auto start = clock.now();

    AutoWordle("REPAS");
    AutoWordle("SAPIN");

    const auto end = clock.now();
    chrono::nanoseconds dt = end - start;
    cout << "Time spent: " << double(dt.count()) / 1'000'000 << "ms" << endl;

    AutoSutom("DIAMETRE");

    FindBestOpening(5);  

    ComputeAveragePerformance(5,10);

    ComputeAverageSutomPerformance(7,10);

    RealInteractiveGame();
    
    return 0;
}
