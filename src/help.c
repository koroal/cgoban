/*
 * src/help.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert
 * See "configure.h.in" for more copyright information.
 */


#include <wms.h>
#include "cgbuts.h"
#ifdef  _HELP_H_
        LEVELIZATION ERROR
#endif
#include "help.h"


/*
 * Used by all help pages
 */
static const AbutHelpText  aboutAuthor[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "About the authors"},
  {butText_just, 0, "   Bill Shubert learned go at a very young age, "
   "but never had any opponents until late 1993, when he learned of the"
   "Internet Go Server.  He was thrilled by the server, but was "
   "disappointed by the client programs available.  He was also "
   "disappointed by the quality of the smart go editing tools that "
   "he could find.  Instead of just whining, he decided to create his own "
   "program to do these tasks!  He called it \"Cgoban\", for Complete "
   "Goban, because it does everything that a computerized goban should."},
  {butText_just, 0, "   Bill began focusing on the Kiseido Go Server and "
   "the cgoban v2 client." },
  {butText_just, 0, "   Kevin Sonney learned Go from a friend in 1995."
   "Impressed with Bill's work, Kevin thought that there was still a lot "
   "of potential for cgoban v1. Kevintook over maintainership of cgoban1 "
   "In October 2002."},
  {butText_just, 0, "   You can always get the latest version of cgoban from "
   "it's Sourceforge project at http://cgoban1.sourceforge.net/ "
   "Comments and Suggestions can be sent through the contact information below"},
  {butText_left, 0, "   Maintainer Email: ksonney@redhat.com"},
  {butText_left, 0, "   WWW: http://cgoban1.sourceforge.net/"},
  {butText_just, 0, ""},
  {butText_just, 0, "   Thanks for trying out our program.  We hope you like "
   "it!"},
  {butText_just, 0, "   Special thanks to all cgoban testers, especially "
   "to Andries Brouwer, Tomas Boman, Jean-Louis Martineau, "
   "Eric Anderson, Dan Niles, Per-Eric Martin, Anthony Thyssen, "
   "Jan van der Steen, "
   "Eric Hoffman, Allen Tollin, Mel Melchner, Saito Takaaki, and "
   "Matt Ritchie, who either helped debug, wrote extra features, or "
   "otherwise hacked on the code to help make cgoban what it is."},

  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  copyright[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert & Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 0, "   The globe pictures were extracted from gifs created "
   "by xearth-1.0, which carries this copyright:"},
  {butText_center, 0,
   "Copyright (C) 1989, 1990, 1993, 1994, 1995 Kirk Lauritz Johnson"},
  {butText_center, 0, "Parts of the source code (as marked) are:"},
  {butText_center, 0, "Copyright (C) 1989, 1990, 1991 by Jim Frost"},
  {butText_center, 0, "Copyright (C) 1992 by Jamie Zawinski <jwz@lucid.com>"},
  {butText_center, 0, "Permission to use, copy, modify and freely distribute "
   "xearth for non-commercial and not-for-profit purposes is hereby granted "
   "without fee, provided that both the above copyright notice and "
   "this permission notice appear in all copies and in supporting "
   "documentation."},
  {butText_just, 0, ""},
  {butText_just, 2, "Distribution"},
  {butText_just, 0, "   All code except for the data described above is "
   "copyright \251 1995-2002 William Shubert and Kevin Sonney and is "
   "distributed under the Gnu General Public License, version 2.  This "
   "should have been included with your copy of the source code is a "
   "file called \"COPYING\".  In any case, the license is also included below."},
  {butText_just, 0, "   Basically, there are three things that most of you "
   "will care about in this license.  One is that you can give away free "
   "copies to all your friends.  Another is that you have to be told where "
   "you can get the source code from.  You can get it from me; see the "
   "\"About the Author\" help page for how to get in touch.  The last is "
   "that there is no warranty, so if this software ruins your life don't "
   "come crying to me."},
  {butText_just, 0, "   If you want to do anything else with this software, "
   "you should read the license below for details."},
  {butText_just, 0, ""},
  {butText_center, 0, "GNU GENERAL PUBLIC LICENSE"},
  {butText_center, 0, "TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND "
   "MODIFICATION"},
  {butText_just, 0, ""},
  {butText_just, 0, "  0. This License applies to any program or other work "
   "which contains a notice placed by the copyright holder saying it may be "
   "distributed under the terms of this General Public License.  The "
   "\"Program\", below, refers to any such program or work, and a \"work "
   "based on the Program\" means either the Program or any derivative work "
   "under copyright law: that is to say, a work containing the Program or a "
   "portion of it, either verbatim or with modifications and/or translated "
   "into another language.  (Hereinafter, translation is included without "
   "limitation in the term \"modification\".)  Each licensee is addressed "
   "as \"you\"."},
  {butText_just, 0, ""},
  {butText_just, 0, "Activities other than copying, distribution and "
   "modification are not covered by this License; they are outside its "
   "scope.  The act of running the Program is not restricted, and the "
   "output from the Program is covered only if its contents constitute a "
   "work based on the Program (independent of having been made by running "
   "the Program). Whether that is true depends on what the Program does."},
  {butText_just, 0, ""},
  {butText_just, 0, "  1. You may copy and distribute verbatim copies of the "
   "Program's source code as you receive it, in any medium, provided that "
   "you conspicuously and appropriately publish on each copy an appropriate "
   "copyright notice and disclaimer of warranty; keep intact all the "
   "notices that refer to this License and to the absence of any warranty; "
   "and give any other recipients of the Program a copy of this License "
   "along with the Program."},
  {butText_just, 0, ""},
  {butText_just, 0, "You may charge a fee for the physical act of "
   "transferring a copy, and you may at your option offer warranty "
   "protection in exchange for a fee."},
  {butText_just, 0, ""},
  {butText_just, 0, "  2. You may modify your copy or copies of the Program "
   "or any portion of it, thus forming a work based on the Program, and "
   "copy and distribute such modifications or work under the terms of "
   "Section 1 above, provided that you also meet all of these "
   "conditions:"},
  {butText_just, 0, ""},
  {butText_just, 0, "    a) You must cause the modified files to carry "
   "prominent notices stating that you changed the files and the date of "
   "any change."},
  {butText_just, 0, ""},
  {butText_just, 0, "    b) You must cause any work that you distribute or "
   "publish, that in whole or in part contains or is derived from the "
   "Program or any part thereof, to be licensed as a whole at no charge to "
   "all third parties under the terms of this License."},
  {butText_just, 0, ""},
  {butText_just, 0, "    c) If the modified program normally reads commands "
   "interactively when run, you must cause it, when started running for "
   "such interactive use in the most ordinary way, to print or display an "
   "announcement including an appropriate copyright notice and a notice "
   "that there is no warranty (or else, saying that you provide a warranty) "
   "and that users may redistribute the program under these conditions, and "
   "telling the user how to view a copy of this License.  (Exception: if "
   "the Program itself is interactive but does not normally print such an "
   "announcement, your work based on the Program is not required to print "
   "an announcement.)"},
  {butText_just, 0, ""},
  {butText_just, 0, "These requirements apply to the modified work as a "
   "whole.  If identifiable sections of that work are not derived from the "
   "Program, and can be reasonably considered independent and separate "
   "works in themselves, then this License, and its terms, do not apply to "
   "those sections when you distribute them as separate works.  But when "
   "you distribute the same sections as part of a whole which is a work "
   "based on the Program, the distribution of the whole must be on the "
   "terms of this License, whose permissions for other licensees extend to "
   "the entire whole, and thus to each and every part regardless of who "
   "wrote it."},
  {butText_just, 0, ""},
  {butText_just, 0, "Thus, it is not the intent of this section to claim "
   "rights or contest your rights to work written entirely by you; rather, "
   "the intent is to exercise the right to control the distribution of "
   "derivative or collective works based on the Program."},
  {butText_just, 0, ""},
  {butText_just, 0, "In addition, mere aggregation of another work not based "
   "on the Program with the Program (or with a work based on the Program) "
   "on a volume of a storage or distribution medium does not bring the "
   "other work under the scope of this License."},
  {butText_just, 0, ""},
  {butText_just, 0, "  3. You may copy and distribute the Program (or a work "
   "based on it, under Section 2) in object code or executable form under "
   "the terms of Sections 1 and 2 above provided that you also do one of "
   "the following:"},
  {butText_just, 0, ""},
  {butText_just, 0, "    a) Accompany it with the complete corresponding "
   "machine-readable source code, which must be distributed under the terms "
   "of Sections 1 and 2 above on a medium customarily used for software "
   "interchange; or,"},
  {butText_just, 0, ""},
  {butText_just, 0, "    b) Accompany it with a written offer, valid for at "
   "least three years, to give any third party, for a charge no more than "
   "your cost of physically performing source distribution, a complete "
   "machine-readable copy of the corresponding source code, to be "
   "distributed under the terms of Sections 1 and 2 above on a medium "
   "customarily used for software interchange; or,"},
  {butText_just, 0, ""},
  {butText_just, 0, "    c) Accompany it with the information you received "
   "as to the offer to distribute corresponding source code.  (This "
   "alternative is allowed only for noncommercial distribution and only if "
   "you received the program in object code or executable form with such an "
   "offer, in accord with Subsection b above.)"},
  {butText_just, 0, ""},
  {butText_just, 0, "The source code for a work means the preferred form of "
   "the work for making modifications to it.  For an executable work, "
   "complete source code means all the source code for all modules it "
   "contains, plus any associated interface definition files, plus the "
   "scripts used to control compilation and installation of the executable. "
   " However, as a special exception, the source code distributed need not "
   "include anything that is normally distributed (in either source or "
   "binary form) with the major components (compiler, kernel, and so on) of "
   "the operating system on which the executable runs, unless that "
   "component itself accompanies the executable."},
  {butText_just, 0, ""},
  {butText_just, 0, "If distribution of executable or object code is made by "
   "offering access to copy from a designated place, then offering "
   "equivalent access to copy the source code from the same place counts as "
   "distribution of the source code, even though third parties are not "
   "compelled to copy the source along with the object code."},
  {butText_just, 0, ""},
  {butText_just, 0, "  4. You may not copy, modify, sublicense, or "
   "distribute the Program except as expressly provided under this License. "
   " Any attempt otherwise to copy, modify, sublicense or distribute the "
   "Program is void, and will automatically terminate your rights under "
   "this License.  However, parties who have received copies, or rights, "
   "from you under this License will not have their licenses terminated so "
   "long as such parties remain in full compliance."},
  {butText_just, 0, ""},
  {butText_just, 0, "  5. You are not required to accept this License, since "
   "you have not signed it.  However, nothing else grants you permission to "
   "modify or distribute the Program or its derivative works.  These "
   "actions are prohibited by law if you do not accept this License.  "
   "Therefore, by modifying or distributing the Program (or any work based "
   "on the Program), you indicate your acceptance of this License to do so, "
   "and all its terms and conditions for copying, distributing or modifying "
   "the Program or works based on it."},
  {butText_just, 0, ""},
  {butText_just, 0, "  6. Each time you redistribute the Program (or any "
   "work based on the Program), the recipient automatically receives a "
   "license from the original licensor to copy, distribute or modify the "
   "Program subject to these terms and conditions.  You may not impose any "
   "further restrictions on the recipients' exercise of the rights granted "
   "herein.  You are not responsible for enforcing compliance by third "
   "parties to this License."},
  {butText_just, 0, ""},
  {butText_just, 0, "  7. If, as a consequence of a court judgment or "
   "allegation of patent infringement or for any other reason (not limited "
   "to patent issues), conditions are imposed on you (whether by court "
   "order, agreement or otherwise) that contradict the conditions of this "
   "License, they do not excuse you from the conditions of this License.  "
   "If you cannot distribute so as to satisfy simultaneously your "
   "obligations under this License and any other pertinent obligations, "
   "then as a consequence you may not distribute the Program at all.  For "
   "example, if a patent license would not permit royalty-free "
   "redistribution of the Program by all those who receive copies directly "
   "or indirectly through you, then the only way you could satisfy both it "
   "and this License would be to refrain entirely from distribution of the "
   "Program."},
  {butText_just, 0, ""},
  {butText_just, 0, "If any portion of this section is held invalid or "
   "unenforceable under any particular circumstance, the balance of the "
   "section is intended to apply and the section as a whole is intended to "
   "apply in other circumstances."},
  {butText_just, 0, ""},
  {butText_just, 0, "It is not the purpose of this section to induce you to "
   "infringe any patents or other property right claims or to contest "
   "validity of any such claims; this section has the sole purpose of "
   "protecting the integrity of the free software distribution system, "
   "which is implemented by public license practices.  Many people have "
   "made generous contributions to the wide range of software distributed "
   "through that system in reliance on consistent application of that "
   "system; it is up to the author/donor to decide if he or she is willing "
   "to distribute software through any other system and a licensee cannot "
   "impose that choice."},
  {butText_just, 0, ""},
  {butText_just, 0, "This section is intended to make thoroughly clear what "
   "is believed to be a consequence of the rest of this License."},
  {butText_just, 0, ""},
  {butText_just, 0, "  8. If the distribution and/or use of the Program is "
   "restricted in certain countries either by patents or by copyrighted "
   "interfaces, the original copyright holder who places the Program under "
   "this License may add an explicit geographical distribution limitation "
   "excluding those countries, so that distribution is permitted only in or "
   "among countries not thus excluded.  In such case, this License "
   "incorporates the limitation as if written in the body of this License."},
  {butText_just, 0, ""},
  {butText_just, 0, "  9. The Free Software Foundation may publish revised "
   "and/or new versions of the General Public License from time to time.  "
   "Such new versions will be similar in spirit to the present version, but "
   "may differ in detail to address new problems or concerns."},
  {butText_just, 0, ""},
  {butText_just, 0, "Each version is given a distinguishing version number.  "
   "If the Program specifies a version number of this License which applies "
   "to it and \"any later version\", you have the option of following the "
   "terms and conditions either of that version or of any later version "
   "published by the Free Software Foundation.  If the Program does not "
   "specify a version number of this License, you may choose any version "
   "ever published by the Free Software Foundation."},
  {butText_just, 0, ""},
  {butText_just, 0, "  10. If you wish to incorporate parts of the Program "
   "into other free programs whose distribution conditions are different, "
   "write to the author to ask for permission.  For software which is "
   "copyrighted by the Free Software Foundation, write to the Free Software "
   "Foundation; we sometimes make exceptions for this.  Our decision will "
   "be guided by the two goals of preserving the free status of all "
   "derivatives of our free software and of promoting the sharing and reuse "
   "of software generally."},
  {butText_just, 0, ""},
  {butText_center, 0, "NO WARRANTY"},
  {butText_just, 0, ""},
  {butText_just, 0, "  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, "
   "THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY "
   "APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT "
   "HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT "
   "WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT "
   "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A "
   "PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE "
   "OF THE PROGRAM IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU "
   "ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION."},
  {butText_just, 0, ""},
  {butText_just, 0, "  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR "
   "AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO "
   "MAY MODIFY AND/OR REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE "
   "LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL "
   "OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE "
   "PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING "
   "RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A "
   "FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF "
   "SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH "
   "DAMAGES."},
  {butText_just, 0, ""},
  {butText_center, 0, "END OF TERMS AND CONDITIONS"},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};


/*
 * From control.c
 */
static const AbutHelpText  ctrlHelp[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Control Panel"},
  {butText_just, 0, "   The control panel of cgoban is used to select one "
   "of Cgoban's features. The text on this help page will only briefly "
   "describe what feature each button activates.  More detailed help "
   "on each of these features is available once the feature has been "
   "started."},
  {butText_just, 0, ""},
  {butText_just, 2, "Server Buttons"},
  {butText_just, 0, "   These two buttons are used to log in to the "
   "internet go servers.  Clicking on a button will connect to the server "
   "named on that button.  To access more than two servers, hold down shift "
   "and click on a server button; this will change the server that the "
   "button connects to."},
  {butText_just, 0, ""},
  {butText_just, 2, "\"New Game\" and \"Load Game\" Buttons"},
  {butText_just, 0, "   To start a new game of Go, click on the \"New Game\" "
   "button.  Here two players can play against each other on a single "
   "go board."},
  {butText_just, 0, "   Pressing the \"Load Game\" button will let you "
   "continue a previously saved game of go.  You can only use \"Load Game\" "
   "for games started with cgoban.  If you want to load a smart go file "
   "that was created with a different program, then use the \"Edit SGF "
   "File\" button instead."},
  {butText_just, 0, ""},
  {butText_just, 2, "\"Edit SGF File\" Button"},
  {butText_just, 0, "   To edit a smart go file, use the \"Edit SGF File\" "
   "button.  Using this button, you can review an sgf file or edit it, "
   "viewing and adding comments, annotations, and variations.  The "
   "\"Edit SGF File\" button can not be used to create a new SGF file.  "
   "To do that use the \"New Game\" button, then once the game has "
   "started use the \"Edit Game\" button on the go board to switch to "
   "SGF editing mode."},
  {butText_just, 0, "   To edit a file without going through the control "
   "window, you may start cgoban by typing \"cgoban -edit <file>\"."},
  {butText_just, 0, ""},
  {butText_just, 2, "\"Go Modem\" Button"},
  {butText_just, 0, "   The Go Modem protocol is a standard protocol for "
   "playing go over a modem that was designed by Bruce Wilcox and others.  "
   "It has also become a standard format for playing computer go "
   "tournaments.  The \"Go Modem\" button can be used to connect cgoban to a "
   "computer program or a device that talks the go modem protocol.  The \"Go "
   "Modem\" button can also be used to connect a go modem protocol program "
   "to one of the internet go servers."},
  {butText_just, 0, ""},
  {butText_just, 2, "\"Help\", \"Setup\", and \"Quit\" Buttons"},
  {butText_just, 0, "   Pressing \"Help\" brings up this window.  Most "
   "Cgoban windows have help buttons.  Pressing the help button on any "
   "window brings up help for that window."},
  {butText_just, 0, "   The \"Setup\" button will open the cgoban setup "
   "window.  In this window you can configure cgoban, "
   "changing the address of the go servers, turning on and off certain "
   "features, etc."},
  {butText_just, 0, "   Pressing the \"Quit\" button will exit from Cgoban.  "
   "Be careful!  If you press \"Quit\", then all games in progress will "
   "end!"},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  ctrlPage[] = {
  {"Control Window", ctrlHelp},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_control = {4, "Control Help Pages", ctrlPage};

/*
 * From gameSetup.c
 */
static const AbutHelpText  gameSetup[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Game Setup Window"},
  {butText_just, 0, "   This window allows you to set up a new game.  At "
   "the top you may enter the names of the players.  You have a choice of "
   "rule sets in the left pane of the window, and a choice of time controls "
   "in the right pane."},
  {butText_just, 0,
   "   The difference in rule sets and time controls is described in "
   "other help pages.  To see these pages, use the menu at the top of this "
   "help window."},
  {butText_just, 0,
   "   Besides selecting the rule set and the time system, players can "
   "choose the handicap and the komi.  \"Handicap\" is the number of "
   "stones that Black is given at the start of the game.  Depending on "
   "the rule set, Black may be free to place these stones where they "
   "choose or may be forced to play them in certain locations.  A handicap "
   "of zero will give Black no stones to begin the game."},
  {butText_just, 0,
   "   The komi is the number of points that White is given at the "
   "beginning of the game to make up for having to move second.  Most "
   "rule sets say that a komi of 5.5 with a handicap of zero lead to a fair "
   "game (the extra half point prevents tie games).  The Ing rules, "
   "however, say that a komi of 8.0 is fair.  Often, a game with no "
   "handicap and a komi of zero is called a \"handicap 1\" game because "
   "the Black player gets the advantage of moving first and no penalty."},
  {butText_just, 0,
   "   When the game is configured the way you want, press the \"Ok\" "
   "button to start the game, or press the \"Cancel\" button if you decide "
   "not to start the game at all."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  ruleSets[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Rule Sets"},
  {butText_just, 0,
   "   Depending on where Go is played, the rules of the game can differ "
   "sligtly.  Cgoban allows you to pick the rule set that you are most "
   "comfortable with.  This help page is not a description of the rules of "
   "go; it is only meant to outline the differences between the rule sets."},
  {butText_just, 0,
   "   The biggest differences between the rule sets are typically the ko "
   "rule used and the way that the game is scored."},
  {butText_just, 0, ""},
  {butText_just, 2, "Chinese Rules"},
  {butText_just, 0,
   "   Chinese rules are played primarily on mainland China.  Chinese "
   "rules use the Superko rule.  The superko rule states that it is illegal "
   "to make a move which returns the board to an earlier position.  A game "
   "played under Chinese rules ends when both players pass; at that point "
   "score is counted.  The score of a player is the number of living stones "
   "that player has on the board, plus the amount of territory surrounded, "
   "plus one-half the number of dame points on the board.  If there is a "
   "dispute over which stones are dead at the end of the game, the players "
   "can simply continue the game where they left off to resolve the "
   "problem."},
  {butText_just, 0,
   "   Chinese rules are considered by some people to be the most "
   "mathematically elegant version of the rules of go."},
  {butText_just, 0, ""},
  {butText_just, 2, "Japanese Rules"},
  {butText_just, 0,
   "   Japanese rules are played in Japan and also have strong followings "
   "in many other countries.  The Japanese ko rule states that it is "
   "illegal to recreate the board position of exactly two moves ago.  "
   "There are certain board positions that can lead to cycles of more than "
   "two moves; if a game ever falls into one of these positions, the "
   "players repeat until they conclude that the game will never finish.  "
   "At this point the game is regarded as \"no result\" (not as a draw).  "
   "If this is a game in a tournament, it must be replayed."},
  {butText_just, 0,
   "   The score in a Japanese game is the total of the number of enemy "
   "stones captured and the amount of territory surrounded.  Territory in "
   "a seki is not counted towards either player's score.  If there is "
   "a dispute over which stones are dead at the end of the game, the "
   "fate of the disputed stones is resolved locally.  Cgoban handles this "
   "by letting the players continue the game.  When resolving the "
   "dispute, ko stones can never be retaken until a player passes, and "
   "after three passes the stones are considered dead only if all of them "
   "have been removed from the board.  At this point the board is returned "
   "to the original end game position but with the disputed stones marked "
   "as alive or dead."},
  {butText_just, 0,
   "   Japanese rules state that suicide (making a move which kills your "
   "own stones) is illegal.  Under some rare circumstances such a move may "
   "be useful, so most other rule sets allow it."},
  {butText_just, 0,
   "   Under some situations cgoban will not recognize a seki unless the "
   "dame are filled in.  If there is a seki position on the board and "
   "you see that cgoban is mis-scoring it, continue the game and fill in "
   "the dame."},
  {butText_just, 0,
   "   Some people find the Japanese rules to be the most natural "
   "to play with.  Most of Europe and North America plays with rule sets "
   "based on the Japanese rules."},
  {butText_just, 0, ""},
  {butText_just, 2, "Ing Rules"},
  {butText_just, 0,
   "   The Ing rules were invented by a wealthy Taiwanese Go player.  They "
   "are encouraged through the Ing Foundation, which sponsors Go "
   "tournaments around the world.  Unfortunately, the Ing Ko rule is very "
   "difficult (some say impossible) to implement on a computer, so the "
   "Ing rules of Cgoban are the same as Chinese rules except that the "
   "standard Komi is 8 and tie games are awarded to Black."},
  {butText_just, 0, ""},
  {butText_just, 2, "AGA Rules"},
  {butText_just, 0,
   "   The American Go Association has a set of rules designed to feel "
   "like Japanese rules, but use the simpler end of game disputes of "
   "Chinese rules.  AGA rules use Superko like Chinese rules, but count "
   "score as captures plus territory (although unlike Japanese rules, eyes "
   "in a seki count for points under AGA rules).  In addition, every time "
   "that you pass under AGA rules you must give your opponent one capture.  "
   "This change makes it reasonable to resolve disputed stones by "
   "continuing the game, as in Chinese rules."},
  {butText_just, 0, ""},
  {butText_just, 2, "New Zealand Rules"},
  {butText_just, 0,
   "   In New Zealand the go organization has adopted the Chinese rules, "
   "with the exception that handicap stones may be placed anywhere on the "
   "board instead of at the preselected star points.  In every other way "
   "it is the same as Chinese rules."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  timeTypes[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Time Formats"},
  {butText_just, 0,
   "   Much like the rule sets, as Go has travelled around the world many "
   "different ways of timing games have been used.  Cgoban makes available "
   "many of the more popular.  To select a time control, first choose the "
   "type of time control with the \"Time System\" menu, then fill in the "
   "appropriate time values."},
  {butText_just, 0, ""},
  {butText_just, 2, "None"},
  {butText_just, 0,
   "   If you select \"None\" as your time system, then the players may "
   "take as long as they wish to make their moves."},
  {butText_just, 0, ""},
  {butText_just, 2, "Absolute Time"},
  {butText_just, 0,
   "   With the \"Absolute\" time system, you simply set a main time for "
   "the players.  Each player must finish the game within the time "
   "specified.  If the player runs out of time, then the player loses."},
  {butText_just, 0, ""},
  {butText_just, 2, "Japanese Time"},
  {butText_just, 0,
   "   In Japan one of the common tournament time systems involves a "
   "main time and what are called \"byo-yomi\" periods.  The easiest way "
   "to explain this is with an example.  If you play with 45 minutes main "
   "time and five one-minute byo-yomi periods, then each player starts with "
   "45 minutes on the clock.  Play goes as with an absolute time limit, but "
   "when a player's clock drops below the five minute mark the enter the "
   "first byo-yomi period.  As long as they finish before their clock drops "
   "below the four minute mark, it will be reset to five minutes.  From "
   "then on, they can play as many moves as they want, but if they take "
   "more than a minute on any move their clock moves to the second byo-yomi "
   "period, which is from four to three minutes.  If they ever use up their "
   "last byo-yomi period, then they lose the game."},
  {butText_just, 0,
   "   This scheme gives players lots of time to think until they have "
   "used up their main time, then forces them to keep moving with only a "
   "few opportunities to think for long after that."},
  {butText_just, 0, ""},
  {butText_just, 2, "Canadian Time"},
  {butText_just, 0,
   "   The canadian time system is also used on the internet go servers.  "
   "It also has a main time period and what are called byo-yomi periods, "
   "but the byo-yomi periods work differently.  Again, an example will "
   "be a good way to show how this works.  If you have a 30 minute main "
   "time period and byo-yomi periods of 25 moves in ten minutes, then each "
   "player starts with 30 minutes on their clock.  After that time is gone, "
   "their clock is reset to ten minutes.  They must make 25 moves in that "
   "ten minutes or lose.  If they make 25 moves, then no matter how much "
   "time is left on their clock it is reset to ten minutes again and they "
   "must make another 25 moves in that ten minutes.  This can continue "
   "indefinitely."},
  {butText_just, 0, ""},
  {butText_just, 2, "Ing Time"},
  {butText_just, 0,
   "   Like the Ing rules, the Ing time controls are encouraged by the "
   "Ing foundation.  Under Ing rules, players are given main time and "
   "extra time periods, but the players are penalized tw, points for "
   "entering each extra time periods."},
  {butText_just, 0,
   "   If players have 45 minutes and three 15 minute periods, a player who "
   "used 55 minutes to make their moves will lose two points.  A player who "
   "used 80 minutes will lose six points, and a player who used only 40 "
   "minutes will lose no points."},
  {butText_just, 0, ""},
  {butText_just, 0, NULL}};
static const AbutHelpPage  gameSetupPage[] = {
  {"Game Setup Window", gameSetup},
  {"Rule Sets", ruleSets},
  {"Time Controls", timeTypes},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_gameSetup = {6, "Game Setup Help Pages", gameSetupPage};

static const AbutHelpText  editToolHelp[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "About Smart Go Files"},
  {butText_just, 0,
   "   Smart go files can contain a game complete with many different "
   "types of annotation.  A game may contain marks on the board that "
   "become visible as the game is viewed, it make contain comments about "
   "the game as it progresses, and it may also contain variations."},
  {butText_just, 0,
   "   The variations of a smart go file make it into what can be called a "
   "move tree.  The starting position (typically an empty board) is at the "
   "start of the move tree.  From each move in the game, you may progress "
   "to the next move.  When there is a variation, there may be several "
   "different moves to go to.  Typically one move will be the move actually "
   "made during game play, and the other moves show what would have "
   "happened if the players had made different choices."},
  {butText_just, 0,
   "   As you are viewing a game, the \"active move list\" is the set of "
   "moves that you will see if you step through the game.  You may choose "
   "which moves are active with the game map (see below).  The active moves "
   "are also the moves that will be affected by the cut and reorder "
   "buttons."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Edit Tool Window"},
  {butText_just, 0,
   "   The edit tool window of cgoban is used to edit a smart go file.  "
   "It contains three parts; the tool selector, the cut/reorder buttons, "
   "and the game map."},
  
  {butText_just, 0, ""},
  {butText_just, 2, "The Tool Selector"},
  {butText_just, 0,
   "   The tool selector is the line of buttons on the top of the edit tool "
   "window.  Click on a button to make that the active tool.  The effects "
   "of the current tool are listed just below the tool selector.  A "
   "complete description of each tool is available through the menu at the "
   "top of this help window."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Delete Moves Button"},
  {butText_just, 0,
   "   Pressing \"Delete Moves\" will remove all of the active moves below "
   "your current move.  Be careful!  There is currently no way to undo if "
   "you accidentally delete some moves"},
  {butText_just, 0, ""},
  {butText_just, 2, "The Reorder Moves Button"},
  {butText_just, 0,
   "   Pressing \"Reorder Moves\" is useful when you have several "
   "variations to your current move.  Pressing this button will change "
   "the order in which the variations appear."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Game Map"},
  {butText_just, 0,
   "   The game map shows the same game that is being viewed on the "
   "board, but it shows the game as a list of moves instead of showing "
   "the current board position.  For a game with no variations, this list "
   "will be a simple line of stones.  For a game with many variations, "
   "however, the game map can be invaluable in finding interesting "
   "variations and keeping your place within the game."},
  {butText_just, 0,
   "   The active move list is shown as a set of moves along a line.  "
   "variation that are not active (that is, variations that you will not "
   "see by just stepping through the move list) are shown in grey.  You "
   "may skip to a new move by clicking on the mark that corresponds to "
   "that move.  If you click on a non-active move, then the active move "
   "list will change to the move list that contains the move you clicked "
   "on.  The move that you see on the board is always shown with a box "
   "around it."},
  {butText_just, 0,
   "   On the game map, the starting board position is in the upper-left "
   "of the map.  Each game move is to the right of the previous move.  "
   "When there are several moves that are variations from a single board "
   "position, these will be seen as a \"fork\" in the game with each "
   "possible variation as a separate move chain."},
  {butText_just, 0,
   "   Different types of moves are represented by different symbols "
   "on the game map.  Simple moves are shown as a black or white stone.  "
   "If there is some sort of comment or annotation to the move, a "
   "triangle move is placed on the stone; otherwise the stone is labeled "
   "with its move number."},
  {butText_just, 0,
   "   Moves that are not strictly game moves, but instead modifications "
   "to the board, are represented by groups of four stones on the map.  "
   "An empty spot on a move chain means that there is no move information "
   "here."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  moveTool[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Move Tool"},
  {butText_just, 0,
   "   The \"Move Tool\" is represented on the tool selector as a plain "
   "stone.  The color of the stone shows whose move it is.  By clicking on "
   "the stone you may set whose turn it is to move."},
  {butText_just, 0,
   "   When you click on the board with the move tool, you will add moves "
   "to the game that you are viewing.  These moves will be added to the "
   "game as variations off of the current move if there is already a move "
   "off of the current move."},
  {butText_just, 0,
   "   Holding down the shift key when you use the move tool will not add "
   "stones to the board.  Instead it will make the current move change to "
   "the point when a specific move was made.  For example, to see the "
   "board when a move was made at location C16, hold down shift and click "
   "on the board at location C16."},
  {butText_just, 0,
   "   Clicking on the move tool when it is already active will change "
   "whose move it is."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  editToolKey[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Edit Tool Keyboard Shortcuts"},
  {butText_just, 0,
   "   Shift-up changes the active move list to the previous variation."},
  {butText_just, 0,
   "   Shift-down changes the active move list to the next variation."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  changeTool[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Change Board Tool"},
  {butText_just, 0,
   "   The \"Change Board Tool\" is represented on the tool selector as a "
   "group of four stones.  This tool lets you rearrange a lot of stones "
   "with a single move; it is useful for setting up board positions and "
   "erasing groups of stones."},
  {butText_just, 0,
   "   Clicking on the board with this tool will erase a stone where you "
   "click, or add a stone if there isn't a stone there.  The added stone "
   "will be white.  To add a black stone, hold down the shift key when you "
   "click."},
  {butText_just, 0,
   "   Pressing \"Pass\" when the change board tool is selected will make "
   "all new board changes become part of a new board.  This lets you create "
   "a sequence of moves, each rearranging the board however you wish."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  scoreTool[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Score Tool"},
  {butText_just, 0,
   "   The \"Score Tool\" is represented on the tool selector as a yin-yang "
   "symbol.  This tallies up the current score of the game and adds marks "
   "to show who own what territory."},
  {butText_just, 0,
   "   When you select this tool, all territory completely surrounded by "
   "one player will be marked as that player's territory.  Now clicking "
   "on stones will mark them as dead.  To unmark some dead stones, click "
   "on them again."},
  {butText_just, 0,
   "   When you are satisfied with which stones are dead and alive, press "
   "the \"Done\" button next to the board.  This will add a comment to "
   "the game record stating the score.  To stop scoring and remove all "
   "marks, select a different tool."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  markTools[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Marking Tools Tool"},
  {butText_just, 0,
   "   The marking tools are represented on the tool selector as stones "
   "marked with a triangle, circle, or square.  All three of these tools "
   "behaves in the same way; only the mark added is different."},
  {butText_just, 0,
   "   When one of the three marking tools is selected and you click on "
   "the board, the appropriate mark will be added.  Holding down shift "
   "when you click will mark a group of stones."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  labelTools[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Label Tools"},
  {butText_just, 0,
   "   The label tools are represented on the tool selector as a stones "
   "with a letter or number on them.  The letter label tool and the "
   "number label tool behave in very similar ways."},
  {butText_just, 0,
   "   When you click on the board with a label tool, the appropriate "
   "type of label (a letter or a number) is added.  The labels will start "
   "at \"A\" or \"1\" and count up for each label that you add.  Holding "
   "down shift when you click with the numbering tool will add the move "
   "number of that stone instead of starting at one."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  editToolPage[] = {
  {"Edit Tool Window", editToolHelp},
  {"Keyboard Shortcuts", editToolKey},
  {"Move Tool", moveTool},
  {"Change Board Tool", changeTool},
  {"Score Tool", scoreTool},
  {"Marking Tools", markTools},
  {"Label Tools", labelTools},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_editTool = {10, "Edit Tool Help Pages", editToolPage};


/*
 * From client/main.c
 */
static const AbutHelpText  cliMain[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Client Main Window"},
  {butText_just, 0,
   "   When cgoban is connected to one of the servers, the client main "
   "window is the central area for information.  There is a log in "
   "the middle of this window which shows the output from the server.  "
   "Above the log are two buttons that open other windows; see the "
   "\"Client Game List Window\" and \"Client Player List Window\" help "
   "pages in the menu at the top of this help page.  Below the log "
   "are a text input box, the help button and the \"Quit\" button.  The "
   "\"Quit\" button will disconnect you from the server."},
  {butText_just, 0,
   "   Any commands that you want to send to the server should be typed "
   "in the text input box below the log.  If you need help on server "
   "commands, type the \"Help\" command here.  These help pages only give "
   "help for this client, not for the servers themselves."},
  {butText_just, 0,
   "   Some server commands won\'t work with this client if you try to "
   "enter them directly.  For example, the \"who\" command should not be "
   "used because its output will be caught by the client and not shown; if "
   "you want a list of players, open the \"Player List\" window.  Other "
   "commands that will not work correctly are the \"games\" and \"observe\" "
   "commands.  Other commands may not be needed because their "
   "functionality is already present in the client, but they will work "
   "if you want to use them."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};

static const AbutHelpText  cliGames[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Client Game List Window"},
  {butText_just, 0,
   "   The game list window presents a list of all games in progress on "
   "the server.  The list will be automatically kept up to date as games "
   "start and end."},
  {butText_just, 0,
   "   Each line of the list is one game.  From left to right, the columns "
   "of the list are the game number, the name and rank of the white "
   "player, the name and rank of the black player, the size of the board "
   "used, the number of handicap stones given to black, the komi points "
   "given to white, the game flags, and the number of players observing the "
   "game."},
  {butText_just, 0, "   When a game ends, the game number will be replaced "
   "with a hyphen and the \"Flags\" and \"Observers\" columns will be "
   "replaced with the result of the game.  This will remain until another "
   "game is started in that game slot or until the \"*\" button is pressed "
   "(see below)."},
  {butText_just, 0,
   "   If you can't see all of the columns, then resize the window to be "
   "bigger.  For a description of exactly what the column means, see "
   "the server's help file on the \"games\" command."},
  {butText_just, 0,
   "   To observe a game in progress, click on the game in the list."},
  {butText_just, 0,
   "   Not all of these fields will be kept up to date.  To refresh the "
   "window at any time press the \"*\" button in the upper left corner of "
   "the list.  This will also clear out all old game results for games "
   "that have ended."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};

static const AbutHelpText  cliPlayers[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Client Player List Window"},
  {butText_just, 0,
   "   The player list window is a list of all players currently logged in "
   "to the server.  As players come and go they will appear and disappear "
   "on this list."},
  {butText_just, 0,
   "   Each line of the list is one player.  From left to right, the "
   "column of the list are the player\'s name, the player\'s rank, the "
   "player\'s state, how long the player has been idle, and the game number "
   "that the player is currently observing or playing in."},
  {butText_just, 0,
   "   The state is one of four options: \"!\" means that the player is "
   "actively looking for games,  \"O\" means that the player is open to "
   "playing, \"X\" means that the player is not available for games, and "
   "\"?\" means that the client does not know what state the player is in"},
  {butText_just, 0,
   "   The list is sorted.  You have three choices on how to sort the list; "
   "pick one by clicking on the buttons at the top of the player list.  "
   "Clicking on \"Name\" will sort the list alphabetically by name.  "
   "Clicking on \"Rank\" will sort the list by rank, strongest players "
   "to weakest players.  Clicking on \"State\" will sort the list by "
   "state, with the players looking for games at the top and players "
   "not interested in games at the bottom."},
  {butText_just, 0,
   "   If you click on a player\'s name, the complete status of that "
   "player will be fetched from the server and displayed in the log in "
   "the main window."},
  {butText_just, 0,
   "   If the player is available for games, then you can click on the "
   "player's state to open up a \"Server Game Setup\" window.  Use this "
   "window to set up games against the other players on the server."},
  {butText_just, 0,
   "   As new players arrive, they are placed at the bottom of the list.  "
   "To update the list and re-sort it, press the \"*\" button in the upper "
   "left corner of the list."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  cliMainPage[] = {
  {"Main Client Window", cliMain},
  {"Game List Window", cliGames},
  {"Player List Window", cliPlayers},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_cliMain = {6, "Client Help Pages", cliMainPage};

/* From "client/match.c" */
static const AbutHelpText  cliMatch[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Server Game Setup Window"},
  {butText_just, 0,
   "   The server game setup window is used to set up games against other "
   "people on the server.  To use this, first you set up the game "
   "parameters as you wish, then when you are ready you click on the "
   "\"OK\" button.  This will send the match request to the other player.  "
   "When they send you a matching request, the game will begin!"},
  {butText_just, 0,
   "   At the top are the names of the players (you and your opponent).  "
   "To change who plays black and who white, press the \"Switch\" button."},
  {butText_just, 0,
   "   Below this are places where you can choose the rules of the game - "
   "the size, handicap, etc.  Most of these should be self-explanatory if "
   "you are familiar with playing go on the servers.  If you have any "
   "questions, then you may want to try the online help files from the "
   "server.  Typing \"help match\", \"help komi\", etc. on the server "
   "command line should give you the help that you need."},
  {butText_just, 0,
   "   The \"Free Game\" button will determine whether the game counts "
   "towards your rank.  Usually you want this to happen, but if you want "
   "to just experiment during a game or play casually, then check this "
   "box."},
  {butText_just, 0,
   "   Click \"Cancel\" if you decide that you don't want to play the game "
   "at all.  This will take away the window."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  cliMatchPage[] = {
  {"Server Game Setup Window", cliMatch},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_cliMatch = {4, "Client Help Pages", cliMatchPage};

/* From "editBoard.c" */
static const AbutHelpText  editBoard[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Edit Board Window"},
  {butText_just, 0,
   "   When editing a smart-go file, the current board position is "
   "displayed in the edit board window.  Around the edit board window are "
   "various buttons and controls."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Title Box"},
  {butText_just, 0,
   "   At the top left of the window is the title box.  This shows the "
   "name of the game in bold, and below that which moves are currently "
   "visible on the board."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Board"},
  {butText_just, 0,
   "   Just below the title box is the board.  Clicking on a point on "
   "the board will have different effects depending on which edit tool "
   "you are using; press \"Help\" in the edit tool window for more "
   "infomation.  With the most common tool, the \"Move Tool\", clicking "
   "on the board will add a variation.  Holding down shift when you "
   "click on the board will change your move number to see when a "
   "particular move was made."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \""CGBUTS_REWCHAR"\", \""CGBUTS_BACKCHAR"\", "
   "\""CGBUTS_FWDCHAR"\", and \""CGBUTS_FFCHAR"\" Buttons"},
  {butText_just, 0,
   "   Pressing these buttons will change the current move number that "
   "you can see on the board.  "CGBUTS_BACKCHAR" and "CGBUTS_FWDCHAR" "
   "will go one move at a time.  "CGBUTS_REWCHAR" moves you to the "
   "beginning of the game, and "CGBUTS_FFCHAR" moves you to the end."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Score Boxes"},
  {butText_just, 0,
   "   To the right of the board are the score boxes.  There are two; one "
   "for white and one for black.  In each box are shown the current score "
   "for that player and the amount of time remaining on that player's "
   "clock.  The score is not the final score for that player, but the sum "
   "of the number of captures and the komi.  To see the final score use "
   "the \"Score Tool\" in the edit tool window."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Save Game\" Button"},
  {butText_just, 0,
   "   Pressing \"Save Game\" will let you save the changes that you have "
   "made to the smart-go file."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Quit\" Button"},
  {butText_just, 0,
   "   Pressing the \"Quit\" button will close the edit board window and "
   "the edit tool window.  If you have made any changes to the smart go "
   "file that you were viewing then you will be asked whether you want to "
   "save these changes before you exit."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Comments Box"},
  {butText_just, 0,
   "   At the very bottom of the window is the comments box.  Here you "
   "will see any comments that have been made about the move you are "
   "viewing.  To edit or add the comments, click in the comments box "
   "and start typing.  Some emacs-style control keys are available; "
   "see the \"Keyboard Shortcuts\" help page for a list of them."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  editBoardInfo[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Game Info Window"},
  {butText_just, 0, "   You open the game info window by pressing the "
   "\"Game Info\" button when you are viewing a smart go file."},
  {butText_just, 0, "   The game info window lets you view and edit the "
   "smart go game properties, such as player names, etc.  To change "
   "the properties simply type in your new value then save the game.  "
   "You should be warned that changing a property does not change the "
   "game itself, only the description of the game.  For example, if you "
   "change the handicap then handicap stones will not be automatically "
   "added to the game; the game will be described as having the new "
   "handicap but to see this change you must add the stones yourself."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  editBoardKey[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Edit Board Keyboard Shortcuts"},
  {butText_just, 0, "   Left arrow: Go back one move."},
  {butText_just, 0, "   Right arrow: Go ahead one move."},
  {butText_just, 0, "   Shift-left arrow: Go back to the previous "
   "variation, comment, or annotation."},
  {butText_just, 0, "   Shift-right arrow: Go ahead to the next "
   "variation, comment, or annotation."},
  {butText_just, 0, "   Shift-up arrow: Change the main variation to "
   "the previous variation."},
  {butText_just, 0, "   Shift-down arrow: Change the main variation to "
   "the next variation."},
  {butText_just, 0, ""},
  {butText_just, 2, "Comment Editing"},
  {butText_just, 0, "   When you are editing a comment the left and right "
   "arrows will not change moves.  Instead they will move the cursor.  "
   "These other commands are available:"},
  {butText_just, 0, "   Ctrl-A: Move cursor to the beginning of the line."},
  {butText_just, 0, "   Ctrl-D: Delete to the right of the cursor"},
  {butText_just, 0, "   Ctrl-E: Move cursor to the end of the line."},
  {butText_just, 0, "   Ctrl-K: Delete to the end of the line."},
  {butText_just, 0, "   Ctrl-U: Delete from the beginning of the line."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};

static const AbutHelpPage  editBoardPage[] = {
  {"Edit Board Window", editBoard},
  {"Game Info Window", editBoardInfo},
  {"Keyboard Shortcuts", editBoardKey},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_editBoard = {6, "Edit Board Help Pages", editBoardPage};

/* From "local.c" */
static const AbutHelpText  localBoard[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Game Board"},
  {butText_just, 0, "   When two players are playing go with Cgoban, the "
   "board is shown in a window with controls used in the game around it.  "
   "To make a move, click on the board where you want your stone to go.  "
   "The last stone played will have a circle marker on it; if the last "
   "move was a ko move, then the stone will have a square marker on it "
   "instead."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Title Box"},
  {butText_just, 0, "   Just above the board is the title box.  Here, the "
   "title of the game is shown along with the current move number and "
   "whose move it is."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Pass\" Button"},
  {butText_just, 0, "   Pressing \"Pass\" will pass your turn.  The game will "
   "enter the scoring stage after both players pass."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Dispute\" Button"},
  {butText_just, 0, "   When you are playing with Japanese scoring, the "
   "button just below the pass button is the \"Dispute\" button.  If "
   "the players disagree on whether a group of stones are alive or dead, "
   "they can resolve the disagreement by pressing \"Dispute\".  After "
   "pressing dispute, the players select which group of stones are "
   "the problem.  Then the game continues, with slightly different rules.  "
   "When you are resolving disputes, kos cannot be retaken until a player "
   "passes and it takes three consecutive passes to end the dispute.  After "
   "the three passes, the board is put back to the position it had before "
   "the dispute.  If any of the disputed stones are still alive, then "
   "they are alive for scoring purposes; if they are all dead, then they "
   "are dead for scoring purposes."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Resume\" button"},
  {butText_just, 0, "   When you are not using Japanese rules, any dispute "
   "at the end of the game can be resolved by simply undoing the last two "
   "passes and continuing the game.  Pressing \"Resume\" will let you do "
   "that."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Done\" button"},
  {butText_just, 0, "   When both players are done marking dead stones, "
   "pressing \"Done\" will finish it and record the score."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \""CGBUTS_REWCHAR"\", \""CGBUTS_BACKCHAR"\", "
   "\""CGBUTS_FWDCHAR"\", and \""CGBUTS_FFCHAR"\" Buttons"},
  {butText_just, 0,
   "   Pressing these buttons will move forward and backward through the "
   "game, letting you undo and redo moves.  "CGBUTS_BACKCHAR" and "
   CGBUTS_FWDCHAR" will go one move at a time.  "CGBUTS_REWCHAR" moves "
   "you to the beginning of the game, and "CGBUTS_FFCHAR" moves you to the "
   "end."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Score Boxes"},
  {butText_just, 0,
   "   To the right of the board are the score boxes.  There are two; one "
   "for white and one for black.  In each box are shown the current score "
   "for that player and the amount of time remaining on that player's "
   "clock.  When you are playing the game, the score is not the final "
   "score for that player, but the sum of the number of captures and the "
   "komi.  When you are selecting dead stones and after pressing the "
   "\"Done\" button, the score is the actual score for that player."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Save Game\" Button"},
  {butText_just, 0,
   "   Pressing \"Save Game\" will save the game as a smart go file.  Later "
   "you can continue the game with the \"Load Game\" button of the control "
   "window.  You can also choose \"Edit Game\" from the control panel to "
   "edit the game that you have saved.  This is currently the only way to "
   "create a new smart go file since the \"Edit Game\" button doesn't "
   "work."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Edit Game\" Button"},
  {butText_just, 0,
   "   Pressing \"Edit Game\" will pop up a second game board with the "
   "game on it.  This will be an edit game board, not a playing game "
   "board, and it will come with a edit tool set.  This is used if you "
   "want to stop playing a game and want to use the editing tools to "
   "annotate your game."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Quit\" Button"},
  {butText_just, 0,
   "   Pressing \"Quit\" will end the game.  If you want to continue the "
   "game later, you should choose \"Save Game\" before you quit."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Comments Box"},
  {butText_just, 0,
   "   At the very bottom of the window is the comments box.  Here you "
   "can make comments about the game that will be saved in the smart go "
   "file if you choose \"Save Game\" later.  To edit or add the comments, "
   "click in the comments box and start typing.  Some emacs-style control "
   "keys are available; see the \"Keyboard Shortcuts\" help page for a "
   "list of them."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  localKeys[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Playing Go Keyboard Shortcuts"},
  {butText_just, 0, "   Left arrow: Go back one move."},
  {butText_just, 0, "   Right arrow: Go ahead one move."},
  {butText_just, 0, ""},
  {butText_just, 2, "Comment Editing"},
  {butText_just, 0, "   When you are editing a comment the left and right "
   "arrows will not change moves.  Instead they will move the cursor.  "
   "These other commands are available:"},
  {butText_just, 0, "   Ctrl-A: Move cursor to the beginning of the line."},
  {butText_just, 0, "   Ctrl-D: Delete to the right of the cursor"},
  {butText_just, 0, "   Ctrl-E: Move cursor to the end of the line."},
  {butText_just, 0, "   Ctrl-K: Delete to the end of the line."},
  {butText_just, 0, "   Ctrl-U: Delete from the beginning of the line."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  localBoardPage[] = {
  {"Playing Go", localBoard},
  {"Keyboard Shortcuts", localKeys},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_localBoard = {5, "Playing Go Help Pages", localBoardPage};

/* From "client/board.c" */
static const AbutHelpText  cliBoardPage[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Game Board"},
  {butText_just, 0, "   When you play or observe a game on the server, the "
   "board is shown in a window with controls used in the game around it.  "
   "To make a move, click on the board where you want your stone to go.  "
   "The last stone played will have a circle marker on it; if the last "
   "move was a ko move, then the stone will have a square marker on it "
   "instead."},
  {butText_just, 0, "   You can also skip around in games or try out "
   "variations by holding down shift or control as you click on the board.  "
   "See the \"Keyboard Shortcuts\" help page for more information on this."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Title Box"},
  {butText_just, 0, "   Just above the board is the title box.  Here, the "
   "title of the game is shown along with the current move number and "
   "whose move it is."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Pass\" Button"},
  {butText_just, 0, "   Pressing \"Pass\" will pass your turn.  The game will "
   "enter the scoring stage after both players pass."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Done\" button"},
  {butText_just, 0, "   When you are done marking dead stones, "
   "pressing \"Done\" will finish it and record the score."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \""CGBUTS_REWCHAR"\", \""CGBUTS_BACKCHAR"\", "
   "\""CGBUTS_FWDCHAR"\", and \""CGBUTS_FFCHAR"\" Buttons"},
  {butText_just, 0,
   "   Pressing these buttons will move forward and backward through the "
   "game to see what order the moves occurred in.  They do not undo or "
   "redo moves; they only let you look back at previous moves.  "
   CGBUTS_BACKCHAR" and "
   CGBUTS_FWDCHAR" will go one move at a time.  "CGBUTS_REWCHAR" moves "
   "you to the beginning of the game, and "CGBUTS_FFCHAR" moves you to the "
   "end."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Score Boxes"},
  {butText_just, 0,
   "   To the right of the board are the score boxes.  There are two; one "
   "for white and one for black.  In each box are shown the current score "
   "for that player and the amount of time remaining on that player's "
   "clock.  When you are playing the game, the score is not the final "
   "score for that player, but the sum of the number of captures and the "
   "komi.  When you are selecting dead stones and after pressing the "
   "\"Done\" button, the score is the actual score for that player."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Adjourn\" Button"},
  {butText_just, 0,
   "   Pressing \"Adjourn\" will send a request to adjourn the game when "
   "you are playing.  The other player must choose to adjourn also; then "
   "the game will be saved on the server and you can safely stop playing."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Resign\" and \"Close\" Buttons"},
  {butText_just, 0,
   "   If you are playing a game, there will be a button labeled "
   "\"Resign\".  Pressing this will resign the game.  You will be asked "
   "whether you really want to resign just in case you pressed the button "
   "by accident!"},
  {butText_just, 0,
   "   If you are not playing in the game, then the button will say "
   "\"close\" instead and pressing it will close the window showing the "
   "game."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Edit Game\" Button"},
  {butText_just, 0,
   "   Pressing \"Edit Game\" will pop up a second game board with the "
   "game on it.  This will be an edit game board, not a server game "
   "board, and it will come with a edit tool set.  This is used if you "
   "want to the editing tools to annotate the game that your are "
   "observing or playing in."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Kibitz Box"},
  {butText_just, 0,
   "   At the very bottom of the window is the kibitz box.  If you are "
   "observing the game, then you will see kibitzes here.  If you are "
   "playing, then you will see messages from your opponent here."},
  {butText_just, 0,
   "   There is an area below the kibitz box where you can type your "
   "own kibitzes.  Observers can only kibitz here; if you are playing, "
   "then the radio button to the right of the text entry box controls "
   "whether your message is seen by the observers (the \"Kib\" setting), "
   "your opponent (the \"Say\" setting) or everybody (the \"Both\" "
   "setting)."},
  {butText_just, 0,
   "   When you are typing a message, the left and right arrows will not "
   "move you forwards and backwards in the game.  Instead they will move "
   "the text cursor.  To get rid of the text cursor and enable the "
   "left and right arrows again, just press \"Return\" on an empty "
   "kibitz."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  cliBoardKeyPage[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Client Play Keyboard Shortcuts"},
  {butText_just, 0, "   Left arrow: Go back one move."},
  {butText_just, 0, "   Right arrow: Go ahead one move."},
  {butText_just, 0,
   "   Holding down shift when you click on the board will move the "
   "game forward or back to when that move was played."},
  {butText_just, 0,
   "   Holding down control when you click on the board will let you try "
   "out variations in the game you are observing or playing.  Press "
   "the \"" CGBUTS_FFCHAR "\" button to return to the game."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  cliBoard[] = {
  {"Server Play", cliBoardPage},
  {"Keyboard Shortcuts", cliBoardKeyPage},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_cliBoard = {5, "Server Play Help Pages", cliBoard};

/* From "client/setup.c" */
static const AbutHelpText  cliSetup[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Connecting to Go Servers"},
  {butText_just, 0,
   "   The internet go servers provide a way for go "
   "players around the world to meet to talk and play go.  If your computer "
   "can connect to these servers then you too can enjoy this.  In some "
   "parts of the world the go servers is the only way to find games of "
   "go on a regular basis!"},
  {butText_just, 0,
   "   To connect, fill out your username and password "
   "in the areas provided.  You will not see your password as you type it "
   "for security reasons.  Your username and password will be stored in "
   "your resource file so that you will not have to type them next time "
   "that you log in.  This does mean, however, that you should be careful "
   "about letting people see your resource file!  The default name for "
   "this file is \".cgobanrc\"; you should protect it if you are worried "
   "about your password getting out."},
  {butText_just, 0,
   "   If you don't have an account on the server you are "
   "connecting to, just follow the directions in the connect window and "
   "you will log in as a guest."},
  {butText_just, 0,
   "   If you are behind a firewall and need to use a proxy "
   "agent to connect to a server, then press the \"setup\" button on the "
   "main window.  This will allow you to configure the servers to go "
   "through a proxy.  It will also enable you to add or remove servers."},
  {butText_just, 0,
   "   To change the machine name or port numbers used to "
   "connect to the servers, press the \"Setup\" button in the control "
   "window."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  cliSetupPage[] = {
  {"Server Connect", cliSetup},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_cliSetup = {4, "Server Connect Help Pages", cliSetupPage};

/* From "gmp/play.c" */
static const AbutHelpText  gmpBoard[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "The Game Board"},
  {butText_just, 0, "   When using go modem protocol to play a game, the "
   "board is shown in a window with controls used in the game around it.  "
   "To make a move, click on the board where you want your stone to go.  "
   "The last stone played will have a circle marker on it; if the last "
   "move was a ko move, then the stone will have a square marker on it "
   "instead."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Title Box"},
  {butText_just, 0, "   Just above the board is the title box.  Here, the "
   "title of the game is shown along with the current move number and "
   "whose move it is."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Pass\" Button"},
  {butText_just, 0, "   Pressing \"Pass\" will pass your turn.  The game will "
   "enter the scoring stage after both players pass."},
  {butText_just, 0, "   Go modem protocol has no way to indicate which stones "
   "are dead, so that must be done by a person."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Dispute\" Button"},
  {butText_just, 0, "   When you are playing with Japanese scoring, the "
   "button just below the pass button is the \"Dispute\" button.  If "
   "the players disagree on whether a group of stones are alive or dead, "
   "they can resolve the disagreement by pressing \"Dispute\".  After "
   "pressing dispute, the players select which group of stones are "
   "the problem.  Then the game continues, with slightly different rules.  "
   "When you are resolving disputes, kos cannot be retaken until a player "
   "passes and it takes three consecutive passes to end the dispute.  After "
   "the three passes, the board is put back to the position it had before "
   "the dispute.  If any of the disputed stones are still alive, then "
   "they are alive for scoring purposes; if they are all dead, then they "
   "are dead for scoring purposes."},
  {butText_just, 0, "   It should be noted that go modem protocol has no "
   "way to resolve disputes, so the disputes must be played out by a "
   "person."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Resume\" button"},
  {butText_just, 0, "   When you are not using Japanese rules, any dispute "
   "at the end of the game can be resolved by simply undoing the last two "
   "passes and continuing the game.  Pressing \"Resume\" will let you do "
   "that."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Done\" button"},
  {butText_just, 0, "   When both players are done marking dead stones, "
   "pressing \"Done\" will finish it and record the score."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \""CGBUTS_REWCHAR"\", \""CGBUTS_BACKCHAR"\", "
   "\""CGBUTS_FWDCHAR"\", and \""CGBUTS_FFCHAR"\" Buttons"},
  {butText_just, 0,
   "   Pressing these buttons will move forward and backward through the "
   "game, letting you undo and redo moves.  "CGBUTS_BACKCHAR" and "
   CGBUTS_FWDCHAR" will go one move at a time.  "CGBUTS_REWCHAR" moves "
   "you to the beginning of the game, and "CGBUTS_FFCHAR" moves you to the "
   "end."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Score Boxes"},
  {butText_just, 0,
   "   To the right of the board are the score boxes.  There are two; one "
   "for white and one for black.  In each box are shown the current score "
   "for that player and the amount of time remaining on that player's "
   "clock.  When you are playing the game, the score is not the final "
   "score for that player, but the sum of the number of captures and the "
   "komi.  When you are selecting dead stones and after pressing the "
   "\"Done\" button, the score is the actual score for that player."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Save Game\" Button"},
  {butText_just, 0,
   "   Pressing \"Save Game\" will save the game as a smart go file.  Later "
   "you can continue the game with the \"Load Game\" button of the control "
   "window.  You can also choose \"Edit Game\" from the control panel to "
   "edit the game that you have saved.  This is currently the only way to "
   "create a new smart go file since the \"Edit Game\" button doesn't "
   "work."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Edit Game\" Button"},
  {butText_just, 0,
   "   Pressing \"Edit Game\" will pop up a second game board with the "
   "game on it.  This will be an edit game board, not a go modem game "
   "board, and it will come with a edit tool set.  This is used if you "
   "want to use the editing tools to annotate the game that is being "
   "played."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Quit\" Button"},
  {butText_just, 0,
   "   Pressing \"Quit\" will end the game.  If you want to continue the "
   "game later, you should choose \"Save Game\" before you quit."},
  {butText_just, 0, ""},
  {butText_just, 2, "The Comments Box"},
  {butText_just, 0,
   "   At the very bottom of the window is the comments box.  Here you "
   "can make comments about the game that will be saved in the smart go "
   "file if you choose \"Save Game\" later.  To edit or add the comments, "
   "click in the comments box and start typing.  Some emacs-style control "
   "keys are available; see the \"Keyboard Shortcuts\" help page for a "
   "list of them."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpText  gmpBoardKey[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Playing Go Keyboard Shortcuts"},
  {butText_just, 0, "   Left arrow: Go back one move."},
  {butText_just, 0, "   Right arrow: Go ahead one move."},
  {butText_just, 0, ""},
  {butText_just, 2, "Comment Editing"},
  {butText_just, 0, "   When you are editing a comment the left and right "
   "arrows will not change moves.  Instead they will move the cursor.  "
   "These other commands are available:"},
  {butText_just, 0, "   Ctrl-A: Move cursor to the beginning of the line."},
  {butText_just, 0, "   Ctrl-D: Delete to the right of the cursor"},
  {butText_just, 0, "   Ctrl-E: Move cursor to the end of the line."},
  {butText_just, 0, "   Ctrl-K: Delete to the end of the line."},
  {butText_just, 0, "   Ctrl-U: Delete from the beginning of the line."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  gmpBoardPage[] = {
  {"Go Modem Play", gmpBoard},
  {"Keyboard Shortcuts", gmpBoardKey},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_gmpBoard = {5, "Go Modem Help Pages", gmpBoardPage};

/* From "gmp/setup.c" */
static const AbutHelpText  gmpSetup[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Go Modem Protocol Setup Window"},
  {butText_just, 0, "   Using this window you may configure how the go modem "
   "protocol will be used.  The different types of connections allowed "
   "will be described."},
  {butText_just, 0, ""},
  {butText_just, 2, "\"Program\" Connections"},
  {butText_just, 0, "   When you select \"Program\" as either the white "
   "or the black player, Cgoban will execute a program and communicate "
   "go modem protocol with that program's stdin/stdout.  You must "
   "specify the program to run in the \"Program\" field.  The full "
   "command line, with the path, must be specified.  If you put \"%d\" "
   "in the command line somewhere, the first \"%d\" will be replaced by "
   "the main time (in seconds) and the second \"%d\" will be replaced by "
   "the byo-yomi time (in seconds).  For example, the program name "
   "\"./goDummy -time %d -byoYomi %d\" will run the program \"goDummy\" "
   "from the current directory with the time and the byo-yomi time "
   "passed in on the command line."},
  {butText_just, 0, "   The send game command checkbox controls whether "
   "commands to start the game will be sent to the program.  Go modem "
   "protocol required the two agents to decide before connecting who will "
   "initiate the communication; this button lets you choose.  Make sure "
   "that either Cgoban or the program send the game command or else the "
   "game will never start!"},
  {butText_just, 0, ""},
  {butText_just, 2, "\"Device\" Connections"},
  {butText_just, 0, "   This behaves much like a \"Program\" connection "
   "except that instead of running a program, Cgoban will connect to "
   "the device specified.  For example, in Linux setting device to "
   "\"/dev/cua0\" will connect up to the computer's COM1 port."},
  {butText_just, 0, ""},
  {butText_just, 2, "\"Human\" Connections"},
  {butText_just, 0, "   Setting the connection to \"Human\" will let you "
   "make moves for this player."},
  {butText_just, 0, ""},
  {butText_just, 2, "\"NNGS\" and \"IGS\" Connections"},
  {butText_just, 0, "   Using \"NNGS\" or \"IGS\" will open a connection "
   "to a go server.  The username and the password should be set "
   "correctly.  If one player is \"NNGS\" or \"IGS\", then the other "
   "player will be started up every time that a game is accepted on "
   "the server.  You will have to type all server commands except for "
   "the actual moves in the game; these will be made by the program or "
   "device.  The \"Send game command\" switch is always set when you "
   "are connected to a server."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  gmpSetupPage[] = {
  {"Go Modem Play Setup", gmpSetup},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_gmpSetup = {4, "Go Modem Setup Help Pages", gmpSetupPage};

/* From "setup.c" */
static const AbutHelpText  configure[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Configure Window"},
  {butText_just, 0, "   The configure window lets you change the behavior "
   "of Cgoban.  There are several control panels in the configure window; "
   "each controls a different part of cgoban."},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Connecting To NNGS\" And \"Connecting To IGS\" "
   "Control Panels"},
  {butText_just, 0, "   Here you can set the location of the go servers. "
   "The default will be set to the current location of the "
   "servers; this window is here in case they move in the future.  "
   "If the locations are wrong then you should fill in the machine name "
   "and port number of the server in the appropriate fields."},
  {butText_just, 0, "   Typically you want to leave \"Connect Directly To "
   "Server\" set.  This is the most efficient way to connect to a go "
   "server.  The only time when you might not want to set this is when "
   "your computer is behind a firewall or other security system.  Then "
   "direct connections out might not work and you will have to go through "
   "a proxy.  To do this, turn off \"Connect Directly To Server\" and "
   "enter the name of a program that can get through the firewall in "
   "the \"Connecting Command\" field.  Instead of connecting to the "
   "server itself, Cgoban will run the command you specify and use it "
   "to get to the server."},
  {butText_just, 0, "   Be careful that you do not confuse the NNGS and the "
   "IGS servers!  There are subtle differences in their protocol and "
   "Cgoban will not operate reliably if it connects to one type of "
   "server when it thinks it is connecting to another!"},
  {butText_just, 0, ""},
  {butText_just, 2, "The \"Miscellaneous\" Control Panel"},
  {butText_just, 0, "   Here are a few different controls."},
  {butText_just, 0, "   When \"Show Coordinates Around Boards\" checkbox "
   "controls whether the coordinates are shown around the boards that "
   "Cgoban draws."},
  {butText_just, 0, "   When \"Number Kibitzes\" is set, any kibitz "
   "in a game on a go server will have the move number shown before it."},
  {butText_just, 0, "   The \"Anti-Slip Moving\" button will not let you "
   "make a move until you have held the mouse over that point for at least "
   "a half a second.  This is useful because sometimes clicking on the "
   "mouse makes it slip to the next space over, placing your move in "
   "the wrong location!  With Anti-Slip on, this can't happen."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  configurePage[] = {
  {"Configuring Cgoban", configure},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_configure = {4, "Configure Help Pages", configurePage};

/* From client/look.c */
static const AbutHelpText  cliLook[] = {
  {butText_center, 2, "Cgoban " VERSION},
  {butText_center, 0, "By Bill Shubert, Kevin Sonney"},
  {butText_center, 0, DATE},
  {butText_just, 0, ""},
  {butText_just, 2, "Client Look Window"},
  {butText_just, 0, "   The \"Look\" and \"Status\" commands on the go "
   "servers show you the current state of a game.  \"Look\" shows you "
   "a saved game, while \"Status\" shows you a game in progress."},
  {butText_just, 0, "   These boards cannot be edited or manipulated in "
   "any way."},
  {butText_just, 0, ""},
  {butText_left, 0, NULL}};
static const AbutHelpPage  cliLookPage[] = {
  {"Client Look Window", cliLook},
  {NULL, NULL},
  {"Copyright and Non-Warranty", copyright},
  {"About the Author", aboutAuthor}};
Help  help_cliLook = {4, "Client Look Help Pages", cliLookPage};
