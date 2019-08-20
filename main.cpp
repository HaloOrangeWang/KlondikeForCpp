#include <stdio.h>
#include "game.h"
#include "ai.h"

int main()
{
    if (AI_MODE){
        //自动游戏模式
        AI* ai = new AI(3, 0);
        if (AI_DEBUG)
            ai->ai_play1game();
        else
            ai->ai_play(5000);
    }else{
        //试玩模式
        Game g(1, 0);
        g.GameInit();
        g.Play();
    }
    getchar();
    return 0;
}
