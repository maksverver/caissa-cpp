#include <iostream>
#include <algorithm>

#include "State.h"

struct TTEntry
{
    long long hash_major;       // 8 bytes
    Depth depth;                // 1 byte
    Value alpha, beta;          // 2 bytes
    Move best;                  // 1 byte
} __attribute__((packed));

const int searchExts = 6;

const int ttSize = 67108859;
TTEntry tt[ttSize];

Value alphabeta(const State &s, Depth depth, Depth ext, Value alpha, Value beta)
{
    Hash hash = s.hash();
    TTEntry &te = tt[hash.minor%ttSize];
    Move best = -1;

    if(te.hash_major == hash.major)
    {
        if(te.depth >= depth)
        {
            if(te.alpha >= beta)
                return te.alpha;
            if(te.beta <= alpha)
                return te.beta;
            alpha = std::max(alpha, te.alpha);
            beta  = std::min(beta,  te.beta);
        }
        best = te.best;
    }

    // Search extention
    if(depth == 0 && (s.last_check || s.check) && ext >= 2)
    {
        depth += 2;
        ext   -= 2;
    }

    Value g = -128;
    if(depth > 0)
    {
        Move moves[1 + MAX_MOVES] = { best };
        int moves_size = 1;

        Value a = alpha;
        for(int m = 0; m < moves_size; ++m)
        {
            if(moves[m] != -1)
            {
                State t = s.perform(moves[m]);
                Value v = -alphabeta(t, depth - 1, ext, -beta, -a);
                if(v > g)
                    if((g = v) > a)
                    {
                        a    = g;
                        best = moves[m];
                    }
                if(g >= beta)
                    break;  // beta cut-off!
            }
            
            if(m == 0)
                moves_size = 1 + s.generateMoves(moves + 1);
        }
    }
    
    if(g == -128)
    {
        g = s.evaluate();
        if(g < 0)
            g -= (depth + ext); // include search depth
    }
    
    if(te.depth <= depth)
    {
        te.hash_major = hash.major;
        te.depth      = depth;
        if(g >= beta)
        {
            te.alpha = g;
            te.beta  = +127;
        }
        else
        if(g <= alpha)
        {
            te.alpha = -127;
            te.beta  = g;
        }
        else
        {
            te.alpha = te.beta = g;
        }
        te.best       = best;
    }
    
    return g;
}

Value mtdf(const State &state, Depth searchDepth, Value estimatedValue)
{
    Value g = estimatedValue, upperBound = +127, lowerBound = -127;
    do {
        Value beta = (g == lowerBound) ? g + 1 : g;
        g = alphabeta(state, searchDepth, searchExts, beta - 1, beta);
        if(g < beta)
            upperBound = g;
        else
            lowerBound = g;
    } while(lowerBound < upperBound);
    return g;
}

Move bestMove(const State &state, Depth searchDepth)
{
    // TODO: return first move if only one move available

    memset(tt, -1, sizeof(tt)); // clear transposition table -- important!
    Move best = -1;
    Value g = 0;
    for(int depth = 2; depth <= searchDepth; depth += 2)
    {
        //g = alphabeta(state, depth, -127, +127);
        g = mtdf(state, depth, g);

        best = tt[state.hash().minor%ttSize].best;

        // Display intermediate results
        std::cout << "  Depth: " << depth
                  << "  Value: " << (int)g
                  << "  Best: " <<  MOVE_STR(best) << " (" << int(best) << ")";
        if(abs(g) >= 100)
            std::cout << "  Mate in " << ((depth + searchExts) - (abs(g) - 100) + 1)/2;
        std::cout << std::endl;
    }
    // Assumes the root is in the TT and the best move is correctly set!
    return tt[state.hash().minor%ttSize].best;
}

using namespace std;
int main()
{
    State::initialize();
    cout << "Transposition table size: "
         << (sizeof(tt) >> 20) << " MiB ("
         << (ttSize/1000) << "k entries)" << endl;

    State s = State::initial();

    /*
    Move start[] = { 68, 121, 104, -65, 73, 100, 70, 122, 66, 120, 70, 100, 66, 66, 15, 70, 41, 42, -1 };
    for(Move *m = start; *m != -1; ++m)
        s = s.perform(*m);
    */

    cout << s;
    do {
        Move best = bestMove(s, 6);
        cout << MOVE_STR(best) << endl;
        s = s.perform(best);
        cout << s;
    } while(!s.gameOver());

};
