#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include <iostream>
#include <string>

typedef long long BitBoard;
typedef char Field, Piece, Move, Value, Depth;

#define MOVE(piece, dest) (((dest)<<2)|(piece))
#define MOVE_PCE(move) ((move)&3)
#define MOVE_DST(move) (((move)>>2)&63)
#define MOVE_STR(move) (moveToString(move))

#define MAX_MOVES (56)

std::string moveToString(Move move);

struct Hash
{
    unsigned long long major;
    unsigned long      minor;

    inline void reset()
    {
        major = 14695981039346656037ull;
        minor = 2166136261;
    }

    void add(unsigned char *data, int count)
    {
        while(count--)
        {
            major ^= *data;
            major *= 1099511628211ull;
    
            minor ^= *data;
            major *= 16777619ll;
            ++data;
        }
    }
} __attribute__((packed));


struct State
{
    BitBoard holes;     // 8 bytes
    Field pos[2][4];    // 8 bytes
    bool check, last_check; // 2 bytes
    Move last;          // 1 byte

protected:
    inline BitBoard getBlockingPieces() const;
    inline void setCheck();
    inline int countValidMoves() const;

public:
    static void initialize();
    static State initial();

    bool gameOver() const;
    Hash hash() const;
    State perform(Move move) const;
    int generateMoves(Move *move_list) const;
    Value evaluate() const;
};

std::ostream &operator<<(std::ostream &os, const State &s);

#endif /* ndef STATE_H_INCLUDED */
