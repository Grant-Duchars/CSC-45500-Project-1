/*******************************************
 * File: Token.cpp                         *
 * Author: S. Blythe                       *
 * Date: 12/2022                           *
 * PURPOSE: implementation for Token       *
 *******************************************/

#include "Token.hpp"

#include <fstream>
#include <iomanip>

const int _NUM_STATES = 20;
const int _NUM_CHARS = 256;
char _BUFFER[512];

using namespace std;

// the promised global for string equivalents of TokenType enumeration
string TokStr[] =
    {"ERROR", "EOF_TOK", "NUM_INT", "NUM_REAL", "ADDOP", "MULOP", "ID", "RELOP", "ASSIGNOP", "LPAREN", "RPAREN", "SEMICOLON", "LBRACK", "RBRACK", "COMMA", "AND", "OR", "INTEGER", "FLOAT", "WHILE", "IF", "THEN", "ELSE", "VOID", "BEGIN", "END"};

// This is a "list" of the keywords. Note that they are in the same order
//   as found on the TokenType enumaration.
static string reserved[] = {"int", "float", "while", "if", "then", "else", "void", "begin", "end"};

/******************************************************
 *  just prints out the info describing this Token    *
 *    to specified stream                             *
 *                                                    *
 *   os  - the stream to add the Token to             *
 *                                                    *
 *   returns: the updated stream                      *
 ******************************************************/
ostream &
Token::print(ostream &os) const
{
    os
        << "{ Type:" << left << setw(10) << TokStr[_type]
        << " Value:" << left << setw(10) << _value
        << " Line Number:" << _line_num
        << " }";
    return os;
}

int **DFA = nullptr;
TokenType *state_to_type = nullptr;

/**
 * Builds the DFA (obviously)
 */
void build_DFA()
{
    // Build DFA
    DFA = new int *[_NUM_STATES];
    for (int state = 0; state < _NUM_STATES; state++)
    {
        DFA[state] = new int[_NUM_CHARS];
    }
    fill_n(&DFA[0][0], _NUM_STATES * _NUM_CHARS, -1);
    // State 0
    fill(&DFA[0][(int)'A'], &DFA[0][(int)'Z'], 1);
    fill(&DFA[0][(int)'a'], &DFA[0][(int)'z'], 1);
    fill(&DFA[0][(int)'0'], &DFA[0][(int)'9'], 2);
    DFA[0][(int)'+'] = 5;
    DFA[0][(int)'-'] = 5;
    DFA[0][(int)'*'] = 6;
    DFA[0][(int)'/'] = 6;
    DFA[0][(int)'<'] = 7;
    DFA[0][(int)'>'] = 7;
    DFA[0][(int)'='] = 9;
    DFA[0][(int)'('] = 10;
    DFA[0][(int)')'] = 11;
    DFA[0][(int)'&'] = 12;
    DFA[0][(int)'|'] = 14;
    DFA[0][(int)';'] = 16;
    DFA[0][(int)'['] = 17;
    DFA[0][(int)']'] = 18;
    DFA[0][(int)','] = 19;
    // State 1
    fill(&DFA[1][(int)'A'], &DFA[1][(int)'Z'], 1);
    fill(&DFA[1][(int)'a'], &DFA[1][(int)'z'], 1);
    fill(&DFA[1][(int)'0'], &DFA[1][(int)'9'], 1);
    // State 2
    fill(&DFA[2][(int)'0'], &DFA[2][(int)'9'], 2);
    DFA[2][(int)'.'] = 3;
    // State 3
    fill(&DFA[3][(int)'0'], &DFA[3][(int)'9'], 4);
    // State 4
    fill(&DFA[4][(int)'0'], &DFA[4][(int)'9'], 4);
    // State 7
    DFA[7][(int)'='] = 8;
    // State 9
    DFA[9][(int)'='] = 8;
    // State 12
    DFA[12][(int)'&'] = 13;
    // State 14
    DFA[14][(int)'|'] = 15;

    // Build state_to_type
    state_to_type = new TokenType[20];
    state_to_type[0] = TokenType::ERROR;
    state_to_type[1] = TokenType::ID;
    state_to_type[2] = TokenType::NUM_INT;
    state_to_type[3] = TokenType::ERROR;
    state_to_type[4] = TokenType::NUM_REAL;
    state_to_type[5] = TokenType::ADDOP;
    state_to_type[6] = TokenType::MULOP;
    state_to_type[7] = TokenType::RELOP;
    state_to_type[8] = TokenType::RELOP;
    state_to_type[9] = TokenType::ASSIGNOP;
    state_to_type[10] = TokenType::LPAREN;
    state_to_type[11] = TokenType::RPAREN;
    state_to_type[12] = TokenType::ERROR;
    state_to_type[13] = TokenType::AND;
    state_to_type[14] = TokenType::ERROR;
    state_to_type[15] = TokenType::OR;
    state_to_type[16] = TokenType::SEMICOLON;
    state_to_type[17] = TokenType::LBRACK;
    state_to_type[18] = TokenType::RBRACK;
    state_to_type[19] = TokenType::COMMA;
}

/**
 * Skips any whitespace and puts back the first character comsumed.
 * @return True if end of input stream
 */
bool skip_whitespace(istream &is)
{
    char c;
    is >> c;
    if (is)
        is.putback(c);
    return (!is) ? true : false;
}

/******************************************************
 *  Fills in information about this Token by reading  *
 *    it from specified input stream                  *
 *                                                    *
 *   is  - the stream to read the Token from          *
 *                                                    *
 *   returns: nothing                                 *
 *                                                    *
 *     **** YOU MUST CODE THIS !!!!!! ****            *
 ******************************************************/
void Token::get(istream &is)
{
    // Check for and build DFA if needed
    if (DFA == nullptr)
    {
        build_DFA();
        _line_num = 1;
    }

    // Reset value of token
    _value = "";

    char curr_char = is.get();
    while (curr_char == '\n' || curr_char == '#')
    {
        if (curr_char == '#')
            is.getline(_BUFFER, 512);
        curr_char = is.get();
        _line_num++;
    }
    if (is)
        is.putback(curr_char);

    // Skip to first charcter on the current line
    if (skip_whitespace(is))
    {
        _type = TokenType::EOF_TOK;
        return;
    }

    // Start processing characters
    int curr_state = 0;
    int prev_state = -1;
    while (curr_state != -1)
    {
        curr_char = is.get();
        // Update states
        prev_state = curr_state;
        curr_state = DFA[curr_state][(int)curr_char];
        if (curr_state != -1)
            _value += curr_char;
    }

    // Last character read is not part of token due to current state so put it back
    if (is)
        is.putback(curr_char);

    // Check for reserved
    if (prev_state == 1)
    {
        for (int i = 0; i < 9; i++)
        {
            if (reserved[i] == _value)
            {
                _type = static_cast<TokenType>(TokenType::INTEGER + i);
                return;
            }
        }
    }
    _type = state_to_type[prev_state];
}