#ifndef VARIABLE_H
#define VARIABLE_H

#define AI_MODE true //这个常量表明游戏是AI运行还是人工运行
#define AI_DEBUG true //这个常量表示AI模式是否为测试模式（测试模式下只运行单局游戏，且相邻两次操作之间停顿3秒）

#define OPE_MOVE_CARD 1
#define OPE_REFRESH_STORE 2
#define OPE_SHOW_NEW_CARD 3
#define OPE_MOVE_BACK 10
#define OPE_GIVE_UP 11

#define MAX_MOVE_NUM 1000 //累计操作大于这个次数后，判定为游戏失败

extern const int R_OPEN_HIDDEN_BLOCK; //打开隐藏区的新牌的计分
extern const int R_OPEN_HIDDEN_BLOCK_MODIFY; //打开隐藏区时，根据该列隐藏区的牌数而对计分进行修正的值
extern const int R_MOVE_FROM_STORE_BLOCK; //能够使贮藏区的牌挪下来的计分
extern const int R_MOVE_K; //能够把非空位上的K挪到空位中时的计分
extern const int R_EMPTY_BLOCK; //挪牌后导致产生空位的计分
extern const int R_GAME_BLOCK_MOVE; //在操作区内部挪牌，而不能打开隐藏区的新牌时的计分
extern const int R_MOVE_TO_DONE_BLOCK; //能够把牌挪到完成区的计分
extern const int R_MOVE_TO_DONE_BLOCK_MODIFY; //根据完成区点数的大小，修正挪到完成区的牌的计分
extern const int R_MOVE_FROM_DONE_BLOCK; //把牌从完成区挪回的计分
extern const int R_MOVE_FROM_DONE_BLOCK_MODIFY; //根据完成区点数的大小，修正从完成区挪回的计分
extern const int R_REFRESH; //刷新贮藏区的计分
extern const int R_MAX; //到达多少分后就可以移动了
extern const int R_MOVE; //移动之后，和目标分数之间至少相差多少分
extern const int MAX_SEARCH_DEPTH; //最大单次搜索深度

extern const char Output_Filename[20]; //将运行结果输出出去的文件名

#endif
