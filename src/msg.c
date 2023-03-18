/*
 * src/msg.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1997 William Shubert
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include <but/but.h>
#include <but/menu.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include <abut/help.h>
#include "cgbuts.h"  /* Included for CGBUTS_WSTONECHAR and CGBUTS_BSTONECHAR */
#ifdef  _MSG_H_
   Levelization Error.
#endif
#include "msg.h"

/**********************************************************************
 * Globals
 **********************************************************************/
const char  msg_mFonts[] =
  "-adobe-utopia-medium-r-normal--%d-*-*-*-*-*-*-*/"
  "-adobe-times-medium-r-normal--%d-*-*-*-*-*-*-*/"
  "-bitstream-charter-medium-r-normal--%d-*-*-*-*-*-*-*";
const char  msg_labelFonts[] = 
  "-adobe-helvetica-bold-r-normal--%d-*-*-*-*-*-*-*";
const char  msg_bFonts[] =
  "-adobe-utopia-bold-r-normal--%d-*-*-*-*-*-*-*/"
  "-adobe-times-bold-r-normal--%d-*-*-*-*-*-*-*/"
  "-bitstream-charter-black-r-normal--%d-*-*-*-*-*-*-*";

/*
 * From crwin.c
 */
const char  msg_byBillShubert[] =
  "Copyright \251 1995-2002 William Shubert and Kevin Sonney";
const char  msg_noWarranty[] = "Cgoban comes with ABSOLUTELY NO WARRANTY "
				 "and is free software.";
const char  msg_seeHelp[] = "Please see the file \"COPYING\" for more "
			      "details.";

/* From "cgoban.c" */
const char  msg_path[] = "Path:";
const char  msg_file[] = "File:";
const char  msg_mask[] = "Mask:";
const char  msg_dirs[] = "Directories";
const char  msg_files[] = "Files";
const char  msg_dirErr[] = 
  "Error \"%s\" occurred while trying to read directory \"%s\".";
const char  msg_notEnoughColors[] =
  "   Cgoban can't get enough colors.  You probably have programs running "
   "that use a lot of colors, like Netscape or image viewers.  Cgoban will "
   "run in black and white mode; if you want to run cgoban in color, then "
   "you must exit from the problem applications and run cgoban again.";

/*
 * From control.c
 */
const char  msg_newGame[] = "New Game";
const char  msg_loadGame[] = "Load Game";
const char  msg_editSGF[] = "Edit SGF File";
const char  msg_goModem[] = "Go Modem";
const char  msg_quit[] = "Quit";
const char  msg_setup[] = "Setup";
const char  msg_loadGameName[] = "Select the file to load:";
const char  msg_editGameName[] = "Select the file to edit:";


/*
 * Buttons from gameSetup.c
 */
const char  msg_gSetup[] = "Game Setup";
const char  msg_wName[] = CGBUTS_WSTONECHAR " White:";
const char  msg_bName[] = CGBUTS_BSTONECHAR " Black:";
const char  msg_ruleSet[] = "Rules Set";
const char  *msg_ruleNames[] =
  {"Chinese", "Japanese", "Ing", "AGA", "New Zealand", "Tibetan",
     BUTMENU_OLEND};
const char  msg_boardSize[] = "Board Size";
const char  msg_handicap[] = "Handicap";
const char  msg_komi[] = "Komi";
const char  msg_ok[] = "OK";
const char  msg_cancel[] = "Cancel";
const char  msg_timeSystem[] = "Time System";
const char  *msg_timeSystems[] =
  {"None", "Absolute", "Japanese", "Canadian", "Ing", BUTMENU_OLEND};
const char  msg_primaryTime[] = "Main Time";
const char  msg_byoYomi[] = "Byo-Yomi Time";
const char  msg_byoYomiStones[] = "Stones per Byo-Yomi";
const char  msg_byoYomiCount[] = "Byo-Yomi Periods";
const char  msg_badSize[] = "\"%s\" is an invalid size.  The size should be "
			  "a number from 2 through %d.";
const char  msg_badHcap[] = "\"%s\" is an invalid handicap.  The handicap "
                          "should be zero or a number from 2 through %d.";
const char  msg_badKomi[] = "\"%s\" is an invalid komi.  The komi should be "
			  "be a number, possibly with 0.5 (one-half) added.";
const char  msg_localTitle[] = "%s (W) vs. %s (B)";
const char  msg_noTitle[] = "No Title";
const char  msg_badTime[] = "\"%s\" is not a legal time.  Use either "
				"just minutes, \"mm:ss\", or \"hh:mm:ss\" "
				"formats.";
const char  msg_badBYStones[] = "\"%s\" is not a legal byo-yomi stone "
				    "count.  Use any positive number.";
const char  msg_badBYCount[] = "\"%s\" is not a legal value for the "
				   "number of byo-yomi periods.  Use a "
				   "positive whole number.";

/*
 * Text from local.c
 */
const char  msg_notCgobanFile[] =
  "\"%s\" is not a Cgoban save game.  You may "
  "edit this SGF file, but you can not continue the game.";
const char  msg_gameInfo[] = "Game Info";
const char  msg_help[] = "Help";
const char  msg_pass[] = "Pass";
const char  msg_done[] = "Done";
const char  msg_resume[] = "Resume";
const char  msg_dispute[] = "Dispute";
const char  msg_saveGame[] = "Save Game";
const char  msg_editGame[] = "Edit Game";
const char  msg_score[] = "Score:";
const char  msg_time[] = "Time: ";
const char  msg_localChiRemDead[] = "Now you must remove all dead stones.  "
				  "Click on a group to mark them as dead.  "
				  "Press \"Done\" when all dead groups are "
				  "removed.  Press \"Resume\" or \""
				  CGBUTS_BACKCHAR "\" if you want to go "
				  "back to the game.";
const char  msg_localJapRemDead[] = "Now you must remove all dead stones.  "
				  "Click on a group to mark them as dead.  "
				  "Press \"Done\" when all dead groups are "
				  "removed.  Press \"Dispute\" if the "
				  "players cannot agree which stones are "
				  "dead.  Press \"" CGBUTS_BACKCHAR
				  "\" to continue the game.";
const char  msg_selectDisputedMsg[] =
  "Click on the stones that are disputed, then play to determine whether "
   "the stones are alive or dead.  Pass when you are satisfied with the fate "
   "of the stones.";
const char  *msg_stoneNames[] = {"White", "Black", "Empty"};
const char  msg_gameStartDesc[] = "Game Start: ";
const char  msg_move1Desc[] = "Move 1 (%c %s): ";
const char  msg_move1OfDesc[] = "Move 1 (%c %s) of %d: ";
const char  msg_moveNDesc[] = "Moves 1 to %d (%c %s): ";
const char  msg_moveNOfDesc[] = "Moves 1 to %d (%c %s) of %d: ";
const char  msg_toPlay[] = " to play";

const char  msg_selectDead[] = "Select Dead Stones";
const char  msg_selectDisputed[] = "Select Disputed Stones";
const char  msg_whiteWon[] = "Game Over: White Won";
const char  msg_blackWon[] = "Game Over: Black Won";
const char  msg_jigo[] = "Game Over: Jigo (Tie Game)";
const char  msg_timeLossInfo[] = "Game Over: %s has run out of time";
const char  msg_disputeAnnounce[] = "Dispute (%s): %s to play";
const char  msg_alive[] = "Alive";
const char  msg_dead[] = "Dead";
const char  msg_disputeOverAlive[] =
  "At least one disputed stone is still alive, so they will all stay on "
     "the board.  You may now select more dead stones, or press \"Done\".";
const char  msg_disputeOverDead[] =
  "All disputed stones are dead, so they will be removed from the board.  "
     "You may now select more dead stones, or press \"Done\".";
const char  msg_scoreKillsComment[] =
  "The game is over.  Final score:\n"
     "   White = %d territory + %d captures + %g komi%s = %g\n"
     "   Black = %d territory + %d captures%s = %g\n";
const char  msg_scoreNoKillsComment[] =
  "The game is over.  Final score:\n"
     "   White = %d territory + %d living stones + %g dame + %g komi%s = %g\n"
     "   Black = %d territory + %d living stones + %g dame%s = %g\n";
const char  msg_ingPenaltyComment[] =
  " - %d Ing Time Penalty";
const char  msg_winnerComment[] = "%s wins by %g.";
const char  msg_jigoComment[] = "Jigo (Tie game).";
const char  msg_gameIsOver[] = "The game is over.  %s";
const char  msg_saveGameName[] = "Enter the file name for the game:";
const char  msg_timeLoss[] = "%s has run out of time.  %s wins.";
const char  msg_reallyQuitGame[] =
  "You have changed this game since you last saved it.  Are you sure that "
  "you want to quit without saving?";

/* From "goban.c" */
const char  msg_say[] = "Say";
const char  msg_both[] = "Both";
const char  msg_kib[] = "Kib";

/* From movechain.c */
const char  msg_mcReadErr[] = "System error \"%s\" occurred while trying to read "
			    " file \"%s\".";
const char  msg_badSGFFile[] = "File \"%s\" has %s on line %d.  It "
			     "cannot be read in.";
const char  msg_sgfBadToken[] = "a bad token";
const char  msg_sgfEarlyEOF[] = "an unexpected EOF";
const char  msg_sgfBadLoc[] = "a bad board position";
const char  msg_sgfBadArg[] = "a bad argument";

/* From editBoard.c */
const char  msg_printGame[] = "Print Game";
const char  msg_noSuchGame[] =
  "Sorry, file \"%s\" cannot be found.  To create a new smart-go file, "
  "use \"New Game\" from the control panel, then press \"Edit Game\" "
  "once the game has started.";
const char  msg_reallyQuit[] =
  "File \"%s\" has been changed since you last saved it.  Are you sure that "
  "you want to quit and lose your changes?";
const char  msg_noCancel[] = "No, Cancel";
const char  msg_yesQuit[] = "Yes, Quit";
const char  msg_changeNodeWhileScoring[] = "You cannot change nodes while "
  "you are scoring a game.  Please change to a different tool.";

/* From editTools.c */
const char  *msg_toolNames[] = {
  "Play Game", "Edit Board", "Compute Score", "Add Triangle", "Add Square",
   "Add Circle", "Add Letter", "Number Stones"};
const char  *msg_toolDesc1[] = {
  "Click to play a stone",
   "Click to add/remove white stones",
   "Click to mark stones as dead",
   "Click to add/remove triangle marks",
   "Click to add/remove square marks",
   "Click to add/remove circle marks",
   "Click to add/remove letters",
   "Click to add/remove numbers starting at 1"};
const char  *msg_toolDesc2[] = {
  "Shift-click to go to when a move was made",
   "Shift-click to add/remove black stones",
   "Press \"Done\" to record score",
   "Shift-click to mark groups of stones",
   "Shift-click to mark groups of stones",
   "Shift-click to mark groups of stones",
   "",
   "Shift-click to add/remove move numbers"};
const char  msg_killNode[] = "Delete Moves";
const char  msg_moveNode[] = "Reorder Moves";

/* From "editInfo.c" */
const char  msg_editInfoTitle[] = "SGF Game Info";
const char  msg_copyrightC[] = "Copyright:";
const char  msg_gameTitle[] = "Title:";
const char  msg_result[] = "Result:";
const char  msg_wStoneName[] = CGBUTS_WSTONECHAR " Name:";
const char  msg_bStoneName[] = CGBUTS_BSTONECHAR " Name:";
const char  msg_wRank[] = CGBUTS_WSTONECHAR " Rank:";
const char  msg_bRank[] = CGBUTS_BSTONECHAR " Rank:";
const char  msg_handicapC[] = "Handicap:";
const char  msg_komiC[] = "Komi:";
const char  msg_date[] = "Date:";
const char  msg_place[] = "Place:";
const char  msg_event[] = "Event:";
const char  msg_source[] = "Source:";

/* From gmpSetup.c */
const char  msg_gmpSetup[] = "Go Modem Protocol Setup";
const char  *msg_gmpPlayers[] = {CGBUTS_WSTONECHAR " White Player",
				 CGBUTS_BSTONECHAR " Black Player"};
const char  *msg_gmpTypes[] =
  {"Program", "Device", "Human", "NNGS", "IGS", "Local Port", "Remote Port",
   BUTMENU_OLEND};
const char  msg_program[] = "Program";
const char  msg_device[] = "Device";
const char  msg_machineName[] = "Machine Name";
const char  msg_port[] = "Port";
const char  msg_username[] = "Username";
const char  msg_password[] = "Password";
const char  msg_waitingForGame[] =
  "Waiting for go modem handshaking to complete.";
const char  msg_noUndoFromGmpToClient[] =
  "A request to undo the last move came from the go modem protocol "
  "connection. This is not supported when you are playing on a go server. "
  "The undo request will be ignored.";

const char  msg_badPortNum[] =
  "\"%s\" is not a valid port.  Your ports should be a value from 1 through "
    "65535.  \"26276\" is the default port.";
const char  msg_progRunErr[] =
  "System error \"%s\" occurred while trying to run program \"%s\".";
const char  msg_devOpenErr[] =
  "System error \"%s\" occurred while trying to open device program \"%s\".";
const char  msg_gmpTooBig[] =
  "The go modem protocol does not permit games to be played on a board "
  "larger than 22x22.";

/* From gmpPlay.c */
const char  msg_gmpMoveOutsideGame[] =
  "A move was received from the go modem protocol when the game was not "
  "in progress!";
const char  msg_gmpBadMove[] =
  "An illegal move at location %s was received from the go modem protocol.";
const char  msg_undoRequested[] =
  "Player %s requested to undo %d moves.";
const char  msg_badNewGameReq[] =
  "Player %s wants to start a new game.  You may restart a new game "
  "manually if you wish.";
const char  msg_gmpCouldntStart[] =
  "Command line \"%s\" could not be executed.  Please make sure that you have "
  "completely specified the programs path.";
const char  msg_gmpProgDied[] =
  "Program \"%s\" exitted unexpectedly with status %d.";
const char  msg_gmpProgKilled[] =
  "Program \"%s\" was unexpectedly killed by signal %d.";
const char  msg_gmpProgWhyDead[] =
  "Program \"%s\" is dead, but I can't figure out why.";

/* From gmp.c */
const char  msg_gmpDead[] =
  "System error \"%s\" occurred over the go modem protocol link.";
const char  msg_gmpTimeout[] =
  "The go modem protocol connection has timed out.";
const char  msg_gmpSendBufFull[] =
  "The go modem protocol is out of send buffer space.";

/* From cliLogin.c */
const char  *msg_loginDesc[] = {
  "Please enter your NNGS username and password here.  If you do not yet "
  "have an account, just leave the password blank and you will be logged in "
  "as an unregistered user.",
  "Please enter your IGS username and password here.  If you do not yet "
  "have an account, fill in \"guest\" for your username."};
const char  msg_usernameColon[] = "Username:";
const char  msg_passwordColon[] = "Password:";
const char  msg_cliOpenSocket[] =
  "Error \"%s\" while trying to open a socket to contact %s.";
const char  msg_cliLookup[] =
  "The machine \"%s\" couldn't be found in host lookup tables.  Either "
  "this machine does not exist or there is something wrong with your "
  "name server.";
const char  msg_cliConnect[] =
  "Error \"%s\" occurred while trying to connect to %s.";
const char  msg_cliHangup[] =
  "Your connection to server %s was lost due to \"%s\".";
const char  msg_notAGuest[] = "Sorry, the username \"%s\" seems to be "
  "taken already!  You must either supply a password or choose another name "
  "to use while you are a guest.";
const char  msg_guest[] = "You have logged in as a guest.  You may use "
  "the \"register\" command to get a full account.";
const char  msg_loginFailed[] = "Sorry, your password seems to be wrong "
  "for the account name you gave.  Please try another password or another "
  "account.";
const char  msg_login[] = "The connection to %s has been established.  "
			    "Login is in progress.";

/* From client/board.c */
const char  msg_cliGameName[] = "Game %d: %s %s (W) vs. %s %s (B)";
const char  msg_close[] = "Close";
const char  msg_adjourn[] = "Adjourn";
const char  msg_resign[] = "Resign";
const char  msg_yesResign[] = "Yes, Resign";
const char  msg_reallyResign[] = "You have pressed \"Resign\" which will "
  "end the game and give your opponent victory.  Are you sure that you "
  "want to do this?";
const char  msg_cliGameBadMove[] = "Move \"%s\" in game %d came from the "
  "server.  This does not seem to be a legal move.  Sorry, I'm not sure what "
  "to do now.";

/* From client/conn.c */
const char  msg_commandError[] = "The command \"%s\" exited with error "
  "return code %d.";

/* From client/look.c */
const char  msg_cliLookName[] = "%s %s (W) vs. %s %s (B)";
const char  msg_cliLookInfo[] = "Static Game Board";

/* From client/main.c */
const char  msg_log[] = "Log";
const char  msg_players[] = "Players";
const char  msg_games[] = "Games";

/* From client/game.c */
const char  msg_gameGoneResign[] =
  "The game has ended; %s has resigned.  Press the \"Close\" button to "
  "remove this board.";
const char  msg_gameResultResign[] = "Move %d: %s resigns";
const char  msg_gameGoneAdjourn[] =
  "The game has been adjourned.  Press the \"Close\" button to remove this "
  "board.";
const char  msg_gameResultAdjourn[] = "Move %d: Game adjourned";
const char  msg_gameGoneTime[] =
  "The game has ended; %s ran out of time and lost.  Press the \"Close\" "
  "button to remove this board.";
const char  msg_gameResultTime[] = "Move %d: %s runs out of time";
const char  msg_gameGoneScore[] =
  "The game has ended.  The final score is White %g points, Black %g "
  "points.  %s wins.";
const char  msg_gameResultScore[] = "Move %d: %s wins, %g to %g";
const char  msg_gameGone[] = "This game has ended on the server.  You can "
  "continue to look at the game, but your kibitzes, etc. will not be seen "
  "by other players.";
const char  msg_gameListDesc[] = "\t#\tWhite\t\tBlack\tMv\tSz\tH\tKm\tFl\tOb";
const char  msg_gameBadElf[] =
  "   Cgoban is having trouble interpreting data from the server.  This "
   "probably means that this cgoban binary was built on an old Linux Elf "
   "system.  Older Linux Elf systems have a bug in sscanf that prevents "
   "cgoban from functioning properly.  Please see the cgoban README for more "
   "information.";

/* From client/player.c */
const char  msg_name[] = "Name";
const char  msg_braceRank[] = "[Rank]";
const char  msg_state[] = "State";
const char  msg_playerListDesc[] = "\tIdle\tGm";

/* From setup.c */
const char  msg_selServer[] = "Server to Edit";
const char  msg_setupTitle[] = "Cgoban Setup";
const char  msg_gsCompName[] = "Machine Name:";
const char  msg_gsPortNum[] = "Port Number:";
const char  msg_clientBadPortNum[] =
  "\"%s\" is not a valid port.  Your ports should be a value from 1 through "
  "65535.  The default ports are 9696 for NNGS servers and 6969 for "
  "IGS servers.";
const char  msg_srvConfig[] = "Connecting To Server";
const char  msg_directConn[] = "Connect Directly To Server";
const char  msg_connCmdLabel[] = "Connecting Command:";
const char  msg_miscellaneous[] = "Miscellaneous";
const char  msg_showCoordinates[] = "Show Coordinates Around Boards";
const char  msg_numberKibitzes[] = "Number Kibitzes";
const char  msg_hiContrast[] = "High Contrast Stones";
const char  msg_noTypo[] = "Anti-Slip Moving";
const char  msg_setupSrvName[] = "Server Name:";
const char  msg_setupProtocol[] = "Server Protocol:";
const char msg_warnLimit[] = "Time Warning Limit (secs):";

/* From client/match.c */
const char  msg_swapColors[] = "Swap Colors";
const char  msg_cliGameSetup[] = "Server Game Setup";
const char  msg_cliGameSetupSent[] = "Server Game Setup - Challenge Sent";
const char  msg_cliGameSetupRej[] = "Server Game Setup - Challenge Rejected";
const char  msg_cliGameSetupRecvd[] = "Server Game Setup - Challenge Received";
const char  msg_freeGame[] = "Free (Unrated) Game";
const char  msg_youDontExist[] = "Sorry, an error has happened.  Somehow "
  "you don't seem to be in the player list, so I can't start up the "
  "challenge!  You might want to try pressing "
  "the \"reload\" button on the player list to see if that helps.  This may "
  "also mean that you are a guest on IGS.";

/* From "sgfPlay.c" */
const char  msg_badMoveInSgf[] = "The SGF file has \"%s\" as move "
  "number %d.  This is not a legal move, so it will not be placed on the "
  "board";
