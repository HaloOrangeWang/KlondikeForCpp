#include "variables.h"

const int R_OPEN_HIDDEN_BLOCK = 50; //打开隐藏区的新牌的计分
const int R_OPEN_HIDDEN_BLOCK_MODIFY = 1; //打开隐藏区时，根据该列隐藏区的牌数而对计分进行修正的值
const int R_MOVE_FROM_STORE_BLOCK = 30; //能够使贮藏区的牌挪下来的计分
const int R_MOVE_K = 10; //能够把非空位上的K挪到空位中时的计分
const int R_EMPTY_BLOCK = -7; //挪牌后导致产生空位的计分
const int R_GAME_BLOCK_MOVE = 0; //在操作区内部挪牌，而不能打开隐藏区的新牌时的计分
const int R_MOVE_TO_DONE_BLOCK = 20; //能够把牌挪到完成区的计分
const int R_MOVE_TO_DONE_BLOCK_MODIFY = 1; //根据完成区点数的大小，修正挪到完成区的牌的计分
const int R_MOVE_FROM_DONE_BLOCK = -20; //把牌从完成区挪回的计分
const int R_MOVE_FROM_DONE_BLOCK_MODIFY = 1; //根据完成区点数的大小，修正从完成区挪回的计分
const int R_REFRESH = 1; //刷新贮藏区的计分
const int R_MAX = 200; //到达多少分后就可以移动了
const int R_MOVE = 50; //移动之后，和目标分数之间至少相差多少分
const int MAX_SEARCH_DEPTH = 15; //最大单次搜索深度

const char Output_Filename[20] = "output.csv";
