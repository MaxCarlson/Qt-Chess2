#include "ai_logic.h"
#include "zobristh.h"

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "bitboards.h"
#include "tile.h"



//best overall move as calced
std::string bestMove;

//variable for returning best move at depth of one
std::string tBestMove;

//holds board state before any moves or trys
std::string board1[8][8];
//holds state of initial board + 1 move
std::string board2[8][8];

//evaluation object - evaluates board position and gives an int value (- for black)
evaluateBB *eval = new evaluateBB;

//bool value to determine if time for search has run out
bool searchCutoff = false;

int qcount = 0;

Ai_Logic::Ai_Logic()
{
    //generate all possible moves for one turn
    newBBBoard->constructBoards();
    //once opponent move is made update the zorbist hash key
    ZKey->getZobristHash(true);
    //update zobrsit hash to correct color
    ZKey->UpdateColor();
}

std::string Ai_Logic::iterativeDeep(int depth)
{

    //iterative deepening start time
    clock_t IDTimeS = clock();

    //time limit in miliseconds
    int timeLimmit = 3550099999999999, currentDepth = 0;

    long endTime = IDTimeS + timeLimmit;

    qcount = 0;

    searchCutoff = false;

    positionCount = 0;
    //std::string bestMove, tBestMove;
    std::string bestMove;

    std::cout << zobKey << std::endl;

    int alpha = -100000;
    int beta = 100000;

    int distance = 1;
    for(distance; distance <= depth && IDTimeS < endTime;){
        positionCount = 0;
        clock_t currentTime = clock();

        if(currentTime >= endTime){
            break;
        }

        //tBestMove = miniMaxRoot(distance, true, currentTime, timeLimmit);
        int val = alphaBeta(distance, alpha, beta, false, currentTime, timeLimmit, currentDepth +1, true);
        //int val = principleV(distance, alpha, beta, false, currentDepth);


        if (val <= alpha || val >= beta) {
            alpha = -1000000;    // We fell outside the window, so try again with a
            beta = 1000000;      //  full-width window (and the same depth).
            continue;
        }
        //if we don't fall out of window, set alpha and beta to a window size to narrow search
        alpha = val - 45;
        beta = val + 45;

        //if the search is not cutoff
        if(!searchCutoff){
            bestMove = tBestMove;
            std::cout << (int)bestMove[0] << (int)bestMove[1] << (int)bestMove[2] << (int)bestMove[3] << ", ";
        }
        //increment distance to travel (inverse of depth)
        distance++;
    }
    std::cout << std::endl;

    //make final move on bitboards
    newBBBoard->makeMove(bestMove);

    newBBBoard->drawBBA();

    clock_t IDTimeE = clock();
    //postion count and time it took to find move
    std::cout << positionCount << " positions searched." << std::endl;
    std::cout << (double) (IDTimeE - IDTimeS) / CLOCKS_PER_SEC << " seconds" << std::endl;
    std::cout << "Depth of " << distance-1 << " reached."<<std::endl;
    //std::cout << zobKey << std::endl;
    std::cout << qcount << std::endl;

    return bestMove;
}

int Ai_Logic::alphaBeta(int depth, int alpha, int beta, bool isWhite, long currentTime, long timeLimmit, int currentDepth, bool extend)
{
    //iterative deeping timer stuff
    clock_t time = clock();
    long elapsedTime = time - currentTime;

    //create unqiue hash from zobkey
    int hash = (int)(zobKey % 15485843);
    HashEntry entry = transpositionT[hash];


    //if the depth of the stored evaluation is greater and the zobrist key matches
    //don't return eval on root node

    if(entry.depth >= depth && entry.zobrist == zobKey){
        //return either the eval, the beta, or the alpha depending on flag
        switch(entry.flag){
            case 3:
            if(entry.flag == 3){
                return entry.eval;
            }
            break;
            case 2:
            if(entry.eval >= beta){
                return beta;
            }
            break;
            case 1:
            if(entry.eval <= alpha){
                return alpha;
            }
            break;
        }
    }

    //if the time limmit has been exceeded finish searching
    if(elapsedTime >= timeLimmit){
        searchCutoff = true;
    }

    if(depth == 0 || searchCutoff){
        int score;
        //evaluate board position (if curentDepth is even return -eval)
        if(!extend){
            score = eval->evalBoard(isWhite);

            extend = true;
        } else {
            score = quiescent(alpha, beta, isWhite, currentDepth, 5);
        }
        //add move to hash table with exact flag
        addMoveTT("0", depth, score, 3);
        return score;
    }

    std::string moves, tempBBMove;
    int bestTempMove;

    //generate normal moves
    moves = newBBBoard->genWhosMove(isWhite);

    moves = mostVVLVA(moves, isWhite);

    //apply heuristics and move entrys from hash table, add to front of moves
    moves = sortMoves(moves, entry, currentDepth);

    //set hash flag equal to alpha Flag
    int hashFlag = 1;

    std::string tempMove, hashMove;
    for(int i = 0; i < moves.length(); i+=4){
        positionCount ++;
        //change board accoriding to i possible move
        tempMove = "";
        //convert move into a single string
        tempMove += moves[i];
        tempMove += moves[i+1];
        tempMove += moves[i+2];
        tempMove += moves[i+3];

        //make move on BB's store data to string so move can be undone
        tempBBMove = newBBBoard->makeMove(tempMove);

        //jump to other color and evaluate all moves that don't cause a cutoff if depth is greater than 1
        bestTempMove = -alphaBeta(depth-1, -beta, -alpha,  ! isWhite, currentTime, timeLimmit, currentDepth +1, extend);

        //undo move on BB's
        newBBBoard->unmakeMove(tempBBMove);

        //if move causes a beta cutoff stop searching current branch
        if(bestTempMove >= beta){
            //add beta to transpositon table with beta flag
            addMoveTT(tempMove, depth, beta, 2);

            //push killer move to top of stack for given depth
            killerHArr[currentDepth].push(tempMove);

            return beta;
        }

        if(bestTempMove > alpha){
            //new best move
            alpha = bestTempMove;

            //store best move useable found for depth of one
            if(currentDepth == 1){
                tBestMove = tempMove;
            }
            //if we've gained a new alpha set hash Flag equal to exact
            hashFlag = 3;
            hashMove = tempMove;
        }
    }
    //add alpha eval to hash table
    addMoveTT(hashMove, depth, alpha, hashFlag);

    return alpha;
}

std::string Ai_Logic::sortMoves(std::string moves, HashEntry entry, int currentDepth)
{
    //call killer moves + add killers to front of moves if there are any
    moves = killerHe(currentDepth, moves);
    //perfrom look up from transpositon table
    if(entry.move.length() == 4 && entry.zobrist == zobKey){
        std::string m;
        //if entry is a beta match, add beta cutoff move to front of moves
        if(entry.flag == 2){
            m = entry.move;
            moves = m + moves;
        //if entry is an exact alpha match add move to front
        }else if(entry.flag == 3){
            m = entry.move;
            moves = m + moves;
        }
    }
    return moves;
}

std::string Ai_Logic::killerHe(int depth, std::string moves)
{
    std::string cutoffs, tempMove, tempMove1;
    int size = killerHArr[depth].size();

    //if no killer moves, return the same list of moves taken
    if(size == 0){
        return moves;
    }

    //loop through killer moves at given depth and test if they're legal
    //(done by testing if they're in move list that's already been legally generated)
    for(int i = 0; i < size ; ++i){
        //grab first killer move for given depth
        tempMove = killerHArr[depth].top();
        for(int j = 0; j < moves.size(); j+=4){
            tempMove1 = moves[j];
            tempMove1 += moves[j+1];
            tempMove1 += moves[j+2];
            tempMove1 += moves[j+3];
            //if killer move matches a move in the moveset for current turn, put it at the front of the list
            if(tempMove == tempMove1){
                cutoffs += tempMove;
                break;
            }
        }
        //remove killer move from depth stack once it's been tested to see if legal or not
        killerHArr[depth].pop();
    }

    //return moves with killer moves in the front
    cutoffs += moves;
    return cutoffs;

}

int Ai_Logic::quiescent(int alpha, int beta, bool isWhite, int currentDepth, int quietDepth)
{
    static int depth = currentDepth;
    int standingPat;

    //transposition hash quiet
    int hash = (int)(zobKey % 338207);
    HashEntry entry = transpositionTQuiet[hash];

    if(entry.depth >= quietDepth && entry.zobrist == zobKey){
        //return either the eval, the beta, or the alpha depending on flag
        switch(entry.flag){
            case 3:
            if(entry.flag == 3){
                return entry.eval;
            }
            break;
            case 2:
            if(entry.eval >= beta){
                return beta;
            }
            break;
            case 1:
            if(entry.eval <= alpha){
                return alpha;
            }
            break;
        }
    }

    //evaluate board position (if curentDepth is even return -eval)

    standingPat = eval->evalBoard(isWhite);


    if(quietDepth == 0){
        return standingPat;
    }


    if(standingPat >= beta){
       return beta;
    }

    if(alpha < standingPat){
       alpha = standingPat;
    }

    std::string moves = newBBBoard->genWhosMove(isWhite);

    U64 enemys;
    std::string tempMove, captures = "";

    //create bitboard of enemys
    if(isWhite){
        enemys = BBBlackPieces & ~BBBlackKing;
    } else {
        enemys = BBWhitePieces & ~BBWhiteKing;
    }

    int x1, y1, xyE;
    U64 pieceMaskE;

    //search through all moves from turn for captures
    for(int i = 0; i < moves.length(); i += 4){
        pieceMaskE = 0LL;

        tempMove = "";
        tempMove += moves[i];
        tempMove += moves[i+1];
        tempMove += moves[i+2];
        tempMove += moves[i+3];
        //find number representing board  xy
        x1 = tempMove[2]-0; y1 = tempMove[3]-0;
        xyE = y1*8+x1;
        //create mask of move end position
        pieceMaskE += 1LL << xyE;
        //if move ends on an enemy, it's a capture
        if(pieceMaskE & enemys){
            captures += tempMove;
        }
    }


//newBBBoard->drawBBA();

    //std::string captures = newBBBoard->generateCaptures(isWhite), tempMove;

    //if there are no captures, return value of board
    if(captures.length() == 0){
        return standingPat;
    }

    captures = mostVVLVA(captures, isWhite);
    sortMoves(captures, entry, currentDepth);

    int score;
    std::string unmake, hashMove;
    //set hash flag equal to alpha Flag
    int hashFlag = 1;

    for(int i = 0; i < captures.length(); i+=4)
    {
        qcount ++;
        tempMove = "";
        //convert move into a single string
        tempMove += captures[i];
        tempMove += captures[i+1];
        tempMove += captures[i+2];
        tempMove += captures[i+3];

        unmake = newBBBoard->makeMove(tempMove);

        score = -quiescent(-beta, -alpha, ! isWhite, currentDepth+1, quietDepth-1);

        newBBBoard->unmakeMove(unmake);

        if(score >= beta){
            //add beta to transpositon table with beta flag
            addTTQuiet(tempMove, quietDepth, beta, 2);

            //push killer move to top of stack for given depth
            killerHArr[currentDepth].push(tempMove);
           return beta;
        }

        if(score > alpha){
           alpha = score;
           //if we've gained a new alpha set hash Flag equal to exact
           hashFlag = 3;
           hashMove = tempMove;
        }
    }
    //add alpha eval to hash table
    addTTQuiet(hashMove, quietDepth, alpha, hashFlag);
    return alpha;
}

std::string Ai_Logic::mostVVLVA(std::string captures, bool isWhite)
{
    //arrays holding different captures 0 position for pawn captures, 1 = knight, 2 = bishops, 3 = rook, 4 = queen captures
    std::string pawnCaps[5];
    std::string knightCaps[5];
    std::string bishopCaps[5];
    std::string rookCaps[5];
    std::string queenCaps[5];
    std::string kingCaps[5];

    U64 epawns, eknights, ebishops, erooks, equeens, pawns, knights, bishops, rooks, queens, king;
    //set enemy bitboards and friendly piece bitboards
    if(isWhite){
        //enemies
        epawns = BBBlackPawns;
        eknights = BBBlackKnights;
        ebishops = BBBlackBishops;
        erooks = BBBlackRooks;
        equeens = BBBlackQueens;
        //friendlys
        pawns = BBWhitePawns;
        knights = BBWhiteKnights;
        bishops = BBWhiteBishops;
        rooks = BBWhiteRooks;
        queens = BBWhiteQueens;
        king = BBWhiteKing;
    } else {
        epawns = BBWhitePawns;
        eknights = BBWhiteKnights;
        ebishops = BBWhiteBishops;
        erooks = BBWhiteRooks;
        equeens = BBWhiteQueens;

        pawns = BBBlackPawns;
        knights = BBBlackKnights;
        bishops = BBBlackBishops;
        rooks = BBBlackRooks;
        queens = BBBlackQueens;
        king = BBBlackKing;
    }


    int x, y, x1, y1, xyI, xyE;
    U64 pieceMaskI = 0LL, pieceMaskE = 0LL;
    std::string tempMove;

    //string for holding all non captures, to be added last to moves
    std::string nonCaptures;

    for(int i = 0; i < captures.length(); i+=4)
    {
        pieceMaskI = 0LL, pieceMaskE = 0LL;
        tempMove = "";
        //convert move into a single string
        tempMove += captures[i];
        tempMove += captures[i+1];
        tempMove += captures[i+2];
        tempMove += captures[i+3];
        x = tempMove[0]-0; y = tempMove[1];
        xyI = y*8+x;
        pieceMaskI += 1LL << xyI;
        //find number representing board end position
        x1 = tempMove[2]-0; y1 = tempMove[3]-0;
        xyE = y1*8+x1;
        //create mask of move end position
        pieceMaskE += 1LL << xyE;

        //if the initial piece is our pawn check which piece he captures and add move to appropriate array
        if(pieceMaskI & pawns){
            if(pieceMaskE & epawns){
                pawnCaps[0]+= tempMove;
            } else if(pieceMaskE & eknights){
                pawnCaps[1]+= tempMove;
            } else if(pieceMaskE & ebishops){
                pawnCaps[2]+= tempMove;
            } else if(pieceMaskE & erooks){
                pawnCaps[3]+= tempMove;
            } else if(pieceMaskE & equeens){
                pawnCaps[4]+= tempMove;
            }
        } else if (pieceMaskI & knights){
            if(pieceMaskE & epawns){
                knightCaps[0]+= tempMove;
            } else if(pieceMaskE & eknights){
                knightCaps[1]+= tempMove;
            } else if(pieceMaskE & ebishops){
                knightCaps[2]+= tempMove;
            } else if(pieceMaskE & erooks){
                knightCaps[3]+= tempMove;
            } else if(pieceMaskE & equeens){
                knightCaps[4]+= tempMove;
            }
        } else if (pieceMaskI & bishops){
            if(pieceMaskE & epawns){
                bishopCaps[0]+= tempMove;
            } else if(pieceMaskE & eknights){
                bishopCaps[1]+= tempMove;
            } else if(pieceMaskE & ebishops){
                bishopCaps[2]+= tempMove;
            } else if(pieceMaskE & erooks){
                bishopCaps[3]+= tempMove;
            } else if(pieceMaskE & equeens){
                bishopCaps[4]+= tempMove;
            }
        } else if (pieceMaskI & rooks){
            if(pieceMaskE & epawns){
                rookCaps[0]+= tempMove;
            } else if(pieceMaskE & eknights){
                rookCaps[1]+= tempMove;
            } else if(pieceMaskE & ebishops){
                rookCaps[2]+= tempMove;
            } else if(pieceMaskE & erooks){
                rookCaps[3]+= tempMove;
            } else if(pieceMaskE & equeens){
                rookCaps[4]+= tempMove;
            }
        } else if (pieceMaskI & queens){
            if(pieceMaskE & epawns){
                queenCaps[0]+= tempMove;
            } else if(pieceMaskE & eknights){
                queenCaps[1]+= tempMove;
            } else if(pieceMaskE & ebishops){
                queenCaps[2]+= tempMove;
            } else if(pieceMaskE & erooks){
                queenCaps[3]+= tempMove;
            } else if(pieceMaskE & equeens){
                queenCaps[4]+= tempMove;
            }
        } else if (pieceMaskI & king){
            if(pieceMaskE & epawns){
                kingCaps[0]+= tempMove;
            } else if(pieceMaskE & eknights){
                kingCaps[1]+= tempMove;
            } else if(pieceMaskE & ebishops){
                kingCaps[2]+= tempMove;
            } else if(pieceMaskE & erooks){
                kingCaps[3]+= tempMove;
            } else if(pieceMaskE & equeens){
                kingCaps[4]+= tempMove;
            }
        //add non capture to string so they can be added last
        } else{
            nonCaptures += tempMove;
        }

    }

    //add all captures in order of least valuable attacker most valuable victim
    std::string orderedCaptures;

    orderedCaptures += pawnCaps[4];
    orderedCaptures += pawnCaps[3];
    orderedCaptures += pawnCaps[2];
    orderedCaptures += pawnCaps[1];

    orderedCaptures += knightCaps[4];
    orderedCaptures += bishopCaps[4];
    orderedCaptures += rookCaps[4];
    orderedCaptures += kingCaps[4];
    orderedCaptures += knightCaps[3];
    orderedCaptures += bishopCaps[3];
    orderedCaptures += knightCaps[2];
    orderedCaptures += bishopCaps[2];
    orderedCaptures += knightCaps[1];
    orderedCaptures += bishopCaps[1];
    orderedCaptures += rookCaps[3];
    orderedCaptures += kingCaps[3];
    orderedCaptures += rookCaps[2];
    orderedCaptures += pawnCaps[0];
    orderedCaptures += rookCaps[1];
    orderedCaptures += knightCaps[0];
    orderedCaptures += bishopCaps[0];
    orderedCaptures += rookCaps[0];
    orderedCaptures += kingCaps[2];
    orderedCaptures += kingCaps[1];
    orderedCaptures += kingCaps[0];

    orderedCaptures += nonCaptures;

 return orderedCaptures;

}

void Ai_Logic::addMoveTT(std::string bestmove, int depth, long eval, int flag)
{
    //get hash of current zobrist key
    int hash = (int)(zobKey % 15485843);

    if(transpositionT[hash].depth <= depth){
        //add position to the table
        transpositionT[hash].zobrist = zobKey;
        transpositionT[hash].depth = depth;
        transpositionT[hash].eval = (int)eval;
        transpositionT[hash].move = bestmove;
        transpositionT[hash].flag = flag;

    }

}

void Ai_Logic::addTTQuiet(std::string bestmove, int quietDepth, long eval, int flag)
{
    //get hash of current zobrist key
    int hash = (int)(zobKey % 338207);

    if(transpositionTQuiet[hash].depth <= quietDepth){
        //add position to the table
        transpositionTQuiet[hash].zobrist = zobKey;
        transpositionTQuiet[hash].depth = quietDepth;
        transpositionTQuiet[hash].eval = (int)eval;
        transpositionTQuiet[hash].move = bestmove;
        transpositionTQuiet[hash].flag = flag;

    }
}

std::string Ai_Logic::debug(std::string ttMove, std::string moves)
{

    std::string tempMove = ttMove, tempMove1;
    for(int j = 0; j < moves.size(); j+=4){
        tempMove1 = moves[j];
        tempMove1 += moves[j+1];
        tempMove1 += moves[j+2];
        tempMove1 += moves[j+3];
        if(tempMove == tempMove1){
            tempMove += moves;
            return tempMove;
        }
    }

    if(ttMove.length() > 0 && moves != ""){
        std::cout << zobKey << std::endl;
        newBBBoard->drawBBA();
    }

    return moves;
}

int Ai_Logic::principleV(int depth, int alpha, int beta, bool isWhite, int currentDepth)
{
    int bestScore;
    int bestMoveIndex = 0;


    if(depth == 0){
        int bestScore = eval->evalBoard(isWhite);

        return bestScore;
    }

    std::string moves, tempBBMove;

    moves = newBBBoard->genWhosMove(isWhite);

    //Do full depth search of first move
    std::string tempMove;
    //convert move into a single string
    tempMove += moves[0];
    tempMove += moves[1];
    tempMove += moves[2];
    tempMove += moves[3];

    //make move on BB's store data to string so move can be undone
    tempBBMove = newBBBoard->makeMove(tempMove);

    //jump to other color and evaluate all moves that don't cause a cutoff if depth is greater than 1
    bestScore = -principleV(depth-1, -beta, -alpha,  ! isWhite, currentDepth+1);

    positionCount ++;

    //undo move on BB's
    newBBBoard->unmakeMove(tempBBMove);

    //NEED a mate check here

    //if move is too good to be true
    if(bestScore >= beta){
        return bestScore;
    }

    //if the moves is an improvement
    if(bestScore > alpha){   
        //if it's better but not too good to be true
        alpha = bestScore;

    }


    //Do a shallower zero window search of moves other than the first, if they're better
    //run a full PV search
    for(int i = bestMoveIndex+4; i < moves.length(); i += 4){
        int score;
        positionCount ++;
        std::string move;
        //convert move into a single string
        move += moves[i];
        move += moves[i+1];
        move += moves[i+2];
        move += moves[i+3];
        //make move
        tempBBMove = newBBBoard->makeMove(tempMove);

        //zero window search
        score = -zWSearch(depth -1, -alpha, !isWhite);

        //unmake move
        newBBBoard->unmakeMove(tempBBMove);

        if((score > alpha) && (score < beta)){

            bestScore = -principleV(depth-1, -beta, -alpha, !isWhite, currentDepth+1);

            if(score > alpha){
                bestMoveIndex = i;
                alpha = score;
            }
        }
        if(score > bestScore){

            if(score >= beta){
                return score;
            }

            bestScore = score;

            //need checkmate check
        }
    }
     return bestScore;
}

int Ai_Logic::zWSearch(int depth, int beta, bool isWhite)
{
    int score = -999999;
    //alpha = beta-1;

    if(depth == 0){
        score = eval->evalBoard(isWhite);
        return score;
    }

    std::string moves, tempBBMove;
    moves = newBBBoard->genWhosMove(isWhite);

    for(int i = 4; i < moves.length(); i += 4){
        std::string move;
        //convert move into a single string
        move += moves[i];
        move += moves[i+1];
        move += moves[i+2];
        move += moves[i+3];

        tempBBMove = newBBBoard->makeMove(move);

        score = -zWSearch(depth - 1, 1 - beta, ! isWhite);

        newBBBoard->unmakeMove(tempBBMove);

        if(score >= beta){

            return score; // fail hard beta cutoff
        }

    }
    return beta-1; //same as alpha
}
