#include <stdio.h>
#include <unistd.h>
#include "ai.h"

OperateWithVal AI::ai_1step()
{
    vector<OperateWithVal> ope_list;
    //首先获取可行的操作列表
    for (int t=0; t<=6; t++){
        if (pGame->GameBlock[t].size() == 0)
            continue;
        //检查有没有操作区之间互相挪动的可能性
        for (int t0 = 0; t0 <= pGame->GameBlock[t].size() - 1; t0++){
            for(int t1=0; t1<=6; t1++){ //把K开头的牌挪到空位 或 把操作区的牌挪到其他操作区的下面
                if (t1 == t)
                    continue;
                if (pGame->GameBlock[t1].size() == 0 && pGame->GameBlock[t][t0].point == 12){ //把K开头的牌挪到空位
                    if (pGame->HiddenBlockLen[t] != 0){
                        ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, 40 + pGame->HiddenBlockLen[t])); //能够打开隐藏区中新的牌的得分为40分
                    }else{
                        ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, 0)); //不能打开隐藏区，只是操作区内部挪牌时，该操作的得分为0分
                    }
                }else if (pGame->GameBlock[t1].size() != 0){ //把操作区的牌挪到其他操作区的下面
                    if (pGame->GameBlock[t][t0].point == pGame->GameBlock[t1].back().point - 1 && ((pGame->GameBlock[t][t0].color & 1) ^ (pGame->GameBlock[t1].back().color & 1))){
                        if (t0 == 0){
                            if (pGame->HiddenBlockLen[t] == 0){ //这张牌下面没有隐藏牌
                                bool have_k = false; //如果在贮藏区或其他操作区有K，且K不在最底层，则记为50分，否则记为0分
                                for (int t2 = 0; t2 <= 6; t2++){
                                    if (t2 != t /*&& t2 != t1*/ && pGame->GameBlock[t2].size() != 0 && pGame->HiddenBlockLen[t2] != 0 && pGame->GameBlock[t2][0].point == 12){ //其他操作区有K
                                        have_k = true;
                                        break;
                                    }
                                }
                                if (pGame->VisStoreBlock[pGame->PointToStore].point == 12){
                                    have_k = true;
                                    break;
                                }
                                if (have_k)
                                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, 50));
                                else
                                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, 0));
                            }else{ //可以打开一张隐藏牌，则该操作的得分为40分
                                ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, 40 + pGame->HiddenBlockLen[t]));
                            }
                        }else{ //挪牌之后，原位置不能腾出空位来接收K开头的牌或打开隐藏区，则此时得分为0分
                            ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, 0));
                        }
                    }
                }
            }
        }
        //检查操作区中的牌能否移动到完成区
        if (pGame->DoneBlock[pGame->GameBlock[t].back().color] + 1 == pGame->GameBlock[t].back().point)
            ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, pGame->GameBlock[t].back().color + 8, 20));
    }
    for (int t=0; t<=6; t++){
        //检查贮藏区中的牌能否移动到操作区
        if (pGame->PointToStore >= 0){
            if (pGame->GameBlock[t].size() != 0){ //将贮藏区的牌挪到操作区的牌的下方
                if (pGame->VisStoreBlock[pGame->PointToStore].point == pGame->GameBlock[t].back().point - 1 && (((pGame->VisStoreBlock[pGame->PointToStore].color & 1) ^ (pGame->GameBlock[t].back().color & 1))))
                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, 7, t, 30));
            }else if (pGame->HiddenBlockLen[t] == 0){ //将贮藏区的k挪到空地
                if (pGame->VisStoreBlock[pGame->PointToStore].point == 12)
                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, 7, t, 30));
            }
        }
    }
    if (pGame->PointToStore >= 0 && pGame->DoneBlock[pGame->VisStoreBlock[pGame->PointToStore].color] + 1 == pGame->VisStoreBlock[pGame->PointToStore].point){
        //检查贮藏区中的牌能否移动到完成区
        ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, 7, 8 + pGame->VisStoreBlock[pGame->PointToStore].color, 60));
    }

    //然后从可行的操作列表中，选取权值最高的
    if (ope_list.size() == 0){
        //没有可行的操作，就刷新贮藏区
        return OperateWithVal(OPE_REFRESH_STORE, 0, 0, 10);
    }
    OperateWithVal ope = ope_list[0]; //找出权值最大的一个操作
    int max_score = ope_list[0].Value;
    for (int t = 1; t <= ope_list.size() - 1; t++){
        if (ope_list[t].Value > max_score){
            max_score = ope_list[t].Value;
            ope = ope_list[t];
        }
    }
    if (max_score > 10){ //有权值在10以上的操作，则执行此操作
        return ope;
    }else //没有权值在10以上的操作，则要求翻开贮藏区
        return OperateWithVal(OPE_REFRESH_STORE, 0, 0, 10);
}

int AI::JudgeLoop()
{
    //找到循环节
    vector<OperateWithVal> ope_cycle = {OperateList.back()};
    for(int t = OperateList.size() - 2; t >= 0; t--){
        if (OperateList[t] == ope_cycle[0])
            break;
        else
            ope_cycle.push_back(OperateList[t]);
    }
    //找到循环长度
    for (int t = OperateList.size() - ope_cycle.size() - 1; t >= 0; t--){
        if (OperateList[t] != ope_cycle[(OperateList.size() - t - 1) % ope_cycle.size()])
            return (OperateList.size() - t - 1) / ope_cycle.size();
    }
    return OperateList.size() / ope_cycle.size();
}

bool AI::JudgeLose()
{
    //AI自身判断是否应该认输. 在走投无路，或自身存在3次以上完全一致的循环时，认为应当认输
    if (pGame->JudgeLose()){
        printf("游戏走投无路，判定失败.\n");
        return true; //已经走投无路了，则判定为认输
    }
    int loop_len = JudgeLoop();
    int card_num_in_store = 52;
    for (int t=0; t<=3; t++)
        card_num_in_store -= pGame->DoneBlock[t];
    for (int t=0; t<=6; t++){
        card_num_in_store -= pGame->HiddenBlockLen[t];
        card_num_in_store -= pGame->GameBlock[t].size();
    }
    if (loop_len >= 3 && OperateList.back().Type != OPE_REFRESH_STORE){ //出现了3次以上完全一致的循环，或把贮藏区刷了个遍
        printf("游戏操作陷入连续循环，判定失败.\n");
        return true;
    }
    if (loop_len >= 2 + card_num_in_store / RefreshStep){ //连续刷了一轮的贮藏区还没有找到其他解法，则认为应该认输
        printf("将贮藏区完整连续一遍后仍未找到可行解，判定失败.\n");
        return true;
    }
    return false;
}

void AI::ai_play1game()
{
    //首先初始化游戏
    pGame = new Game(RefreshStep, MaxRefreshTimes);
    pGame->GameInit();
    if (AI_DEBUG)
        pGame->GameShow();
    OperateList.clear();
    //然后进行游戏运行中的操作
    while(true)
    {
        //进行一步操作
        OperateWithVal ope = ai_1step();
        OperateList.push_back(ope);
        if (ope.Type == OPE_MOVE_CARD){ //挪牌
            pGame->MoveTo(ope.Pos1, ope.Pos2);
            if (AI_DEBUG)
                pGame->GameShow();
        }else if (ope.Type == OPE_REFRESH_STORE){ //刷新贮藏区
            pGame->StoreRefresh();
            if (AI_DEBUG)
                pGame->GameShow();
        }//else if (ope.Type == OPE_MOVE_BACK){ //悔牌
        //    Back1Step();
        //}
        if (AI_DEBUG)
            sleep(1);
        if (pGame->JudgeWin()){ //一局游戏获胜了。把获胜信息记录下来
            GameWon += 1;
            printf("游戏胜利.\n");
            break;
        }
        if (JudgeLose() || ope.Type == OPE_GIVE_UP){ //一局游戏失败了。分析游戏的完成率、地图打开率等信息，并记录下来
            GameLost += 1;
            break;
        }
    }
    //收尾处理
    delete pGame;
}

bool AI::ai_play(int match_cnt)
{
    for (int t = 0; t <= match_cnt - 1; t++){
        printf("游戏序号：%d, ", t);
        ai_play1game();
        sleep(1);
    }
    printf("\n\n\n运行结束：获胜%d局，失败%d局.\n", GameWon, GameLost);

    return true;
}
