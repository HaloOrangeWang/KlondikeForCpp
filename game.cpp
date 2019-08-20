#include "game.h"
#include <time.h>
#include <random>
#include <string.h>
#include <sys/time.h>

Game::Game(const Game* pg): CurrRefreshTimes(pg->CurrRefreshTimes), RefreshStep(pg->RefreshStep), MaxRefreshTimes(pg->MaxRefreshTimes), StoreCardNumToShow(pg->StoreCardNumToShow), StoreRefreshOverrun(pg->StoreRefreshOverrun)
{
    //在游戏内容复制的过程中，所有变量全部使用深拷贝的方法
    for (int t = 0; t <= 6; t++)
        GameBlock[t] = pg->GameBlock[t];
    memcpy(VisHiddenBlock, pg->VisHiddenBlock, sizeof(Card) * 7 * 6);
    memcpy(HiddenBlock, pg->HiddenBlock, sizeof(Card) * 7 * 6);
    memcpy(HiddenBlockLen, pg->HiddenBlockLen, sizeof(int) * 7);
    memcpy(VisStoreBlock, pg->VisStoreBlock, sizeof(Card) * 24);
    memcpy(StoreBlock, pg->StoreBlock, sizeof(Card) * 24);
    memcpy(DoneBlock, pg->DoneBlock, sizeof(int) * 4);
    PointToStore = pg->PointToStore;
    MoveHist = pg->MoveHist;
}

int Game::GetMinNumberInDoneBlock() const
{
    int min_number = DoneBlock[0];
    for (int t=1; t<=3; t++){
        if (DoneBlock[t] > min_number)
            min_number = DoneBlock[t];
    }
    return min_number;
}

void Game::GameInit()
{
    //首先生成一个新的随机种子
    timeval tv;
    gettimeofday(&tv, nullptr);
    srand(tv.tv_sec * 1000000 + tv.tv_usec);

    //然后把数组0, 1, ..., 51打乱顺序
    int disrupt_array[52];
    for (int t=0; t<=51; t++)
        disrupt_array[t] = -1;
    for (int t=0; t<=51; t++){
        int dx = rand() % (52 - t);
        int t0 = 0;
        while (true){
            if (disrupt_array[t0] == -1){
                dx -= 1;
                if (dx == -1){
                    disrupt_array[t0] = t;
                    break;
                }
            }
            t0++;
        }
    }

    //然后用生成的随机数填充牌局中的位置
    for(int t=0; t<=23; t++) //初始化贮藏区中的牌
        StoreBlock[t] = Card(disrupt_array[t] % 4, disrupt_array[t] / 4);
    for (int t=0; t<= 6; t++){ //先将隐藏区中的牌全部置为“没有牌”
        HiddenBlockLen[t] = t;
        for (int t0=0; t0<= 5; t0++)
            HiddenBlock[t][t0] = Card(-2, -2);
    }
    for (int t=24; t<=44; t++){ //初始化隐藏区中的牌
        Card c(disrupt_array[t] % 4, disrupt_array[t] / 4);
        if (t == 24)
            HiddenBlock[1][t - 24] = c;
        else if (t <= 26)
            HiddenBlock[2][t - 25] = c;
        else if (t <= 29)
            HiddenBlock[3][t - 27] = c;
        else if (t <= 33)
            HiddenBlock[4][t - 30] = c;
        else if (t <= 38)
            HiddenBlock[5][t - 34] = c;
        else
            HiddenBlock[6][t - 39] = c;
    }
    for (int t=45; t<=51; t++) //初始化可直接操作的区域中的牌
        GameBlock[t - 45].push_back(Card(disrupt_array[t] % 4, disrupt_array[t] / 4));
    //最后初始化其他数组
    for (int t=0; t<= 6; t++){ //隐藏区的牌全部不可见
        for (int t0=0; t0<= 5; t0++){
            if (t0 < t)
                VisHiddenBlock[t][t0] = Card(-1, -1);
            else
                VisHiddenBlock[t][t0] = Card(-2, -2);
        }
    }
    for (int t=0; t<=23; t++) //贮藏区的牌全部不可见
        VisStoreBlock[t] = Card(-1, -1);
    for (int t=0; t<=4; t++) //没有一张牌是已经完成的
        DoneBlock[t] = -1;
    PointToStore = -1; //指针指向-1表示贮藏区还没有被打开，或已经全部完成
}

bool Game::MoveTo(int pos1, int pos2)
{
    //首先判断输入参数是否正确
    if (pos1 < 0 || pos1 > 11)
        return false;
    if (pos2 <0 || pos2 > 11 || pos2 == 7)
        return false;
    if (pos1 == pos2)
        return false;

    if (pos1 <= 6){
        if (GameBlock[pos1].size() == 0)
            return false; //原始位置没有牌，当然不能移动
        if (pos2 <= 6){
            //将可操作区域的牌挪到另一个可操作区域
            if (GameBlock[pos2].size() == 0){ //目标位置为空，则需要把K开头的牌挪过去
                if (GameBlock[pos1].at(0).point == 12) //如果原始位置是以K开头的，则把所有的牌都挪过去，否则不能移动
                {
                    //把所有的牌都挪过去
                    int card_number = GameBlock[pos1].size();
                    for (vector<Card>::iterator it = GameBlock[pos1].begin(); it != GameBlock[pos1].end(); it++)
                        GameBlock[pos2].push_back(*it);
                    GameBlock[pos1].clear();
                    //记录这一次挪牌
                    Operate ope;
                    ope.Pos1 = pos1;
                    ope.Pos2 = pos2;
                    ope.CardNumber = card_number;
                    ope.Type = OPE_MOVE_CARD;
                    ope.StoreCardNumToShow = StoreCardNumToShow;
                    MoveHist.push_back(ope);
                    //是否需要在隐藏区域翻开一张新的牌
                    GetNewCardFromHiddenBlock(pos1);
                    return true;
                }else{
                    return false;
                }
            }else{ //目标位置不为空
                //首先找到要把从哪张牌之后的全都挪过去
                bool found = false;
                //vector<Card>::iterator card_it;
                int card_it;
                for(card_it = 0; card_it <= GameBlock[pos1].size() - 1; card_it++){
                    if (GameBlock[pos1][card_it].point == GameBlock[pos2].back().point - 1 && ((GameBlock[pos1][card_it].color & 1) ^ (GameBlock[pos2].back().color & 1))){
                        found = true; //找到从哪张牌开始挪了
                        break;
                    }
                }
                if (!found)
                    return false;
                //把这张牌之后的全挪过去
                int card_number = 0;
                int old_size = GameBlock[pos1].size();
                for (int card_it_new = card_it; card_it_new <= old_size - 1; card_it_new++){
                    card_number++;
                    GameBlock[pos2].push_back(GameBlock[pos1][card_it_new]);
                }
                for(; card_it <= old_size - 1; card_it++)
                    GameBlock[pos1].pop_back();
                //记录这一次挪牌
                Operate ope;
                ope.Pos1 = pos1;
                ope.Pos2 = pos2;
                ope.CardNumber = card_number;
                ope.Type = OPE_MOVE_CARD;
                ope.StoreCardNumToShow = StoreCardNumToShow;
                MoveHist.push_back(ope);
                //是否需要在隐藏区域翻开一张新的牌
                if (GameBlock[pos1].size() == 0)
                    GetNewCardFromHiddenBlock(pos1);
                return true;
            }
        }else{
            //将一张操作区的牌挪到完成区
            if (pos2 - 8 != GameBlock[pos1].back().color)
                return false; //完成区对应位置的花色和该牌的花色并不一致，则不能挪动
            if (GameBlock[pos1].back().point != DoneBlock[pos2 - 8] + 1)
                return false;
            else{
                //将这张牌挪到完成区
                GameBlock[pos1].pop_back();
                DoneBlock[pos2 - 8] += 1;
                //记录这一次挪牌
                Operate ope;
                ope.Pos1 = pos1;
                ope.Pos2 = pos2;
                ope.CardNumber = 1;
                ope.Type = OPE_MOVE_CARD;
                ope.StoreCardNumToShow = StoreCardNumToShow;
                MoveHist.push_back(ope);
                //是否需要在隐藏区域翻开一张新的牌
                if (GameBlock[pos1].size() == 0)
                    GetNewCardFromHiddenBlock(pos1);
                return true;
            }
        }
    }else if (pos1 == 7){
        if (PointToStore == -1)
            return false; //贮藏区没有已经打开的牌，不能移动
        if (pos2 <= 6){
            //将贮藏区的一张牌挪到操作区
            if (GameBlock[pos2].size() == 0){ //目标位置为空，则需要把K挪过去
                if (StoreBlock[PointToStore].point == 12){ //如果贮藏区的那张牌是K开头的，就可以挪，否则不能挪
                    //记录这一次挪牌
                    Operate ope;
                    ope.Pos1 = PointToStore + 12;
                    ope.Pos2 = pos2;
                    ope.CardNumber = 1;
                    ope.Type = OPE_MOVE_CARD;
                    ope.StoreCardNumToShow = StoreCardNumToShow;
                    MoveHist.push_back(ope);
                    //挪牌
                    GameBlock[pos2].push_back(StoreBlock[PointToStore]);
                    StoreBlock[PointToStore] = Card(-2, -2);
                    VisStoreBlock[PointToStore] = Card(-2, -2);
                    do{
                        PointToStore--;
                    }while(PointToStore >= 0 && StoreBlock[PointToStore].point < 0);
                    if (StoreCardNumToShow >= 2)
                        StoreCardNumToShow--;
                    return true;
                }else{
                    return false;
                }
            }else{ //目标位置不为空
                if (StoreBlock[PointToStore].point == GameBlock[pos2].back().point - 1 && ((StoreBlock[PointToStore].color & 1) ^ (GameBlock[pos2].back().color & 1))){
                    //记录这一次挪牌
                    Operate ope;
                    ope.Pos1 = PointToStore + 12;
                    ope.Pos2 = pos2;
                    ope.CardNumber = 1;
                    ope.Type = OPE_MOVE_CARD;
                    ope.StoreCardNumToShow = StoreCardNumToShow;
                    MoveHist.push_back(ope);
                    //挪牌
                    GameBlock[pos2].push_back(StoreBlock[PointToStore]);
                    StoreBlock[PointToStore] = Card(-2, -2);
                    VisStoreBlock[PointToStore] = Card(-2, -2);
                    do{
                        PointToStore--;
                    }while(PointToStore >= 0 && StoreBlock[PointToStore].point < 0);
                    if (StoreCardNumToShow >= 2)
                        StoreCardNumToShow--;
                    return true;
                }else{
                    return false;
                }
            }
        }else{
            //将贮藏区的牌挪到完成区
            if (pos2 - 8 != StoreBlock[PointToStore].color)
                return false; //完成区对应位置的花色和该牌的花色并不一致，则不能挪动
            if (StoreBlock[PointToStore].point != DoneBlock[pos2 - 8] + 1)
                return false;
            else{
                //记录这一次挪牌
                Operate ope;
                ope.Pos1 = PointToStore + 12;
                ope.Pos2 = pos2;
                ope.CardNumber = 1;
                ope.Type = OPE_MOVE_CARD;
                ope.StoreCardNumToShow = StoreCardNumToShow;
                MoveHist.push_back(ope);
                //挪牌
                DoneBlock[pos2 - 8] += 1;
                StoreBlock[PointToStore] = Card(-2, -2);
                VisStoreBlock[PointToStore] = Card(-2, -2);
                do{
                    PointToStore--;
                }while(PointToStore >= 0 && StoreBlock[PointToStore].point < 0);
                return true;
            }
        }
    }else{
        //将完成区的牌挪到操作区
        if (DoneBlock[pos1 - 8] == -1)
            return false; //还没有已完成的牌，则不能移动
        if (pos2 >= 7)
            return false; //完成区的牌只能挪到操作区，不能挪到其他的完成区
        if (GameBlock[pos2].size() == 0){ //目标位置为空，则需要把K挪过去
            if (DoneBlock[pos1 - 8] == 12){ //如果完成区的那张牌是K开头的，就可以挪，否则不能挪
                //记录这一次挪牌
                Operate ope;
                ope.Pos1 = pos1;
                ope.Pos2 = pos2;
                ope.CardNumber = 1;
                ope.Type = OPE_MOVE_CARD;
                ope.StoreCardNumToShow = StoreCardNumToShow;
                MoveHist.push_back(ope);
                //挪牌
                GameBlock[pos2].push_back(Card(pos1 - 8, DoneBlock[pos1 - 8]));
                DoneBlock[pos1 - 8] -= 1;
                return true;
            }else{
                return false;
            }
        }else{
            if (DoneBlock[pos1 - 8] == GameBlock[pos2].back().point - 1 && (((pos1 - 8) & 1) ^ (GameBlock[pos2].back().color & 1))){
                //记录这一次挪牌
                Operate ope;
                ope.Pos1 = pos1;
                ope.Pos2 = pos2;
                ope.CardNumber = 1;
                ope.Type = OPE_MOVE_CARD;
                ope.StoreCardNumToShow = StoreCardNumToShow;
                MoveHist.push_back(ope);
                //挪牌
                GameBlock[pos2].push_back(Card(pos1 - 8, DoneBlock[pos1 - 8]));
                DoneBlock[pos1 - 8] -= 1;
                return true;
            }else{
                return false;
            }
        }
    }
}

void Game::StoreRefresh()
{
    int point_to_store_old = PointToStore;
    int point_to_store_bk = PointToStore;
    StoreCardNumToShow = 0;
    if (MaxRefreshTimes >= 1 && CurrRefreshTimes >= MaxRefreshTimes){
        StoreRefreshOverrun = true;
        return; //刷新贮藏区的次数超过了限制，不予展示
    }
    for(int t = 0; t <= RefreshStep - 1; t++){
        do{ //遍历整个贮藏区，直到遍历出牌为止
            point_to_store_bk++;
        }while(point_to_store_bk < 24 && StoreBlock[point_to_store_bk].point < 0);
        if (point_to_store_bk >= 24){ //已经遍历完成了整个贮藏区，但仍然没有刷出牌
            if (t == 0){ //一张牌都没刷出来，则直接把贮藏区指针挪到最开头
                PointToStore = -1;
                CurrRefreshTimes++; //又刷新了一轮
            }
            break;
        }else{ //遍历出牌了，把指针挪过去
            PointToStore = point_to_store_bk;
            StoreCardNumToShow++;
            VisStoreBlock[PointToStore] = StoreBlock[PointToStore];
        }
    }
    //记录此次刷新
    Operate ope;
    ope.Pos1 = point_to_store_old;
    ope.Pos2 = PointToStore;
    ope.CardNumber = 1;
    ope.Type = OPE_REFRESH_STORE;
    ope.StoreCardNumToShow = StoreCardNumToShow;
    MoveHist.push_back(ope);
}

bool Game::JudgeWin()
{
    if (DoneBlock[0] == 12 && DoneBlock[1] == 12 && DoneBlock[2] == 12 && DoneBlock[3] == 12)
        return true;
    return false;
}

bool Game::JudgeLose()
{
    //游戏失败的两个条件：1.走投无路；2.连续操作1000次
    if (MoveHist.size() >= MAX_MOVE_NUM)
        return true;
    //判断贮藏区是否刚刚完全遍历了一轮，并找出贮藏区中所有可操作的牌
    if (PointToStore != -1)
        return false;
    vector<Card> store_optb;
    int store_card_cnt = 0;
    for(int t=0; t<=23; t++){
        if (StoreBlock[t].point >= 0){
            store_card_cnt++;
            if (store_card_cnt % RefreshStep == 0)
                store_optb.push_back(StoreBlock[t]);
        }
    }
    //判断操作区里的牌能否挪到完成区
    for (int t=0; t<=6; t++){
        Card c = GameBlock[t].back();
        if (DoneBlock[c.color] + 1 == c.point)
            return false;
    }
    //判断操作区里的牌能否挪到其他操作区
    for (int t=0; t<=6; t++){
        if (GameBlock[t].size() == 0)
            continue;
        for (int t0 = 0; t0 <= GameBlock[t].size() - 1; t0++){
            for(int t1=0; t1<=6; t1++){
                if (t1 == t)
                    continue;
                if (GameBlock[t1].size() == 0){ //待挪动的操作区没有牌，此时检查原位置是否为K
                    if (GameBlock[t][t0].point == 12)
                        return false;
                }else{
                    Card card_up = GameBlock[t1][GameBlock[t1].size() - 1];
                    if (GameBlock[t][t0].point == card_up.point - 1 && ((GameBlock[t][t0].color & 1) ^ (card_up.color & 1)))
                        return false;
                }
            }
        }
    }
    //判断贮藏区里可操作的牌能否移动到操作区和完成区
    if (MaxRefreshTimes == 0 || CurrRefreshTimes < MaxRefreshTimes){ //当贮藏区刷新次数超过上限时，不进行此项判断
        for(vector<Card>::iterator it = store_optb.begin(); it != store_optb.end(); it++){
            if (DoneBlock[it->color] + 1 == it->point) //说明贮藏区中的这张牌可以移动到完成区
                return false;
            for (int t=0; t<=6; t++){
                if (GameBlock[t].size() == 0)
                    continue;
                if (GameBlock[t].back().point == it->point + 1 && ((GameBlock[t].back().color & 1) ^ (it->color & 1))) //说明贮藏区中的这张牌可以移动到操作区
                    return false;
            }
        }
    }
    return true;
}

bool Game::Back1Step()
{
    if (MoveHist.size() == 0)
        return false; //当前没有操作时，不能悔牌

    Operate ope = MoveHist[MoveHist.size() - 1];
    if (ope.Type == OPE_REFRESH_STORE){
        //对“刷新贮藏区”的悔牌。指针向前挪位置即可
        if (PointToStore == -1)
            CurrRefreshTimes--;
        PointToStore = ope.Pos1;
        StoreCardNumToShow = ope.StoreCardNumToShow;
        MoveHist.pop_back();
    }else if (ope.Type == OPE_SHOW_NEW_CARD){
        //对“打开隐藏区”的悔牌。将操作区的一张牌变更为隐藏区的牌，并修改计数器
        HiddenBlock[ope.Pos1][HiddenBlockLen[ope.Pos1]] = GameBlock[ope.Pos1][0];
        GameBlock[ope.Pos1].clear();
        HiddenBlockLen[ope.Pos1]++;
        MoveHist.pop_back();
        return Back1Step();
    }else if (ope.Type == OPE_MOVE_CARD){
        //对挪牌的悔牌
        if (ope.Pos1 <= 6 && ope.Pos2 <= 6){
            //将可操作区域的牌你挪到另一个可操作区域
            for (int t = GameBlock[ope.Pos2].size() - ope.CardNumber; t <= GameBlock[ope.Pos2].size() - 1; t++)
                GameBlock[ope.Pos1].push_back(GameBlock[ope.Pos2][t]);
            for(int t=0; t <= ope.CardNumber - 1; t++)
                GameBlock[ope.Pos2].pop_back();
        }else if (ope.Pos1 >= 8 && ope.Pos1 <= 11 && ope.Pos2 <= 6){
            //将可操作区域的牌重新放回到完成区
            DoneBlock[ope.Pos1 - 8]++;
            GameBlock[ope.Pos2].pop_back();
        }else if (ope.Pos1 >= 12 && ope.Pos2 <= 6){
            //将可操作区域的牌重新放回到贮藏区
            StoreBlock[ope.Pos1 - 12] = GameBlock[ope.Pos2][GameBlock[ope.Pos2].size() - 1];
            PointToStore = ope.Pos1 - 12;
            StoreCardNumToShow = ope.StoreCardNumToShow;
            GameBlock[ope.Pos2].pop_back();
        }else if (ope.Pos1 <= 6 && ope.Pos2 >= 8 && ope.Pos2 <= 11){
            //将完成区的牌重新放回到可操作区域
            GameBlock[ope.Pos1].push_back(Card(ope.Pos2 - 8, DoneBlock[ope.Pos2 - 8]));
            DoneBlock[ope.Pos2 - 8]--;
        }else if (ope.Pos1 >= 12 && ope.Pos2 >= 8 && ope.Pos2 <= 11){
            //将完成区的牌重新放回到贮藏区
            StoreBlock[ope.Pos1 - 12] = Card(ope.Pos2 - 8, DoneBlock[ope.Pos2 - 8]);
            PointToStore = ope.Pos1 - 12;
            StoreCardNumToShow = ope.StoreCardNumToShow;
            DoneBlock[ope.Pos2 - 8]--;
        }
        MoveHist.pop_back();
    }else{
        return false;
    }
    return true;
}

void Game::PrintACard(Card c)
{
    if (c.point == 9)
        printf(" ");
    else
        printf("  ");
    switch(c.color){
    case 0:
        printf("♦ ");
        break;
    case 1:
        printf("♣ ");
        break;
    case 2:
        printf("♥ ");
        break;
    case 3:
        printf("♠ ");
        break;
    }
    if (c.point == 0){
        printf("A");
    }else if (c.point <= 9){
        printf("%d", c.point + 1);
    }else if (c.point == 10){
        printf("J");
    }else if (c.point == 11){
        printf("Q");
    }else{
        printf("K");
    }
}

void Game::GameShow()
{
    system("clear");
    //printf("%d %d %d %d\n", DoneBlock[0], DoneBlock[1], DoneBlock[2], DoneBlock[3]);
    //首先显示完成区的牌
    printf("               ");
    for (int t=0; t<=3; t++){
        if (DoneBlock[t] >= 0){
            PrintACard(Card(t, DoneBlock[t]));
        }else{
            printf("  □ □");
        }
    }
    printf("\n");
    //然后显示贮藏区的牌
    if (StoreRefreshOverrun)
        printf("××  ");
    else
        printf("██  ");
    int cards_to_show = StoreCardNumToShow;
    vector<Card> show_cards;
    for (int t=PointToStore; t>=0; t--){
        if (StoreBlock[t].point >= 0){
            show_cards.push_back(StoreBlock[t]);
            //PrintACard(StoreBlock[t]);
            cards_to_show--;
            if (cards_to_show <= 0)
                break;
        }
    }
    for(int t = static_cast<int>(show_cards.size()) - 1; t >= 0; t--){
        PrintACard(show_cards[t]);
    }
    printf("\n\n\n");
    //最后显示操作区和隐藏区的牌
    int row, col;
    for (row=0; row<=17; row++){
        int col_num_no_card = 0;
        for (col=0; col<=6; col++){
            if (HiddenBlockLen[col] > row)
                printf("  ███");
            else{
                if (GameBlock[col].size() >= row - HiddenBlockLen[col] + 1){
                    PrintACard(GameBlock[col][row - HiddenBlockLen[col]]);
                }else{
                    printf("     ");
                    col_num_no_card++;
                }
            }
        }
        printf("\n");
        if (col_num_no_card == 7)
            break;
    }
}

void Game::Play()
{
    //首先显示初始内容，然后反复接受玩家操作并输出游戏界面，直到输或赢为止
    GameShow();
    while(true)
    {
        int ope_type, pos1, pos2;
        scanf("%d", &ope_type);
        if (ope_type == OPE_MOVE_CARD){ //挪牌
            scanf("%d %d", &pos1, &pos2);
            MoveTo(pos1, pos2);
            GameShow();
        }else if (ope_type == OPE_REFRESH_STORE){ //刷新贮藏区
            StoreRefresh();
            GameShow();
        }else if (ope_type == OPE_MOVE_BACK){ //悔牌
            Back1Step();
            GameShow();
        }
        if (JudgeWin()){
            printf("游戏获胜!\n");
            break;
        }
        if (JudgeLose()){
            printf("游戏失败!\n");
            break;
        }
    }//while(!JudgeWin() && !JudgeLose());
}

void Game::GetNewCardFromHiddenBlock(int pos)
{
    if (HiddenBlockLen[pos] != 0)
    {
        GameBlock[pos].push_back(HiddenBlock[pos][HiddenBlockLen[pos] - 1]);
        HiddenBlock[pos][HiddenBlockLen[pos] - 1] = Card(-2, -2);
        VisHiddenBlock[pos][HiddenBlockLen[pos] - 1] = Card(-2, -2);
        Operate ope2;
        ope2.Type = OPE_SHOW_NEW_CARD;
        ope2.Pos1 = pos;
        ope2.CardNumber = 1;
        ope2.StoreCardNumToShow = StoreCardNumToShow;
        MoveHist.push_back(ope2);
        HiddenBlockLen[pos] -= 1;
    }
}
