#ifndef GAME_H
#define GAME_H

#include "variables.h"

#include <vector>

using namespace std;

struct Card
{
    int color; //花色（0方块1梅花2红桃3黑桃）
    int point; //点数（取值范围为0-12）。特别的，点数为-1表示隐藏状态，点数为-2表示此处没有牌
    Card(int clr=0, int pt=0): color(clr), point(pt){}
};

struct Operate
{
    int Type; //操作类型。1为正常移动，2为贮藏区翻页，3为点开一张隐藏牌
    int Pos1; //移动前牌的位置，或点开隐藏牌的位置（0-6表示可操作区域，8-11表示已完成区域，12-35表示贮藏区）
    int Pos2; //移动后牌的位置
    int CardNumber; //一共挪了多少张牌
    int StoreCardNumToShow; //在操作之前，贮藏区一共显示了多少张牌
};

class Game
{
public:
    Game(int refresh_step=1, int max_refresh_times=0): RefreshStep(refresh_step), MaxRefreshTimes(max_refresh_times), CurrRefreshTimes(0), StoreCardNumToShow(0), StoreRefreshOverrun(false){}
    Game(const Game* pg);
    void GameInit(); //游戏内容初始化
    bool MoveTo(int pos1, int pos2); //将一个位置的牌挪到另一个位置。（pos：0-6表示可操作区域，7表示贮藏区域，8-11表示完成区域）
    bool JudgeLose(); //判断是否已经失败
    bool JudgeWin(); //判断是否已经获胜
    bool Back1Step(); //悔牌
    void StoreRefresh(); //刷新贮藏区

    void Play();
    void GameShow(); //显示游戏信息
    int GetMinNumberInDoneBlock() const;

    vector<Card> GameBlock[7]; //当前游戏可直接操作的区域
    Card VisHiddenBlock[7][6]; //游戏已经被打开的隐藏区域
    Card VisStoreBlock[24]; //游戏已经被打开的贮藏区域
    int DoneBlock[4]; //记录每种花色分别有几张牌已经完成了（0方块1梅花2红桃3黑桃）
    int PointToStore; //指向贮藏区的指针，表明当前贮藏区可操作的牌是哪一张
    int HiddenBlockLen[7]; //隐藏区的长度
    int CurrRefreshTimes; //当前刷新了多少次

private:
    Card HiddenBlock[7][6]; //游戏隐藏区域
    Card StoreBlock[24]; //游戏贮藏区域
    vector<Operate> MoveHist; //牌的移动记录

    void GetNewCardFromHiddenBlock(int pos); //从隐藏区刷出一张新牌
    void PrintACard(Card c);

    const int RefreshStep; //一次性翻多少张牌
    const int MaxRefreshTimes; //最多可以刷新多少次。0表示不限制
    int StoreCardNumToShow; //当前应该显示多少张贮藏区的牌
    bool StoreRefreshOverrun; //“维加斯”模式下，刷新次数是否已超限
};

#endif
