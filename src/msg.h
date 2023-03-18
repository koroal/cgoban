/*
 * src/msg.h, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 */


#ifndef  _MSG_H_
#define  _MSG_H_  1

/**********************************************************************
 * Global variables
 **********************************************************************/
extern const char  msg_mFonts[], msg_labelFonts[], msg_bFonts[];

/* From "crwin.c" */
extern const char  msg_byBillShubert[];
extern const char  msg_noWarranty[];
extern const char  msg_seeHelp[];

/* From "cgoban.c" */
extern const char  msg_path[], msg_file[], msg_mask[];
extern const char  msg_dirs[], msg_files[];
extern const char  msg_dirErr[];
extern const char  msg_notEnoughColors[];

/* From "control.c" */
extern const char  msg_newGame[], msg_loadGame[];
extern const char  msg_editSGF[], msg_goModem[];
extern const char  msg_quit[], msg_setup[];
extern const char  msg_loadGameName[], msg_editGameName[];

/* From "local.c" */
extern const char  msg_gSetup[];
extern const char  msg_wName[], msg_bName[];
extern const char  msg_ruleSet[], *msg_ruleNames[];
extern const char  msg_boardSize[];
extern const char  msg_handicap[];
extern const char  msg_komi[];
extern const char  msg_ok[];
extern const char  msg_cancel[];
extern const char  msg_timeSystem[], *msg_timeSystems[];
extern const char  msg_primaryTime[], msg_byoYomi[];
extern const char  msg_byoYomiCount[], msg_byoYomiStones[];
extern const char  msg_badSize[], msg_badHcap[], msg_badKomi[];
extern const char  msg_localTitle[], msg_noTitle[];
extern const char  msg_badTime[], msg_badBYStones[], msg_badBYCount[];

/*
 * From "local.c"
 */
extern const char  msg_notCgobanFile[];
extern const char  msg_gameInfo[], msg_help[];
extern const char  msg_pass[], msg_done[], msg_resume[], msg_dispute[];
extern const char  msg_saveGame[], msg_editGame[];
extern const char  msg_score[], msg_time[];
extern const char  msg_localChiRemDead[], msg_localJapRemDead[];
extern const char  msg_selectDisputed[], msg_selectDisputedMsg[];
extern const char  *msg_stoneNames[];
extern const char  msg_gameStartDesc[], msg_toPlay[];
extern const char  msg_move1Desc[], msg_move1OfDesc[];
extern const char  msg_moveNDesc[], msg_moveNOfDesc[];
extern const char  msg_selectDead[];
extern const char  msg_whiteWon[], msg_blackWon[], msg_jigo[];
extern const char  msg_timeLossInfo[];
extern const char  msg_disputeAnnounce[], msg_alive[], msg_dead[];
extern const char  msg_disputeOverAlive[];
extern const char  msg_disputeOverDead[];
extern const char  msg_scoreKillsComment[], msg_scoreNoKillsComment[];
extern const char  msg_ingPenaltyComment[];
extern const char  msg_winnerComment[], msg_jigoComment[];
extern const char  msg_gameIsOver[];
extern const char  msg_timeLoss[];
extern const char  msg_saveGameName[];
extern const char  msg_reallyQuitGame[];

/* From "goban.c" */
extern const char  msg_say[], msg_both[], msg_kib[];

/* From movechain.c */
extern const char  msg_mcReadErr[];
extern const char  msg_badSGFFile[];
extern const char  msg_sgfBadToken[];
extern const char  msg_sgfEarlyEOF[];
extern const char  msg_sgfBadLoc[];
extern const char  msg_sgfBadArg[];

/* From editBoard.c */
extern const char  msg_printGame[], msg_noSuchGame[], msg_reallyQuit[];
extern const char  msg_noCancel[], msg_yesQuit[];
extern const char  msg_changeNodeWhileScoring[];

/* From editTool.c */
extern const char  *msg_toolNames[];
extern const char  *msg_toolDesc1[], *msg_toolDesc2[];
extern const char  msg_killNode[], msg_moveNode[];

/* From "editInfo.c" */
extern const char  msg_editInfoTitle[], msg_copyrightC[];
extern const char  msg_gameTitle[], msg_result[];
extern const char  msg_wStoneName[], msg_bStoneName[];
extern const char  msg_wRank[], msg_bRank[];
extern const char  msg_handicapC[], msg_komiC[];
extern const char  msg_date[], msg_place[], msg_event[], msg_source[];

/* From gmp/setup.c */
extern const char  msg_gmpSetup[];
extern const char  *msg_gmpTypes[];
extern const char  *msg_gmpPlayers[];
extern const char  msg_program[];
extern const char  msg_device[];
extern const char  msg_machineName[];
extern const char  msg_port[];
extern const char  msg_username[];
extern const char  msg_password[];
extern const char  msg_badPortNum[];
extern const char  msg_progRunErr[], msg_devOpenErr[];
extern const char  msg_gmpTooBig[];
extern const char  msg_waitingForGame[];
extern const char  msg_noUndoFromGmpToClient[];

/* From gmp/play.c */
extern const char  msg_gmpMoveOutsideGame[];
extern const char  msg_gmpBadMove[];
extern const char  msg_undoRequested[];
extern const char  msg_badNewGameReq[];
extern const char  msg_gmpCouldntStart[];
extern const char  msg_gmpProgDied[], msg_gmpProgKilled[];
extern const char  msg_gmpProgWhyDead[];

/* From gmp/engine.c */
extern const char  msg_gmpDead[], msg_gmpTimeout[], msg_gmpSendBufFull[];

/* From client/setup.c */
extern const char  *msg_loginDesc[];
extern const char  msg_usernameColon[], msg_passwordColon[];

/* From client/login.c */
extern const char  msg_cliOpenSocket[], msg_cliLookup[], msg_cliConnect[];
extern const char  msg_cliHangup[], msg_login[];
extern const char  msg_notAGuest[], msg_guest[], msg_loginFailed[];

/* From client/board.c */
extern const char  msg_cliGameName[], msg_close[], msg_adjourn[];
extern const char  msg_resign[], msg_yesResign[], msg_reallyResign[];
extern const char  msg_cliGameBadMove[];

/* From client/conn.c */
extern const char  msg_commandError[];

/* From client/look.c */
extern const char  msg_cliLookName[];
extern const char  msg_cliLookInfo[];

/* From client/main.c */
extern const char  msg_players[], msg_games[];

/* From client/game.c */
extern const char  msg_gameGoneResign[], msg_gameGoneAdjourn[];
extern const char  msg_gameGoneTime[], msg_gameGoneScore[];
extern const char  msg_gameResultResign[], msg_gameResultAdjourn[];
extern const char  msg_gameResultTime[], msg_gameResultScore[];
extern const char  msg_gameListDesc[];
extern const char  msg_gameBadElf[];

/* From client/player.c */
extern const char  msg_name[], msg_braceRank[], msg_state[];
extern const char  msg_playerListDesc[];

/* From setup.c */
extern const char  msg_setupTitle[];
extern const char  msg_selServer[];
extern const char  msg_gsCompName[];
extern const char  msg_gsPortNum[];
extern const char  msg_clientBadPortNum[];
extern const char  msg_srvConfig[];
extern const char  msg_directConn[];
extern const char  msg_connCmdLabel[];
extern const char  msg_miscellaneous[];
extern const char  msg_showCoordinates[], msg_numberKibitzes[];
extern const char  msg_hiContrast[];
extern const char  msg_noTypo[];
extern const char  msg_setupSrvName[];
extern const char  msg_setupProtocol[];
extern const char msg_warnLimit[];

/* From client/match.c */
extern const char  msg_swapColors[];
extern const char  msg_cliGameSetup[];
extern const char  msg_cliGameSetupSent[];
extern const char  msg_cliGameSetupRej[];
extern const char  msg_cliGameSetupRecvd[];
extern const char  msg_freeGame[];
extern const char  msg_youDontExist[];

/* From "sgfPlay.c" */
extern const char  msg_badMoveInSgf[];

#endif  /* _MSG_H_ */
