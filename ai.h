#ifndef AI_H
#define AI_H

#include <stdio.h>
#include <stdint.h>
#include <set>
#include "game.h"

using namespace std;

class OperateWithVal //带权值的操作
{
public:
    int Type; //操作类型。
    int Pos1; //移动前牌的位置，或点开隐藏牌的位置（0-6表示可操作区域，8-11表示已完成区域，12-35表示贮藏区）
    int Pos2; //移动后牌的位置
    int Value; //这个操作的收益权值
    bool ClearCircleHist; //是否应该清空检查自循环的数组
    OperateWithVal(int ope_type = OPE_REFRESH_STORE, int pos1 = 0, int pos2 = 0, int value = 0, bool clear = false): Type(ope_type), Pos1(pos1), Pos2(pos2), Value(value), ClearCircleHist(clear){}
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

struct MethodNode //策略树的节点
{
    int Depth; //该节点的深度
    uint32_t ParentNode; //父节点的索引值
    vector<uint32_t> ChildNode; //这个节点所有子节点的索引值
    int AwardScore; //这个节点下的奖励分
    bool DidSearch; //这个节点是否已经被搜索过（如果已经被搜索过，则直接跳转至其子节点）
    Game* IGame; //在这个状态下，假象的地图情况
    vector<int> MoveCircleHist; //用于检查是否存在自循环的变量
    OperateWithVal Ope;
    MethodNode(const OperateWithVal& ope, int depth=0, uint32_t parent_node=0, int award_score=0): Depth(depth), ParentNode(parent_node), AwardScore(award_score), DidSearch(false), Ope(ope){}
};

class AI
{
public:
    AI(int refresh_step=1, int max_refresh_times=0): GamePlayed(0), GameWon(0), GameLost(0), RefreshStep(refresh_step), MaxRefreshTimes(max_refresh_times), pGame(nullptr){}
    bool ai_play1game(int game_no = 0); //AI玩一局游戏
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

    bool JudgeLose(); //AI自身判断是否应该认输
    void ResultAnalyse(); //分析AI的运行结果
    vector<OperateWithVal> JudgeLoop(int* loop_len); //判断是否存在循环节

private:
    uint32_t CurrentNode; //当前游戏中，AI所在的节点位置。在搜索时，将以此节点作为根节点
    uint32_t CurrentNodeNum; //当前实际搜索到了多少节点
    vector<MethodNode> MethodTree; //策略树

    uint32_t search_by_ida(bool* psuc); //搜索当前情况下的最优解
    void search_in_max_depth(uint32_t root_node, int max_depth); //给定最大的搜索深度，搜索这个根节点下所有的解
    vector<OperateWithVal> ai_get_opes_1step(uint32_t node); //对这个根节点，查找所有可行的操作   
private:
    FILE* pFile;
    set<vector<int>> IStatusList; //已经遍历过的状态列表。之后再次遍历时，会忽略掉类似的状态，不插入到状态数组中
    bool JudgeIStatus(Game* pg); //如果已经存在了指定的状态，就把它忽略掉；否则把它插进去
};

#endif
