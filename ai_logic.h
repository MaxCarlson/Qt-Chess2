#ifndef AI_LOGIC_H
#define AI_LOGIC_H
#include <string>
#include <stack>
#include <thread>
#include "move.h"
//#include "hashentry.h"
#include "slider_attacks.h"
class ZobristH;
class BitBoards;
class evaluateBB;
class HashEntry;
class Move;

class Ai_Logic
{
public:
    Ai_Logic();

    //iterative deepening
    Move iterativeDeep(int depth);


private:
    //minmax with alpha beta, the main component of our search
    int alphaBeta(int depth, int alpha, int beta, bool isWhite, long currentTime, long timeLimmit, int currentDepth, bool allowNull);

    //Null moves function
    int nullMoves(int depth, int alpha, int beta, bool isWhite, long currentTime, long timeLimmit, int currentDepth);

    //Quiescent search ~~ search positions farther if there are captures on horizon
    int quiescent(int alpha, int beta, bool isWhite, int currentDepth, int quietDepth, long currentTime, long timeLimmit);
    //if a capture cannot increase alpha, don't bother searching it
    /*
    bool deltaPruning(std::string move, int eval, bool isWhite, int alpha, bool isEndGame, BitBoards *BBBoard);
    */

//heuristics

    void addKiller(Move move, int ply);
    void ageHistorys();

//transposition table functions
    //add best move to TT
    void addMoveTT(Move move, int depth, int eval, int flag);

//object variables

    //counts number of piece postitions tried
    int positionCount = 0;

    //count of quiescnce positions checked
    int qCount = 0;

    //number representing amount to reduce search with Null-Moves
    int depthR = 2;

};

#endif // AI_LOGIC_H

