#pragma once
#include "includes.h"

class Chess
{
public:
   static int getcolor( char chPiece );

   static bool iswhite ( char chPiece );

   static bool isblack( char chPiece );

   static std::string describePiece( char chPiece );

   enum PieceColor
   {
      white = 0,
      black = 1
   };

   enum Player
   {
      whiteP = 0,
      blackP = 1
   };

   enum Side
   {
      queens = 2,
      kings  = 3
   };

   enum Direction
   {
      horiz = 0,
      vert,
      diag,
      lshape
   };

   struct Position
   {
      int iRow;
      int iColumn;
   };

   struct EnPassant
   {
      bool bApplied;
      Position PawnCaptured;
   };

   struct Castling
   {
      bool bApplied;
      Position rook_before;
      Position rook_after;
   };

   struct Promotion
   {
      bool bApplied;
      //Position  pos;
      char chBefore;
      char chAfter;
   };

   struct IntendedMove
   {
      char chPiece;
      Position from;
      Position to;
   };

   struct Attacker
   {
      Position  pos;
      Direction dir;
   };

   struct UnderAttack
   {
      bool bUnderAttack;
      int iNumAttackers;
      Attacker attacker[9]; //maximum theorical number of attackers
   };

   const char initial_board[8][8] =
   {
      // This represents the pieces on the board.
      // Keep in mind that pieces[0][0] represents A1
      // pieces[1][1] represents B2 and so on.
      // Letters in CAPITAL are white
      { 'R',  'N',  'B',  'Q',  'K',  'B',  'N',  'R' },
      { 'P',  'P',  'P',  'P',  'P',  'P',  'P',  'P' },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 'p',  'p',  'p',  'p',  'p',  'p',  'p',  'p' },
      { 'r',  'n',  'b',  'q',  'k',  'b',  'n',  'r' },
   };
};

class Game : Chess
{
public:
   Game();
   ~Game();

   void movePiece( Position present, Position future, Chess::EnPassant* S_enPassant, Chess::Castling* S_castling, Chess::Promotion* S_promotion );

   void undoLastMove();

   bool undoIsPossible();

   bool castlingAllowed( Side iSide, int iColor );

   char pieceAtPos( int iRow, int iColumn );

   char pieceAtPos( Position pos );

   char getPiece_considerMove( int iRow, int iColumn, IntendedMove* intended_move = nullptr );

   UnderAttack isUnderAttack( int iRow, int iColumn, int iColor, IntendedMove* pintended_move = nullptr );

   bool isReachable( int iRow, int iColumn, int iColor );

   bool isSquareOccupied( int iRow, int iColumn );

   bool isPathFree( Position startingPos, Position finishingPos, int iDirection );

   bool canBeBlocked( Position startingPos, Position finishinPos, int iDirection );

   bool isCheckMate();

   bool isKingInCheck( int iColor, IntendedMove* intended_move = nullptr );

   bool playerKingInCheck( IntendedMove* intended_move = nullptr );

   bool wouldKingBeInCheck( char chPiece, Position present, Position future, EnPassant* S_enPassant );

   Position findKing( int iColor );

   void change_t( void );

   bool isFinished( void );

   int getCurrentTurn( void );

   int getOpponentColor( void );

   void parseMove( string move, Position* pFrom, Position* pTo, char* chPromoted = nullptr );

   void logMove( std::string &to_record );

   string getLastMove( void );

   void deleteLastMove( void );

   // Save all the moves
   struct Round
   {
      string white_move;
      string black_move;
   };

   //std::deque<std::string> moves;
   std::deque<Round> rounds;

   // Save the captured pieces
   std::vector<char> white_captured;
   std::vector<char> black_captured;

private:

   // Represent the pieces in the board
   char board[8][8];

   // Undo is possible?
   struct Undo
   {
      bool bCanUndo;
      bool bCapturedLastMove;

      bool bcastKingAllowed;
      bool bcastQueenAllowed;

      EnPassant en_passant;
      Castling  castling;
      Promotion promo;
   } m_undo;

   // Castling requirements
   bool mbcastKingAllowed[ 2 ];
   bool mbcastQueenAllowed[ 2 ];

   // Holds the current turn
   int  m_CurrentTurn;

   // Has the game finished already?
   bool m_bGameFinished;
};
