#include <algorithm>

#include "State.h"

static char pieceChars[] = "NBRQnbrq";
static const Piece knight = 0, bishop = 1, rook = 2, queen = 3;
static Field *moves[4][50][9];       // piece, pos + 1, rays
static BitBoard blockers[49][4][50]; // pos_queen, piece, pos_piece + 1

std::string moveToString(Move move)
{
    char buf[4] = {
        pieceChars[MOVE_PCE(move)],
        char('a' + MOVE_DST(move)%7),
        char('1' + MOVE_DST(move)/7) };
    return buf;
}

void State::initialize()
{
    // Used for 'moves' table.
    static Field spots[2*49];
    static Field rays[8][49 + 7 + 6];

    // Set up rays/spots
    for(int pos = 0; pos < 49; ++pos)
    {
        spots[2*pos + 0] = pos;
        spots[2*pos + 1] = -1;
    }
    const int dr[8] = { +1, +1,  0, -1, -1, -1,  0, +1 };
    const int dc[8] = {  0, +1, +1, +1,  0, -1, -1, -1 };
    for(int d = 0; d < 8; ++d)
    {
        int n = 0;
        bool used[7][7] = { };
        for(int br = 0; br < 7; ++br)
            for(int bc = 0; bc < 7; ++bc)
            {
                int r = br - dr[d], c = bc - dc[d];
                if(r >= 0 && r < 7 && c >= 0 && c < 7)
                    continue;

                r = br, c = bc;
                while(r >= 0 && r < 7 && c >= 0 && c < 7)
                {
                    rays[d][n++] = 7*r + c;
                    used[r][c] = true;
                    r += dr[d], c += dc[d];
                }
                rays[d][n++] = -1;
            }
    }
    
    // Set up move tables for knight
    *moves[0][0] = NULL;
    for(int r = 0; r < 7; ++r)
        for(int c = 0; c < 7; ++c)
        {
            Field **f = moves[0][7*r + c + 1];
            const int dr[8] = { +2, +1, +2, -1, -2, +1, -2, -1 };
            const int dc[8] = { +1, +2, -1, +2, +1, -2, -1, -2 };
            for(int d = 0; d < 8; ++d)
            {
                int nr = r + dr[d], nc = c + dc[d];
                if(nr >= 0 && nr < 7 && nc >= 0 && nc < 7)
                    *(f++) = spots + 2*(7*nr + nc);
            }
            *f = NULL;
        }
    
    // Set up move tables for rook, bishop, queen
    for(int pce = 1; pce < 4; ++pce)
    {
        *moves[pce][0] = NULL;
        for(int pos = 0; pos < 49; ++pos)
        {
            Field **f = moves[pce][pos + 1];
            for(int d = (pce == bishop ? 1 : 0); d < 8; d += (pce == queen ? 1 : 2))
            {
                Field *r = rays[d];
                while(*r != pos) ++r;
                if(*++r)
                    *(f++) = r;
            }
            *f = NULL;
        }
    }

    // Set up blockers table
    for(int q = 0; q < 49; ++q)
        for(int pce = 0; pce < 4; ++pce)
            for(int p = 0; p < 50; ++p)
                blockers[q][pce][p] = 1ll << 50;

    for(int pce = 0; pce < 4; ++pce)
        for(int pos = 0; pos < 49; ++pos)
        {
            for(Field **m = moves[pce][1 + pos]; *m; ++m)
            {
                BitBoard between = 0;
                for(Field *f = *m; *f >= 0; ++f)
                {
                    blockers[*f][pce][1 + pos] = between;
                    between |= (1ll << *f);
                }
            }
        }
}

State State::initial()
{
    State s = {
        0,
        { { 7*1 + 1, 7*1 + 3, 7*2 + 2, 7*1 + 2 },
            { 7*5 + 5, 7*5 + 3, 7*4 + 4, 7*5 + 4 } },
        false, false, -1 };
    return s;
}

Hash State::hash() const
{
    Hash h;
    h.reset();
    h.add((unsigned char*)this, 17);
    return h;
}

BitBoard State::getBlockingPieces() const
{
    return (1ll << 50)
        | (1ll << pos[0][knight]) | (1ll << pos[0][bishop]) | (1ll << pos[0][rook])
        | (1ll << pos[1][knight]) | (1ll << pos[1][bishop]) | (1ll << pos[1][rook]);
}
    
void State::setCheck()
{
    BitBoard blockingPieces = getBlockingPieces();
    check = (blockingPieces&blockers[pos[0][queen]][knight][1 + pos[1][knight]]) == 0 ||
            (blockingPieces&blockers[pos[0][queen]][bishop][1 + pos[1][bishop]]) == 0 ||
            (blockingPieces&blockers[pos[0][queen]][rook  ][1 + pos[1][rook  ]]) == 0;
}
    
State State::perform(Move move) const
{
    Field mdst = MOVE_DST(move);
    Piece mpce = MOVE_PCE(move);

    State r = {
        holes,
        { { pos[1][0], pos[1][1], pos[1][2], pos[1][3] },
            { pos[0][0], pos[0][1], pos[0][2], pos[0][3] } },
        0, check,
        -1 };

    // Create/move holes
    if(mpce == queen)
        r.holes |= (1ll << pos[0][queen]);
    else
    if(r.holes & (1ll << mdst))
        r.holes ^= (1ll << mdst) | (1ll << r.pos[1][mpce]);

    // Swap piece
    for(int pce = 0; pce < 8; ++pce)
        if(r.pos[0][pce] == mdst)
        {
            if(mpce == queen)
                r.pos[0][pce] = -1;
            else
            {
                r.pos[0][pce] = r.pos[1][mpce];
                r.last = move;
            }
        }

    // Finally, move piece
    r.pos[1][mpce] = mdst;

    // Compute check
    r.setCheck();
    return r;
}

int State::generateMoves(Move *move_list) const
{
    int count = 0;
    BitBoard myPieces    = (1ll << pos[0][0]) | (1ll << pos[0][1])
                            | (1ll << pos[0][2]) | (1ll << pos[0][3]),
                enemyPieces = (1ll << pos[1][0]) | (1ll << pos[1][1])
                            | (1ll << pos[1][2]) | (1ll << pos[1][3]);

    BitBoard blockingPieces = getBlockingPieces();
    for(int pce = 0; pce < 4; ++pce)
    {
        for(Field **m = moves[pce][1 + pos[0][pce]]; *m; ++m)
        {
            bool blocked = false;
            for(Field *f = *m; *f >= 0 && !blocked; ++f)
            {
                BitBoard x = 1ll << *f;
                if( (blocked = x&myPieces) &&
                    (pce == queen || *f == pos[0][queen]) )
                {
                    break;  // forbid swapping with queen
                }

                // Forbid queen from moving to hole
                if(pce != queen || (x&holes) == 0)
                {
                    // Calculate new blocking pieces...
                    BitBoard newBlockingPieces = blockingPieces | (1<<*f);
                    if(pce == queen || blockingPieces != newBlockingPieces)
                        newBlockingPieces ^=  1ll << pos[0][pce];
    
                    // Check for self-check
                    Field queenPos = (pce == queen) ? *f : pos[0][queen];
                    if( (newBlockingPieces&blockers[queenPos][0][1 + pos[1][0]]) &&
                        (newBlockingPieces&blockers[queenPos][1][1 + pos[1][1]]) &&
                        (newBlockingPieces&blockers[queenPos][2][1 + pos[1][2]]) &&
                        (newBlockingPieces&blockers[queenPos][3][1 + pos[1][3]]) )
                    {
                        // TODO: connection rule!!!
    
                        Move move = MOVE(pce, *f);
                        if(move != last)
                            move_list[count++] = move;
                    }
                }

                blocked = blocked || (x&enemyPieces)
                    || (pce == queen && check);   // reduced movement in check
            }
        }
    }
    return count;
}

int State::countValidMoves() const
{
    Move move_list[MAX_MOVES];
    return generateMoves(move_list);
}

bool State::gameOver() const
{
    return countValidMoves() == 0;
}

Value State::evaluate() const
{
    int valid_moves = countValidMoves();
    if(valid_moves == 0)
        return check ? -100 : 0;

    // Number of moves available
    Value r = valid_moves/2;

    // Material value (knight, bishop, rook)
    Value value[3] = { 30, 20, 40 };
    for(int pce = 0; pce < 3; ++pce)
    {
        r += value[pce]*(pos[0][pce] != -1);
        r -= value[pce]*(pos[0][pce] != -1);
    }

    // Queen-aligned bishop
    r += 2*((pos[0][bishop]&1) == (pos[1][queen]&1));
    r -= 2*((pos[1][bishop]&1) == (pos[0][queen]&1));

    // Pinned piece (approximation!)
    BitBoard pieces[2] = {
        (1ll << pos[0][0]) | (1ll << pos[0][1]) | (1ll << pos[0][2]),
        (1ll << pos[1][0]) | (1ll << pos[1][1]) | (1ll << pos[1][2]) };
    r -= 2*(pieces[0]&blockers[pos[0][queen]][queen][1 + pos[1][queen]]);
    r += 2*(pieces[1]&blockers[pos[0][queen]][queen][1 + pos[1][queen]]);

    // Free fields around queen
    // TODO?
    const BitBoard edge = 560802875597055ll;
    r -= 3*((edge >> pos[0][queen])&1);
    r += 3*((edge >> pos[1][queen])&1);
            

    return r;
}


std::ostream &operator<<(std::ostream &os, const State &s)
{
    char c[7][7];
    for(int p = 0; p < 49; ++p)
        c[0][p] = ((s.holes >> p)&1) ? '*' : '.';

    for(int n = 0; n < 8; ++n)
        if(s.pos[0][n] >= 0)
            c[0][s.pos[0][n]] = pieceChars[n];

    for(int r = 6; r >= 0; --r)
    {
        os << char('1' + r);
        os.write(c[r], 7);
        os << '\n';
    }
    os << ' ';
    for(int c = 0; c < 7; ++c)
        os << char('A' + c);
    return os << '\n';
}
