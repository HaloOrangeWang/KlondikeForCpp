#include <unistd.h>
#include <algorithm>
#include <assert.h>
#include "ai.h"

vector<OperateWithVal> AI::ai_get_opes_1step(uint32_t node)
{
    vector<OperateWithVal> ope_list;
    Game* pigame = MethodTree[node].IGame;

    //能够把比完成区最小的牌+2点数以内的牌挪到完成区时，必须这样选择
    for (int t=0; t<=6; t++){
        if (pigame->GameBlock[t].size() == 0)
            continue;
        if (pigame->DoneBlock[pigame->GameBlock[t].back().color] - pigame->GetMinNumberInDoneBlock() <= 1 && pigame->DoneBlock[pigame->GameBlock[t].back().color] + 1 == pigame->GameBlock[t].back().point){
            int score = R_MOVE_TO_DONE_BLOCK + R_MOVE_TO_DONE_BLOCK_MODIFY * (pigame->GameBlock[t].back().point - pigame->GetMinNumberInDoneBlock());
            if (pigame->GameBlock[t].size() == 1){
                if (pigame->HiddenBlockLen[t] == 0)
                    score += R_EMPTY_BLOCK; //挪牌之后可以腾出一个空位的情况
                else
                    score += R_OPEN_HIDDEN_BLOCK + pigame->HiddenBlockLen[t] * R_OPEN_HIDDEN_BLOCK_MODIFY; //挪牌之后可以打开隐藏区的情况
            }
            ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, pigame->GameBlock[t].back().color + 8, score, true));
            return ope_list;
        }
    }
    if (pigame->PointToStore >= 0){
        if (pigame->DoneBlock[pigame->VisStoreBlock[pigame->PointToStore].color] - pigame->GetMinNumberInDoneBlock() <= 1 && pigame->DoneBlock[pigame->VisStoreBlock[pigame->PointToStore].color] + 1 == pigame->VisStoreBlock[pigame->PointToStore].point){
            int score = R_MOVE_FROM_STORE_BLOCK + R_MOVE_TO_DONE_BLOCK - R_MOVE_TO_DONE_BLOCK_MODIFY * (pigame->VisStoreBlock[pigame->PointToStore].point - pigame->GetMinNumberInDoneBlock());
            ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, 7, 8 + pigame->VisStoreBlock[pigame->PointToStore].color, score, true));
            return ope_list;
        }
    }

    //找出所有可行的操作
    for (int t=0; t<=6; t++){
        if (pigame->GameBlock[t].size() == 0)
            continue;
        //检查有没有操作区之间互相挪动的可能性
        for (uint32_t t0 = 0; t0 <= pigame->GameBlock[t].size() - 1; t0++){
            for(int t1=0; t1<=6; t1++){ //把K开头的牌挪到空位 或 把操作区的牌挪到其他操作区的下面
                if (t1 == t)
                    continue;
                if (pigame->GameBlock[t1].size() == 0 && pigame->GameBlock[t][t0].point == 12){ //把K开头的牌挪到空位
                    if (pigame->HiddenBlockLen[t] != 0){
                        int score = R_OPEN_HIDDEN_BLOCK + pigame->HiddenBlockLen[t] * R_OPEN_HIDDEN_BLOCK_MODIFY + R_MOVE_K;
                        ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, score, true)); //能够打开隐藏区中新的牌的得分
                    }else{
                        if (MethodTree[node].MoveCircleHist.size() == 0 || t != MethodTree[node].MoveCircleHist.back() || find(MethodTree[node].MoveCircleHist.begin(), MethodTree[node].MoveCircleHist.end(), t1) == MethodTree[node].MoveCircleHist.end()) //这个条件判断是为了避免自循环
                            ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, R_GAME_BLOCK_MOVE, false)); //不能打开隐藏区，只是操作区内部挪牌时，该操作的得分
                    }
                }else if (pigame->GameBlock[t1].size() != 0){ //把操作区的牌挪到其他操作区的下面
                    if (pigame->GameBlock[t][t0].point == pigame->GameBlock[t1].back().point - 1 && ((pigame->GameBlock[t][t0].color & 1) ^ (pigame->GameBlock[t1].back().color & 1))){
                        if (t0 == 0){
                            if (pigame->HiddenBlockLen[t] == 0){ //这张牌下面没有隐藏牌. 此时不能打开隐藏区，且会产生空位
                                if (MethodTree[node].MoveCircleHist.size() == 0 || t != MethodTree[node].MoveCircleHist.back() || find(MethodTree[node].MoveCircleHist.begin(), MethodTree[node].MoveCircleHist.end(), t1) == MethodTree[node].MoveCircleHist.end()){ //这个条件判断是为了避免自循环
                                    int score = R_GAME_BLOCK_MOVE + R_EMPTY_BLOCK;
                                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, score, false));
                                }
                            }else{ //可以打开一张隐藏牌，则该操作的得分
                                int score = R_OPEN_HIDDEN_BLOCK + pigame->HiddenBlockLen[t] * R_OPEN_HIDDEN_BLOCK_MODIFY;
                                ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, score, true));
                            }
                        }else{ //挪牌之后，原位置不能腾出空位来接收K开头的牌或打开隐藏区的得分
                            if (MethodTree[node].MoveCircleHist.size() == 0 || t != MethodTree[node].MoveCircleHist.back() || find(MethodTree[node].MoveCircleHist.begin(), MethodTree[node].MoveCircleHist.end(), t1) == MethodTree[node].MoveCircleHist.end()){ //这个条件判断是为了避免自循环
                                ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, t1, R_GAME_BLOCK_MOVE, false));
                            }
                        }
                    }
                }
            }
        }
        //检查操作区中的牌能否移动到完成区
        if (pigame->DoneBlock[pigame->GameBlock[t].back().color] + 1 == pigame->GameBlock[t].back().point){
            int score = R_MOVE_TO_DONE_BLOCK - R_MOVE_TO_DONE_BLOCK_MODIFY * (pigame->GameBlock[t].back().point - pigame->GetMinNumberInDoneBlock());
            bool clear = false;
            if (pigame->GameBlock[t].size() == 1){
                if (pigame->HiddenBlockLen[t] == 0)
                    score += R_EMPTY_BLOCK; //挪牌之后可以腾出一个空位的情况
                else {
                    clear = true;
                    score += R_OPEN_HIDDEN_BLOCK + pigame->HiddenBlockLen[t] * R_OPEN_HIDDEN_BLOCK_MODIFY; //挪牌之后可以打开隐藏区的情况
                }
            }
            if (clear)
                ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, pigame->GameBlock[t].back().color + 8, score, true));
            else{
                if (MethodTree[node].MoveCircleHist.size() == 0 || t != MethodTree[node].MoveCircleHist.back() || find(MethodTree[node].MoveCircleHist.begin(), MethodTree[node].MoveCircleHist.end(), pigame->GameBlock[t].back().color + 8) == MethodTree[node].MoveCircleHist.end()){ //这个条件判断是为了避免自循环
                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, t, pigame->GameBlock[t].back().color + 8, score, false));
                }
            }
        }
    }
    for (int t=0; t<=6; t++){
        //检查贮藏区中的牌能否移动到操作区
        if (pigame->PointToStore >= 0){
            if (pigame->GameBlock[t].size() != 0){ //将贮藏区的牌挪到操作区的牌的下方
                if (pigame->VisStoreBlock[pigame->PointToStore].point == pigame->GameBlock[t].back().point - 1 && (((pigame->VisStoreBlock[pigame->PointToStore].color & 1) ^ (pigame->GameBlock[t].back().color & 1))))
                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, 7, t, R_MOVE_FROM_STORE_BLOCK, true));
            }else if (pigame->HiddenBlockLen[t] == 0){ //将贮藏区的k挪到空地
                if (pigame->VisStoreBlock[pigame->PointToStore].point == 12)
                    ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, 7, t, R_MOVE_FROM_STORE_BLOCK + R_MOVE_K, true));
            }
        }
    }
    if (pigame->PointToStore >= 0 && pigame->DoneBlock[pigame->VisStoreBlock[pigame->PointToStore].color] + 1 == pigame->VisStoreBlock[pigame->PointToStore].point){
        //检查贮藏区中的牌能否移动到完成区
        int score = R_MOVE_FROM_STORE_BLOCK + R_MOVE_TO_DONE_BLOCK - R_MOVE_TO_DONE_BLOCK_MODIFY * (pigame->VisStoreBlock[pigame->PointToStore].point - pigame->GetMinNumberInDoneBlock());
        ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, 7, 8 + pigame->VisStoreBlock[pigame->PointToStore].color, score, true));
    }
    for (int t=0; t<=6; t++){
        //检查完成区的牌能否移动到操作区
        if (pigame->GameBlock[t].size() == 0)
            continue;
        if (pigame->DoneBlock[pigame->GameBlock[t].back().color] + 1 == pigame->GameBlock[t].back().point){
            if (MethodTree[node].MoveCircleHist.size() == 0 || pigame->GameBlock[t].back().color + 8 != MethodTree[node].MoveCircleHist.back() || find(MethodTree[node].MoveCircleHist.begin(), MethodTree[node].MoveCircleHist.end(), t) == MethodTree[node].MoveCircleHist.end()){ //这个条件判断是为了避免自循环
                int score = R_MOVE_FROM_DONE_BLOCK + R_MOVE_FROM_DONE_BLOCK_MODIFY * (pigame->GameBlock[t].back().point - pigame->GetMinNumberInDoneBlock());
                ope_list.push_back(OperateWithVal(OPE_MOVE_CARD, pigame->GameBlock[t].back().color + 8, t, score, false));
            }
        }
    }

    //最后增加刷新贮藏区的操作
    ope_list.push_back(OperateWithVal(OPE_REFRESH_STORE, 0, 0, R_REFRESH, true));
    return ope_list;
}

bool AI::JudgeIStatus(Game* pg)
{
    //首先将操作区牌的点数、贮藏区牌的数量、完成区牌的数量，整合成一个vector
    vector<int> i_status(19);
    for(uint32_t t = 0; t <= 6; t++){
        if (pg->GameBlock[t].size() == 0)
            i_status[t] = -1;
        else
            i_status[t] = pg->GameBlock[t].back().point;
    }
    for(uint32_t t = 0; t <= 6; t++)
        i_status[t + 7] = pg->HiddenBlockLen[t];
    for(uint32_t t = 0; t <= 3; t++)
        i_status[t + 14] = pg->DoneBlock[t];
    i_status[18] = pg->CurrRefreshTimes * 25 + pg->PointToStore + 1;
    //i_status[18] = 0;
    //判断这个状态是否已经存在了
    if (IStatusList.size() == 0 || IStatusList.find(i_status) == IStatusList.end()){
        IStatusList.insert(i_status);
        return true;
    }else{
        return false;
    }
}

void AI::search_in_max_depth(uint32_t r_node, int max_depth)
{
    if (AI_DEBUG){ //为避免内存占用太高，当状态数量达到10w时，直接结束
        if (MethodTree.size() >= 155000){
            printf("155000!\n");
            getchar();
            abort();
        }
    }
    if (!MethodTree[r_node].DidSearch){
        //这个根节点没有被搜索过的情况
        //首先获取可行的操作列表
        vector<OperateWithVal> ope_list = ai_get_opes_1step(r_node);
        assert(ope_list.size() != 0);
        for (uint32_t t = 0; t <= ope_list.size() - 1; t++){
            //根据操作列表，对假想的游戏地图进行操作
            MethodNode node(ope_list[t], MethodTree[r_node].Depth + 1, r_node, MethodTree[r_node].AwardScore + ope_list[t].Value);
            node.IGame = new Game(MethodTree[r_node].IGame);
            if (ope_list[t].ClearCircleHist == true)
                node.MoveCircleHist.clear();
            else{
                node.MoveCircleHist = MethodTree[r_node].MoveCircleHist;
                node.MoveCircleHist.push_back(ope_list[t].Pos2);
            }
            if (ope_list[t].Type == OPE_MOVE_CARD){ //挪牌
                node.IGame->MoveTo(ope_list[t].Pos1, ope_list[t].Pos2);
            }else if (ope_list[t].Type == OPE_REFRESH_STORE){ //刷新贮藏区
                node.IGame->StoreRefresh();
            }
            if (node.IGame->JudgeWin())
                node.AwardScore += 1000000; //如果这个操作能够使游戏直接获胜，则将这个操作的收益值赋值成非常大的数
            //状态判重
            bool status_unique = JudgeIStatus(node.IGame);
            if (!status_unique && !node.IGame->JudgeWin()){
                delete node.IGame;
                continue;
            }
            //将对应的操作添加进策略树中
            MethodTree.push_back(node);
            MethodTree[r_node].ChildNode.push_back(MethodTree.size() - 1);
            //如果深度有剩余值，则进行递归操作
            if (max_depth >= 2)
                search_in_max_depth(MethodTree.size() - 1, max_depth - 1);
        }
        MethodTree[r_node].DidSearch = true; //这个根节点的一层子节点已经被搜索过了
    }else{
        //这个根节点的一层字节点已经被搜索过了，直接遍历它的一层子节点即可
        if (max_depth >= 2 && MethodTree[r_node].ChildNode.size() > 0){
            for(uint32_t t = 0; t <= MethodTree[r_node].ChildNode.size() - 1; t++){
                search_in_max_depth(MethodTree[r_node].ChildNode[t], max_depth - 1);
            }
        }
    }
}

uint32_t AI::search_by_ida(bool* psuc)
{
    //对于一种特殊情况：根节点已经遍历过但没有任何子节点，此时为了避免卡死，要将根节点状态重置为“没有子节点”，并清空判重用的数组
    if (MethodTree[CurrentNode].ChildNode.size() == 0 && MethodTree[CurrentNode].DidSearch == true){
        MethodTree[CurrentNode].DidSearch = false;
        IStatusList.clear();
        if (AI_DEBUG)
            printf("根节点已经遍历过但没有任何子节点. 为避免卡死，并清空判重用的数组.\n");
    }
    //用ida*的方法找出当前情况下的最优解
    int max_depth = 5;
    while (true){
        search_in_max_depth(CurrentNode, max_depth);
        //找出这个深度下，所能达到的最高分
        int max_score = 0;
        uint32_t max_score_inode = CurrentNode;
        for(uint32_t inode = CurrentNodeNum; inode <= MethodTree.size() - 1; inode++){
            if (MethodTree[inode].AwardScore > max_score){
                max_score = MethodTree[inode].AwardScore;
                max_score_inode = inode;
            }
        }
        //如果最高分达到了目标，则返回那个节点，否则深度+1重新搜索
        if (max_score - MethodTree[CurrentNode].AwardScore >= R_MAX){
            if (AI_DEBUG){
                //printf("为了获取%d的收益值，搜索深度为%d，新增了%lu个节点，当前总共节点数量达到了%lu.\n", max_score, max_depth, MethodTree.size() - CurrentNodeNum, MethodTree.size());
                //sleep(2);
            }
            *psuc = true;
            return max_score_inode;
        }else if (max_depth >= MAX_SEARCH_DEPTH){
            if (AI_DEBUG){
                //printf("为了获取%d的收益值，搜索深度为%d，新增了%lu个节点，当前总共节点数量达到了%lu.\n", max_score, max_depth, MethodTree.size() - CurrentNodeNum, MethodTree.size());
                //sleep(2);
            }
            *psuc = false;
            return max_score_inode;
        }else{
            max_depth += 1;
        }
    }
}

vector<OperateWithVal> AI::JudgeLoop(int* loop_len)
{
    //找到循环节
    vector<OperateWithVal> ope_cycle = {OperateList.back()};
    for(int t = static_cast<int>(OperateList.size()) - 2; t >= 0; t--){
        if (OperateList[t] == ope_cycle[0])
            break;
        else
            ope_cycle.push_back(OperateList[t]);
    }
    //找到循环长度
    for (int t = static_cast<int>(OperateList.size()) - static_cast<int>(ope_cycle.size()) - 1; t >= 0; t--){
        if (OperateList[t] != ope_cycle[(OperateList.size() - t - 1) % ope_cycle.size()]){
            *loop_len = (OperateList.size() - t - 1) / ope_cycle.size();
            return ope_cycle;
        }
    }
    *loop_len = OperateList.size() / ope_cycle.size();
    return ope_cycle;
}

bool AI::JudgeLose()
{
    //为避免内存占用太高，当状态数量达到5w时，直接结束
    if (MethodTree.size() >= 140000){
        printf("当状态数量达到14w时，直接结束.\n");
        return true;
    }

    //AI自身判断是否应该认输. 在走投无路，或自身存在3次以上完全一致的循环时，认为应当认输
    if (pGame->JudgeLose()){
        printf("游戏走投无路，判定失败，当前总共节点数量达到了%u.\n", CurrentNodeNum);
        return true; //已经走投无路了，则判定为认输
    }
    int loop_len;
    vector<OperateWithVal> ope_cycle = JudgeLoop(&loop_len);
    int card_num_in_store = 52;
    for (int t=0; t<=3; t++)
        card_num_in_store -= pGame->DoneBlock[t];
    for (int t=0; t<=6; t++){
        card_num_in_store -= pGame->HiddenBlockLen[t];
        card_num_in_store -= pGame->GameBlock[t].size();
    }
    if (loop_len >= 3 && OperateList.back().Type != OPE_REFRESH_STORE){ //出现了3次以上完全一致的循环，或把贮藏区刷了个遍
        //对循环节做判断，两种情况不应被判负：一是循环节中包含从贮藏区挪下来的内容，二是从其他地方挪到完成区的牌的数量大于从完成区挪到操作区的牌的数量
        int to_done_block_cnt = 0;
        int from_done_block_cnt = 0;
        bool has_from_store = false;
        for (uint32_t t = 0; t <= ope_cycle.size() - 1; t++){
            if (ope_cycle[t].Pos1 == 7){
                has_from_store = true;
                break;
            }
            if (ope_cycle[t].Pos1 >= 8)
                from_done_block_cnt += 1;
            if (ope_cycle[t].Pos2 >= 8)
                to_done_block_cnt += 1;
        }
        if (has_from_store == true)
            return false;
        if (to_done_block_cnt > from_done_block_cnt)
            return false;
        printf("游戏操作陷入连续循环，判定失败，当前总共节点数量达到了%u.\n", CurrentNodeNum);
        return true;
    }
    if (loop_len >= 2 + card_num_in_store / RefreshStep){ //连续刷了一轮的贮藏区还没有找到其他解法，则认为应该认输
        printf("将贮藏区完整连续一遍后仍未找到可行解，判定失败，当前总共节点数量达到了%u.\n", CurrentNodeNum);
        return true;
    }
    return false;
}

bool AI::ai_play1game(int game_no)
{
    sleep(10);
    //首先初始化游戏
    pGame = new Game(RefreshStep, MaxRefreshTimes);
    pGame->GameInit();
    if (AI_DEBUG)
        pGame->GameShow();

    if (MethodTree.size() >= 1){
        for (uint32_t inode = 0; inode <= MethodTree.size() - 1; inode++){
            delete MethodTree[inode].IGame;
        }
    }
    MethodTree.clear();
    OperateList.clear();
    IStatusList.clear();
    CurrentNode = 0;
    CurrentNodeNum = 0;
    bool game_win = false;
    bool game_lose = false;
    //插入初始状态下的策略树节点
    MethodNode node(OperateWithVal(), 0, 0, 0); //根节点里的父节点和操作这两个变量没有什么实际意义
    node.IGame = new Game(pGame);
    MethodTree.push_back(node);
    //然后进行游戏运行中的操作
    while(true)
    {
        //找到一个使局部收益最大的策略组合nodelist
        bool suc;
        uint32_t max_score_inode = search_by_ida(&suc);
        vector<uint32_t> nodelist = {max_score_inode};
        while(true){
            if (MethodTree[nodelist.back()].ParentNode != CurrentNode)
                nodelist.push_back(MethodTree[nodelist.back()].ParentNode);
            else
                break;
        }
        //按照这个策略组合向前操作。注意：和最终局部收益值保持一定的距离，以便使AI具有一定的前瞻性
        bool nodelist_use_up = false;
        uint32_t inode = nodelist.size() - 1;
        do
        {
            OperateList.push_back(MethodTree[nodelist[inode]].Ope);
            if (MethodTree[nodelist[inode]].Ope.Type == OPE_MOVE_CARD){ //挪牌
                pGame->MoveTo(MethodTree[nodelist[inode]].Ope.Pos1, MethodTree[nodelist[inode]].Ope.Pos2);
                if (AI_DEBUG)
                    pGame->GameShow();
            }else if (MethodTree[nodelist[inode]].Ope.Type == OPE_REFRESH_STORE){ //刷新贮藏区
                pGame->StoreRefresh();
                if (AI_DEBUG)
                    pGame->GameShow();
            }
            if (AI_DEBUG)
                usleep(320000);
            if (pGame->JudgeWin()){ //一局游戏获胜了。把获胜信息记录下来
                printf("游戏胜利，当前总共节点数量达到了%u.\n", CurrentNodeNum);
                game_win = true;
                break;
            }
            if (JudgeLose() || MethodTree[nodelist[inode]].Ope.Type == OPE_GIVE_UP){ //一局游戏失败了。分析游戏的完成率、地图打开率等信息，并记录下来
                game_lose = true;
                break;
            }

            if (inode == 0){
                nodelist_use_up = true;
                break;
            }
            inode--;
        }while(suc == false || MethodTree[nodelist[0]].AwardScore - MethodTree[nodelist[inode]].AwardScore > R_MOVE);
        if (game_win || game_lose)
            break;
        //调整当前的根节点位置
        if (nodelist_use_up)
            CurrentNode = nodelist[inode];
        else
            CurrentNode = nodelist[inode + 1];
        CurrentNodeNum = MethodTree.size();
    }
    //收尾处理
    if (!AI_DEBUG){
        int done_card_cnt = 0;
        int remain_card_cnt = 52;
        for(int t = 0; t <= 3; t++){
            done_card_cnt += (pGame->DoneBlock[t] + 1);
            remain_card_cnt -= (pGame->DoneBlock[t] + 1);
        }
        for (int t = 0; t <= 6; t++){
            remain_card_cnt -= pGame->GameBlock[t].size();
        }
        fprintf(pFile, "%d,%d,%u,%d,%d\n", game_no, game_win ? 1 : 0, CurrentNodeNum, remain_card_cnt, done_card_cnt);
    }
    delete pGame;

    if (game_win == true)
        return true;
    else
        return false;
}

bool AI::ai_play(int match_cnt)
{
    pFile = fopen(Output_Filename, "w");
    fprintf(pFile, "game,res,ncnt,remcnt,donecnt\n"); //需要输出的内容为：胜负、节点数量、未打开的隐藏牌和贮藏区牌数量、完成区牌数量
    for (int t = 0; t <= match_cnt - 1; t++){
        printf("游戏序号：%d, ", t);
        ai_play1game(t);
        usleep(1000);
        //sleep(1);
    }
    //printf("\n\n\n运行结束：获胜%d局，失败%d局.\n", GameWon, GameLost);
    fclose(pFile);
    return true;
}
