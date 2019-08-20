#ifndef AI_H
#define AI_H

#include "game.h"

class OperateWithVal //带权值的操作
{
public:
    int Type; //操作类型。
    int Pos1; //移动前牌的位置，或点开隐藏牌的位置（0-6表示可操作区域，8-11表示已完成区域，12-35表示贮藏区）
    int Pos2; //移动后牌的位置
    int Value; //这个操作的收益权值
    OperateWithVal(int ope_type = OPE_REFRESH_STORE, int pos1 = 0, int pos2 = 0, int value = 0): Type(ope_type), Pos1(pos1), Pos2(pos2), Value(value){}
    bool operator == (const OperateWithVal& ope){
        if (this->Type == ope.Type && this->Pos1 == ope.Pos1 && this->Pos2 == ope.Pos2)
            return true;
        return false;
    }
    bool operator != (const OperateWithVal& ope){
        if (this->Type != ope.Type || this->Pos1 != ope.Pos1 || this->Pos2 != ope.Pos2)
            return true;
        return false;
    }
};

class AI
{
public:
    AI(int refresh_step=1, int max_refresh_times=0): GamePlayed(0), GameWon(0), GameLost(0), RefreshStep(refresh_step), MaxRefreshTimes(max_refresh_times), pGame(nullptr){}
    void ai_play1game(); //AI玩一局游戏
    bool ai_play(int match_cnt); //AI玩多局游戏

private:
    int GamePlayed; //已经玩了多少局游戏
    int GameWon; //已经赢了多少局游戏
    int GameLost; //已经输了多少局游戏
    vector<int> DoneRatio; //完成比例
    vector<int> HiddenLeftRatio; //游戏结束时，贮藏区和隐藏区剩余牌的比例
    vector<OperateWithVal> OperateList; //已经执行的操作列表

    const int RefreshStep; //一次性翻多少张牌
    const int MaxRefreshTimes; //最多可以刷新多少次。0表示不限制
    Game* pGame;

    OperateWithVal ai_1step(); //AI进行一轮操作
    bool JudgeLose(); //AI自身判断是否应该认输
    void ResultAnalyse(); //分析AI的运行结果
    int JudgeLoop(); //判断是否存在循环节
};

#endif
