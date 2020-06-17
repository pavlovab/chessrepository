#include "includes.h"
#include "chess.h"
#include "user_interface.h"


// -------------------------------------------------------------------
// Chess class
// -------------------------------------------------------------------
int Chess::getPieceColor(char chPiece)
{
   if (isupper(chPiece))
   {
       //the piece is white
       return WHITE_PIECE ;
   }
   else
   {
       //the piece is black
      return BLACK_PIECE ;
   }
}

bool Chess::isWhitePiece (char chPiece)
{
    //true if the piece is white and false if the piece is black
   return getPieceColor(chPiece) == Chess::WHITE_PIECE  ? true : false;
}

bool Chess::isBlackPiece (char chPiece)
{
    //true if the piece is black and false if the piece is white
   return getPieceColor (chPiece) == Chess::BLACK_PIECE ? true : false;
}

std::string Chess::describePiece (char chPiece)
   {
      std::string description;

      if ( isWhitePiece (chPiece) )
      {
         description += "White ";
      }
      else
      {
         description += "Black ";
      }

      switch ( toupper(chPiece) )
      {
         case 'P':
         {
            description += "pawn";
         }
         break;

         case 'N':
         {
            description += "knight";
         }
         break;

         case 'B':
         {
            description += "bishop";
         }
         break;

         case 'R':
         {
            description += "rook";
         }
         break;

         case 'Q':
         {
            description += "queen";
         }
         break;

         default:
         {
            description += "unknown piece";
         }
         break;
      }
    //return the type of the piece
      return description;
   }


// -------------------------------------------------------------------
// Game class
// -------------------------------------------------------------------
Game::Game()
{
   // White player always starts
   mCurrentTurn = WHITE_PLAYER;

   // Game on!
   mbGameFinished = false;


   mUndo.bCapturedLastMove         = false;
   mUndo.bCanUndo                  = false;
   mUndo.bCastlingKingSideAllowed  = false;
   mUndo.bCastlingQueenSideAllowed = false;
   mUndo.en_passant.bApplied       = false;
   mUndo.castling.bApplied         = false;


   memcpy(board,initial_board,sizeof ( char )*8*8);

   // Castling is allowed (to each side) until the player moves the king or the rook
   mbCastlingKingSideAllowed[WHITE_PLAYER]  = true;
   mbCastlingKingSideAllowed[BLACK_PLAYER ]  = true;

   mbCastlingQueenSideAllowed[WHITE_PLAYER] = true;
   mbCastlingQueenSideAllowed[BLACK_PLAYER ] = true;
}

Game::~Game()
{
   whiteCaptured.clear();
   blackCaptured.clear();
   rounds.clear();
}

void Game::movePiece (Position present,Position future,Chess::EnPassant*S_enPassant,Chess::Castling* S_castling, Chess::Promotion*S_promo)
{

   char chPiece = getPieceAtPosition ( present );

   // Is the destination square occupied?
   char chCapturedPiece = getPieceAtPosition ( future );

   // So, was a piece captured in this move?
   if (0x20 != chCapturedPiece)
   {
      if (WHITE_PIECE  == getPieceColor(chCapturedPiece))
      {

         whiteCaptured.push_back(chCapturedPiece);
      }
      else
      {
         // A black piece was captured
         blackCaptured.push_back(chCapturedPiece);
      }


      mUndo.bCapturedLastMove = true;

      //reset
      memset( &mUndo.en_passant, 0, sizeof( Chess::EnPassant ));
   }
   else if (true == S_enPassant->bApplied)
   {
      char chCapturedEP = getPieceAtPosition(S_enPassant->PawnCaptured.iRow, S_enPassant->PawnCaptured.iColumn);

      if (WHITE_PIECE  == getPieceColor(chCapturedEP))
      {
         // A white piece was captured
         whiteCaptured.push_back(chCapturedEP);
      }
      else
      {
         // A black piece was captured
         blackCaptured.push_back(chCapturedEP);
      }

      // Now, remove the captured pawn
      board[S_enPassant->PawnCaptured.iRow][S_enPassant->PawnCaptured.iColumn] = EMPTY_SQUARE ;

      // Set Undo structure as piece was captured and "en passant" move was performed
      mUndo.bCapturedLastMove = true;
      memcpy ( &mUndo.en_passant, S_enPassant, sizeof ( Chess::EnPassant ) );
   }
   else
   {
      mUndo.bCapturedLastMove   = false;

      // Reset mUndo.castling
      memset ( &mUndo.en_passant,0, sizeof( Chess::EnPassant ));
   }

   // Remove piece
   board[present.iRow][present.iColumn] = EMPTY_SQUARE ;

   // Move piece to new position
   if( true == S_promo->bApplied )
   {
      board[future.iRow][future.iColumn] = S_promo->chAfter;

      // Set Undo structure as a promo occurred
      memcpy(&mUndo.promotion, S_promo, sizeof(Chess::Promotion));
   }
   else
   {
      board[future.iRow][future.iColumn] = chPiece;

      // Reset mUndo.promo
      memset( &mUndo.promotion, 0, sizeof( Chess::Promotion ));
   }


   if( S_castling->bApplied == true  )
   {
      // The king was already move, but we still have to move the rook to 'jump' the king
      char chPiece = getPieceAtPosition(S_castling->rook_before.iRow, S_castling->rook_before.iColumn);

      // Remove the rook from present position
      board[S_castling->rook_before.iRow][S_castling->rook_before.iColumn] = EMPTY_SQUARE ;


      board[ S_castling->rook_after.iRow ][ S_castling->rook_after.iColumn ] =      chPiece;

      // Write this information to the mUndo struct
      memcpy ( &mUndo.castling, S_castling, sizeof( Chess::Castling ) );

      // Save the 'CastlingAllowed' information in case the move is undone
      mUndo.bCastlingKingSideAllowed  = mbCastlingKingSideAllowed[getCurrentTurn ()] ;
      mUndo.bCastlingQueenSideAllowed      = mbCastlingQueenSideAllowed[getCurrentTurn ()];
   }
   else
   {
      // Reset
      memset( &mUndo.castling, 0, sizeof( Chess::Castling ));
   }

   // Castling requirements
   if ( 'K' == toupper ( chPiece ) )
   {
      // After the king has moved once, no more castling allowed
      mbCastlingKingSideAllowed[getCurrentTurn()]  = false;
      mbCastlingQueenSideAllowed[getCurrentTurn()] = false;
   }
   else if ('R' == toupper(chPiece) )
   {
      // If the rook moved from column 'A', no more castling allowed on the queen side
      if ( 0 == present.iColumn )
      {
         mbCastlingQueenSideAllowed[getCurrentTurn()] = false;
      }


      else if ( 7 == present.iColumn )
      {
         mbCastlingKingSideAllowed[getCurrentTurn()] = false;
      }
   }

   // Change turns
   changeTurns ( );

   // This move can be undone
   mUndo.bCanUndo = true;
}

void Game::undoLastMove()
{
    // get the last move
   string last_move = getLastMove();

   // Parse the line
   Chess::Position from;
   Chess::Position to;
   parseMove(last_move, &from, &to);

   // Since we want to undo a move, we will be moving the piece from (iToRow, iToColumn) to (iFromRow, iFromColumn)
   char chPiece = getPieceAtPosition(to.iRow, to.iColumn);

   // Moving it back

   if( true == mUndo.promotion.bApplied )
   {
      board[ from.iRow ][from.iColumn] = mUndo.promotion.chBefore;
   }
   else
   {
      board[from.iRow][from.iColumn] = chPiece;
   }

   // Change turns
   changeTurns();

   // If a piece was captured, move it back to the board
   if ( mUndo.bCapturedLastMove )
   {
      // Retrieve the last captured piece
      char chCaptured;

      // Since we already changed turns back, it means we should pop a piece from the opponents vector
      if (WHITE_PLAYER == mCurrentTurn)
      {
         chCaptured = blackCaptured.back();
         blackCaptured.pop_back();
      }
      else
      {
         chCaptured = whiteCaptured.back();
         whiteCaptured.pop_back();
      }


      if(mUndo.en_passant.bApplied)
      {
         // Move the captured piece back
         board[mUndo.en_passant.PawnCaptured.iRow][mUndo.en_passant.PawnCaptured.iColumn] = chCaptured;

         // Remove the attacker
         board[to.iRow][to.iColumn] = EMPTY_SQUARE ;
      }
      else
      {
         board[to.iRow][to.iColumn] = chCaptured;
      }
   }
   else
   {
      board [to.iRow][to.iColumn]  = EMPTY_SQUARE ;
   }

   // If there was a castling
   if ( mUndo.castling.bApplied )
   {
      char chRook = getPieceAtPosition(mUndo.castling.rook_after.iRow, mUndo.castling.rook_after.iColumn);

      // Remove the rook from present position
      board[mUndo.castling.rook_after.iRow][mUndo.castling.rook_after.iColumn] = EMPTY_SQUARE ;

      // 'Jump' into to new position
      board[mUndo.castling.rook_before.iRow][mUndo.castling.rook_before.iColumn] = chRook;

      // Restore the values of castling allowed or not
      mbCastlingKingSideAllowed[ getCurrentTurn()]  = mUndo.bCastlingKingSideAllowed;
      mbCastlingQueenSideAllowed[ getCurrentTurn()] = mUndo.bCastlingQueenSideAllowed;
   }


   mUndo.bCanUndo             = false;
   mUndo.bCapturedLastMove    = false;
   mUndo.en_passant.bApplied  = false;
   mUndo.castling.bApplied         = false;
   mUndo.promotion.bApplied   = false;

   // If it was a checkmate, toggle back to game not finished
   mbGameFinished = false;

   // Finally, remove the last move from the list
   deleteLastMove();
}

bool Game::undoIsPossible()
{
   return mUndo.bCanUndo;
}

bool Game::castlingAllowed ( Side iSide,int iColor )
{
   if( QUEEN_SIDE  == iSide )
   {
      return mbCastlingQueenSideAllowed[ iColor ];
   }
   else //if ( KING_SIDE == iSide )
   {
      return mbCastlingKingSideAllowed[ iColor ];
   }
}

char Game::getPieceAtPosition( int iRow,int iColumn )
{
   return board[ iRow ][ iColumn ];
}

char Game::getPieceAtPosition( Position pos )
{
   return board[ pos.iRow ][ pos.iColumn ];
}

char Game::getPiece_considerMove ( int iRow,int iColumn,IntendedMove* intended_move )
{
   char chPiece;

   // If there is no intended move, just return the current position of the board
   if ( nullptr == intended_move )
   {
      chPiece = getPieceAtPosition( iRow, iColumn );
   }
   else
   {
      // In this case, we are trying to understand what WOULD happed if the move was made,
      // so we consider a move that has not been made yet
      if( intended_move->from.iRow == iRow && intended_move->from.iColumn == iColumn )
      {
         // The piece wants to move from that square, so it would be empty
         chPiece = EMPTY_SQUARE ;
      }
      else if( intended_move->to.iRow == iRow && intended_move->to.iColumn == iColumn )
      {
         // The piece wants to move to that square, so return the piece
         chPiece = intended_move->chPiece;
      }
      else
      {
         chPiece = getPieceAtPosition( iRow, iColumn );
      }
   }
    //return piece
   return chPiece;
}

Chess::UnderAttack Game::isUnderAttack( int iRow,int iColumn,int iColor,IntendedMove* pintended_move )
{
   UnderAttack attack = { 0 };

   // a) Direction: HORIZONTAL
   {
      // Check all the way to the right
      for (int i = iColumn + 1;i < 8;i++)
      {
         char chPieceFound = getPiece_considerMove(iRow, i, pintended_move);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if( iColor == getPieceColor(chPieceFound ) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if( ( toupper(chPieceFound) == 'Q' ) ||
                   ( toupper(chPieceFound) == 'R' ) )
         {
            // There is a queen or a rook to the right, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow = iRow;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = i;
            attack.attacker[attack.iNumAttackers-1].dir = HORIZONTAL;
            break;
         }
         else
         {

            break;
         }
      }

      // Check all the way to the left
      for (int i = iColumn - 1; i >= 0; i--)
      {
         char chPieceFound = getPiece_considerMove( iRow,i,pintended_move );
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if  ( iColor == getPieceColor(chPieceFound ) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if( ( toupper(chPieceFound) == 'Q' ) ||
                   ( toupper(chPieceFound) == 'R' ) )
         {

            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = iRow;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = i;
            attack.attacker[attack.iNumAttackers-1].dir = HORIZONTAL;
            break;
         }
         else
         {

            break;
         }
      }
   }

   // b) Direction: VERTICAL
   {
      // Check all the way up
      for ( int i = iRow + 1; i < 8; i++ )
      {
         char chPieceFound = getPiece_considerMove ( i, iColumn, pintended_move );
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R')  )
         {
            // There is a queen or a rook to the right, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumn;
            attack.attacker[attack.iNumAttackers-1].dir = VERTICAL;
            break;
         }
         else
         {
            //its ok
            break;
         }
      }

      // Check all the way down
      for(int i = iRow - 1; i >= 0; i--)
      {
         char chPieceFound = getPiece_considerMove(i, iColumn, pintended_move);
         if (EMPTY_SQUARE  == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ((toupper(chPieceFound) == 'Q') ||
                  (toupper(chPieceFound) == 'R') )
         {

            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumn;
            attack.attacker[attack.iNumAttackers-1].dir = VERTICAL;
            break;
         }
         else
         {
            // There is a piece that does not attack VERTICALically, so no problem
            break;
         }
      }
   }

   // c) Direction: DIAGONAL
   {
      // Check the DIAGONAL up-right
      for (int i = iRow + 1, j = iColumn + 1; i < 8 && j < 8; i++, j++)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if (EMPTY_SQUARE  == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor ( chPieceFound ) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    + 1        ) &&
                   (   j   == iColumn + 1        ) &&
                   (iColor == WHITE_PIECE  ) )
         {
            // A pawn only puts another piece in jeopardy if it's (DIAGONALonally) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B'))
         {
            // There is a queen or a bishop in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[ attack.iNumAttackers-1 ].pos.iRow    = i;
            attack.attacker[ attack.iNumAttackers-1 ].pos.iColumn   = j;
            attack.attacker[ attack.iNumAttackers-1 ].dir = DIAGONAL;
            break;
         }
         else
         {
            // There is a piece that does not attack DIAGONALly, so no problem
            break;
         }
      }


      for (int i = iRow + 1, j = iColumn - 1; i < 8 && j > 0; i++, j--)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    + 1        ) &&
                   (   j   == iColumn - 1        ) &&
                   ( iColor == WHITE_PIECE  )     )
         {
            // A pawn only puts another piece in jeopardy if it's (diagonally) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[ attack.iNumAttackers-1 ].pos.iRow    = i;
            attack.attacker[ attack.iNumAttackers-1 ].pos.iColumn = j;
            attack.attacker[ attack.iNumAttackers-1 ].dir = DIAGONAL;
            break;
         }
         else if ( ( toupper(chPieceFound) == 'Q' ) ||
                   ( toupper(chPieceFound) == 'B' ))
         {

            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else
         {
            // There is a piece that does not attack DIAGONALly, so no problem
            break;
         }
      }

      // Check the DIAGONAL down-right
      for (int i = iRow - 1, j = iColumn + 1; i > 0 && j < 8; i--, j++)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor( chPieceFound ) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    - 1        ) &&
                   (   j   == iColumn + 1        ) &&
                   (iColor == BLACK_PIECE ) )
         {
            // A pawn only puts another piece in jeopardy if it's (DIAGONALly) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if( ( toupper(chPieceFound) == 'Q' ) || ( toupper(chPieceFound)
                                                        == 'B' ) )
         {
            // There is a queen or a bishop in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[ attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[ attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[ attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else
         {

            break;
         }
      }

      // Check the DIAGONAL down-left
      for ( int i = iRow - 1, j = iColumn - 1; i > 0 && j > 0; i--, j--)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    - 1        ) &&
                   (   j   == iColumn - 1        ) &&
                   ( iColor == BLACK_PIECE )   )
         {
            // A pawn only puts another piece in jeopardy if it's (DIAGONALly) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                     (toupper(chPieceFound) == 'B') )
         {
            // There is a queen or a bishop in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else
         {
            // There is a piece that does not attack DIAGONALly, so no problem
            break;
         }
      }
   }

   // d) Direction: L_SHAPED
   {
      // Check if the piece is put in jeopardy by a knigh

      Position knight_moves[ 8 ] = { {  1, -2 }, {  2, -1 }, {  2, 1 }, {  1, 2 },
                                   { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 } };
      for (int i = 0;i < 8;i++)
      {
         int iRowToTest    = iRow    + knight_moves[i].iRow;
         int iColumnToTest = iColumn + knight_moves[i].iColumn;

         if (iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 || iColumnToTest > 7)
         {
            // This square does not even exist, so no need to test
            continue;
         }

         char chPieceFound = getPiece_considerMove(iRowToTest, iColumnToTest, pintended_move);
         if (EMPTY_SQUARE  == chPieceFound)
         {

            continue;
         }

         if (iColor == getPieceColor(chPieceFound))
         {
            // This is a piece of the same color, so no problem
            continue;
         }
         else if ( (toupper(chPieceFound) == 'N') )
         {
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = iRowToTest;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumnToTest;
            break;
         }
      }
   }

   return attack;
}

bool Game::isReachable( int iRow, int iColumn, int iColor )
{
   bool bReachable = false;

   // a) Direction: HORIZONTAL
   {
      // Check all the way to the right
      for ( int i = iColumn + 1; i < 8; i++)
      {
         char chPieceFound = getPieceAtPosition(iRow, i);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor( chPieceFound ) )
         {
            // This is a piece of the same color
            break;
         }
         else if( ( toupper(chPieceFound) == 'Q') ||
                   ( toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook to the right, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {

            break;
         }
      }

      // Check all the way to the left
      for (int i = iColumn - 1; i >= 0; i--)
      {
         char chPieceFound = getPieceAtPosition(iRow, i);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if  (iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( ( toupper(chPieceFound) == 'Q' ) ||
                   ( toupper(chPieceFound) == 'R' ) )
         {
            // There is a queen or a rook to the left, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move HORIZONTALly
            break;
         }
      }
   }

   // b) Direction: VERTICAL
   {
      // Check all the way up
      for (int i = iRow + 1; i < 8; i++)
      {
         char chPieceFound = getPieceAtPosition(i, iColumn);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor( chPieceFound ) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( ( toupper(chPieceFound)       == 'P' )         &&
                   ( getPieceColor(chPieceFound) == BLACK_PIECE ) &&
                   ( i  == iRow + 1 )                            )
         {
            // There is a pawn one square up, so the square is reachable
            bReachable = true;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R')  )
         {

            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move VERTICALly
            break;
         }
      }

      // Check all the way down
      for ( int i = iRow - 1; i >= 0; i-- )
      {
         char chPieceFound = getPieceAtPosition(i, iColumn);
         if( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor( chPieceFound ) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( ( toupper(chPieceFound)       == 'P' )         &&
                   ( getPieceColor(chPieceFound) == WHITE_PIECE ) &&
                   ( i  == iRow - 1 )                            )
         {
            // There is a pawn one square down, so the square is reachable
            bReachable = true;
            break;
         }
         else if ((toupper(chPieceFound) == 'Q') ||
                  (toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook on the way down, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move VERTICALly
            break;
         }
      }
   }

   // c) Direction: DIAGONAL
   {

      for (int i = iRow + 1, j = iColumn + 1; i < 8 && j < 8; i++, j++)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if (EMPTY_SQUARE  == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B'))
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move DIAGONALly
            break;
         }
      }

      // Check the DIAGONAL up-left
      for (int i = iRow + 1, j = iColumn - 1; i < 8 && j > 0; i++, j--)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if (EMPTY_SQUARE  == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B'))
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move DIAGONALly
            break;
         }
      }

      // Check the DIAGONAL down-right
      for( int i = iRow - 1, j = iColumn + 1; i > 0 && j < 8; i--, j++)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if(EMPTY_SQUARE  == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {

            break;
         }
         else if ( ( toupper(chPieceFound) == 'Q') ||
                   ( toupper(chPieceFound) == 'B') )
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move DIAGONALly
            break;
         }
      }

      // Check the DIAGONAL down-left
      for (int i = iRow - 1, j = iColumn - 1; i > 0 && j > 0; i--, j--)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if (EMPTY_SQUARE  == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B') )
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {

            break;
         }
      }
   }

   // d) Direction: L_SHAPED
   {
      // Check if the piece is put in jeopardy by a knigh

      Position knight_moves[8] = { {  1, -2 }, {  2, -1 }, {  2, 1 }, {  1, 2 },
                                   { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 } };
      for (int i = 0; i < 8; i++)
      {
         int iRowToTest    = iRow    + knight_moves[i].iRow;
         int iColumnToTest = iColumn + knight_moves[i].iColumn;

         if (iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 || iColumnToTest > 7)
         {
            // This square does not even exist, so no need to test
            continue;
         }

         char chPieceFound = getPieceAtPosition(iRowToTest, iColumnToTest);
         if ( EMPTY_SQUARE  == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            continue;
         }
         else if ( (toupper(chPieceFound) == 'N') )
         {
            bReachable = true;
            break;
         }
      }
   }

   return bReachable;
}

bool Game::isSquareOccupied(int iRow, int iColumn)
{
   bool bOccupied = false;

   if( 0x20 != getPieceAtPosition ( iRow,iColumn) )
   {
      bOccupied = true;
   }

   return bOccupied;
}

bool Game::isPathFree(Position startingPos, Position finishingPos, int iDirection)
{
   bool bFree = false;

   switch(iDirection)
   {
      case Chess::HORIZONTAL:
      {
         // If it is a HORIZONTAL move, we can assume the startingPos.iRow == finishingPos.iRow
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iColumn == finishingPos.iColumn)
         {
            cout << "Error. Movement is HORIZONTAL but column is the same\n";
         }


         else if(startingPos.iColumn < finishingPos.iColumn)
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for(int i = startingPos.iColumn + 1; i < finishingPos.iColumn; i++)
            {
               if(isSquareOccupied(startingPos.iRow, i))
               {
                  bFree = false;
                  cout << "HORIZONTAL path to the right is not clear!\n";
               }
            }
         }

         // Moving to the left
         else
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = startingPos.iColumn - 1; i > finishingPos.iColumn; i--)
            {
               if (isSquareOccupied(startingPos.iRow, i))
               {
                  bFree = false;
                  cout << "HORIZONTAL path to the left is not clear!\n";
               }
            }
         }
      }
      break;

      case Chess::VERTICAL:
      {
         // If it is a VERTICAL move, we can assume the startingPos.iColumn == finishingPos.iColumn
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iRow == finishingPos.iRow)
         {
            cout << "Error. Movement is VERTICAL but row is the same\n";
           throw("Error. Movement is VERTICAL but row is the same");
         }

         // Moving up
         else if (startingPos.iRow < finishingPos.iRow)
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = startingPos.iRow + 1; i < finishingPos.iRow; i++)
            {
               if ( isSquareOccupied(i, startingPos.iColumn) )
               {
                  bFree = false;
                  cout << "VERTICAL path up is not clear!\n";
               }
            }
         }

         // Moving down
         else //if (startingPos.iColumn > finishingPos.iRow)
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = startingPos.iRow - 1; i > finishingPos.iRow; i--)
            {
               if ( isSquareOccupied(i, startingPos.iColumn) )
               {
                  bFree = false;
                  cout << "VERTICAL path down is not clear!\n";
               }
            }
         }
      }
      break;

      case Chess::DIAGONAL:
      {

         if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {

            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow + i, startingPos.iColumn + i))
               {
                  bFree = false;
                  cout << "DIAGONAL path up-right is not clear!\n";
               }
            }
         }

         // Moving up and left
         else if ( ( finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow+i, startingPos.iColumn-i))
               {
                  bFree = false;
                  cout << "DIAGONAL path up-left is not clear!\n";
               }
            }
         }

         // Moving down and right
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow - i, startingPos.iColumn + i))
               {
                  bFree = false;
                  cout << "DIAGONAL path down-right is not clear!\n";
               }
            }
         }

         // Moving down and left
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for ( int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( isSquareOccupied(startingPos.iRow - i, startingPos.iColumn - i))
               {
                  bFree = false;
                  cout << "DIAGONAL path down-left is not clear!\n";
               }
            }
         }

         else
         {
             //throw an error
            throw("Error. DIAGONAL move not allowed");
         }
      }
      break;
   }

   return bFree;
}

bool Game::canBeBlocked(Position startingPos, Position finishingPos, int iDirection)
{
    //blocked is first false
   bool bBlocked = false;

   Chess::UnderAttack blocker = {0};

   switch ( iDirection )
   {
      case Chess::HORIZONTAL:
      {
         // If it is a HORIZONTALontal move, we can assume the startingPos.iRow == finishingPos.iRow
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iColumn == finishingPos.iColumn)
         {
            cout << "Error. Movement is HORIZONTALontal but column is the same\n";
         }

         // Moving to the right
         else if (startingPos.iColumn < finishingPos.iColumn)
         {
            for (int i = startingPos.iColumn + 1; i < finishingPos.iColumn; i++)
            {
               if ( true == isReachable( startingPos.iRow, i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving to the left
         else //if (startingPos.iColumn > finishingPos.iColumn)
         {
            for (int i = startingPos.iColumn - 1; i > finishingPos.iColumn; i--)
            {
               if ( true == isReachable( startingPos.iRow, i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }
      }
      break;

      case Chess::VERTICAL:
      {
         // If it is a VERTICAL move, we can assume the startingPos.iColumn == finishingPos.iColumn
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iRow == finishingPos.iRow)
         {
             //throw an error
            cout << "Error. Movement is VERTICAL but row is the same\n";
           throw( "Error. Movement is VERTICAL but row is the same" );
         }

         // Moving up
         else if (startingPos.iRow < finishingPos.iRow)
         {
            for (int i = startingPos.iRow + 1; i < finishingPos.iRow; i++)
            {
               if ( true == isReachable( i, startingPos.iColumn, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving down
         else //if (startingPos.iColumn > finishingPos.iRow)
         {
            for (int i = startingPos.iRow - 1; i > finishingPos.iRow; i--)
            {
               if ( true == isReachable( i, startingPos.iColumn, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }
      }
      break;

      case Chess::DIAGONAL:
      {
         // Moving up and right
         if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow + i, startingPos.iColumn + i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }


         else if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow + i, startingPos.iColumn - i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving down and right
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow - i, startingPos.iColumn + i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving down and left
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow - i, startingPos.iColumn - i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }
        //OR
         else
         {
            cout << "Error. DIAGONAL move not allowed\n";
            throw( "Error. DIAGONAL move not allowed" );
         }
      }
      break;
   }

   return bBlocked;
}

bool Game::isCheckMate()
{
   bool bCheckmate = false;

   // 1. First of all, it the king in check?
   if ( false == playerKingInCheck() )
   {
      return false;
   }

   // 2. Can the king move the other square?
   Chess::Position king_moves[8]  = { {  1, -1 },{  1, 0 },{  1,  1 }, { 0,  1 },
                                      { -1,  1 },{ -1, 0 },{ -1, -1 }, { 0, -1 } };

   Chess::Position king = findKing(getCurrentTurn() );

   for (int i = 0; i < 8; i++)
   {
      int iRowToTest    = king.iRow    + king_moves[i].iRow;
      int iColumnToTest = king.iColumn + king_moves[i].iColumn;

      if ( iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 || iColumnToTest > 7 )
      {
         // This square does not even exist, so no need to test
         continue;
      }

      if ( EMPTY_SQUARE  != getPieceAtPosition(iRowToTest, iColumnToTest) )
      {

         continue;
      }

      Chess::IntendedMove intended_move;
      intended_move.chPiece      = getPieceAtPosition(king.iRow, king.iColumn);
      intended_move.from.iRow    = king.iRow;
      intended_move.from.iColumn = king.iColumn;
      intended_move.to.iRow      = iRowToTest;
      intended_move.to.iColumn   = iColumnToTest;

      // Now, for every possible move of the king, check if it would be in jeopardy
      // Since the move has already been made, current_game->getCurrentTurn() now will return
      // the next player's color. And it is in fact this king that we want to check for jeopardy
      Chess::UnderAttack king_moved = isUnderAttack( iRowToTest, iColumnToTest, getCurrentTurn(), &intended_move );

      if ( false == king_moved.bUnderAttack )
      {
         // This means there is at least one position when the king would not be in jeopardy, so that's not a checkmate
         return false;
      }
   }

   // 3. Can the attacker be taken or another piece get into the way?
   Chess::UnderAttack king_attacked = isUnderAttack( king.iRow, king.iColumn, getCurrentTurn() );
   if ( 1 == king_attacked.iNumAttackers )
   {
      // Can the attacker be taken?
      Chess::UnderAttack king_attacker = { 0 };
      king_attacker = isUnderAttack( king_attacked.attacker[0].pos.iRow, king_attacked.attacker[0].pos.iColumn, getOpponentColor() );

      if ( true == king_attacker.bUnderAttack )
      {
         // This means that the attacker can be taken, so it's not a checkmate
         return false;
      }
      else
      {
         // Last resort: can any piece get in between the attacker and the king?
         char chAttacker = getPieceAtPosition( king_attacked.attacker[0].pos.iRow, king_attacked.attacker[0].pos.iColumn );

         switch( toupper(chAttacker) )
         {
            case 'P':
            case 'N':
            {
               // If it's a pawn, there's no space in between the attacker and the king
               // If it's a knight, it doesn't matter because the knight can 'jump'
               // So, this is checkmate
               bCheckmate = true;
            }
            break;

            case 'B':
            {
               if ( false == canBeBlocked(king_attacked.attacker[0].pos, king, Chess::DIAGONAL ) )
               {

                  bCheckmate = true;
               }
            }
            break;

            case 'R':
            {
               if ( false == canBeBlocked(king_attacked.attacker[0].pos, king, king_attacked.attacker[0].dir ) )
               {
                  // If no piece can get in the way, it's a checkmate
                  bCheckmate = true;
               }
            }
            break;

            case 'Q':
            {
               if ( false == canBeBlocked(king_attacked.attacker[0].pos, king, king_attacked.attacker[0].dir ) )
               {
                  // If no piece can get in the way, it's a checkmate
                  bCheckmate = true;
               }
            }
            break;


            default:
            {
               throw("!!!!Should not reach here. Invalid piece");
            }
            break;
         }
      }
   }
   else
   {
      // If there is more than one attacker and we reached this point, it's a checkmate
      bCheckmate      = true;
   }

   // If the game has ended, store in the class variable
   mbGameFinished = bCheckmate;

   return bCheckmate;
}

bool Game::isKingInCheck (int iColor, IntendedMove* pintended_move)
{
   bool bCheck = false;

   Position king = { 0 };

   // Must check if the intended move is to move the king itself
   if ( nullptr != pintended_move && 'K' == toupper( pintended_move->chPiece) )
   {
      king.iRow    = pintended_move->to.iRow;
      king.iColumn = pintended_move->to.iColumn;
   }
   else
   {
      king = findKing( iColor );
   }

   UnderAttack king_attacked = isUnderAttack( king.iRow, king.iColumn, iColor, pintended_move );

   if ( true == king_attacked.bUnderAttack )
   {
      bCheck = true;
   }

   return bCheck;
}

bool Game::playerKingInCheck ( IntendedMove* intended_move )
{
    //is the king in checkmate
   return isKingInCheck ( getCurrentTurn(), intended_move);
}

bool Game::wouldKingBeInCheck(char chPiece, Position present, Position future, EnPassant* S_enPassant)
{
   IntendedMove intended_move;

   intended_move.chPiece      = chPiece;
   intended_move.from.iRow    = present.iRow;
   intended_move.from.iColumn = present.iColumn;
   intended_move.to.iRow      = future.iRow;
   intended_move.to.iColumn   = future.iColumn;

   return playerKingInCheck(&intended_move);
}

Chess::Position Game::findKing(int iColor)
{
   char chToLook = (WHITE_PIECE == iColor) ? 'K': 'k';
   Position king = { 0 };

   for (int i = 0; i < 8; i++)
   {
      for (int j = 0; j < 8; j++)
      {
         if ( chToLook == getPieceAtPosition(i, j) )
         {
            king.iRow    = i;
            king.iColumn = j;
         }
      }
   }

   return king;
}

void Game::changeTurns(void)
{
   if (WHITE_PLAYER == mCurrentTurn)
   {
      mCurrentTurn = BLACK_PLAYER ;
   }
   else
   {
      mCurrentTurn = WHITE_PLAYER;
   }
}

bool Game::isFinished( void )
{
   return mbGameFinished;
}

int Game::getCurrentTurn(void)
{
   return mCurrentTurn;
}

int Game::getOpponentColor(void)
{
   int iColor;

   if (Chess::WHITE_PLAYER == getCurrentTurn())
   {
      iColor = Chess::BLACK_PLAYER ;
   }
   else
   {
      iColor = Chess::WHITE_PLAYER;
   }

   return iColor;
}

void Game::parseMove(string move, Position* pFrom, Position* pTo, char* chPromoted)
{
   pFrom->iColumn =  move[0];
   pFrom->iRow    = move[1];
   pTo->iColumn   =  move[3];
   pTo->iRow      = move[4];

   // ConVERTICAL columns from ['A'-'H'] to [0x00-0x07]
   pFrom->iColumn = pFrom->iColumn - 'A';
   pTo->iColumn   = pTo->iColumn   - 'A';

   // ConVERTICAL row from ['1'-'8'] to [0x00-0x07]
   pFrom->iRow  = pFrom->iRow  - '1';
   pTo->iRow    = pTo->iRow    - '1';

   if ( chPromoted != nullptr )
   {
      if ( move[5] == '=' )
      {
         *chPromoted = move[6];
      }
      else
      {
          //pointer
         *chPromoted = EMPTY_SQUARE ;
      }
   }
}

void Game::logMove(std::string &to_record)
{
   // If record contains only 5 chracters, add spaces
   // Because when
   if ( to_record.length() == 5 )
   {
      to_record += "  ";
   }

   if ( WHITE_PLAYER == getCurrentTurn() )
   {

      Round round;
      round.whiteMove = to_record;
      round.blackMove = "";

      rounds.push_back(round);
   }
   else
   {
      // If this was a blackMove, just update the last Round
      Round round = rounds[rounds.size() - 1];
      round.blackMove = to_record;

      // Remove the last round and put it back, now with the black move
      rounds.pop_back();
      rounds.push_back(round);
   }
}

string Game::getLastMove(void)
{
   string last_move;

   // Who did the last move?
   if (BLACK_PLAYER  == getCurrentTurn())
   {
      // If it's black's turn now, white had the last move
      last_move = rounds[rounds.size() - 1].whiteMove;
   }
   else
   {
      // Last move was black's
      last_move = rounds[rounds.size() - 1].blackMove;
   }

   return last_move;
}

void Game::deleteLastMove( void )
{
   // Notice we already changed turns back
   if ( WHITE_PLAYER == getCurrentTurn() )
   {
      // Last move was white's turn, so simply pop from the back
      rounds.pop_back();
   }
   else
   {
      // Last move was black's, so let's
      Round round = rounds[ rounds.size() - 1 ];
      round.blackMove = "";


      rounds.pop_back();
      rounds.push_back( round );
   }
}
