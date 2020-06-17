#include "includes.h"
#include "chess.h"
#include "user_interface.h"


// -------------------------------------------------------------------
// Chess class
// -------------------------------------------------------------------
int Chess::getcolor(char chPiece)
{
   if (!isupper(chPiece))
   {
       //the piece is black
      return black;
   }
   else
   {
        //the piece is white
      return white;
   }
}

bool Chess::iswhite (char chPiece)
{
    //true if the piece is white and false if the piece is black
   return getcolor(chPiece) == Chess::white ? true : false;
}

bool Chess::isblack (char chPiece)
{
    //true if the piece is black and false if the piece is white
   return getcolor (chPiece) == Chess::black ? true : false;
}

std::string Chess::describePiece (char chPiece)
   {
      std::string description;

      if ( !iswhite (chPiece) )
      {
         description += "Black ";

      }
      else
      {
         description += "White ";
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
   m_CurrentTurn = whiteP;

   // Game on!
   m_bGameFinished = false;


   m_undo.bCapturedLastMove         = false;
   m_undo.bCanUndo                  = false;
   m_undo.bcastKingAllowed  = false;
   m_undo.bcastQueenAllowed = false;
   m_undo.en_passant.bApplied       = false;
   m_undo.castling.bApplied         = false;


   memcpy(board,initial_board,sizeof ( char )*8*8);

   // Castling is allowed (to each side) until the player moves the king or the rook
   mbcastKingAllowed[whiteP]  = true;
   mbcastKingAllowed[blackP]  = true;

   mbcastQueenAllowed[whiteP] = true;
   mbcastQueenAllowed[blackP] = true;
}

Game::~Game()
{
   white_captured.clear();
   black_captured.clear();
   rounds.clear();
}

void Game::movePiece (Position present,Position future,Chess::EnPassant*S_enPassant,Chess::Castling* S_castling, Chess::Promotion*S_promo)
{

   char chPiece = pieceAtPos ( present );

   // Is the destination square occupied?
   char chCapturedPiece = pieceAtPos ( future );

   // So, was a piece captured in this move?
   if (0x20 != chCapturedPiece)
   {
      if (white == getcolor(chCapturedPiece))
      {

         white_captured.push_back(chCapturedPiece);
      }
      else
      {
         // A black piece was captured
         black_captured.push_back(chCapturedPiece);
      }


      m_undo.bCapturedLastMove = true;

      //reset
      memset( &m_undo.en_passant, 0, sizeof( Chess::EnPassant ));
   }
   else if (true == S_enPassant->bApplied)
   {
      char chCapturedEP = pieceAtPos(S_enPassant->PawnCaptured.iRow, S_enPassant->PawnCaptured.iColumn);

      if (white == getcolor(chCapturedEP))
      {
         // A white piece was captured
         white_captured.push_back(chCapturedEP);
      }
      else
      {
         // A black piece was captured
         black_captured.push_back(chCapturedEP);
      }

      // Now, remove the captured pawn
      board[S_enPassant->PawnCaptured.iRow][S_enPassant->PawnCaptured.iColumn] = emptySQ;

      // Set Undo structure as piece was captured and "en passant" move was performed
      m_undo.bCapturedLastMove = true;
      memcpy ( &m_undo.en_passant, S_enPassant, sizeof ( Chess::EnPassant ) );
   }
   else
   {
      m_undo.bCapturedLastMove   = false;

      // Reset m_undo.castling
      memset ( &m_undo.en_passant,0, sizeof( Chess::EnPassant ));
   }

   // Remove piece
   board[present.iRow][present.iColumn] = emptySQ;

   // Move piece to new position
   if( true == S_promo->bApplied )
   {
      board[future.iRow][future.iColumn] = S_promo->chAfter;

      // Set Undo structure as a promo occured
      memcpy(&m_undo.promo, S_promo, sizeof(Chess::Promotion));
   }
   else
   {
      board[future.iRow][future.iColumn] = chPiece;

      // Reset m_undo.promo
      memset( &m_undo.promo, 0, sizeof( Chess::Promotion ));
   }


   if( S_castling->bApplied == true  )
   {
      // The king was already move, but we still have to move the rook to 'jump' the king
      char chPiece = pieceAtPos(S_castling->rook_before.iRow, S_castling->rook_before.iColumn);

      // Remove the rook from present position
      board[S_castling->rook_before.iRow][S_castling->rook_before.iColumn] = emptySQ;


      board[ S_castling->rook_after.iRow ][ S_castling->rook_after.iColumn ] =      chPiece;

      // Write this information to the m_undo struct
      memcpy ( &m_undo.castling, S_castling, sizeof( Chess::Castling ) );

      // Save the 'CastlingAllowed' information in case the move is undone
      m_undo.bcastKingAllowed  = mbcastKingAllowed[getCurrentTurn ()] ;
      m_undo.bcastQueenAllowed      = mbcastQueenAllowed[getCurrentTurn ()];
   }
   else
   {
      // Reset
      memset( &m_undo.castling, 0, sizeof( Chess::Castling ));
   }

   // Castling requirements
   if ( 'K' == toupper ( chPiece ) )
   {
      // After the king has moved once, no more castling allowed
      mbcastKingAllowed[getCurrentTurn()]  = false;
      mbcastQueenAllowed[getCurrentTurn()] = false;
   }
   else if ('R' == toupper(chPiece) )
   {
      // If the rook moved from column 'A', no more castling allowed on the queen side
      if ( 0 == present.iColumn )
      {
         mbcastQueenAllowed[getCurrentTurn()] = false;
      }


      else if ( 7 == present.iColumn )
      {
         mbcastKingAllowed[getCurrentTurn()] = false;
      }
   }

   // Change turns
   change_t();

   // This move can be undone
   m_undo.bCanUndo = true;
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
   char chPiece = pieceAtPos(to.iRow, to.iColumn);

   // Moving it back

   if( true == m_undo.promo.bApplied )
   {
      board[ from.iRow ][from.iColumn] = m_undo.promo.chBefore;
   }
   else
   {
      board[from.iRow][from.iColumn] = chPiece;
   }

   // Change turns
   change_t();

   // If a piece was captured, move it back to the board
   if ( m_undo.bCapturedLastMove )
   {
      // Retrieve the last captured piece
      char chCaptured;

      // Since we already changed turns back, it means we should pop a piece from the opponents vector
      if (whiteP == m_CurrentTurn)
      {
         chCaptured = black_captured.back();
         black_captured.pop_back();
      }
      else
      {
         chCaptured = white_captured.back();
         white_captured.pop_back();
      }


      if(m_undo.en_passant.bApplied)
      {
         // Move the captured piece back
         board[m_undo.en_passant.PawnCaptured.iRow][m_undo.en_passant.PawnCaptured.iColumn] = chCaptured;

         // Remove the attacker
         board[to.iRow][to.iColumn] = emptySQ;
      }
      else
      {
         board[to.iRow][to.iColumn] = chCaptured;
      }
   }
   else
   {
      board [to.iRow][to.iColumn]  = emptySQ;
   }

   // If there was a castling
   if ( m_undo.castling.bApplied )
   {
      char chRook = pieceAtPos(m_undo.castling.rook_after.iRow, m_undo.castling.rook_after.iColumn);

      // Remove the rook from present position
      board[m_undo.castling.rook_after.iRow][m_undo.castling.rook_after.iColumn] = emptySQ;

      // 'Jump' into to new position
      board[m_undo.castling.rook_before.iRow][m_undo.castling.rook_before.iColumn] = chRook;

      // Restore the values of castling allowed or not
      mbcastKingAllowed[ getCurrentTurn()]  = m_undo.bcastKingAllowed;
      mbcastQueenAllowed[ getCurrentTurn()] = m_undo.bcastQueenAllowed;
   }


   m_undo.bCanUndo             = false;
   m_undo.bCapturedLastMove    = false;
   m_undo.en_passant.bApplied  = false;
   m_undo.castling.bApplied         = false;
   m_undo.promo.bApplied   = false;

   // If it was a checkmate, toggle back to game not finished
   m_bGameFinished = false;

   // Finally, remove the last move from the list
   deleteLastMove();
}

bool Game::undoIsPossible()
{
   return m_undo.bCanUndo;
}

bool Game::castlingAllowed ( Side iSide,int iColor )
{
   if( queens == iSide )
   {
      return mbcastQueenAllowed[ iColor ];
   }
   else //if ( KING_SIDE == iSide )
   {
      return mbcastKingAllowed[ iColor ];
   }
}

char Game::pieceAtPos( int iRow,int iColumn )
{
   return board[ iRow ][ iColumn ];
}

char Game::pieceAtPos( Position pos )
{
   return board[ pos.iRow ][ pos.iColumn ];
}

char Game::getPiece_considerMove ( int iRow,int iColumn,IntendedMove* intended_move )
{
   char chPiece;

   // If there is no intended move, just return the current position of the board
   if ( nullptr == intended_move )
   {
      chPiece = pieceAtPos( iRow, iColumn );
   }
   else
   {
      // In this case, we are trying to understand what WOULD happed if the move was made,
      // so we consider a move that has not been made yet
      if( intended_move->from.iRow == iRow && intended_move->from.iColumn == iColumn )
      {
         // The piece wants to move from that square, so it would be empty
         chPiece = emptySQ;
      }
      else if( intended_move->to.iRow == iRow && intended_move->to.iColumn == iColumn )
      {
         // The piece wants to move to that square, so return the piece
         chPiece = intended_move->chPiece;
      }
      else
      {
         chPiece = pieceAtPos( iRow, iColumn );
      }
   }
    //return piece
   return chPiece;
}

Chess::UnderAttack Game::isUnderAttack( int iRow,int iColumn,int iColor,IntendedMove* pintended_move )
{
   UnderAttack attack = { 0 };

   // a) Direction: horiz
   {
      // Check all the way to the right
      for (int i = iColumn + 1;i < 8;i++)
      {
         char chPieceFound = getPiece_considerMove(iRow, i, pintended_move);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if( iColor == getcolor(chPieceFound ) )
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
            attack.attacker[attack.iNumAttackers-1].dir = horiz;
            break;
         }
         else
         {

            break;
         }
      }

      // Check all the way to the left
      for int i = iColumn - 1; i >= 0; i--)
      {
         char chPieceFound = getPiece_considerMove( iRow,i,pintended_move );
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if  ( iColor == getcolor(chPieceFound ) )
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
            attack.attacker[attack.iNumAttackers-1].dir = horiz;
            break;
         }
         else
         {

            break;
         }
      }
   }

   // b) Direction: vert
   {
      // Check all the way up
      for ( int i = iRow + 1; i < 8; i++ )
      {
         char chPieceFound = getPiece_considerMove ( i, iColumn, pintended_move );
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
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
            attack.attacker[attack.iNumAttackers-1].dir = vert;
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
         if (emptySQ == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
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
            attack.attacker[attack.iNumAttackers-1].dir = vert;
            break;
         }
         else
         {
            // There is a piece that does not attack vertically, so no problem
            break;
         }
      }
   }

   // c) Direction: diag
   {
      // Check the diag up-right
      for (int i = iRow + 1, j = iColumn + 1; i < 8 && j < 8; i++, j++)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if (emptySQ == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor ( chPieceFound ) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    + 1        ) &&
                   (   j   == iColumn + 1        ) &&
                   (iColor == white ) )
         {
            // A pawn only puts another piece in jeopardy if it's (diagonally) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = diag;
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
            attack.attacker[ attack.iNumAttackers-1 ].dir = diag;
            break;
         }
         else
         {
            // There is a piece that does not attack diagly, so no problem
            break;
         }
      }


      for (int i = iRow + 1, j = iColumn - 1; i < 8 && j > 0; i++, j--)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    + 1        ) &&
                   (   j   == iColumn - 1        ) &&
                   ( iColor == white )     )
         {
            // A pawn only puts another piece in jeopardy if it's (diagly) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[ attack.iNumAttackers-1 ].pos.iRow    = i;
            attack.attacker[ attack.iNumAttackers-1 ].pos.iColumn = j;
            attack.attacker[ attack.iNumAttackers-1 ].dir = diag;
            break;
         }
         else if ( ( toupper(chPieceFound) == 'Q' ) ||
                   ( toupper(chPieceFound) == 'B' ))
         {

            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = diag;
            break;
         }
         else
         {
            // There is a piece that does not attack diagly, so no problem
            break;
         }
      }

      // Check the diag down-right
      for (int i = iRow - 1, j = iColumn + 1; i > 0 && j < 8; i--, j++)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor( chPieceFound ) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    - 1        ) &&
                   (   j   == iColumn + 1        ) &&
                   (iColor == black ) )
         {
            // A pawn only puts another piece in jeopardy if it's (diagly) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = diag;
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
            attack.attacker[ attack.iNumAttackers-1].dir = diag;
            break;
         }
         else
         {

            break;
         }
      }

      // Check the diag down-left
      for ( int i = iRow - 1, j = iColumn - 1; i > 0 && j > 0; i--, j--)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    - 1        ) &&
                   (   j   == iColumn - 1        ) &&
                   ( iColor == black )   )
         {
            // A pawn only puts another piece in jeopardy if it's (diagly) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = diag;
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
            attack.attacker[attack.iNumAttackers-1].dir = diag;
            break;
         }
         else
         {
            // There is a piece that does not attack diagly, so no problem
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
         if (emptySQ == chPieceFound)
         {

            continue;
         }

         if (iColor == getcolor(chPieceFound))
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

   // a) Direction: horiz
   {
      // Check all the way to the right
      for ( int i = iColumn + 1; i < 8; i++)
      {
         char chPieceFound = pieceAtPos(iRow, i);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor( chPieceFound ) )
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
         char chPieceFound = pieceAtPos(iRow, i);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if  (iColor == getcolor(chPieceFound) )
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
            // There is a piece that does not move horizly
            break;
         }
      }
   }

   // b) Direction: vert
   {
      // Check all the way up
      for (int i = iRow + 1; i < 8; i++)
      {
         char chPieceFound = pieceAtPos(i, iColumn);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor( chPieceFound ) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( ( toupper(chPieceFound)       == 'P' )         &&
                   ( getcolor(chPieceFound) == black ) &&
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
            // There is a piece that does not move vertly
            break;
         }
      }

      // Check all the way down
      for ( int i = iRow - 1; i >= 0; i-- )
      {
         char chPieceFound = pieceAtPos(i, iColumn);
         if( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor( chPieceFound ) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( ( toupper(chPieceFound)       == 'P' )         &&
                   ( getcolor(chPieceFound) == white ) &&
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
            // There is a piece that does not move vertly
            break;
         }
      }
   }

   // c) Direction: diag
   {

      for (int i = iRow + 1, j = iColumn + 1; i < 8 && j < 8; i++, j++)
      {
         char chPieceFound = pieceAtPos(i, j);
         if (emptySQ == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
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
            // There is a piece that does not move diagly
            break;
         }
      }

      // Check the diag up-left
      for (int i = iRow + 1, j = iColumn - 1; i < 8 && j > 0; i++, j--)
      {
         char chPieceFound = pieceAtPos(i, j);
         if (emptySQ == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
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
            // There is a piece that does not move diagly
            break;
         }
      }

      // Check the diag down-right
      for( int i = iRow - 1, j = iColumn + 1; i > 0 && j < 8; i--, j++)
      {
         char chPieceFound = pieceAtPos(i, j);
         if(emptySQ == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
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
            // There is a piece that does not move diagly
            break;
         }
      }

      // Check the diag down-left
      for (int i = iRow - 1, j = iColumn - 1; i > 0 && j > 0; i--, j--)
      {
         char chPieceFound = pieceAtPos(i, j);
         if (emptySQ == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
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

         char chPieceFound = pieceAtPos(iRowToTest, iColumnToTest);
         if ( emptySQ == chPieceFound )
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getcolor(chPieceFound) )
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

   if( 0x20 != pieceAtPos ( iRow,iColumn) )
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
      case Chess::horiz:
      {
         // If it is a horiz move, we can assume the startingPos.iRow == finishingPos.iRow
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iColumn == finishingPos.iColumn)
         {
            cout << "Error. Movement is horiz but column is the same\n";
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
                  cout << "horiz path to the right is not clear!\n";
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
                  cout << "horiz path to the left is not clear!\n";
               }
            }
         }
      }
      break;

      case Chess::vert:
      {
         // If it is a vert move, we can assume the startingPos.iColumn == finishingPos.iColumn
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iRow == finishingPos.iRow)
         {
            cout << "Error. Movement is vert but row is the same\n";
           throw("Error. Movement is vert but row is the same");
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
                  cout << "vert path up is not clear!\n";
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
                  cout << "vert path down is not clear!\n";
               }
            }
         }
      }
      break;

      case Chess::diag:
      {

         if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {

            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow + i, startingPos.iColumn + i))
               {
                  bFree = false;
                  cout << "diag path up-right is not clear!\n";
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
                  cout << "diag path up-left is not clear!\n";
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
                  cout << "diag path down-right is not clear!\n";
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
                  cout << "diag path down-left is not clear!\n";
               }
            }
         }

         else
         {
             //throw an error
            throw("Error. diag move not allowed");
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
      case Chess::horiz:
      {
         // If it is a horizontal move, we can assume the startingPos.iRow == finishingPos.iRow
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iColumn == finishingPos.iColumn)
         {
            cout << "Error. Movement is horizontal but column is the same\n";
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

      case Chess::vert:
      {
         // If it is a vert move, we can assume the startingPos.iColumn == finishingPos.iColumn
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iRow == finishingPos.iRow)
         {
             //throw an error
            cout << "Error. Movement is vert but row is the same\n";
           throw( "Error. Movement is vert but row is the same" );
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

      case Chess::diag:
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
            cout << "Error. diag move not allowed\n";
            throw( "Error. diag move not allowed" );
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

      if ( emptySQ != pieceAtPos(iRowToTest, iColumnToTest) )
      {

         continue;
      }

      Chess::IntendedMove intended_move;
      intended_move.chPiece      = pieceAtPos(king.iRow, king.iColumn);
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
         char chAttacker = pieceAtPos( king_attacked.attacker[0].pos.iRow, king_attacked.attacker[0].pos.iColumn );

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
               if ( false == canBeBlocked(king_attacked.attacker[0].pos, king, Chess::diag ) )
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
   m_bGameFinished = bCheckmate;

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
   char chToLook = (white == iColor) ? 'K': 'k';
   Position king = { 0 };

   for (int i = 0; i < 8; i++)
   {
      for (int j = 0; j < 8; j++)
      {
         if ( chToLook == pieceAtPos(i, j) )
         {
            king.iRow    = i;
            king.iColumn = j;
         }
      }
   }

   return king;
}

void Game::change_t(void)
{
   if (whiteP == m_CurrentTurn)
   {
      m_CurrentTurn = blackP;
   }
   else
   {
      m_CurrentTurn = whiteP;
   }
}

bool Game::isFinished( void )
{
   return m_bGameFinished;
}

int Game::getCurrentTurn(void)
{
   return m_CurrentTurn;
}

int Game::getOpponentColor(void)
{
   int iColor;

   if (Chess::whiteP == getCurrentTurn())
   {
      iColor = Chess::blackP;
   }
   else
   {
      iColor = Chess::whiteP;
   }

   return iColor;
}

void Game::parseMove(string move, Position* pFrom, Position* pTo, char* chPromoted)
{
   pFrom->iColumn =  move[0];
   pFrom->iRow    = move[1];
   pTo->iColumn   =  move[3];
   pTo->iRow      = move[4];

   // Convert columns from ['A'-'H'] to [0x00-0x07]
   pFrom->iColumn = pFrom->iColumn - 'A';
   pTo->iColumn   = pTo->iColumn   - 'A';

   // Convert row from ['1'-'8'] to [0x00-0x07]
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
         *chPromoted = emptySQ;
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

   if ( whiteP == getCurrentTurn() )
   {

      Round round;
      round.white_move = to_record;
      round.black_move = "";

      rounds.push_back(round);
   }
   else
   {
      // If this was a black_move, just update the last Round
      Round round = rounds[rounds.size() - 1];
      round.black_move = to_record;

      // Remove the last round and put it back, now with the black move
      rounds.pop_back();
      rounds.push_back(round);
   }
}

string Game::getLastMove(void)
{
   string last_move;

   // Who did the last move?
   if (blackP == getCurrentTurn())
   {
      // If it's black's turn now, white had the last move
      last_move = rounds[rounds.size() - 1].white_move;
   }
   else
   {
      // Last move was black's
      last_move = rounds[rounds.size() - 1].black_move;
   }

   return last_move;
}

void Game::deleteLastMove( void )
{
   // Notice we already changed turns back
   if ( whiteP == getCurrentTurn() )
   {
      // Last move was white's turn, so simply pop from the back
      rounds.pop_back();
   }
   else
   {
      // Last move was black's, so let's
      Round round = rounds[ rounds.size() - 1 ];
      round.black_move = "";


      rounds.pop_back();
      rounds.push_back( round );
   }
}
